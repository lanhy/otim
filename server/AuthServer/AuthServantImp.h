#ifndef _AuthServantImp_H_
#define _AuthServantImp_H_

#include "servant/Application.h"
#include "AuthServant.h"

/**
 *
 *
 */
class AuthServantImp : public otim::AuthServant
{
public:
    /**
     *
     */
    virtual ~AuthServantImp() {}

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
	virtual tars::Int32 auth(const otim::ClientContext & clientContext,const otim::AuthParam & authParam,std::string &extraData,tars::TarsCurrentPtr _current_);
};
/////////////////////////////////////////////////////
#endif
