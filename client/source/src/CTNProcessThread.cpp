//
//  CTNDataProcessThread.cpp
//  OMTP
//
//  Created by 兰怀玉 on 16/4/14.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#include "CTNProcessThread.h"
#include  <mutex>
#include <thread>
#include <time.h>

#include "CTNSendRecvThread.h"
#include "otim.h"
#include "otim_err.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <regex>
#include <random>
#include "CTNInflightVector.h"
#include "OMTPConst.h"
#include "CLog.h"

//#define REQ_ID getReqId()

std::string getReqId(){
//    char buf[64] = {0};
//    static int s_reqId = 1000;
//    s_reqId++;
    std::uniform_int_distribution<uint64_t> dis;
    
    std::random_device rd;
    stringstream stff;
    stff<<rd();
//    sprintf(buf, "%lld", rd());
    
    return  stff.str();
}

string intToString(int i)
{
    stringstream stff;
    stff<<i;
 
    return stff.str();
}

CTNProcessThread::CTNProcessThread(IOMTPResponse *resp):m_response(resp)
{
    OMTPLOG("CTNProcessThread::CTNProcessThread:"<<getId());
    
    m_pingReqTime = 0;
    m_reconnects = 0;
    m_offMsgCountOk = false;
    m_netStatus = OMTPNetConnecting;
    
    m_pSRThread = NULL;
}


CTNProcessThread::~CTNProcessThread()
{
    OMTPLOG("CTNProcessThread::~CTNProcessThread enter:"<<this->getId());
    this->forceDestoryAllThread();
    
    OMTPLOG("CTNProcessThread::~CTNProcessThread exit:"<<this->getId());
}


void CTNProcessThread::setNetStatus(OMTPNetStatus status)
{
//    std::unique_lock<std::mutex> unilock(m_mutexNetStatus);
    CTNCMutexGuard guard(m_mutexNetStatus, "setNetStatus");

    if (m_netStatus == status) {
        return;
    }
    
    m_netStatus = status;
    
    OMTPLOG("setNetStatus:"<<m_netStatus<<" thread:"<<this->getId());
    
    if (m_response != NULL) {
        m_response->netStatusChanged(status);
    }
    else {
        OMTPLOG("m_response is NULL don't callback to caller netstatus:"<<m_netStatus);
    }
}

OMTPNetStatus CTNProcessThread::getNetStatus()
{
//    std::unique_lock<std::mutex> unilock(m_mutexNetStatus);
    CTNCMutexGuard guard(m_mutexNetStatus, "getNetStatus");

    OMTPLOG("getNetStatus :"<<m_netStatus<<" thread:"<<this->getId());
    
    return m_netStatus;
}

bool CTNProcessThread::isLogout()
{
    CTNCMutexGuard guard(m_mutexNetStatus, "isLogout");
    if (m_netStatus == OMTPNetLogout){
        return true;
    }

    return false;
}


void CTNProcessThread::postPacket(TNPacket &packet)
{
    OMTPLOG("postPacket type:"<<packet.type);
    
    m_queuePacket.push(packet);
}

void CTNProcessThread::createSocketThread()
{
    OMTPLOG("createSocketThread enter"<<" thread:"<<this->getId());
    
    if (m_pSRThread != NULL) {
        m_pSRThread->stop();
        m_pSRThread = NULL;
    }
    
    OMTPLOG("createSocketThread 1"<<" thread:"<<this->getId());
    
    OMTPHostInfo hostInfo = CONFIG->hostInfo();
    for (int i = 0; i < 1; i++)
    {
        CTNSendRecvThread *pSRThread = new CTNSendRecvThread(this);
        pSRThread->setHostInfo(hostInfo);
        
        pSRThread->start();
        
        m_vctRSThread.push_back(pSRThread);
        OMTPLOG("start RSThread:"<<pSRThread->getId());
        
    }
    OMTPLOG("createSocketThread 3"<<" thread:"<<this->getId());

    m_pingReqTime = 0;
}

