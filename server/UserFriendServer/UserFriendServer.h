#ifndef _UserFriendServer_H_
#define _UserFriendServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class UserFriendServer : public Application
{
public:
    /**
     *
     **/
    virtual ~UserFriendServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern UserFriendServer g_app;

////////////////////////////////////////////
#endif
