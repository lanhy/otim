#ifndef _OlapServer_H_
#define _OlapServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class OlapServer : public Application
{
public:
    /**
     *
     **/
    virtual ~OlapServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern OlapServer g_app;

////////////////////////////////////////////
#endif
