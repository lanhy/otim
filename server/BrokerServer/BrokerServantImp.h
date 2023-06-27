#ifndef _BorkerServantImp_H_
#define _BorkerServantImp_H_

#include "servant/Application.h"
#include "BorkerServant.h"
#include "otim.h"
#include "log.h"

/**
 * otim::BorkerServant
 *
 */
class BrokerServantImp : public Servant
{
public:
    /**
     *
     */
    virtual ~BrokerServantImp() {}

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();

protected:
    int doRequest(tars::TarsCurrentPtr current, vector<char>& response);
    int doClose(TarsCurrentPtr current);
   
    void processReqRPC(tars::TarsCurrentPtr current, const otim::ClientContext &context, const otim::OTIMPack &req);

    void processLoginReq(tars::TarsCurrentPtr current, const otim::ClientContext &context, const otim::OTIMPack &req);
    void processLogoutReq(tars::TarsCurrentPtr current, const otim::ClientContext &context, const otim::OTIMPack &req);
    void processKickoutReq(tars::TarsCurrentPtr current, const otim::ClientContext &context, const otim::OTIMPack &req);
    void processPingReq(tars::TarsCurrentPtr current, const otim::ClientContext &context, const otim::OTIMPack &req);

    int processAuthReq(tars::TarsCurrentPtr current, const otim::ClientContext &context, const std::string & packId, const otim::LoginReq &loginReq);
//    void sendLoginAckToClient(tars::TarsCurrentPtr current,const std::string & clientId, int code, const otim::OTIMPack &req);
    
    std::map<int, std::string> _mapServerName;//保存服务名称跟type的对应关系

};
/////////////////////////////////////////////////////
#endif
