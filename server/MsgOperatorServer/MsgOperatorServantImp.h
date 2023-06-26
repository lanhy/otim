#ifndef _MsgOperatorServantImp_H_
#define _MsgOperatorServantImp_H_

#include "servant/Application.h"
#include "MsgOperatorServant.h"
#include "baseservant.h"
#include "otim.h"

/**
 *
 *
 */
class MsgOperatorServantImp : public otim::BaseServant
{
public:
    /**
     *
     */
    virtual ~MsgOperatorServantImp() {}
    
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
    virtual tars::Int32 request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr _current_);
    
private:
    tars::Int32 processMsgUnreadReq(const otim::ClientContext & clientContext,const otim::OTIMPack & reqPack,otim::OTIMPack &respPack);
    tars::Int32 processMsgCTRLReq(const otim::ClientContext & clientContext,const otim::OTIMPack & reqPack,otim::OTIMPack &respPack);

};
/////////////////////////////////////////////////////
#endif
