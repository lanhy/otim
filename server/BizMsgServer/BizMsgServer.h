#ifndef _BizMsgServer_H_
#define _BizMsgServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class BizMsgServer : public Application
{
public:
    /**
     *
     **/
    virtual ~BizMsgServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern BizMsgServer g_app;

////////////////////////////////////////////
#endif
