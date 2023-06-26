#ifndef _HistoryMsgServer_H_
#define _HistoryMsgServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace tars;

/**
 *
 **/
class HistoryMsgServer : public Application
{
public:
    /**
     *
     **/
    virtual ~HistoryMsgServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
};

extern HistoryMsgServer g_app;

////////////////////////////////////////////
#endif
