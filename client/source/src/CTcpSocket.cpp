//
//  CTcpSocket.cpp
//  OMTP
//
//  Created by 兰怀玉 on 16/4/13.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#include "CTcpSocket.h"

#include <iostream>
#include <string.h>
#include <map>
#include "CLog.h"
#include "OMTPConfig.h"
#include "CTNSocketSelect.h"

#ifndef __APPLE__
#define SO_NOSIGPIPE    MSG_NOSIGNAL
#endif

int setNonBlock(socket_t sock)
{
#ifndef WIN32
    long arg;
    if ((arg = fcntl(sock, F_GETFL, NULL)) < 0) {
        //OMTPLOG("Error 0 fcntl(..., F_GETFL):"<<strerror(errno));
        return -1;
    }
    
    arg |= O_NONBLOCK;
    if (fcntl(sock, F_SETFL, arg) < 0) {
        //OMTPLOG("Error 1 fcntl(..., F_GETFL):"<<strerror(errno));
        return -1;
    }
#else
    unsigned long opt = 1;
    if (ioctlsocket(sock, FIONBIO, &opt)){
        closesocket(sock);
        return -1;
    }
#endif
    return 0;
}


int socket_error(socket_t sock) {
    int error = 0;
    socklen_t len = sizeof(error);
    if (0 != getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len)){
        error = TN_SOCKET_ERROR;
    }
    
    return error;
}



CTNCMutexGuard::CTNCMutexGuard(std::mutex& mutex, const char* inditify)
{
    _mutex = &mutex;
    _inditify = inditify;
//    OMTPLOG("CTNCMutexGuard::lock:"<<_inditify<<" mutex:"<<(long)_mutex);
    _mutex->lock();
    
}

CTNCMutexGuard::~CTNCMutexGuard()
{
    _mutex->unlock();
//    OMTPLOG("CTNCMutexGuard::unlock:"<<_inditify<<" mutex:"<<(long)_mutex);
}



CTcpSocket::CTcpSocket()
{
    m_socket = INVALID_SOCKET;
    _isClosed = false;
    
    this->initNet();
}

CTcpSocket::~CTcpSocket()
{
    OMTPLOG("ENTER CTcpSocket::~CTcpSocket:"<<m_socket);
    closeSocket();
    OMTPLOG("EXIT CTcpSocket::~CTcpSocket:"<<m_socket);
}



int CTcpSocket::connectNoBlock(int soc, struct sockaddr *addr, int addr_len, int timeout) {
    if (addr == nullptr || soc <= 0 || addr_len == 0){
        OMTPLOG("connectNoBlock param is invalid:"<< soc<<"  addr_len:"<<addr_len);
        return OMTP_ERR_CONNECT;
    }
    
    // Set non-blocking
    setNonBlock(soc);
    
    // Trying to connect with timeout
    int res = connect(soc, addr, addr_len);
    if (res == 0){
        OMTPLOG("TCP CONNECT OK:"<<soc);
        return 0;
    }
    
    int eorrorCode = TN_SOCKET_ERROR;
    if (eorrorCode != TN_EINPROGRESS && eorrorCode != TN_EWOULDBLOCK) {
        OMTPLOG("Error connecting:"<<eorrorCode);
        return OMTP_ERR_CONNECT;
    }
    
    OMTPLOG("EINPROGRESS|TN_EWOULDBLOCK in connect():"<<eorrorCode);
    do {
        CTNSocketSelect sel(_connectBreaker, true);
        sel.preSelect();
        sel.write_FD_SET(soc);
        sel.exception_FD_SET(soc);
        
        OMTPLOG("before connect select");
        int selectRet = sel.select(5000);
        OMTPLOG("after connect select");
        
        //        eorrorCode  = sel.getErrno();
        if (0 == selectRet) {
            OMTPLOG("TIME OUT");
            return OMTP_ERR_CONNECT;
        }
        
        if (selectRet < 0) {
            OMTPLOG("Connect selectRet<0");
            return OMTP_ERR_CONNECT;
        }
        
        eorrorCode = socket_error(soc);
        if (eorrorCode == 0){
            break;
        }
        
        if (sel.exception_FD_ISSET(soc)) {
            OMTPLOG("Connect exception_FD_ISSET");
            return OMTP_ERR_CONNECT;
        }
        
        if (sel.write_FD_ISSET(soc)) {
            OMTPLOG("Connect write_FD_ISSET");
            return OMTP_ERR_CONNECT;
        }
        
    } while (true);
    
    OMTPLOG("TCP CONNECT OK 2:"<<soc);
    return 0;
}

