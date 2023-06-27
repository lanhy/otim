#ifndef GLOBALEVENT_H_
#define GLOBALEVENT_H_


#include <iostream>
//#include "DbPool.h"
#include "RedisPool.h"
#include "RedisDBInterface.h"
#include "util/tc_option.h"
#include "util/tc_clientsocket.h"
#include "otim.h"
#include "Common.h"
#include "ptcldefine.h"

using namespace std;

#define MLOG(logstr) try{   std::stringstream stream; stream <<"[T] "<<logstr; std::cout<<stream.str()<<std::endl;  }catch(...){std::cout<<"++++Excecption occur on log++++++++"<<std::endl;}

template<typename  T> otim::OTIMPack sendReq(tars::TC_TCPClient* tcpClient, uint16_t type, T * req){

    if (tcpClient == nullptr){
        MLOG("tcpClient is null!");
        return otim::OTIMPack();
    }
    
    otim::OTIMPack pack;
    pack.header.type = type;
    pack.header.version = 1;
    pack.header.packId = otim::genUUID();

    if (req != nullptr){
        tars::TarsOutputStream<tars::BufferWriter> tos;
        req->writeTo(tos);
        pack.payload = tos.getByteBuffer();
    }

    std::vector<char> data;
    otim::packTars<otim::OTIMPack>(pack, data);
 
    uint32_t packLen = 4 + data.size();
    packLen = htonl(packLen);

    tars::TC_NetWorkBuffer buffer(NULL);
    buffer.addBuffer((char*)&packLen, 4);
    buffer.addBuffer(data);
    std::vector<char> sendData = buffer.getBuffers();
    size_t recvLen = 0;
    int ret = tcpClient->send((const char*)sendData.data(), (size_t)sendData.size());
    MLOG("reqPack:"<<pack.header.writeToJsonString());

    recvLen = 4;
    otim::OTIMPack respPack;
    ret = tcpClient->recvLength((char*)&recvLen, recvLen);
    if (ret != 0){
        return respPack;
    }

    recvLen = ntohl(recvLen);
    if (recvLen < 4 || recvLen > 65535){
        return respPack;
    }
    
    recvLen -= 4;
    std::vector<char> recvData;
    recvData.resize(recvLen);
    ret = tcpClient->recvLength((char*)recvData.data(), recvLen);
   // std::cout<<"recv data ret:"<<ret<<" recvLen:"<<recvLen<<std::endl;
     
    otim::unpackTars<otim::OTIMPack>(recvData, respPack);
    MLOG("respPack:"<<respPack.header.writeToJsonString());
    
    return respPack;
}



class GlobalEnv : public testing::Environment
{

public:
    static GlobalEnv* instance(){
        static GlobalEnv* env = nullptr;
		if (env == nullptr){
			env = new GlobalEnv();
		}
		return env;
    }
    GlobalEnv()
    {
        _tcpClient = nullptr;
    }
    
    tars::TC_TCPClient* createTCPClient(){
        tars::TC_TCPClient* tcpClient = new tars::TC_TCPClient();
        tcpClient->init("10.250.0.112", 19000, 30000);
 
        return tcpClient;
    }
    
    
    virtual void SetUp()
    {
        _tcpClient = createTCPClient();
        _clientId = "100086";
        _friendId = "100087";

        this->login(_tcpClient);
        MLOG("GlobalEnv::SetUp");
    }
    virtual void TearDown()
    {
        MLOG("GlobalEnv::TearDown");
    }
    
    bool login(tars::TC_TCPClient *tcpClient)
    {
     // test
       otim::LoginReq loginReq;
       loginReq.clientId = _clientId;
       loginReq.userName = "testUser";
       loginReq.password = "testPwd";
       loginReq.deviceType = otim::DEVICE_IOS;
       loginReq.deviceId = "28900-2891-2829333@test";
       loginReq.version = 1;
       loginReq.deviceName = "Apple iOS for lhy";
        MLOG("login:"<<loginReq.writeToJsonString());

        otim::OTIMPack respPack = sendReq<otim::LoginReq>(tcpClient, otim::PT_LOGIN, &loginReq);
        otim::LoginResp loginResp;
        otim::unpackTars<otim::LoginResp>(respPack.payload, loginResp);
    //    ASSERT_EQ(loginResp.errorCode.code, 0);
        MLOG("loginResp:"<<loginResp.writeToJsonString());

        if (loginResp.errorCode.code != 0){
            return false;
        }
       return true;
    }
    
    tars::TC_TCPClient* getTcpClient(){
        return _tcpClient;
    }
    
    std::string getClientId(){
        return _clientId;
    }
    
    std::string getFriendId(){
        return _friendId;
    }

    void setGroupId(const std::string & groupId){
        _groupId = groupId;
    }
    std::string getGroupId(){
        return _groupId;
    }

private:
    tars::TC_TCPClient* _tcpClient;

    std::string _clientId;
    std::string _friendId;
    std::string _groupId;

};






#endif /* GLOBALEVENT_H_ */
