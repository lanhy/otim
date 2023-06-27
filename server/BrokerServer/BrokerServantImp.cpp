#include "BrokerServantImp.h"
#include "servant/Application.h"
#include "ScopeLogger.h"
#include "BrokerServer.h"
#include "ptcldefine.h"
#include "otim_err.h"
#include "LonglinkManager.h"
#include "RPCCallback.h"
#include "log.h"
#include "AuthServant.h"
#include "Common.h"


using namespace std;
extern BrokerServer g_app;

//////////////////////////////////////////////////////
void BrokerServantImp::initialize()
{
    //initialize servant here:
    std::string myConfFile = ServerConfig::BasePath+ServerConfig::ServerName +".conf";
    TC_Config myConf;
    myConf.parseFile(myConfFile);
    MLOG_DEBUG("myconf:"<<myConfFile);


    std::vector<std::string> vList = myConf.getDomainVector("/otim/servermap");
    for (auto strItem : vList)
    {
        std::string keypath = "/otim/servermap/" + strItem;
        std::string strType = myConf.get(keypath+"<type>");
        MLOG_DEBUG("keypath:" << keypath<<"  value:"<<strType);
        
        vector<std::string> vctType = TC_Common::sepstr<std::string>(strType, ",", false);
        std::string serverName = myConf.get(keypath+"<name>");
        if (serverName.empty()){
            MLOG_ERROR("servername empty:"<<keypath);
            continue;
        }
        
        for (auto strType : vctType){
            if (strType.empty()){
                continue;
            }
            
            int type = TC_Common::strto<int>(strType);
            _mapServerName[type] = serverName;
            MLOG_DEBUG("type:" << type<<"  serverName:"<<serverName);
        }
    }
    
    //get brokerId
    g_app.getBrokerId();
}

//////////////////////////////////////////////////////
void BrokerServantImp::destroy()
{
    //destroy servant here:
    //...
}

int BrokerServantImp::doRequest(tars::TarsCurrentPtr current, vector<char>& response)
{
    try {
        const vector<char>& request = current->getRequestBuffer();
        
        otim::OTIMPack pack;
        otim::unpackTars<otim::OTIMPack>(request, pack);
        MLOG_DEBUG("pack type:"<<otim::etos((otim::PACK_TYPE)pack.header.type)<<" header:"<<pack.header.writeToJsonString());
        //get the context;
        LongLinkPtr pLongLink = LongLinkManager::getInstance()->update(current->getUId(), pack.header);
        
        MLOG_DEBUG("uid:"<<current->getUId()<<" context:"<<pLongLink->context.writeToJsonString()<<" pack header:"<<pack.header.writeToJsonString());
        //check process
        switch (pack.header.type) {
            case otim::PT_PING:
                this->processPingReq(current, pLongLink->context, pack);
                
                break;
            case otim::PT_LOGIN:
                this->processLoginReq(current, pLongLink->context, pack);
                break;
            case otim::PT_LOGOUT:
                this->processLogoutReq(current, pLongLink->context, pack);
                break;
            case otim::PT_KICKOUT:
                this->processKickoutReq(current, pLongLink->context, pack);
                
                break;
            default:
                this->processReqRPC(current, pLongLink->context, pack);
                break;
        }
    } catch (std::exception& ex) {
        MLOG_ERROR("exception occur uid:"<<current->getUId()<<" ex:"<<ex.what());
        current->close();
    }
    
    return 0;
}

int BrokerServantImp::doClose(TarsCurrentPtr current)
{
    MLOG_DEBUG("close ip: " << current->getIp()<<" uid:"<<current->getUId());
    LongLinkManager::getInstance()->remove(current->getUId());
    return 0;
}


void BrokerServantImp::processReqRPC(tars::TarsCurrentPtr current, const otim::ClientContext &context, const otim::OTIMPack &req)
{
    try
    {
        if (context.clientId.empty()){
            MLOG_DEBUG("clientId is empty, maybe not auth:" << req.header.type <<" context:"<<context.writeToJsonString());
            current->close();
            return;
        }
        
        auto iter = _mapServerName.find(req.header.type);
        if (iter == _mapServerName.end()){
            MLOG_ERROR("serverName empty, type can't support:" << req.header.type <<" clientId:"<<context.clientId);
            return;
        }
        
        std::string serverName = iter->second;
        
        MLOG_DEBUG("uid:"<<current->getUId()<<" type:"<<otim::etos((otim::PACK_TYPE)req.header.type) << "("  << req.header.type << ")" <<" server:"<<serverName<<" clientId:"<<context.clientId);
        
        otim::BaseServantPrx proxy;
        Application::getCommunicator()->stringToProxy(serverName, proxy);
        tars::TC_AutoPtr<CommonServantPrxBrokerCallback> cb = new CommonServantPrxBrokerCallback(current, context, req);
        
        proxy->async_request(cb, context, req);
    }
    catch (std::exception& e)
    {
        MLOG_ERROR("async_request error:" << e.what());
    }
    catch (...)
    {
        MLOG_ERROR("async_request unknown exception");
    }
}


