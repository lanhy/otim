#include "HistoryMsgServantImp.h"
#include "servant/Application.h"
#include "otim.h"
#include "otim_err.h"
#include "otim_const.h"
#include "ptcldefine.h"
#include "log.h"
#include "ScopeLogger.h"
#include "RedisPool.h"
#include "GroupChatRPCServant.h"
#include "Common.h"

using namespace std;

//////////////////////////////////////////////////////
void HistoryMsgServantImp::initialize()
{
    //initialize servant here:
    std::string myConfFile = ServerConfig::BasePath+ServerConfig::ServerName +".conf";
    TC_Config myConf;
    myConf.parseFile(myConfFile);
    _maxHotSessionCount = TC_Common::strto<int>(myConf.get("/otim<max_hotsession_count>", "100"));
    MLOG_DEBUG("myconf:"<<myConfFile<<" _maxHotSessionCount:"<<_maxHotSessionCount);
}

//////////////////////////////////////////////////////
void HistoryMsgServantImp::destroy()
{
    //destroy servant here:
    //...
}


tars::Int32 HistoryMsgServantImp::request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr _current_)
{
    MLOG_DEBUG("clientContext:"<<clientContext.writeToJsonString()<<" pack:"<<req.writeToJsonString());
    switch(req.header.type){
        case otim::PT_HOTSESSION_SYNC:
            processHotsessionReq(clientContext, req, resp);
            break;
        case otim::PT_HISTORY_MSG_PULL:
            processPullHistoryReq(clientContext, req, resp);
          break;
        case otim::PT_HIGH_PRIOR_MSG_SYNC:
            processHighPriorMsgSyncReq(clientContext, req, resp);
          break;
         default:
            MLOG_ERROR("type is error:"<<req.header.type);
    }
 
    return otim::EC_SUCCESS;
}


tars::Int32 HistoryMsgServantImp::processHotsessionReq(const otim::ClientContext & clientContext,const otim::OTIMPack & reqPack,otim::OTIMPack &respPack)
{
    
    SCOPELOGGER(scopelogger);
    scopelogger<<"getHotSessionInfo:"<<clientContext.writeToJsonString();
    
 
    otim::HotSessionReq req;
    otim::unpackTars<otim::HotSessionReq>(reqPack.payload, req);
    MLOG_DEBUG("clientContext:"<<clientContext.writeToJsonString()<<" req:"<<req.writeToJsonString());

    respPack = reqPack;
    respPack.header.flags |= otim::PF_ISACK;
    
    
    otim::HotSessionResp respData;
    respData.errorCode.code = otim::EC_SUCCESS;
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
 
//    EMRStatus ret = redis->ZSetAdd(otim::RKEY_HOTSESSION + msgReq.to, msgReq.seqId, msgReq.sessionId);

    // 读取热会话，从传入的seqId向最近读取
    //不包含timestamp对应的消息id，需要input_req.timestamp+1
    std::vector<std::string> sessions, scoreItems;
    redis->ZRangeByScoreWithScores(otim::RKEY_HOTSESSION + clientContext.clientId, req.timestamp+1, MAX_SCORE, sessions, scoreItems);
    
    MLOG_DEBUG("getHotSessionInfo size: " << sessions.size() << ", userId: " << clientContext.clientId+" timestamp:"<<req.timestamp);
    
    uint64_t lastUpdateTime = req.timestamp;
    // 先把离线消息表中的数据，拷贝到返回结构中，并对上面的sessions排重
    for (size_t i = 0; i < sessions.size(); i++)
    {
        std::string sessionId = sessions[i];
        
        otim::HotSessionItem hsi;
        hsi.sessionId  = sessionId;
        hsi.readSeqId = 0;
//        hsi.attribute = getSessionAttribute(client_info.client_id, sessionId);
//        hsi.lastSeqId = TC_Common::strto<int64_t>(scoreItems[i]);

        std::string unreadKey = otim::RKEY_UNREADCOUNT + clientContext.clientId + "." + sessionId;
        redis->ZCard(unreadKey, hsi.unreadCount);
        MLOG_INFO("unreadKey:"<<unreadKey<<" unreadCount:"<<hsi.unreadCount);

 		uint64_t currSeqId = 0;
        vector<string> ms, ss;
        redis->ZReverRangeByScoreAndLimitWithScores(otim::RKEY_MSG + sessionId, "+inf", 10, ms, ss);
        if (ms.size() > 0){
            currSeqId = TC_Common::strto<uint64_t>(ss[0]);
            //未读数为0时获取最新一条消息seqid为read_seq_id
            if (hsi.unreadCount == 0){
                hsi.readSeqId = currSeqId;
            }
            
            otim::OTIMPack itemPack;
            std::vector<char> vctItem(ms[0].begin(), ms[0].end());
            otim::unpackTars<otim::OTIMPack>(vctItem, itemPack);
            hsi.lastMsgs.push_back(itemPack);
        }
   
        //get read seqId when the readSeqId is not get
        if (hsi.readSeqId == 0 && hsi.unreadCount > 0){
            vector<string> msgIds, seqIds;
            redis->ZRangeByScoreAndLimitWithScore(unreadKey, 0, 1, msgIds, seqIds);
            if (seqIds.size() > 0){
                hsi.readSeqId = TC_Common::strto<uint64_t>(seqIds[0]);
            }
        }
        MLOG_INFO("session_id: " << sessionId << ", unreadCount: " << hsi.unreadCount <<"  readSeqId:"<<hsi.readSeqId<<" lastmsg size:"<<hsi.lastMsgs.size()<<" currSeqId:"<<currSeqId);
        
        
        respData.sessions.push_back(hsi);
        
        lastUpdateTime = std::max(currSeqId, lastUpdateTime);
        
        if (respData.sessions.size() > _maxHotSessionCount)
        {
            MLOG_DEBUG("session_id: " << sessionId << ", will return ,the number >=: " << _maxHotSessionCount);
            break;
        }
    }
    
    MLOG_DEBUG("clientId:"<<clientContext.clientId <<" sessions size: " << respData.sessions.size());
    
    respData.timestamp = lastUpdateTime;
    scopelogger<<" session size:"<<respData.sessions.size()<<" reqId:"<<respPack.header.writeToJsonString() << " timestamp:"<<respData.timestamp;
 
    
    otim::packTars<otim::HotSessionResp>(respData, respPack.payload);
   
    return otim::EC_SUCCESS;
}

