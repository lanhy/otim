//
//  CTNSendRecvThread.cpp
//
//  Created by 兰怀玉 on 16/4/15.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#include "CTNSendRecvThread.h"
#include "OMTPConfig.h"
#include <iostream>
#include "CSSLSocket.h"
#include "CLog.h"
#include "CTNProcessThread.h"
#include "CTNSocketSelect.h"

#ifndef WIN32

#include <unistd.h>
#else
#include <Windows.h>

#endif


using namespace std;

CTNSendRecvThread::CTNSendRecvThread(CTNProcessThread* processThread):_queuePacket(false)
{
    OMTPLOG("CTNSendRecvThread::CTNSendRecvThread...:"<<getId()<<" processThread:"<<processThread->getId());
    _pSocket = NULL;
    
    _processThread = processThread;
    _reconnectTimes = 0;
}

CTNSendRecvThread::~CTNSendRecvThread()
{
    OMTPLOG("CTNSendRecvThread::~CTNSendRecvThread...:"<<getId());
    if (_pSocket != NULL) {
        delete _pSocket;
        _pSocket = NULL;
    }
}


void CTNSendRecvThread::stop()
{
    if (!isRunning()) {
        OMTPLOG("CTNSendRecvThread isn't running return:"<<getId());
        return;
    }
    
    
    CThreadEx::stop();
    OMTPLOG("CTNSendRecvThread stopped:"<<getId()<<" isRunning:"<<isRunning());

    //notify exit the select wait
    _rwBreaker.fireon();
    
    if (_pSocket != NULL) {
        OMTPLOG("_pSocket will close:"<<getId());
        _pSocket->closeSocket();
    }
    
}

void  CTNSendRecvThread::postRecvPacketBuffer(const TNPacketBuffer * packetBuffer)
{
    if  (_processThread == NULL){
        OMTPLOG("_processThread is NULL...:"<<getId());
        return;
    }
    
    tars::TarsInputStream<tars::BufferReader> is;
    is.setBuffer(packetBuffer->payload, packetBuffer->length - PACK_MIN_SIZE);
    
  
    TNPacket packet;
    packet.type = OMTP_PACKET_TYPE_RECV;
    packet.param = this->getId();
    packet.packData.readFrom(is);
    
    OMTPLOG("RecvPacket type:"<<otim::etos((otim::PACK_TYPE)packet.packData.header.type)<<" pack:"<<packet.packData.header.writeToJsonString());

    _processThread->postPacket(packet);
}

void CTNSendRecvThread::socketClosed()
{
    OMTPLOG("CTNSendRecvThread socketClosed:"<<getId());
    TNPacket packet;
    packet.type = OMTP_PACKET_TYPE_SOKCET_CLOSED;
    packet.param = this->getId();
    _processThread->postPacket(packet);
}

#define  RECV_LEN   65534

//超过这个数就废了，直接丢弃数据包
#define  MAX_RECV_LEN   65534000

void CTNSendRecvThread::run()
{
    OMTPLOG("CTNSendRecvThread thread start RSThread:"<<this->getId());
	
    char recvBytes[RECV_LEN] = {0};
    
    //wait 100-500 ms
    int sleepTime = (this->getId()%5) * 100;

    tn_msleep(sleepTime);
    
    connect();
#if 0
    while (isRunning()){
        int len = _pSocket->recvData(recvBytes, RECV_LEN);
        if (len > 0){
            //            OMTPLOG("recv data:"<<len);
            _recvByteQueue.push(recvBytes, len);
            
            if (_recvByteQueue.size() < PACK_MIN_SIZE) {
                //OMTPLOG("not enough continue recv:"<<PACK_MIN_SIZE);
                continue;
            }
            
            processRecvData();
        }
        else if (!isRunning()){
             OMTPLOG("Thread will stop...:"<<getId());
            break;
        }
        else if (len == 0 || (len == -1 && errno != EINTR)){
             OMTPLOG("Recv data len , will close socket:"<<len);
            //中断需要继续重试 退出循环有判断，此判断重复，去掉
            break;
        }
    }

#else
    while (isRunning()){
        socket_t socketFD = _pSocket->socketFD();

        CTNSocketSelect sel(_rwBreaker, true);
        sel.preSelect();
        sel.read_FD_SET(socketFD);
        sel.exception_FD_SET(socketFD);
        if (!_queuePacket.empty()){
            sel.write_FD_SET(socketFD);
        }
        
        OMTPLOG("BEFORE CTNSendRecvThread sel.select"<<getId());
        int ret = sel.select();
        OMTPLOG("AFTER CTNSendRecvThread sel.select:"<<getId());
        if (ret < 0){
            OMTPLOG("CTNSendRecvThread will stop ret<0...:"<<getId());
            break;
        }
        if (sel.isException()) {
            OMTPLOG("sel isException will stop...:"<<getId());
            break;
        }
        
        if (sel.exception_FD_ISSET(socketFD)) {
            OMTPLOG("sel exception_FD_ISSET will stop...:"<<getId());
            break;
        }
        
        if (sel.read_FD_ISSET(socketFD)){
            int len = _pSocket->recvData(recvBytes, RECV_LEN);
            if (len == OMTP_ERR_SOCK_CLOSE){
                OMTPLOG("recvData socket will close...:"<<getId());
               break;
            }
            
            //process recv data
            _recvByteQueue.push(recvBytes, len);
            processRecvData();
        }
        
        if (sel.write_FD_ISSET(socketFD) && !_queuePacket.empty()){
            otim::OTIMPack packet;
            _queuePacket.pop(packet, 1);
            OMTPLOG("Send packet thread:"<<this->getId()<<" data:"<<packet.header.writeToJsonString());
            
            tars::TarsOutputStream<tars::BufferWriter> tos;
            packet.writeTo(tos);

            uint32_t packSize = (uint32_t)tos.getLength() + PACK_MIN_SIZE;
            TNPacketBuffer* packBuffer = NEW_PACKET(packSize);
            packBuffer->length = packSize;
            memcpy(packBuffer->payload, tos.getBuffer(), tos.getLength());
            packBuffer->encode();
            
            ret = _pSocket->sendData((char*)packBuffer, packSize);
            DELETE_PACKET(packBuffer);
            
            if (ret == OMTP_ERR_SOCK_CLOSE){
                OMTPLOG("sendData socket will close...:"<<getId());
                break;
            }
            else if (ret < packSize){
                OMTPLOG("sendData failed,packet will be lost ret:"<<ret);
            }
        
        }
    }
#endif
    
    //非主动断掉，需要重连
    if (isRunning()) {
        OMTPLOG("before socketClosed 2:"<<getId());
        this->socketClosed();
    }

    OMTPLOG("CTNSendRecvThread socket 3:"<<getId());
    this->stop();
  
    OMTPLOG("CTNSendRecvThread thread end...:"<<getId());
}

