#ifndef _GroupChatServer_H_
#define _GroupChatServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class GroupChatServer : public Application
{
public:
    /**
     *
     **/
    virtual ~GroupChatServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern GroupChatServer g_app;

////////////////////////////////////////////
#endif
