#ifndef _BizMsgServantImp_H_
#define _BizMsgServantImp_H_

#include "servant/Application.h"
//#include "BizMsgServant.h"
#include "baseservant.h"

/**
 *
 *
 */
class BizMsgServantImp : public otim::BaseServant
{
public:
    /**
     *
     */
    virtual ~BizMsgServantImp() {}

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
//    int savePriorityMsg(const otim::OTIMPack & pack,const std::string & to,tars::Int64 seqId);
    int updateSession(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::MsgReq &msgReq);
    int dispatchMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req, const std::string &to);
};
/////////////////////////////////////////////////////
#endif
