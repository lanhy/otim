#ifndef _AuthServer_H_
#define _AuthServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class AuthServer : public Application
{
public:
    /**
     *
     **/
    virtual ~AuthServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern AuthServer g_app;

////////////////////////////////////////////
#endif
