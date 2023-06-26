#ifndef _OlapServantImp_H_
#define _OlapServantImp_H_

#include "servant/Application.h"
#include "OlapServant.h"

/**
 *
 *
 */
class OlapServantImp : public otim::OlapServant
{
public:
    /**
     *
     */
    virtual ~OlapServantImp() {}

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
    virtual tars::Int32 saveMsg(const otim::ClientContext & clientContext,const otim::OTIMPack & pack,const std::string & sessionId,tars::Int64 seqId,tars::TarsCurrentPtr _current_);
    
    tars::Int32 saveToMysql(const otim::ClientContext & clientContext,const otim::OTIMPack & pack,const std::string & sessionId,tars::Int64 seqId);

};
/////////////////////////////////////////////////////
#endif