void CTNProcessThread::processSuccessRSThread(int threadId)
{
    OMTPLOG("processSuccessRSThread:"<<threadId);
    if (m_pSRThread != NULL) {
        OMTPLOG("TCP CONNECT SUCCESS RS Thread:"<<threadId<<" current RSThread:"<<m_pSRThread->getId());
        return ;
    }
    
    
    for (int i = 0; i < m_vctRSThread.size(); i++)
    {
        CTNSendRecvThread* pThread = m_vctRSThread[i];
        if (pThread->getId() == threadId) {
            m_pSRThread = pThread;
           
            //need dataInfo
#ifndef WIN32
            map<string, string> property;
            if (m_pSRThread->hostInfo().isSSL){
                property["channel_type"] = "SSL加密";
            }
            else{
                property["channel_type"] = "无加密";
            }
            
            this->daInfo("MNetworkSuccess", property);
#endif
            OMTPLOG("SELECT TCP CONNECT SUCCESS RS Thread:"<<m_pSRThread->getId());
        }
        else{
            pThread->stop();
        }
    }
}


void CTNProcessThread::destoryRSThread(bool forceStop)
{
    OMTPLOG("ENTER destoryRSThread:"<<forceStop<<" size:"<<m_vctRSThread.size());

    if (this->getNetStatus() == OMTPNetConnecting){
        OMTPLOG("destoryRSThread  OMTPNetConnecting don't destory:"<<m_vctRSThread.size());
        return;
    }
    
    if (m_vctRSThread.size() == 0) {
        OMTPLOG("m_vctRSThread size:"<<m_vctRSThread.size());
        return ;
    }
    if (m_pSRThread != NULL && forceStop) {
        m_pSRThread->stop();
        m_pSRThread = NULL;
    }

    OMTPLOG("destoryRSThread:"<<forceStop);

    std::vector<CTNSendRecvThread*>::iterator itTmp = m_vctRSThread.begin();
    while (itTmp != m_vctRSThread.end()){
        CTNSendRecvThread* pThread = *itTmp;
        if (pThread == m_pSRThread){
            OMTPLOG("current Thread don't erase:"<<pThread->getId());
            itTmp ++;
            continue;
        }
        
        if (pThread->isDied()) {
            OMTPLOG("destoryRSThread will delete thread:"<<pThread->getId());
            delete pThread;
            itTmp = m_vctRSThread.erase(itTmp);
            continue;
        }
        
        pThread->stop();
        OMTPLOG("RSThread is not Died:"<<pThread->getId());
     
        itTmp ++;
    }

    OMTPLOG("EXIT destoryRSThread");
}

//退出时强制退出所有线程，最多重试1分钟
void CTNProcessThread::forceDestoryAllThread()
{
    OMTPLOG("CTNProcessThread::forceDestoryAllThread enter:"<<this->getId());

    for (int i = 0; i < 10; i++) {
        if (m_vctRSThread.empty()) {
            break;
        }

        this->destoryRSThread(true);

        tn_msleep(3000);
    }
    
    OMTPLOG("CTNProcessThread::forceDestoryAllThread exit:"<<this->getId());
    
}

