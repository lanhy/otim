#include "BrokerPushServantImp.h"
#include "servant/Application.h"
#include "ScopeLogger.h"
#include "LonglinkManager.h"
#include "log.h"
#include "BrokerServer.h"

using namespace std;
extern BrokerServer g_app;

//////////////////////////////////////////////////////
void BrokerPushServantImp::initialize()
{
    //initialize servant here:
}

//////////////////////////////////////////////////////
void BrokerPushServantImp::destroy()
{
    //destroy servant here:
    //...
}

tars::Int32 BrokerPushServantImp::kickout(const otim::ClientContext & clientContext,tars::Int64 uid,tars::TarsCurrentPtr current)
{
    LongLinkPtr link = LongLinkManager::getInstance()->getLonglinkByUId(uid);
    if (link.get() == nullptr){
        MLOG_DEBUG("link is null:"<<uid);
    }

    MLOG_DEBUG("kickout uid:" << uid<<" clientId:"<<link->context.clientId<<" new Uid:"<<clientContext.uid);
    
    otim::OTIMPack pack;
    pack.header.type = otim::PT_KICKOUT;
    pack.header.version = link->ptclVersion;
    g_app.sendDataToClient(link->current, pack);

    MLOG_DEBUG("[KICKOUT]  uid:"<< link->context.uid << " clientId:"<< link->context.clientId << ", " << link->context.deviceType << " deviceId:" << link->context.deviceId);
    
    link->current->close();

    return 0;
}

tars::Int32 BrokerPushServantImp::push(tars::Int64 uid, const otim::OTIMPack & pack, tars::TarsCurrentPtr current)
{
    try
    {
        MLOG_DEBUG("uid:"<<uid<<" pack type:"<<otim::etos((otim::PACK_TYPE)pack.header.type)<<" header:"<<pack.header.writeToJsonString());

        LongLinkPtr longlink = LongLinkManager::getInstance()->getLonglinkByUId(uid);
        if (longlink.get() == nullptr){
            MLOG_DEBUG("longlink is null:"<<uid);
            return 0;
        }
        
        MLOG_DEBUG("longlink uid:" << longlink->context.uid<<" ptcl version:"<<longlink->ptclVersion);
        g_app.sendDataToClient(longlink->current, pack);

        MLOG_DEBUG("clientId:"<<longlink->context.clientId<<" type:"<<pack.header.type<<" payload size:" <<  pack.payload.size()<<" version:"<<pack.header.version);
    }
    catch (std::exception &e)
    {
        MLOG_ERROR("BrokerPushServantImp::push error:" << e.what());
    }
    catch (...)
    {
        MLOG_ERROR("BrokerPushServantImp::push error: unknown exception.");
    }
    return 0;
}

tars::Int32 BrokerPushServantImp::syncMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & pack,tars::TarsCurrentPtr current)
{
    try
    {
        std::vector<LongLinkPtr> listLink = LongLinkManager::getInstance()->getLongLinkByClientId(clientContext.clientId);
        
        MLOG_DEBUG("clientId:"<<clientContext.clientId<<" type:"<<pack.header.type<<" payload size:" <<  pack.payload.size()<<" version:"<<pack.header.version);
        for (auto longlink : listLink)
        {
            if (longlink.get() == nullptr){
                MLOG_DEBUG("longlink is null:"<<clientContext.clientId);
                continue;
            }
            if (longlink->context.uid == clientContext.uid){
                MLOG_DEBUG("the current is myself will ignore uid:" << longlink->context.uid<<" clientId:"<<clientContext.clientId);
                continue;
            }
            
            MLOG_DEBUG("longlink uid:" << longlink->context.uid<<" ptcl version:"<<longlink->ptclVersion);
            g_app.sendDataToClient(longlink->current, pack);
        }
    }
    catch (std::exception &e)
    {
        MLOG_ERROR("BrokerPushServantImp::push error:" << e.what());
    }
    catch (...)
    {
        MLOG_ERROR("BrokerPushServantImp::push error: unknown exception.");
    }
   
    return 0;
}

