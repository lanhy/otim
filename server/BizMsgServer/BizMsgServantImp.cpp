#include "BizMsgServantImp.h"
#include "servant/Application.h"
#include "otim_err.h"
#include "ScopeLogger.h"
#include "LongLinkRedis.h"
#include "log.h"
#include "ptcldefine.h"
#include "BrokerPushServant.h"
#include "OlapServant.h"
#include "Common.h"
#include "RedisPool.h"
#include "otim_const.h"


using namespace std;

//////////////////////////////////////////////////////
void BizMsgServantImp::initialize()
{
    //initialize servant here:
    std::string myConfFile = ServerConfig::BasePath+ServerConfig::ServerName +".conf";
    TC_Config myConf;
    myConf.parseFile(myConfFile);
    _msgMaxLen = TC_Common::strto<int>(myConf.get("/otim<maxlen>", "65536"));
    MLOG_DEBUG("myconf:"<<myConfFile<<" _msgMaxLen:"<<_msgMaxLen);
}

//////////////////////////////////////////////////////
void BizMsgServantImp::destroy()
{
    //destroy servant here:
    //...
}

//////////////////////////////////////////////////////
tars::Int32 BizMsgServantImp::request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr current)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();

    MLOG_DEBUG("header:"<<req.header.writeToJsonString());    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    otim::MsgAck msgAck;
    msgAck.timestamp = TC_Common::now2ms();
    msgAck.errorCode.code = otim::EC_SUCCESS;
    if (req.payload.size() < 4){
        msgAck.errorCode.code = otim::EC_MSG_TOO_SHORT;
        otim::packTars<otim::MsgAck>(msgAck, resp.payload);
        return otim::EC_SUCCESS;
    }
 
    if (req.payload.size() > _msgMaxLen){
        msgAck.errorCode.code = otim::EC_MSG_TOO_LONG;
        otim::packTars<otim::MsgAck>(msgAck, resp.payload);
        return otim::EC_SUCCESS;
    }

    
    if (req.header.type != otim::PT_MSG_BIZ_NOTIFY){
        MLOG_DEBUG("the type is not PT_MSG_BIZ_NOTIFY, type:"<<req.header.type );
        msgAck.errorCode.code = otim::EC_PROTOCOL;
        
        otim::packTars<otim::MsgAck>(msgAck, resp.payload);
        return otim::EC_SUCCESS;
    }
    
    otim::MsgReq msgReq;
    otim::unpackTars<otim::MsgReq>(req.payload, msgReq);
    if (msgReq.from.empty() || msgReq.to.empty() || msgReq.sessionId.empty()){
        MLOG_ERROR("msg from , to or sessionId is empty, packId:"<<req.header.packId<<" from:" <<msgReq.from<<" to:"<<msgReq.to<<" sessionId:"<<msgReq.sessionId);
        msgAck.errorCode.code = otim::EC_PARAM;
        
        otim::packTars<otim::MsgAck>(msgAck, resp.payload);
        return otim::EC_SUCCESS;
    }
   
    //get the seqId
    msgReq.timestamp = msgAck.timestamp;
    msgReq.seqId = otim::genSeqId();
    msgAck.seqId = msgReq.seqId;
    msgAck.sessionId = msgReq.sessionId;

    //check repeat
    otim::PT_FLAGS_BITS flagbits = otim::getHeaderFlagBits(req.header.flags);
    if (flagbits.dup){
        otim::RedisConnPtr redis(otim::RedisPool::instance());
        bool isReapted = otim::isRepeatedPackId(redis.get(), req.header.packId, msgAck.seqId);
        if (isReapted){
            msgAck.errorCode.code = otim::EC_MSG_REPEATED;
            
            otim::packTars<otim::MsgAck>(msgAck, resp.payload);
            MLOG_DEBUG("Msg Repeated seqId:"<<msgReq.seqId<<" packId:"<<req.header.packId);

            return otim::EC_MSG_REPEATED;
        }
    }
    
    otim::OTIMPack pack;
    pack.header = req.header;
    otim::packTars<otim::MsgReq>(msgReq, pack.payload);

    MLOG_DEBUG("Msg seqId:"<<msgReq.seqId<<" packId:"<<pack.header.packId);
    

    //check sensi word
    
    //save
    saveMsg(clientContext, pack, msgReq.sessionId, msgReq.seqId);
         
    //update the session
    updateSession(clientContext, pack, msgReq);
    
    //dispatch
    dispatchMsg(clientContext, pack, msgReq.to);

    otim::packTars<otim::MsgAck>(msgAck, resp.payload);

    return otim::EC_SUCCESS;
}