//记录，如果超过两个keeplive 未收到CONNACK， 重连
//m_pingReqTime > 0 表示已经调用过login；
void CTNProcessThread::run()
{
    OMTPLOG("start CTNProcessThread:"<<getId());

    this->setNetStatus(OMTPNetConnecting);
    //重连reset index
    CONFIG->resetCurrHostIndex();
 
    this->createSocketThread();
    
    auto lambdaCheckInflight = [this](bool isResend, InflightMessage* inflightMsg)->void{
        if (isResend){
            inflightMsg->req.header.flags |= otim::PF_ISDUP;
//            this->sendMsgReq(inflightMsg->req.header.type, inflightMsg->req, false);
        }
        else if (m_response != NULL){
//            m_response->msgAck(inflightMsg->req.session_id, inflightMsg->req.header.packId, 0, 0, -1);
        }
    };
    
    while (isRunning()) {
        if (this->isLogout()) {
            OMTPLOG("getNetStatus is OMTPNetLogout, exit Process trhead!");
            break;
        }
        
        TNPacket packet;
        int ret = m_queuePacket.pop(packet, CONFIG->keepAlive());
        if (!this->isRunning()){
            OMTPLOG("stoped!!, exit Process trhead!"<<this->isRunning());
            break;
        }
        
        if (this->getNetStatus() == OMTPNetConnected) {
            INFLIGHT_VECTOR->checkInflight(false, lambdaCheckInflight);
        }
        else if (this->getNetStatus() == OMTPNetNone){
            this->reconnect();
        }
        
        if (ret == SFQUEUE_TIMEOUT) {
            
            OMTPLOG("Wait time out, will checkPing");
            
            this->checkPing();
            
            //check RSThread
            this->destoryRSThread(false);
            continue;
        }

        OMTPLOG("PACKET Process type:"<<packet.type<<" param:"<<packet.param);
        switch (packet.type) {
            case OMTP_PACKET_TYPE_RECV:
            {
                this->processRecvPack(packet.packData);
                
                break;
            }
            case OMTP_PACKET_TYPE_SEND:
            {
                if (m_pSRThread != nullptr){
                    m_pSRThread->sendPacket(packet.packData);
                }
                else{
                    this->reconnect();
                }
            
                break;
            }
            case OMTP_PACKET_TYPE_SOKCET_SUCCESS:
            {
                int threadId = (int)packet.param;
                OMTPLOG("OMTP_PACKET_TYPE_SOKCET_SUCCESS:"<<threadId);
                this->processSuccessRSThread(threadId);
                
                if (m_pingReqTime == 0) {
                    m_pingReqTime = time(NULL);
                    this->login();
                }
                
                break;
            }
            case OMTP_PACKET_TYPE_LOGOUT:
            {
                OMTPLOG("OMTP_PACKET_TYPE_LOGOUT;");
//                setNetStatus(OMTPNetLogout);

                if (m_pSRThread != nullptr){
                    if (packet.param != 0){
                        //用户主动退出时需要置空，防止消息串用户
                        m_response = NULL;
                        
                        otim::OTIMPack packet;
                        packet.header.type = otim::PT_LOGOUT;
                        packet.header.version = PTCL_VERSION;
                        m_pSRThread->sendPacket(packet);
                        OMTPLOG("SEND TYPE_DISCONNECT TO SERVER");
                    }

                    tn_msleep(300);//延时关闭socket，避免与上次发送消息时间间距过短导致服务端无法处理之前的消息的情况 wxy
                    
                    m_pSRThread->stop();
                }
                
                setNetStatus(OMTPNetLogout);
                m_reconnects = 0;
                
                goto threadStop;
                break;
            }
            case OMTP_PACKET_TYPE_SOKCET_FAILED:
            {
                OMTPLOG("OMTP_PACKET_TYPE_SOKCET_FAILED;");
                
                this->daInfo("MNetworkFail");

                this->setNetStatus(OMTPNetNone);
                this->reconnect();
                break;
            }
            case OMTP_PACKET_TYPE_SOKCET_CLOSED:
            {
                //增加判断解决Android 黑屏后连接上后闪断频繁重连问题
                OMTPLOG("CTNProcessThread OMTP_PACKET_TYPE_SOKCET_CLOSED RSThread:"<<packet.param);
                if (m_pSRThread != nullptr && packet.param == m_pSRThread->getId()){
                    if (this->getNetStatus() != OMTPNetLogout) {
                        this->setNetStatus(OMTPNetNone);
                    }
                    
                    this->reconnect();
                }
             
                break;
            }
                
            default:
                OMTPLOG("TCP can't parse on type:"<<packet.type);
                break;
        }
    }
    
threadStop:
    OMTPLOG("CTNProcessThread exit 1:"<<getId());
    
    try {
        m_response = NULL;
        //set run flag
        this->stop();
        this->setNetStatus(OMTPNetNone);
        
        this->forceDestoryAllThread();
    } catch (...) {
        OMTPLOG("Exception occur on CTNProcessThread run :"<<getId());
    }
    
    OMTPLOG("CTNProcessThread exit 2:"<<getId());
}

void CTNProcessThread::setClientInfo(std::vector<OMTPHostInfo> & vctHostInfo, const char* clientId, int appType, const char* version, int deviceType, int userType, int authType)
{
    CONFIG->setHostInfos(vctHostInfo);

    CONFIG->clientId(clientId);
    CONFIG->deviceType(deviceType);
    CONFIG->version(version);

    CONFIG->appType(appType);
}



void CTNProcessThread::setUserInfo(const char* username, const char* password, const char* deviceId, const char* deviceToken)
{
    if (deviceToken != NULL) {
        OMTPLOG("setUserInfo  deviceToken:"<<deviceToken);
    }
    
    CONFIG->username(username);
    CONFIG->password(password);
    CONFIG->deviceId(deviceId);
    
    this->setNetStatus(OMTPNetConnecting);
}