void BrokerServantImp::processPingReq(tars::TarsCurrentPtr current, const otim::ClientContext &context, const otim::OTIMPack &req)
{
    if (context.clientId.empty()){
        MLOG_DEBUG("clientId is empty, maybe not auth:" << req.header.type <<" context:"<<context.writeToJsonString());
        current->close();
        return;
    }

    MLOG_DEBUG("ping Req:"<<req.writeToJsonString());
    otim::OTIMPack respPack = req;
    respPack.header.flags |= otim::PF_ISACK;
 
    g_app.sendDataToClient(current, respPack);
}

void BrokerServantImp::processLoginReq(tars::TarsCurrentPtr current, const otim::ClientContext &context, const otim::OTIMPack &req)
{
    otim::LoginReq loginReq;
    otim::unpackTars<otim::LoginReq>(req.payload, loginReq);
    
    unsigned uid = current->getUId();
    ScopeLogger scopelogger("login req");
    scopelogger << " recv loginreq Uid:" << uid << ", ip:" << current->getIp() << ", port:" << current->getPort()<<" pack:"<<loginReq.writeToJsonString();
    
    if (loginReq.clientId.empty()
        || loginReq.password.empty()
        || loginReq.deviceId.empty()
        || loginReq.deviceType == otim::DEVICE_NONE)
    {
        scopelogger << ", login failed, clientId or password is invalid. socket close";
        UserAuthServantCallback::sendLoginAckToClient(current, req.header.packId, otim::EC_CONN_INDICATE, loginReq, "");
        current->close();
        return;
    }
    
//
//    RedisConnPtr redis(RedisPool::instance());
//    CDisLockObj cDisObj(redis.get(), Toon::loginLockTableName + strClientId);
//    if (!cDisObj.DoLock(DEFAULT_OVERTIME)) {
//        MLOG_ERROR("client:" << strClientId << " loginlock  overtime, close socket.Uid:" << uid);
//        SendLoginAckToClient(Toon::CONNECT_RESULT_SERVICE_UNAVAILABLE, "Remote Services is invalid", "", pc, current);
//        current->close();
//        return;
//    }
//
//    std::string strUUID = cDisObj.getLockUUID();
    int ret = processAuthReq(current, context, req.header.packId, loginReq);
    if (ret == otim::EC_SERVICE_UNAVAILABLE) {
//        cDisObj.DoUnLock();
        UserAuthServantCallback::sendLoginAckToClient(current, req.header.packId, otim::EC_CONN_INDICATE, loginReq, "");

        MLOG_ERROR("AuthServer unavailable socket close:"<<context.writeToJsonString());
        current->close();
    }

 }

int BrokerServantImp::processAuthReq(tars::TarsCurrentPtr current, const otim::ClientContext &context, const std::string & packId, const otim::LoginReq &loginReq)
{
    int ret = 0;
    try
    {
	    otim::AuthParam authParam;
        authParam.packId = packId;
        authParam.loginReq = loginReq;
        
        otim::AuthServantPrx authServantPrx = otim::getServantPrx<otim::AuthServantPrx>(PRXSTR_AUTH);
//        Application::getCommunicator()->stringToProxy("otim.AuthServer.AuthServantObj", authServantPrx);

        tars::TC_AutoPtr<UserAuthServantCallback> authSrvCB = new UserAuthServantCallback(current, context, authParam);
        
        authServantPrx->async_auth(authSrvCB, context, authParam);
    }
    catch (std::exception& e)
    {
        ret = otim::EC_SERVICE_UNAVAILABLE;
        MLOG_ERROR("SERVICEALARM authServantPrx->auth error:" << e.what());
    }
    catch (...)
    {
        ret = otim::EC_SERVICE_UNAVAILABLE;
        MLOG_ERROR("SERVICEALARM authServantPrx->auth unknown exception");
    }
    
    MLOG_DEBUG("recv loginreq Uid:" << context.writeToJsonString()<<" result:"<<ret);

    return ret;
}

//void BrokerServantImp::sendLoginAckToClient(tars::TarsCurrentPtr current,const std::string & clientId, int code, const otim::OTIMPack &req)
//{
//
//    otim::OTIMPack pack = req;
//    pack.header.flags |= otim::PF_ISACK;
//
//    otim::LoginResp loginResp;
//    loginResp.code = code;
//    loginResp.clientId = clientId;
//
//    otim::packTars<otim::LoginResp>(loginResp, pack.payload);
//
//    g_app.sendDataToClient(current, pack);
//}


void BrokerServantImp::processKickoutReq(tars::TarsCurrentPtr current, const otim::ClientContext &context, const otim::OTIMPack &req)
{
    if (context.clientId.empty()){
        MLOG_DEBUG("clientId is empty, maybe not auth:" << req.header.type <<" context:"<<context.writeToJsonString());
        current->close();
        return;
    }

}

void BrokerServantImp::processLogoutReq(tars::TarsCurrentPtr current, const otim::ClientContext &context, const otim::OTIMPack &req)
{
    if (!context.clientId.empty()){
        MLOG_DEBUG("clientId is empty, maybe not auth:" << req.header.type <<" context:"<<context.writeToJsonString());
        return;
    }
      
	current->close();

}



