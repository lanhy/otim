#include "MsgOperatorServer.h"
#include "MsgOperatorServantImp.h"
#include "RedisPool.h"
#include "otim_const.h"

using namespace std;

MsgOperatorServer g_app;

/////////////////////////////////////////////////////////////////
void
MsgOperatorServer::initialize()
{
    //initialize application here:
    TENLOCAL(false);
    bool ret = addAppConfig(otim::CONF_REDIS);
    MLOG_DEBUG("addAppConfig:"<<otim::CONF_REDIS<<" ret:"<<ret);

    std::string redisConf = ServerConfig::BasePath+otim::CONF_REDIS;
    MLOG_DEBUG("redisConf File:"<<redisConf);
  
    ret = addAppConfig(otim::CONF_MYSQL);
 
 
    ret = addConfig(ServerConfig::ServerName + ".conf");
    std::string serverConfFile = ServerConfig::BasePath+ServerConfig::ServerName+".conf";

    MLOG_DEBUG("addConfig:"<<serverConfFile);
    TC_Config myConf;
    myConf.parseFile(serverConfFile);
    
    int redisConnCount = TC_Common::strto<int>(myConf.get("/otim/pool<redis>", "5"));

    otim::initRedisPool(redisConnCount, redisConf);


    addServant<MsgOperatorServantImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".MsgOperatorServantObj");
}
/////////////////////////////////////////////////////////////////
void
MsgOperatorServer::destroyApp()
{
    //destroy application here:
    //...
}
/////////////////////////////////////////////////////////////////
int
main(int argc, char* argv[])
{
    try
    {
        g_app.main(argc, argv);
        g_app.waitForShutdown();
    }
    catch (std::exception& e)
    {
        cerr << "std::exception:" << e.what() << std::endl;
    }
    catch (...)
    {
        cerr << "unknown exception." << std::endl;
    }
    return -1;
}
/////////////////////////////////////////////////////////////////