void CTNProcessThread::setAppPath(const char* appPath)
{
    if (appPath == NULL){
        return;
    }
    
    CONFIG->appPath(appPath);
}

void CTNProcessThread::login()
{
    otim::LoginReq loginReq;
    loginReq.clientId = CONFIG->clientId();
    loginReq.userName = CONFIG->username();
    loginReq.password = CONFIG->password();
    loginReq.deviceType = (otim::DEVICE_TYPE)CONFIG->deviceType();
    loginReq.deviceId = CONFIG->deviceId();
    loginReq.version = CONFIG->version();

    if (loginReq.password.length() == 0){
        OMTPLOG("CTNProcessThread::login password is EMPTY!!!");
    }

    OMTPLOG("CTNProcessThread::login:"<<loginReq.writeToJsonString());
    sendReq<otim::LoginReq>(otim::PT_LOGIN, &loginReq);
}



void CTNProcessThread::logout(bool notifyServer)
{
    OMTPLOG("CTNProcessThread::logout notifyServer:"<<notifyServer<<" thread:"<<getId());
    //以下操作放到异步队列处理之后，该处若提前设置状态，导致后面发送同步未读数操作由于状态原因无法发送 wxy
//    setNetStatus(OMTPNetLogout);
//    m_reconnects = 0;
    
    setNetStatus(OMTPNetLogouting);

    TNPacket packet;
    packet.type = OMTP_PACKET_TYPE_LOGOUT;
    packet.param = notifyServer;
    m_queuePacket.push(packet);
}



void CTNProcessThread::reconnect()
{
    if (getNetStatus() == OMTPNetConnecting){
        OMTPLOG("OMTPNetConnecting don't connect"<<getId());
        return;
    }
    
    setNetStatus(OMTPNetConnecting);
    
    m_reconnects ++;
    
    int sleepTime = RECONN_STEP[m_reconnects%RECONN_MAX];
    
    OMTPLOG("CTNProcessThread reconnect:"<<sleepTime);
	tn_msleep(sleepTime*1000);
   
    OMTPLOG("will reconnect:"<<getId());
    createSocketThread();
    
    this->daInfo("BreakReconnection");
}

void CTNProcessThread::sendMsgReq(uint16_t type, otim::MsgReq& req, bool inflight)
{
    //修正时间，防止不必要的重发
    req.timestamp = time(NULL);
    tars::TarsOutputStream<tars::BufferWriter> tos;
    req.writeTo(tos);

    otim::OTIMPack pack;
    pack.header.packId = getReqId();
    pack.header.type = type;
    pack.header.version = PTCL_VERSION;
    pack.payload = tos.getByteBuffer();

//    离线未拉完，不能发送消息，否则离线消息数会发生错误
    this->sendPacket(pack);
//    sendPacket(type, tos.getBuffer(), tos.getLength());
    
    if (inflight) {
        //增加
        InflightMessage* inflightMsg = new InflightMessage();
        inflightMsg->req = pack;
        
        INFLIGHT_VECTOR->addToInflight(inflightMsg);
    }

}

