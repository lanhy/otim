#include "MsgOperatorServantImp.h"
#include "servant/Application.h"
#include "otim.h"
#include "otim_err.h"
#include "otim_const.h"
#include "ptcldefine.h"
#include "log.h"
#include "ScopeLogger.h"
#include "RedisPool.h"
#include "Common.h"
#include "GroupChatRPCServant.h"

using namespace std;

//////////////////////////////////////////////////////
void MsgOperatorServantImp::initialize()
{
    //initialize servant here:
    //...
}

//////////////////////////////////////////////////////
void MsgOperatorServantImp::destroy()
{
    //destroy servant here:
    //...
}

tars::Int32 MsgOperatorServantImp::request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr _current_)
{
    MLOG_DEBUG("clientContext:"<<clientContext.writeToJsonString()<<" pack:"<<req.header.writeToJsonString());
    switch(req.header.type){
        case otim::PT_MSG_READ:
            processMsgUnreadReq(clientContext, req, resp);
            break;
        case otim::PT_MSG_CTRL:
            processMsgCTRLReq(clientContext, req, resp);
          break;
        default:
            MLOG_ERROR("type is error:"<<req.header.type);
    }
 
    return otim::EC_SUCCESS;
}

tars::Int32 MsgOperatorServantImp::processMsgUnreadReq(const otim::ClientContext & clientContext,const otim::OTIMPack & reqPack,otim::OTIMPack &respPack)
{
    
    SCOPELOGGER(scopelogger);
    scopelogger<<"processMsgUnreadReq:"<<clientContext.writeToJsonString();
    
 
    otim::MsgReaded req;
    otim::unpackTars<otim::MsgReaded>(reqPack.payload, req);
    MLOG_DEBUG("clientContext:"<<clientContext.writeToJsonString()<<" req:"<<req.writeToJsonString());

    respPack = reqPack;
    respPack.header.flags |= otim::PF_ISACK;
    
    
    otim::CommonErrorCode respData;
    respData.code = otim::EC_SUCCESS;
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
 
    std::string key = otim::RKEY_UNREADCOUNT + clientContext.clientId + "." + req.sessionId;
    MLOG_DEBUG("key:" << key << " seqId:" << req.seqId );
    
    EMRStatus ret = redis->ZSetRemoveByScore(key, 0, req.seqId);
    if (ret != EMRStatus::EM_KVDB_SUCCESS){
        MLOG_WARN("delete RKEY_UNREADCOUNT msg failed:"<<key);
    }
    
    //update the hotsession
    int64_t seqId = otim::genSeqId();
    ret = redis->ZSetAdd(otim::RKEY_HOTSESSION + clientContext.clientId, seqId, req.sessionId);
    if (ret != EMRStatus::EM_KVDB_SUCCESS){
        MLOG_ERROR("update HOTSESSION fail, to" << clientContext.clientId << ", " << req.sessionId);
    }
    
    //need send to myself
    
    
    otim::packTars<otim::CommonErrorCode>(respData, respPack.payload);
   
    return otim::EC_SUCCESS;
}

