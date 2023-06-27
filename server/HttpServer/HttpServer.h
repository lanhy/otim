#ifndef _HttpServer_H_
#define _HttpServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class HttpServer : public Application
{
public:
    /**
     *
     **/
    virtual ~HttpServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern HttpServer g_app;

////////////////////////////////////////////
#endif