tars::Int32 HistoryMsgServantImp::processPullHistoryReq(const otim::ClientContext & clientContext,const otim::OTIMPack & reqPack,otim::OTIMPack &respPack)
{
    
    SCOPELOGGER(scopelogger);
    scopelogger<<"getHotSessionInfo:"<<clientContext.writeToJsonString();
    
    
    otim::HistoryMsgPullReq req;
    otim::unpackTars<otim::HistoryMsgPullReq>(reqPack.payload, req);
    MLOG_DEBUG("clientContext:"<<clientContext.writeToJsonString()<<" req:"<<req.writeToJsonString());
    if (req.count <= 0){
        req.count = 30;
    }
    if (req.count > 200){
        req.count = 200;
    }
    
    if (req.seqId <= 0){
        req.seqId = LONG_MAX;
    }
    MLOG_DEBUG("req:"<<req.writeToJsonString());

    respPack = reqPack;
    respPack.header.flags |= otim::PF_ISACK;
    
    otim::HistoryMsgPullResp respData;
    respData.errorCode.code = otim::EC_SUCCESS;
    respData.sessionId = req.sessionId;
    
    if (!isAllowPullMsg(clientContext.clientId, req.sessionId)){
        MLOG_DEBUG("Do not Allow PullMsg:"<<req.writeToJsonString());
        respData.errorCode.code = otim::EC_HISTORYMSG_DENY;
        otim::packTars<otim::HistoryMsgPullResp>(respData, respPack.payload);
        return otim::EC_HISTORYMSG_DENY;
    }
    

    otim::RedisConnPtr redis(otim::RedisPool::instance());
    
    std::vector<std::string> msgs,scores;
    std::string strTable = otim::RKEY_MSG+req.sessionId;
    EMRStatus ret = redis->ZReverRangeByScoreAndLimitWithScores(strTable, req.seqId, req.count, msgs, scores);
    MLOG_DEBUG("ret:"<<(int)ret<<" msgs size:"<<msgs.size());
//    struct HistoryMsgPullResp
//    {
//        0 require CommonErrorCode errorCode;
//        1 require string sessionId; //起始sessionId
//        2 require vector<OTIMPack> msgs;
//    };

    for (auto item : msgs){
        otim::OTIMPack itemPack;
		std::vector<char> vctItem(item.begin(), item.end());
        otim::unpackTars<otim::OTIMPack>(vctItem, itemPack);
        
        if (itemPack.header.packId.empty()){
            MLOG_DEBUG("itemPack packId empty:"<<itemPack.header.writeToJsonString());
            continue;
        }
        
        MLOG_INFO("itemPack:"<<itemPack.header.writeToJsonString());
        respData.msgs.push_back(itemPack);
    }
    
    MLOG_DEBUG("result:"<<respPack.header.writeToJsonString()<<" count:"<<respData.msgs.size());

    otim::packTars<otim::HistoryMsgPullResp>(respData, respPack.payload);

    return otim::EC_SUCCESS;
}