//PT_NONE       = 0,      //00-09 保留
//PT_PING       = 10,     //心跳
//PT_LOGIN      = 11,     //登录认证
//PT_LOGOUT      = 12,    //登出
//PT_KICKOUT     = 13,    //踢出
//PT_MSG_SINGLE_CHAT  = 14,//单聊消息
//PT_MSG_GROUP_CHAT   = 15,//群聊消息
//PT_MSG_BIZ_NOTIFY   = 16,//业务通知消息
//PT_MSG_CTRL      = 17,  //消息控制指令（比如撤销，删除等）
//PT_MSG_READ      = 18,  //消息已读
//PT_HOTSESSION_SYNC     = 19, //热会话同步
//PT_HIGH_PRIOR_MSG_SYNC     = 20,  //高优先级消息同步
//PT_HISTORY_MSG_PULL        = 21, //拉历史消息
//PT_SESSION_MONITOR_START   = 22,//监控会话开始
//PT_SESSION_MONITOR_STOP    = 23,//监控会话结束
//PT_SESSION_MONITOR_SYNC = 24,   //监控会话同步（同步登陆用户的会话）
//PT_SYNC_DATA_CMD      = 25, //同步数据指令,根据该指令可以客户端可以进行各种增量数据同步（比如会话，好友记录，群聊等）
//
//PT_GROUPCHAT_SYNC      = 40,    //同步我的群(我创建，加入，监听的群里)
//PT_GROUPCHAT_CREATE    = 41,    //创建群聊
//PT_GROUPCHAT_JION      = 42,    //加入群聊
//PT_GROUPCHAT_QUIT      = 43,    //退出群聊
//PT_GROUPCHAT_DISMISS   = 44,    //解散群聊
//PT_GROUPCHAT_UPDATE_CREATOR  = 45, //换群主
//PT_GROUPCHAT_INFO_UPDATE     = 46, //更新群资料
//PT_GROUPCHAT_MEMBERS_GET     = 47,  //获取群成员
//
//PT_FRIEND_ADD      = 51,    //添加好友
//PT_FRIEND_DEL      = 52,    //删除好友
//PT_FRIEND_SYNC     = 53,    //同步好友
//
//PT_USERINFO_GET            = 54, //获取用户信息
//PT_USERINFO_UPDATE         = 55, //用户信息更新
//PT_USERATTRIBUTE_SET      = 56, //设置用户属性，比如昵称
//PT_USERATTRIBUTE_GET      = 57, //获取用户属性，比如昵称
//PT_SESSIONATTRIBUTE_SET   = 58, //设置会话属性，比如置顶，免打扰
//
void CTNProcessThread::processRecvPack(otim::OTIMPack &pack)
{
   
    OMTPLOG("processPacketMessage:"<<pack.header.writeToJsonString());
    
    switch (pack.header.type) {
        case otim::PT_MSG_BIZ_NOTIFY:
        case otim::PT_MSG_SINGLE_CHAT:
        case otim::PT_MSG_GROUP_CHAT:
        {
            m_pingReqTime = 0;
            if (pack.header.flags & otim::PF_ISACK){
                this->processMsgAck(pack);
            }
            else{
                this->processRecvMsgReq(pack);
            }
            
            break;
        }
        case otim::PT_SYNC_DATA_CMD:
        {
            m_pingReqTime = 0;
//            this->processRecvMsgReq(pack);
            
            break;
        }
        case otim::PT_HOTSESSION_SYNC:
        {
            m_pingReqTime = 0;
            
            this->processHotSessionResp(pack);
            break;
        }
        case otim::PT_GROUPCHAT_SYNC:
        {
            m_pingReqTime = 0;
            
//            this->processMyGroupchatResp(pack);
            break;
        }
        case otim::PT_FRIEND_SYNC:
        {
            m_pingReqTime = 0;

//            this->processFriendsResp(pack);
            break;
        }
        case otim::PT_USERINFO_GET:
        {
            m_pingReqTime = 0;

//            this->processGetUserinfoResp(pack);
            break;
        }

        case otim::PT_HISTORY_MSG_PULL:
        {
            m_pingReqTime = 0;
//            this->processPullHistoryMsgResp(pack);
            
            break;
        }
        case otim::PT_PING:
        {
            OMTPLOG("PING RECV");
            m_pingReqTime = 0;
            break;
        }
        case otim::PT_LOGIN:
        {
            this->processLoginResp(pack);
            
            CONFIG->resetCurrHostIndex();
            
            break;
        }
        case otim::PT_KICKOUT:
        {
            m_pingReqTime = 0;
            this->processKickout();
            break;
        }
    }
    
}

void CTNProcessThread::processRecvMsgReq(otim::OTIMPack& pack)
{
    
    otim::MsgReq msgReq;
    tars::TarsInputStream<tars::BufferReader> is;
    
    is.setBuffer(pack.payload);
    
    msgReq.readFrom(is);
    
    OMTPLOG("processRecvMsgReq:"<<msgReq.writeToJsonString());
    
    if (m_response == nullptr || !m_offMsgCountOk){
        InflightMessage* inflightMsg = new InflightMessage();
//        inflightMsg->type = packetBuffer->type;
//        inflightMsg->req = msgReq;
        
        _recvCache.push_back(inflightMsg);
        
        OMTPLOG("processRecvMsgReq m_offMsgCountOk is false ,recving msg will be cached!!!");
        return;
    }
    
    //call back to caller
    m_response->msgRecv(pack.header.type,pack.header.packId, &msgReq);
    
    //ack
//    sendAck(packetBuffer->type, msgReq);
    
//    delete msgReq;
}


