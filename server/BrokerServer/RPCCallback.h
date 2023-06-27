//
//  RPCCallback.hpp
//  tarsim
//
//  Created by lanhy on 2022/3/22.
//

#ifndef RPCCallback_hpp
#define RPCCallback_hpp

#include <stdio.h>
#include "otim.h"
#include "baseservant.h"
#include "AuthServant.h"


class CommonServantPrxBrokerCallback : public otim::BaseServantPrxCallback
{
public:

    otim::ClientContext _context;
    otim::OTIMPack _req;
    tars::TarsCurrentPtr _current;
public:
    CommonServantPrxBrokerCallback(tars::TarsCurrentPtr current, const otim::ClientContext& context, const otim::OTIMPack& req);
    
    virtual void callback_request(tars::Int32 ret, const otim::OTIMPack &resp);
    
    virtual void callback_request_exception(tars::Int32 ret);
};


class UserAuthServantCallback : public otim::AuthServantPrxCallback
{
public:
	otim::ClientContext _context;

    otim::AuthParam _authParam;
    tars::TarsCurrentPtr _current;
public:
    UserAuthServantCallback(tars::TarsCurrentPtr current,const otim::ClientContext& clientContext, otim::AuthParam& param);
    
    virtual void callback_auth(tars::Int32 ret, const std::string &resp);
    
    virtual void callback_auth_exception(tars::Int32 ret);
    
    static void sendLoginAckToClient(tars::TarsCurrentPtr current,const std::string &packId, int code, const otim::LoginReq & loginReq, const std::string& extraData);
};


#endif /* RPCCallback_hpp */
