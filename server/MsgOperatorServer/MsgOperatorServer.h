#ifndef _MsgOperatorServer_H_
#define _MsgOperatorServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class MsgOperatorServer : public Application
{
public:
    /**
     *
     **/
    virtual ~MsgOperatorServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern MsgOperatorServer g_app;

////////////////////////////////////////////
#endif