void CTNProcessThread::processMsgAck(otim::OTIMPack& pack)
{
    otim::MsgAck msgAck;
    tars::TarsInputStream<tars::BufferReader> is;
    
    is.setBuffer(pack.payload);
    
    msgAck.readFrom(is);
    
    OMTPLOG("processMsgAck:"<<msgAck.writeToJsonString());
    
    //call back to caller
    if (m_response != NULL) {
        m_response->msgAck(pack.header.packId, &msgAck);
    }
    else{
        OMTPLOG("processMsgAck m_response is NULL:"<< pack.header.packId);
    }

    INFLIGHT_VECTOR->removeInflightMutex(pack.header.packId);
}

void CTNProcessThread::processLoginResp(otim::OTIMPack& pack)
{
    otim::LoginResp loginResp ;
    tars::TarsInputStream<tars::BufferReader> is;
    
    is.setBuffer(pack.payload);
    loginResp.readFrom(is);
    OMTPLOG("Login RESP:"<<loginResp.writeToJsonString());
    
    //call back to caller
    if (m_response != NULL) {
        m_response->loginResp(loginResp.errorCode.code, loginResp.clientId, loginResp.extraData);
    }
    
    if (loginResp.errorCode.code == otim::EC_SUCCESS) {
        setNetStatus(OMTPNetRecving);
        
        if (loginResp.clientId.length() > 0){
            CONFIG->clientId(loginResp.clientId.c_str());
        }
        
        m_offMsgCountOk = false;
   
        this->reqHotSession(CONFIG->hsTimestamp());
  
        m_reconnects = 0;
        
    }
    else if (loginResp.errorCode.code == otim::EC_SERVICE_UNAVAILABLE){
        setNetStatus(OMTPNetNone);
    }
    else
    {
        setNetStatus(OMTPNetLogout);
        m_reconnects = 0;
    }
  
}

void CTNProcessThread::daInfo(string attrName, map<string, string> property)
{
    if (m_response != NULL) {
//        m_response->daInfo(attrName, property);
    }
}

void CTNProcessThread::processKickout()
{
    OMTPLOG("CTNProcessThread::processKickout!");
    setNetStatus(OMTPNetLogout);
    
    m_reconnects = 0;
    
    if (m_response != NULL) {
        m_response->loginResp(otim::EC_CONN_KICKOUT, CONFIG->clientId(), "");
    }
    else{
        OMTPLOG("processKickout m_response is NULL");
    }
}

void CTNProcessThread::forceResendInflight()
{
    OMTPLOG("forceResendInflight enter");
    m_offMsgCountOk = true;
    this->setNetStatus(OMTPNetConnected);
 
    this->checkRecvCache();
    
    //重新登录，强行重发堆积消息
    auto lambda = [this](InflightMessage* inflightMsg)->void{
       
        inflightMsg->req.header.flags |= otim::PF_ISDUP;
        OMTPLOG("resend msg:"<<inflightMsg->req.header.packId<<" flags:"<<inflightMsg->req.header.flags);
        
//        this->sendMsgReq(inflightMsg->req.header.type, inflightMsg->req, false);
        
     };
    
    INFLIGHT_VECTOR->forceResend(lambda);
    
    OMTPLOG("forceResendInflight exit");
}


//将缓存收取到的消息，通知到业务处理方
void CTNProcessThread::checkRecvCache()
{
//    std::unique_lock<std::mutex> unilock(m_mutexRSThread);
//    CTNCMutexGuard guard(m_mutexRSThread, "checkRecvCache");
    
    OMTPLOG("checkRecvCache:")
    
    if (m_response == nullptr){
        return;
    }
    
    for (int i = 0; i < _recvCache.size(); i++){
        InflightMessage* inflightMsg = _recvCache[i];
//        m_response->msgRecv(inflightMsg->type, inflightMsg->req);
        
        OMTPLOG("checkRecvCache:"<<inflightMsg->req.header.packId)
        delete inflightMsg;
    }
    
    _recvCache.clear();
}



void CTNProcessThread::reqHotSession(int64_t timestamp)
{
    otim::HotSessionReq req;
    req.timestamp = timestamp;

    sendReq<otim::HotSessionReq>(otim::PT_HOTSESSION_SYNC, &req);
}

