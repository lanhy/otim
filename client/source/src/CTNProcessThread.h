//
//  CTNDataProcessThread.h
//  OMTP
//
//  Created by 兰怀玉 on 16/4/14.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef CTNDataProcessThread_h
#define CTNDataProcessThread_h

#include "CThreadEx.h"
#include "CSafeQueue.h"

#include "otim.h"
#include "CLog.h"

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include "OMTPConst.h"
#include "OMTPResponse.h"
#include "CTNPacket.h"

class CTNSendRecvThread;
class TNPacketBuffer;
class CTcpSocket;
class OMTPHostInfo;
class InflightMessage;


struct TNPacket{
    int16_t type;
    int64_t param;
    otim::OTIMPack packData;
    
    TNPacket(){
        type  = 0;
        param = 0;
//        packetBuffer = nullptr;
    }
};


std::string getReqId();

#define REQ_ID getReqId()



const uint16_t OMTP_PACKET_TYPE_SEND = 1;
const uint16_t OMTP_PACKET_TYPE_RECV = 2;
const uint16_t OMTP_PACKET_TYPE_LOGOUT = 3;
const uint16_t OMTP_PACKET_TYPE_SOKCET_CLOSED = 4;
const uint16_t OMTP_PACKET_TYPE_SOKCET_FAILED = 5;
const uint16_t OMTP_PACKET_TYPE_SOKCET_SUCCESS = 6;



class CTNProcessThread : public CThreadEx
{
public:
    CTNProcessThread(IOMTPResponse *resp);
    ~CTNProcessThread();
  
    virtual void run();
    
    void setClientInfo(std::vector<OMTPHostInfo> & vctHostInfo, const char* clientId, int appType, const char* version, int deviceType, int userType = 1, int authType = 1);
    void setUserInfo(const char* username, const char* password, const char* deviceId,const char* deviceToken);
    void setAppPath(const char* appPath);
    
    void postPacket(TNPacket &packet);
  
    OMTPNetStatus getNetStatus();
    
    void login();
    void logout(bool notifyServer);
    
    void sendMsgReq(uint16_t type, otim::MsgReq& req, bool inflight);

    template<typename  T> void sendReq(uint16_t type, T * req){
        if (this->isLogout()){
             OMTPLOG("sendReq logout status,give up type:"<<type);
             return;
         }

        OMTPLOG("sendReq type:"<<type);

        otim::OTIMPack pack;
        pack.header.type = type;
        pack.header.version = PTCL_VERSION;
        pack.header.packId = getReqId();

        if (req != nullptr){
            tars::TarsOutputStream<tars::BufferWriter> tos;
            req->writeTo(tos);
            pack.payload = tos.getByteBuffer();
            OMTPLOG("req content:"<<req->writeToJsonString());
        }
   
        sendPacket(pack);
    }
        
private:
    
    bool currSocketValid();
    void createSocketThread();
    void processSuccessRSThread(int theadId);
    void destoryRSThread(bool forceStop);
    void forceDestoryAllThread();
    
    void reconnect();
    bool isLogout();
    
 
    void sendPacket(uint16_t type, const char *payload, size_t length);
    void sendPacket(otim::OTIMPack &pack);
    void sendAck(int type, otim::MsgReq* req);
    void checkPing();
    void reqHotSession(int64_t timestamp);

    void forceResendInflight();
    void checkRecvCache();
 
    void processRecvPack(otim::OTIMPack &pack);
    void processRecvMsgReq(otim::OTIMPack &pack);
//    void processSessionStatusReq(TNPacketBuffer* packetBuffer);
    void processMsgAck(otim::OTIMPack &pack);
    void processLoginResp(otim::OTIMPack &pack);
    void processPullHistoryMsgResp(otim::OTIMPack &pack);
    void processHotSessionResp(otim::OTIMPack &pack);
    
    void processSyncSessionStatusResp(otim::OTIMPack &pack);
    void processUserOnlineStatus(otim::OTIMPack &pack);
  
    void processKickout();
  
    //friend group userinfo
    void processFriendsResp(otim::OTIMPack &pack);
    void processGetUserinfoResp(otim::OTIMPack &pack);
    void processMyGroupchatResp(otim::OTIMPack &pack);
    void processGroupchatMemberResp(otim::OTIMPack &pack);
    void processGroupInfoResp(otim::OTIMPack &pack);

    
    void sendPacket();
    
    virtual void setNetStatus(OMTPNetStatus status);
    
    void daInfo(string attrName, map<string, string> propertyJson =  map<string, string>());
private:
    
    IOMTPResponse *m_response;
    int64_t m_pingReqTime;
    int m_reconnects;
    bool m_offMsgCountOk;//离线消息数是否已经获取，断线重连需要复位
 
    std::mutex m_mutexNetStatus;
    OMTPNetStatus m_netStatus;
    
    //跑马策略保存地址
//    std::mutex m_mutexRSThread;
    std::vector<CTNSendRecvThread*> m_vctRSThread;
    CTNSendRecvThread *m_pSRThread;
   
    CSafeQueue<TNPacket> m_queuePacket;
    
    std::vector<InflightMessage*> _recvCache;
    
    
 };

#endif /* CTNDataProcessThread_hpp */
