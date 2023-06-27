//
//  RPCCallback.cpp
//  tarsim
//
//  Created by lanhy on 2022/3/22.
//

#include "RPCCallback.h"
#include "log.h"
#include "LonglinkManager.h"
#include "BrokerServer.h"
#include "otim_err.h"

extern BrokerServer g_app;

CommonServantPrxBrokerCallback::CommonServantPrxBrokerCallback(tars::TarsCurrentPtr current, const otim::ClientContext& context, const otim::OTIMPack& req)
{
    _current = current;
    _context = context;
    _req = req;
}

void CommonServantPrxBrokerCallback::callback_request(tars::Int32 ret, const otim::OTIMPack &resp)
{
    MLOG_DEBUG("rpc success ret:"<<ret<<"  context:"<<_context.writeToJsonString()<<" req:"<<_req.writeToJsonString()<<" resp:"<<resp.writeToJsonString());
   
    //send to client
    g_app.sendDataToClient(_current, resp);
}

void CommonServantPrxBrokerCallback::callback_request_exception(tars::Int32 ret)
{
    
    MLOG_DEBUG("rpc exception ret:"<<ret<<" context:"<<_context.writeToJsonString()<<" req:"<<_req.writeToJsonString());

//    g_app.sendDataToClient(_current, resp);

}


/**
 UserAuthServantCallback
 */
UserAuthServantCallback::UserAuthServantCallback(tars::TarsCurrentPtr current,const otim::ClientContext& clientContext, otim::AuthParam& param)
{
    _current = current;
    _authParam = param;
    _context = clientContext;

    _context.clientId = _authParam.loginReq.clientId;
    _context.deviceType = _authParam.loginReq.deviceType;
    _context.deviceId = _authParam.loginReq.deviceId;
    _context.brokerId = g_app.getBrokerId();
    
}


void UserAuthServantCallback::callback_auth(tars::Int32 ret, const std::string &extraData)
{
    MLOG_DEBUG("Auth result:"<<ret<<" extraData:"<<extraData<<" authParam:"<<_authParam.writeToJsonString());

    if (ret == 0)
    {
	    LongLinkManager::getInstance()->update(_context.uid, _context, _current);
    }

    UserAuthServantCallback::sendLoginAckToClient(_current, _authParam.packId, ret, _authParam.loginReq, extraData);
}

void UserAuthServantCallback::callback_auth_exception(tars::Int32 ret)
{
    MLOG_DEBUG("Auth callback_auth_exception:"<<ret<<" param:"<<_authParam.writeToJsonString());
   UserAuthServantCallback::sendLoginAckToClient(_current, _authParam.packId, otim::EC_SERVER_EXCEPTION, _authParam.loginReq, "RPC auth Exception");
    _current->close();
}

void UserAuthServantCallback::sendLoginAckToClient(tars::TarsCurrentPtr current,const std::string &packId, int code, const otim::LoginReq & loginReq, const std::string& extraData)
{
    
    otim::OTIMPack pack;
    pack.header.packId = packId;
    pack.header.type = otim::PT_LOGIN;
    pack.header.flags |= otim::PF_ISACK;
    
    otim::LoginResp loginResp;
    loginResp.errorCode.code = code;
    loginResp.clientId = loginReq.clientId;
    loginResp.extraData = extraData;
    
    otim::packTars<otim::LoginResp>(loginResp, pack.payload);
    
    g_app.sendDataToClient(current, pack);
}
