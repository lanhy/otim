//
//  CTcpSocket.h
//
//  Created by 兰怀玉 on 16/4/13.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef CTcpSocket_hpp
#define CTcpSocket_hpp

#include <stdio.h>
#include <string>
#include <mutex>

#include "CTNNetwork.h"

enum OMTP_ERROR_T {
    OMTP_ERR_SUCCESS = 0,
    OMTP_ERR_SOCK_CLOSE = -1,
    OMTP_ERR_NO_CONN = -2,
    OMTP_ERR_TLS = -3,
    OMTP_ERR_CONNECT = -4
 };

using namespace std;

class CTNCMutexGuard{
public:
    CTNCMutexGuard(std::mutex& mutex, const char* inditify);
    ~CTNCMutexGuard();
    
private:
    std::mutex* _mutex;
    std::string _inditify;
};


class CTcpSocket{
    
protected:
    socket_t m_socket;
    bool _isClosed;
    CTNSocketBreaker _connectBreaker;

    
    virtual void initNet();
    
    int connectNoBlock(int soc, struct sockaddr *addr, int addr_len, int timeout) ;
//    int setSocketBlock(socket_t sock, bool block = false);
    
public:
    CTcpSocket();
    virtual ~CTcpSocket();
    
    socket_t socketFD(){
        return m_socket;
    }
    
    virtual int connectToHost(const char* host, short port);
    virtual void closeSocket();
    
    virtual int sendData(const char *buf, size_t len);
    virtual int recvData(char *buf, size_t len);
        
};

#endif /* CTcpSocket_hpp */