bool HistoryMsgServantImp::isAllowPullMsg(const std::string &clientId, const std::string &sessionId)
{
    //check the session is group
    if (sessionId.find("GC_") == sessionId.npos){
        return true;
    }
    
   //call rpc check group member
    otim::GroupChatRPCServantPrx groupChatRPCServantPrx = otim::getServantPrx<otim::GroupChatRPCServantPrx>(PRXSTR_GROUP_CHAT_RPC);
    bool isMember = groupChatRPCServantPrx->isGroupMember(sessionId, clientId);
    MLOG_DEBUG("isGroupMember  sessionId:"<<sessionId<<" isMember:"<<isMember);

    return isMember;
}


tars::Int32 HistoryMsgServantImp::processHighPriorMsgSyncReq(const otim::ClientContext & clientContext,const otim::OTIMPack & reqPack,otim::OTIMPack &respPack)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"getHotSessionInfo:"<<clientContext.writeToJsonString();
    
    
    otim::MsgHighPrioritySyncReq req;
    otim::unpackTars<otim::MsgHighPrioritySyncReq>(reqPack.payload, req);
    MLOG_DEBUG("clientContext:"<<clientContext.writeToJsonString()<<" req:"<<req.writeToJsonString());
    if (req.count <= 0){
        req.count = 200;
    }
    
    if (req.seqId <= 0){
        req.seqId = LONG_MAX;
    }
    MLOG_DEBUG("req:"<<req.writeToJsonString());

    respPack = reqPack;
    respPack.header.flags |= otim::PF_ISACK;
    
    otim::MsgHighPrioritySyncResp respData;
    respData.errorCode.code = otim::EC_SUCCESS;
  
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    
    std::vector<std::string> msgs,scores;
    std::string strTable = otim::RKEY_MSG_HIGHPRJ+clientContext.clientId;
    EMRStatus ret = redis->ZReverRangeByScoreAndLimitWithScores(strTable, req.seqId, req.count, msgs, scores);
    MLOG_DEBUG("ret:"<<(int)ret<<" msgs size:"<<msgs.size());

    for (size_t i = 0; i < msgs.size(); i++){
        auto item = msgs[i];
        otim::OTIMPack itemPack;
        std::vector<char> vctItem(item.begin(), item.end());
        otim::unpackTars<otim::OTIMPack>(vctItem, itemPack);
        
        if (itemPack.header.packId.empty()){
            MLOG_DEBUG("itemPack packId empty:"<<itemPack.header.writeToJsonString());
            continue;
        }
        
        MLOG_INFO("itemPack:"<<itemPack.header.writeToJsonString());
        respData.msgs.push_back(itemPack);
        
        int64_t seqId = TC_Common::strto<int64_t>(scores[i]);
        respData.lastSeqId = max(seqId, respData.lastSeqId);
    }
    
    MLOG_DEBUG("result:"<<respPack.header.writeToJsonString()<<" count:"<<respData.msgs.size()<<" lastSeqId:"<<respData.lastSeqId);

    otim::packTars<otim::MsgHighPrioritySyncResp>(respData, respPack.payload);

    return otim::EC_SUCCESS;

}
