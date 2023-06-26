#ifndef _PushServantImp_H_
#define _PushServantImp_H_

#include "servant/Application.h"
#include "PushServant.h"

/**
 *
 *
 */
class PushServantImp : public otim::PushServant
{
public:
    /**
     *
     */
    virtual ~PushServantImp() {}

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
    virtual int test(tars::TarsCurrentPtr current) { return 0;};
};
/////////////////////////////////////////////////////
#endif