class OlapServantCallback : public otim::OlapServantPrxCallback{
public:
    OlapServantCallback(const std::string & packId, const std::string & sessionId,tars::Int64 seqId){
        _packId = packId;
        _sessionId = sessionId;
        _seqId = seqId;
    }
    
    virtual void callback_saveMsg(tars::Int32 ret)
    {
        MLOG_DEBUG("save ok ret:"<<ret<<" _packId:"<<_packId<<" sessionId:"<<_sessionId<<" seqId:"<<_seqId);
    }
    virtual void callback_saveMsg_exception(tars::Int32 ret)
    {
        MLOG_DEBUG("save exception ret:"<<ret<<" _packId:"<<_packId<<" sessionId:"<<_sessionId<<" seqId:"<<_seqId);
    }
    
private:
    std::string _packId;
    std::string _sessionId;
    tars::Int64 _seqId;
};

int BizMsgServantImp::saveMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req,const std::string & sessionId,tars::Int64 seqId)
{
    SCOPELOGGER(scopelogger);
    int ret = otim::EC_SUCCESS;
    try
    {
        otim::OlapServantPrx servantPrx = otim::getServantPrx<otim::OlapServantPrx>(PRXSTR_OLAP);

        tars::TC_AutoPtr<OlapServantCallback> srvCB = new OlapServantCallback(req.header.packId, sessionId, seqId);
        
        servantPrx->async_saveMsg(srvCB, clientContext, req, sessionId, seqId);
    }
    catch (std::exception& e)
    {
        ret = otim::EC_SERVICE_UNAVAILABLE;
        MLOG_ERROR("SERVICEALARM servantPrx->async_saveMsg error:" << e.what());
    }
    catch (...)
    {
        ret = otim::EC_SERVICE_UNAVAILABLE;
        MLOG_ERROR("SERVICEALARM servantPrx->async_saveMsg unknown exception");
    }
    

    return ret;
}


int BizMsgServantImp::dispatchMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req, const std::string &to)
{
    std::vector<LongLinkPtr> vctLink = LongLinkRedis::getInstance()->getLongLinkByClientId(to);
    if (vctLink.size() == 0){
        //need push
        MLOG_DEBUG("the user offline ,will push msg:"<<to<<" packId:"<<req.header.packId);
        return otim::EC_SUCCESS;
    }
    
    for (auto item : vctLink){
        try
        {
            if (item->context.brokerId.empty()){
                MLOG_ERROR("brokerId is empty uid: " << item->context.uid);
                continue;
            }
  
            otim::BrokerPushServantPrx brokerPrx = otim::getServantPrx<otim::BrokerPushServantPrx>(item->context.brokerId);
//            brokerPrx->async_push(NULL, item->context.uid, req);
            int ret = brokerPrx->push(item->context.uid, req);
            MLOG_DEBUG("USERONLINE push uid:" << item->context.uid << " userId:" << to << " packId:" << req.header.packId << " size: " << req.payload.size()<<" ret:"<<ret);
        }
        catch (std::exception& e)
        {
            MLOG_ERROR("USERONLINE push error: " << e.what());
            continue;
        }
        catch (...)
        {
            MLOG_ERROR("USERONLINE push unknown exception");
        }
    }
    
    return otim::EC_SUCCESS;
}

int BizMsgServantImp::updateSession(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::MsgReq &msgReq)
{
    MLOG_DEBUG("packId:" << req.header.packId <<" to:" << msgReq.to << " sessionId:" << msgReq.sessionId<<" seqId:"<<msgReq.seqId);
    if (msgReq.sessionId.empty()){
        MLOG_ERROR("_updateSessionListofUser sessionId is empty!!!!!!");
        return otim::EC_SUCCESS;
    }
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    assert(redis.get() != nullptr);

    EMRStatus ret = redis->ZSetAdd(otim::RKEY_HOTSESSION + msgReq.to, msgReq.seqId, msgReq.sessionId);
    if (ret != EMRStatus::EM_KVDB_SUCCESS){
        MLOG_ERROR("update HOTSESSION fail, to" << msgReq.to << ", " << msgReq.sessionId);
    }
    
    //save unread count
    std::string key = otim::RKEY_UNREADCOUNT + msgReq.to + "." + msgReq.sessionId;
    ret = redis->ZSetAdd(key, msgReq.seqId, req.header.packId);
    if(ret != EMRStatus::EM_KVDB_SUCCESS)
    {
        MLOG_ERROR("update RKEY_UNREADCOUNT fail, key" << key << " packId:"<< req.header.packId);
    }

    return otim::EC_SUCCESS;
}


