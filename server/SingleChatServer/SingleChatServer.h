#ifndef _SingleChatServer_H_
#define _SingleChatServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class SingleChatServer : public Application
{
public:
    /**
     *
     **/
    virtual ~SingleChatServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern SingleChatServer g_app;

////////////////////////////////////////////
#endif