int CTcpSocket::connectToHost(const char *host, short port)
{
    this->initNet();
    
    OMTPLOG("CTcpSocket connectToHost:"<<this);
    int sock = 0;
    
    void* svraddr = 0;
    int error = -1, svraddr_len = 0;
    int ret = -1;
    struct sockaddr_in svraddr_4;
    struct sockaddr_in6 svraddr_6;
    
    char ip[128] = { 0 };
    strncpy(ip, host, 127);
    
    OMTPLOG("Before DNS:"<<host);
    //获取网络协议
    struct addrinfo *result = 0;
    error = ::getaddrinfo(host, NULL, NULL, &result);
    if (result == NULL){
        OMTPLOG("getaddrinfo error:"<<host<<" error:"<<error);
        return -1;
    }
    OMTPLOG("AFTER DNS:"<<result->ai_addr->sa_data);
    
    const struct sockaddr *sa = result->ai_addr;
    socklen_t maxlen = 128;
    switch(sa->sa_family)
    {
        case AF_INET://ipv4
            OMTPLOG("IPV4 CONNECT");
            if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
                OMTPLOG("IPV4 socket create failed");
                break;
            }else{
                ret = 0;
            }
            if (inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr), ip, maxlen) == 0){
                ret = -1;
                OMTPLOG("inet_ntop ipv4 failed:"<<ip);
                break;
            }
            
            svraddr_4.sin_family = AF_INET;
            svraddr_4.sin_addr.s_addr = inet_addr(ip);
            svraddr_4.sin_port = htons(port);
            svraddr_len = sizeof(svraddr_4);
            svraddr = &svraddr_4;
            break;
        case AF_INET6://ipv6
            
            OMTPLOG("IPV6 CONNECT");
            
            if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0){
                OMTPLOG("socket create failed");
                break;
            }else{
                ret = 0;
            }
            
            if (inet_ntop(AF_INET6, &(((struct sockaddr_in6 *)sa)->sin6_addr), ip, maxlen) == 0){
                ret = -1;
                OMTPLOG("inet_ntop ipv6 failed:"<<ip);
                break;
            }
            
            memset(&svraddr_6, 0, sizeof(svraddr_6));
            svraddr_6.sin6_family = AF_INET6;
            svraddr_6.sin6_port = htons(port);
            if (inet_pton(AF_INET6, ip, &svraddr_6.sin6_addr) < 0 ){
                ret = -1;
                OMTPLOG("inet_ntop ipv6 failed:"<<ip);
                break;
            }
            svraddr_len = sizeof(svraddr_6);
            svraddr = &svraddr_6;
            break;
            
        default:
            OMTPLOG("Unknown AF");
            ret = -1;
    }
    freeaddrinfo(result);
    if(-1 == ret){
        OMTPLOG("Cannot create socket:"<<ret);
        TN_SOCKET_CLOSE(sock);
        return ret;
    }
    OMTPLOG("CONNECT TO IP:"<<ip<<" port:"<<port);
    
    //忽略SIGPIPE 信号
#ifdef __APPLE__
    int value = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value)) != 0) {
        OMTPLOG("setsockopt SO_NOSIGPIPE err:" << strerror(errno));
    }
#endif
    
    //    ret = connect(sock, (struct sockaddr*)svraddr, svraddr_len);
    ret = this->connectNoBlock(sock, (struct sockaddr*)svraddr, svraddr_len, 5);
    if(ret == -1 ){
        OMTPLOG("Cannot Connect the server!"<<errno<<" desc:"<<strerror(errno));
        TN_SOCKET_CLOSE(sock);
        return ret;
    }
    
    //缓存
    OMTPLOG("Connect the server success");
    
    m_socket = sock;
    
    return ret;
}


void CTcpSocket::closeSocket()
{
    OMTPLOG("CTcpSocket::closeSocket:"<<m_socket);
    _connectBreaker.fireon();
    
    if(m_socket != INVALID_SOCKET){
        TN_SOCKET_CLOSE(m_socket);
        m_socket = INVALID_SOCKET;
    }
    _isClosed = true;
}


void CTcpSocket::initNet()
{
#ifdef WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        OMTPLOG("initiate socket error!");
    }
#endif
    
    //NEED CLEAR WHEN STOP
#ifdef WIN32
    //WSACleanup();
#endif
    
}


int CTcpSocket::sendData(const char *buf, size_t len)
{
    if (m_socket == INVALID_SOCKET) {
        OMTPLOG("CTcpSocket::sendData INVALID_SOCKET");
        return OMTP_ERR_NO_CONN;
    }
    
    int sendLen = 0;
    while (len - sendLen > 0){
        ssize_t ret = ::send(m_socket, &buf[sendLen], len - sendLen, 0);
        if (ret == -1 && TN_SOCKET_ERROR != TN_EINTR && TN_SOCKET_ERROR != TN_EAGAIN){
            OMTPLOG("CTcpSocket::send socket close:"<<ret<<" errno:"<<TN_SOCKET_ERROR);
            return OMTP_ERR_SOCK_CLOSE;
        }
        else if (ret < 0){
            ret = 0;
        }
        
        sendLen += ret;
    }
    
    //cout << "send return:" << ret << endl;
    return sendLen;
}

int CTcpSocket::recvData(char *buf, size_t len)
{
    if (m_socket == INVALID_SOCKET) {
        OMTPLOG("CTcpSocket::recvData INVALID_SOCKET");
        return OMTP_ERR_NO_CONN;
    }
    
    ssize_t ret = ::recv(m_socket, buf, len, 0);
    if (ret == 0 || (ret == -1 && TN_SOCKET_ERROR != TN_EINTR && TN_SOCKET_ERROR != TN_EAGAIN)){
        OMTPLOG("CTcpSocket::recvData socket close:"<<ret<<" errno:"<<TN_SOCKET_ERROR);
        ret = OMTP_ERR_SOCK_CLOSE;
    }
    
    return (int)ret;
}


