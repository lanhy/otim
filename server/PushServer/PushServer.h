#ifndef _PushServer_H_
#define _PushServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class PushServer : public Application
{
public:
    /**
     *
     **/
    virtual ~PushServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern PushServer g_app;

////////////////////////////////////////////
#endif
