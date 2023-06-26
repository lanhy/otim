#ifndef _SingleChatServantImp_H_
#define _SingleChatServantImp_H_

#include "servant/Application.h"
//#include "SingleChatServant.h"
#include "baseservant.h"

/**
 *
 *
 */
class SingleChatServantImp : public otim::BaseServant
{
public:
    /**
     *
     */
    virtual ~SingleChatServantImp() {}

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();

    /**
     *
     */
    virtual tars::Int32 request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr current);
    
private:
    size_t _msgMaxLen;
    
    int saveMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req,const std::string & sessionId,tars::Int64 seqId);
    int updateSession(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::MsgReq &msgReq);
};
/////////////////////////////////////////////////////
#endif
