#include "BrokerServer.h"
#include "BrokerServantImp.h"
#include "BrokerPushServantImp.h"
#include "log.h"
#include "RedisPool.h"
#include "otim_const.h"

using namespace std;

BrokerServer g_app;

const uint32_t PKG_MIN_SIZE = 4;
const uint32_t PKG_MAX_SIZE = 640000;

/////////////////////////////////////////////////////////////////

struct OTIMProtocol
{
    static TC_NetWorkBuffer::PACKET_TYPE parse(TC_NetWorkBuffer &in, vector<char> &out)
    {
        if (in.getBufferLength() < PKG_MIN_SIZE)
        {
            return TC_NetWorkBuffer::PACKET_LESS;
        }
        
        string header;
        in.getHeader(PKG_MIN_SIZE, header);
        
        assert(header.size() == PKG_MIN_SIZE);
        
        uint32_t packLen = 0;
        
        ::memcpy(&packLen, header.c_str(), PKG_MIN_SIZE);
        
        packLen = ntohl(packLen);
        
        MLOG_DEBUG("packLen:"<<packLen<<" buffer len:"<<in.getBufferLength());
        
        if (packLen > PKG_MAX_SIZE || packLen < PKG_MIN_SIZE)
        {
            throw TarsDecodeException("packet length too long or too short,len:" + TC_Common::tostr(packLen));
        }
        
        if (in.getBufferLength() < (uint32_t)packLen)
        {
            return TC_NetWorkBuffer::PACKET_LESS;
        }
        
        in.moveHeader(PKG_MIN_SIZE);
        
        in.getHeader(packLen-PKG_MIN_SIZE, out);
        in.moveHeader(packLen-PKG_MIN_SIZE);
        
        return TC_NetWorkBuffer::PACKET_FULL;
    }
};

void BrokerServer::initialize()
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
 
    addServant<BrokerServantImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".BrokerServantObj");
 
    addServant<BrokerPushServantImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".BrokerPushServantObj");

    
    
    //设置监听端口的协议解析器
    addServantProtocol(ServerConfig::Application + "." + ServerConfig::ServerName + ".BrokerServantObj", &OTIMProtocol::parse);
   MLOG_DEBUG("BrokerServer start ok!"); 
}


/////////////////////////////////////////////////////////////////
void BrokerServer::destroyApp()
{
    //destroy application here:
    //...
}

void BrokerServer::sendDataToClient(tars::TarsCurrentPtr current, const otim::OTIMPack &req)
{
    if (current.get() == nullptr)
    {
        MLOG_ERROR("current is null:"<<req.header.writeToJsonString());
        return;
    }
    
    std::vector<char> data;
    otim::packTars<otim::OTIMPack>(req, data);
    MLOG_DEBUG("pack:"<<req.header.writeToJsonString());
 
    uint32_t packLen = PKG_MIN_SIZE + data.size();
    MLOG_DEBUG("packLen:"<<packLen);
    packLen = htonl(packLen);

    TC_NetWorkBuffer buffer(NULL);
    buffer.addBuffer((char*)&packLen, PKG_MIN_SIZE);
    buffer.addBuffer(data);
    std::vector<char> sendData = buffer.getBuffers();
    
    MLOG_DEBUG("getBufferLength:"<<buffer.getBufferLength()<<" sendData size:"<<sendData.size());
    current->sendResponse(sendData.data(), sendData.size());
}

std::string BrokerServer::getBrokerId()
{
    if (!_brokerId.empty()){
        return _brokerId;
    }
    
    std::string key = "/tars/application/server/"+ServerConfig::Application + "." + ServerConfig::ServerName + ".BrokerPushServantObjAdapter";
    TC_Config & conf = Application::getConfig();
    auto strBrokerIdPrx = conf.get(key+"<servant>");
    if (strBrokerIdPrx.length() == 0){
        MLOG_ERROR("broker servant can't get: " << key);
        return "";
    }
    std::string endpoint = conf.get(key+"<endpoint>");
    if (endpoint.length() == 0){
        MLOG_ERROR("broker endpoint can't get: " << key);
        return "";
    }
    MLOG_DEBUG("strBrokerIdPrx: " << strBrokerIdPrx <<" endpoint:" << endpoint);
    _brokerId = strBrokerIdPrx+"@"+endpoint;
    MLOG_DEBUG("_strBrokerId: " << _brokerId);

    return _brokerId;
}

/////////////////////////////////////////////////////////////////
int main(int argc, char* argv[])
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
