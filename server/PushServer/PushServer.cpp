#include "PushServer.h"
#include "PushServantImp.h"

using namespace std;

PushServer g_app;

/////////////////////////////////////////////////////////////////
void
PushServer::initialize()
{
    //initialize application here:
    //...

    addServant<PushServantImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".PushServantObj");
}
/////////////////////////////////////////////////////////////////
void
PushServer::destroyApp()
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