tars::Int32 MsgOperatorServantImp::processMsgCTRLReq(const otim::ClientContext & clientContext,const otim::OTIMPack & reqPack,otim::OTIMPack &respPack)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"reqPack:"<<reqPack.header.writeToJsonString();
  
    otim::MsgControl req;
    otim::unpackTars<otim::MsgControl>(reqPack.payload, req);
    MLOG_DEBUG("clientContext:"<<clientContext.writeToJsonString()<<" req:"<<req.writeToJsonString());

    respPack = reqPack;
    respPack.header.flags |= otim::PF_ISACK;


    otim::CommonErrorCode respData;
    respData.code = otim::EC_SUCCESS;
    if (req.sessionId.empty() || req.seqId == 0 || req.packId.empty()){
        respData.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(respData, respPack.payload);
        MLOG_DEBUG("sessionId,packId or seqId is is empty req:"<<req.writeToJsonString());
        return  respData.code;
    }
       
       
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    //get old msg
    std::vector<std::string> msgs;
    std::vector<std::string> scores;
    EMRStatus ret = redis->ZRangeByScoreAndLimit(otim::RKEY_MSG + req.sessionId, req.seqId, 5, msgs);
    if (EMRStatus::EM_KVDB_ERROR == ret){
        MLOG_ERROR("get msg fail!, sessionId:" << req.sessionId<<" msgId:"<<req.seqId);
        respData.code = otim::EC_DB_ERROR;
        otim::packTars<otim::CommonErrorCode>(respData, respPack.payload);
        return otim::EC_DB_ERROR;
    }
    
   
    MLOG_DEBUG("get old msg size:"<<msgs.size());
    otim::OTIMPack packOrg;
    for (auto item : msgs){
        
        std::vector<char> vctItem(item.begin(), item.end());
        otim::OTIMPack packItem;
        otim::unpackTars<otim::OTIMPack>(vctItem, packItem);
        MLOG_DEBUG("msgs :"<<packItem.header.writeToJsonString());

        if (packItem.header.packId == req.packId){
            packOrg = packItem;
        }
    }
    if (packOrg.header.packId.empty())
    {
        MLOG_WARN("The org msg is not exist:"<<req.sessionId<<" packId:"<<req.packId <<" seqId:"<<req.seqId);
        respData.code = otim::EC_MSG_NOT_EXIST;
        otim::packTars<otim::CommonErrorCode>(respData, respPack.payload);
        return respData.code;
    }

    MLOG_DEBUG("org msg:"<<packOrg.header.writeToJsonString());
   
    std::string to;
    if (req.command == otim::MC_REVOKE){
        packOrg.header.flags |= otim::PF_REVOKE;
        MLOG_DEBUG("revoke msg:"<<req.packId);

    }
    else if (req.command == otim::MC_OVERRIDE){
        packOrg.header.flags |= otim::PF_OVERRIDE;
        
        otim::MsgReq msgReq;
        otim::unpackTars<otim::MsgReq>(packOrg.payload, msgReq);
        msgReq.content = req.content;
        
        otim::packTars<otim::MsgReq>(msgReq, packOrg.payload);
        
        to = msgReq.to;
        MLOG_DEBUG("override msg:"<<req.packId);

    }
    else if (req.command == otim::MC_DELETE){
//        packOrg.header.flags |= otim::PF_REVOKE;
        MLOG_DEBUG("delete msg:"<<req.packId);
    }
    else{
        MLOG_WARN("The command is error:"<<req.command<<" packId:"<<req.packId);
        respData.code = otim::EC_MSG_OP_CMD;
        otim::packTars<otim::CommonErrorCode>(respData, respPack.payload);
        return otim::EC_MSG_OP_CMD;
    }
  

    ret = redis->ZSetRemoveByScore(otim::RKEY_MSG + req.sessionId, req.seqId, req.seqId);
    if (EMRStatus::EM_KVDB_SUCCESS != ret ){
        MLOG_ERROR("delete original msg fail:"<<(int)ret);
    }
    
    //增加新的消息
    if (req.command != otim::MC_DELETE){
        std::string msgSave;
        otim::packTars<otim::OTIMPack>(packOrg, msgSave);
        ret = redis->ZSetAdd(otim::RKEY_MSG + req.sessionId, req.seqId, msgSave);
        if ( EMRStatus::EM_KVDB_SUCCESS != ret ){
            MLOG_ERROR("save cancel msg fail!");
        }
    }
 
    //通知在线接收者其他端
    otim::sendPackToMySelf(clientContext, reqPack);

//  send to user
    std::vector<std::string> vctUserId;
    if (packOrg.header.type == otim::PT_MSG_SINGLE_CHAT || packOrg.header.type == otim::PT_MSG_BIZ_NOTIFY){
        if (to.empty()){
            otim::MsgReq msgReq;
            otim::unpackTars<otim::MsgReq>(packOrg.payload, msgReq);
            to = msgReq.to;
        }
       
        vctUserId.push_back(to);
        MLOG_DEBUG("single or notify chat  packId:"<<packOrg.header.packId<<" to:"<<to);
    }
    else if (packOrg.header.type == otim::PT_MSG_GROUP_CHAT){
        //get groupMember
        otim::GroupChatRPCServantPrx groupChatRPCServantPrx = otim::getServantPrx<otim::GroupChatRPCServantPrx>(PRXSTR_GROUP_CHAT_RPC);
        groupChatRPCServantPrx->getGroupMember(req.sessionId, vctUserId);
        MLOG_DEBUG("group chat  packId:"<<packOrg.header.packId<<" to:"<<req.sessionId<<" member Size:"<<vctUserId.size());
    }
    
    int64_t seqId = otim::genSeqId();
    for (auto userId : vctUserId){
        
        otim::savePriorityMsg(redis.get(), reqPack, userId, seqId);

        otim::dispatchMsg(clientContext, reqPack, userId);
    }
    
    respData.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(respData, respPack.payload);

    return otim::EC_SUCCESS;
}