void CTNProcessThread::sendPacket(otim::OTIMPack &pack)
{
    if (m_pSRThread == NULL) {
        OMTPLOG("sendPacket m_pSRThread IS NULL getNetStatus:"<<getNetStatus());
        return;
    }

    pack.header.version = PTCL_VERSION;
    if (pack.header.packId.empty()){
        pack.header.packId = getReqId();
    }
    OMTPLOG("sendPacket pack:"<<pack.header.writeToJsonString());

    
    TNPacket packet;
    packet.type = OMTP_PACKET_TYPE_SEND;
    packet.packData = pack;
    
    this->postPacket(packet);

}



//1.网络状态为OMTPNetNone， 重连；
//2.两个keeplive 收不到PINGRESQ响应数据，重连,
//3.网络状态OMTPNetConnecting，一个周期收不到CONNACK也，重连
//4.网络状态OMTPNetConnected 发送PING
void CTNProcessThread::checkPing()
{
//    cout<<"CONFIG->keepAlive():"<< CONFIG->keepAlive()<<endl;
    //need judge reconnect
    int64_t now = time(NULL);
    OMTPNetStatus netStatus = getNetStatus();
    if (netStatus == OMTPNetNone){
        OMTPLOG("CheckPing OMTPNetNone, will reconnect");
        reconnect();
        return;
    }
    else if (netStatus == OMTPNetConnecting
             && m_pingReqTime > 0
             && now - m_pingReqTime > CONFIG->keepAlive()){
        setNetStatus(OMTPNetNone);
        
        reconnect();
        return;
    }
    
    
    if (m_pingReqTime > 0 && now - m_pingReqTime > 2 * CONFIG->keepAlive()) {
        OMTPLOG("CheckPing PING TIME OUT, will reconnect");
        
        setNetStatus(OMTPNetNone);
        
        reconnect();
    }
    else if (netStatus == OMTPNetConnected) {
        OMTPLOG("PING SEND!");
           
        sendReq<otim::OTIMPack>(otim::PT_PING, nullptr);
        //如果收不到MsgCount，一个keeplive后重新收取

        if (!m_offMsgCountOk) {
			this->reqHotSession(CONFIG->hsTimestamp());
        }
        
        //已经发过PING，未收到服务器端响应，不更新PING时间戳；
        if (m_pingReqTime == 0){
            m_pingReqTime = now;
        }
        
    }
}

void CTNProcessThread::sendAck(int type, otim::MsgReq* msgReq)
{
    if (!m_offMsgCountOk) {
        OMTPLOG("sendAck will giveup:not recv offMsgCountResp");
        return;
    }
    
    //need send ack
    otim::MsgAck ack;
//    ack.type = type;
//    ack.msg_id = msgReq->msg_id;
//    ack.seq_id = msgReq->seq_id;
//    ack.from = msgReq->from;
//    ack.to = msgReq->to;
//    ack.priority = msgReq->priority;
//    ack.flags = msgReq->flags;
//
//    tars::TarsOutputStream<tars::BufferWriter> tos;
//    ack.writeTo(tos);
//
//    sendPacket(TYPE_MSGACK, tos.getBuffer(), tos.getLength());
}

void  CTNProcessThread::processPullHistoryMsgResp(otim::OTIMPack& pack)
{
//    otim::OffMsgResp offMsgResp;
//    tars::TarsInputStream<tars::BufferReader> is;
//
//    is.setBuffer(packetBuffer->payload, packetBuffer->length);
//
//    offMsgResp.readFrom(is);
//    OMTPLOG("processOffMsgResp msg size:"<<offMsgResp.msgs.size());
//
//    //call back to caller
//    if (m_response != NULL) {
//        m_response->offMsgResp(&offMsgResp);
//    }
//    else{
//        OMTPLOG("processOffMsgResp m_response is NULL");
//    }

}

