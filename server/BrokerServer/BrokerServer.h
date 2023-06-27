#ifndef _BrokerServer_H_
#define _BrokerServer_H_

#include <iostream>
#include "servant/Application.h"
#include "otim.h"
#include "ptcldefine.h"

using namespace tars;

/**
 *
 **/
class BrokerServer : public Application
{
public:
    /**
     *
     **/
    virtual ~BrokerServer() {};

    /**
     *
     **/
    virtual void initialize();

    /**
     *
     **/
    virtual void destroyApp();
    
    std::string getBrokerId();
    void sendDataToClient(tars::TarsCurrentPtr current, const otim::OTIMPack &req);    
private:
    std::string _brokerId;
};

extern BrokerServer g_app;

////////////////////////////////////////////
#endif
