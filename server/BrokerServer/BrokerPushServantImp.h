#ifndef _BorkerPushServantImp_H_
#define _BorkerPushServantImp_H_

#include "servant/Application.h"
#include "BrokerPushServant.h"

/**
 *
 *
 */
class BrokerPushServantImp : public otim::BrokerPushServant
{
public:
    /**
     *
     */
    virtual ~BrokerPushServantImp() {}

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();

	virtual tars::Int32 kickout(const otim::ClientContext & clientContext,tars::Int64 uid, tars::TarsCurrentPtr current);
	virtual tars::Int32 push(tars::Int64 uid,const otim::OTIMPack & pack,tars::TarsCurrentPtr current);
	virtual tars::Int32 syncMsg(const otim::ClientContext & clientContext,const otim::OTIMPack & pack,tars::TarsCurrentPtr current);

protected:

};
/////////////////////////////////////////////////////
#endif