void CTNProcessThread::processHotSessionResp(otim::OTIMPack& pack)
{
    
    otim::HotSessionResp hotSessionResp;
    tars::TarsInputStream<tars::BufferReader> is;
    
    is.setBuffer(pack.payload);
    
    hotSessionResp.readFrom(is);

    OMTPLOG("processHotSessionResp vSessionInfo size:"<<hotSessionResp.sessions.size()<<" code:"<<hotSessionResp.errorCode.code<<" timestamp:"<<hotSessionResp.timestamp);

//    call back to caller
    if (m_response != NULL) {
        m_response->hotSessionResp(&hotSessionResp);
    }
    else{
        OMTPLOG("processHotSessionResp m_response is NULL");
    }

    if (otim::EC_HOTSESSION_MORE == hotSessionResp.errorCode.code) {
        this->reqHotSession(hotSessionResp.timestamp);
    }
    else if (otim::EC_SUCCESS == hotSessionResp.errorCode.code) {
        CONFIG->setHSTimestamp(hotSessionResp.timestamp);
        this->forceResendInflight();
    }
    else {
        OMTPLOG("processHotSessionResp failed!,will reconnect");

        setNetStatus(OMTPNetNone);

        reconnect();
    }
    
}

void CTNProcessThread::processFriendsResp(otim::OTIMPack& pack)
{
     
//     otim::GetFriendsResp resp;
//     tars::TarsInputStream<tars::BufferReader> is;
//
//     is.setBuffer(packetBuffer->payload, packetBuffer->length);
//
//     resp.readFrom(is);
//     OMTPLOG("processFriendsResp size:"<<resp.friends.size()<<" reqId:"<<resp.reqId);
//
//     //call back to caller
//     if (m_response != NULL) {
//         m_response->getFriendsResp(&resp);
//     }
//     else{
//         OMTPLOG("processFriendsResp m_response is NULL");
//     }
    

}

void CTNProcessThread::processGetUserinfoResp(otim::OTIMPack& pack)
{
     
//     otim::GetUserInfoResp resp;
//     tars::TarsInputStream<tars::BufferReader> is;
//
//     is.setBuffer(packetBuffer->payload, packetBuffer->length);
//
//     resp.readFrom(is);
//     OMTPLOG("processGetUserinfoResp size:"<<resp.users.size()<<" reqId:"<<resp.reqId);
//
//     //call back to caller
//     if (m_response != NULL) {
//         m_response->getUserinfoResp(&resp);
//     }
//     else{
//         OMTPLOG("processGetUserinfoResp m_response is NULL");
//     }
    

}

void CTNProcessThread::processMyGroupchatResp(otim::OTIMPack& packetBuffer)
{
//     otim::SyncMyGroupChatsResp resp;
//     tars::TarsInputStream<tars::BufferReader> is;
//
//     is.setBuffer(packetBuffer->payload, packetBuffer->length);
//
//     resp.readFrom(is);
//     OMTPLOG("processMyGroupchatResp size:"<<resp.groupChats.size()<<" reqId:"<<resp.reqId);
//
//     //call back to caller
//     if (m_response != NULL) {
//         m_response->syncMyGroupchatResp(&resp);
//     }
//     else{
//         OMTPLOG("processMyGroupchatResp m_response is NULL");
//     }
    

}

void CTNProcessThread::processGroupchatMemberResp(otim::OTIMPack& packetBuffer)
{
//    if  (packetBuffer == NULL || packetBuffer->length == 0){
//         return;
//     }
     
//     otim::GetGroupMemberResp resp;
//     tars::TarsInputStream<tars::BufferReader> is;
//     
//     is.setBuffer(packetBuffer->payload, packetBuffer->length);
//     
//     resp.readFrom(is);
//     OMTPLOG("processGroupchatMemberResp size:"<<resp.users<<" reqId:"<<resp.req_id);
//     
//     //call back to caller
//     if (m_response != NULL) {
//         m_response->getGroupchatMemberResp(&resp);
//     }
//     else{
//         OMTPLOG("processGroupchatMemberResp m_response is NULL");
//     }
//    

}

//void CTNProcessThread::processGroupInfoResp(otim::OTIMPack& packetBuffer)
//{
//    if  (packetBuffer == NULL || packetBuffer->length == 0){
//         return;
//     }
//
//     otim::GetGroupInfoResp resp;
//     tars::TarsInputStream<tars::BufferReader> is;
//
//     is.setBuffer(packetBuffer->payload, packetBuffer->length);
//
//     resp.readFrom(is);
//     OMTPLOG("processGroupInfoResp group_info:"<<resp.group_info<<" reqId:"<<resp.req_id);
//
//     //call back to caller
//     if (m_response != NULL) {
//         m_response->getGroupInfoResp(&resp);
//     }
//     else{
//         OMTPLOG("processGroupInfoResp m_response is NULL");
//     }
//
//
//}

