#include "SingleChatServantImp.h"
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
#include "UserInfoManager.h"

using namespace std;

//////////////////////////////////////////////////////
void SingleChatServantImp::initialize()
{
    //initialize servant here:
    std::string myConfFile = ServerConfig::BasePath+ServerConfig::ServerName +".conf";
    TC_Config myConf;
    myConf.parseFile(myConfFile);
    _msgMaxLen = TC_Common::strto<int>(myConf.get("/otim<maxlen>", "65536"));
    MLOG_DEBUG("myconf:"<<myConfFile<<" _msgMaxLen:"<<_msgMaxLen);
}

//////////////////////////////////////////////////////
void SingleChatServantImp::destroy()
{
    //destroy servant here:
    //...
}

tars::Int32 SingleChatServantImp::request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr current)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();

    if (req.header.type != otim::PT_MSG_SINGLE_CHAT){
        MLOG_DEBUG("the type is not PT_MSG_SINGLE_CHAT");
        return otim::EC_PROTOCOL;
    }
    
    
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

    
    if (req.header.type != otim::PT_MSG_SINGLE_CHAT){
        MLOG_DEBUG("the type is not PT_MSG_SINGLE_CHAT");
        msgAck.errorCode.code = otim::EC_PARAM;
        
        otim::packTars<otim::MsgAck>(msgAck, resp.payload);
        return otim::EC_SUCCESS;
    }
    
    otim::MsgReq msgReq;
    otim::unpackTars<otim::MsgReq>(req.payload, msgReq);
    if (msgReq.from.empty() || msgReq.to.empty() || msgReq.sessionId.empty()){
        MLOG_DEBUG("msg from , to or sessionId is empty, packId:"<<req.header.packId<<" from:" <<msgReq.from<<" to:"<<msgReq.to<<" sessionId:"<<msgReq.sessionId);
        msgAck.errorCode.code = otim::EC_PARAM;
        
        otim::packTars<otim::MsgAck>(msgAck, resp.payload);
        return otim::EC_SUCCESS;
    }
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());
    if (!umIns.checkUserFriend(msgReq.from, msgReq.to)){
        MLOG_DEBUG("from and to is not friend, packId:"<<req.header.packId<<" from:" <<msgReq.from<<" to:"<<msgReq.to<<" sessionId:"<<msgReq.sessionId);
        msgAck.errorCode.code = otim::EC_USER_RELATION_INVALID;
        
        otim::packTars<otim::MsgAck>(msgAck, resp.payload);
        return otim::EC_USER_RELATION_INVALID;
    }
    
    
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
    
   
    //get the seqId
    msgReq.timestamp = msgAck.timestamp;
    msgReq.seqId = otim::genSeqId();
    msgAck.seqId = msgReq.seqId;
    msgAck.sessionId = msgReq.sessionId;

    otim::OTIMPack pack;
    pack.header = req.header;
    otim::packTars<otim::MsgReq>(msgReq, pack.payload);

    MLOG_DEBUG("Msg seqId:"<<msgReq.seqId<<" packId:"<<pack.header.packId);
    //check sensi word
    
    //save
    saveMsg(clientContext, pack, msgReq.sessionId, msgReq.seqId);
         
    if (flagbits.highPRJ){
        otim::RedisConnPtr redis(otim::RedisPool::instance());

        otim::savePriorityMsg(redis.get(), pack, msgReq.to, msgReq.seqId);
    }

    //update the session
    updateSession(clientContext, pack, msgReq);
    
    //dispatch
    otim::dispatchMsg(clientContext, pack, msgReq.to);
    
    //发送给其他端
    otim::sendPackToMySelf(clientContext, pack);


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

int SingleChatServantImp::saveMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req,const std::string & sessionId,tars::Int64 seqId)
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

int SingleChatServantImp::updateSession(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::MsgReq &msgReq)
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
    
	ret = redis->ZSetAdd(otim::RKEY_HOTSESSION + msgReq.from, msgReq.seqId, msgReq.sessionId);
    if (ret != EMRStatus::EM_KVDB_SUCCESS){
        MLOG_ERROR("update HOTSESSION fail, from" << msgReq.from << ", " << msgReq.sessionId);
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