void CTNSendRecvThread::processRecvData()
{
    TNPacketBuffer headerParser;
    while (_recvByteQueue.size() >= PACK_MIN_SIZE) {
        headerParser = *(TNPacketBuffer *)_recvByteQueue.getArray();
        headerParser.decode();
        
        int packLen = headerParser.length;
        if (packLen > 65534000) {
            OMTPLOG("packLen error size:"<<packLen);
            this->stop();
  
            this->socketClosed();

            break;
        }
        
        if (packLen > _recvByteQueue.size()){
            OMTPLOG("not enough continue curr size:"<<_recvByteQueue.size()<<" except size:"<<packLen);
            break;
        }
        
        
        char* buffer =  _recvByteQueue.pop(packLen);
        TNPacketBuffer * packetBuffer = (TNPacketBuffer *)buffer;
        packetBuffer->decode();
        OMTPLOG("RECV OK length:"<<packetBuffer->length<<" thread:"<<getId());

        postRecvPacketBuffer(packetBuffer);
        
        DELETE_PACKET(packetBuffer);
    }
    
}

void CTNSendRecvThread::sendPacket(otim::OTIMPack& packet)
{
    if (_pSocket == NULL) {
        OMTPLOG("_pSocket is null");
        return;
    }
    
    _queuePacket.push(packet);
    _rwBreaker.fireon();
}

void CTNSendRecvThread::connect()
{
    int connects = 0;
    
    while (isRunning()) {
        if (_pSocket != nullptr) {
            delete _pSocket;
            _pSocket = nullptr;
        }
        
        if (_hostInfo.isSSL){
            _pSocket = new CSSLSocket();
            OMTPLOG("CSSLSocket will connect:"<<_hostInfo.host<<" port:"<<_hostInfo.port<<" thread:"<<getId());
            
        }
        else {
            _pSocket = new CTcpSocket();
            OMTPLOG("CTcpSocket Will connect to host:"<<_hostInfo.host<<" port:"<<_hostInfo.port<<" thread:"<<getId());
        }
        
        OMTPLOG("Will connect to host:"<<_hostInfo.host<<" port:"<<_hostInfo.port<<" thread:"<<getId());
        int ret = _pSocket->connectToHost(_hostInfo.ip.c_str(), _hostInfo.port);
        OMTPLOG("after connect to host:"<<_hostInfo.ip<<" port:"<<_hostInfo.port<<" result:"<<ret<<" thread:"<<getId());
       
        //防止一段时间连接不上后，重新连接放弃此连接
        if (!isRunning()) {
            OMTPLOG("STOPED! The Current TCP will give up, thread:"<<getId());
            break;
        }
   
        if (ret == 0) {
            TNPacket packet;
            packet.type = OMTP_PACKET_TYPE_SOKCET_SUCCESS;
            packet.param = this->getId();
            _processThread->postPacket(packet);
            
            break;
        }
        else if (connects >= RECONN_MAX){
            OMTPLOG("connect failed retry times:"<<connects<<" thread:"<<getId());
            TNPacket packet;
            packet.type = OMTP_PACKET_TYPE_SOKCET_FAILED;
            packet.param = this->getId();
            _processThread->postPacket(packet);

            this->stop();
            
            CONFIG->removeIpCache(_hostInfo.host.c_str());
            
            break;
        }
        else {
            int sleepTime = RECONN_STEP[connects%RECONN_MAX];
            
            OMTPLOG("need reconnect:"<<sleepTime<<" thread:"<<getId());
			tn_msleep(sleepTime*1000);
            
            connects ++;
            _hostInfo = CONFIG->hostInfo();
            OMTPLOG("Reget host:"<<_hostInfo.host<<" port:"<<_hostInfo.port<<" thread:"<<getId());
        }
    }
}



