#ifndef _HistoryMsgServantImp_H_
#define _HistoryMsgServantImp_H_

#include "servant/Application.h"
#include "baseservant.h"
#include "otim.h"

/**
 *
 *
 */
class HistoryMsgServantImp : public otim::BaseServant
{
public:
    /**
     *
     */
    virtual ~HistoryMsgServantImp() {}

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
    tars::Int32 processHotsessionReq(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp);
    tars::Int32 processPullHistoryReq(const otim::ClientContext & clientContext,const otim::OTIMPack & reqPack,otim::OTIMPack &respPack);
    tars::Int32 processHighPriorMsgSyncReq(const otim::ClientContext & clientContext,const otim::OTIMPack & reqPack,otim::OTIMPack &respPack);

    
    bool isAllowPullMsg(const std::string &clientId, const std::string &sessionId);
    
    size_t _maxHotSessionCount;
};
/////////////////////////////////////////////////////
#endif
