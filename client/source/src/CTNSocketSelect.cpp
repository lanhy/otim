//
//  CCTNSocketSelect.cpp
//
//  Created by lanhy on 2018/4/12.
//  Copyright © 2018年 lanhy. All rights reserved.
//

#include "CTNSocketSelect.h"
#include "CTNNetwork.h"

#ifndef MAX
# define MAX(A,B) ((A)>(B)?(A):(B))
#endif

CTNSocketSelect::CTNSocketSelect(CTNSocketBreaker& breaker, bool autoclear)
: _breaker(breaker), _maxsocket(0), _errno(0), _autoclear(autoclear) {
    // inital FD
    FD_ZERO(&_readSet);
    FD_ZERO(&_writefd);
    FD_ZERO(&_exceptionfd);
}

CTNSocketSelect::~CTNSocketSelect(){
    
}

void CTNSocketSelect::preSelect() {
//    xassert2(!IsBreak(), "Already break!");
    FD_ZERO(&_readSet);
    FD_ZERO(&_writefd);
    FD_ZERO(&_exceptionfd);
    
    FD_SET(_breaker.breakerFD(), &_readSet);
    FD_SET(_breaker.breakerFD(), &_exceptionfd);
    _maxsocket = _breaker.breakerFD();
    _errno = 0;
}

int CTNSocketSelect::select() {
    int ret = ::select(_maxsocket + 1, &_readSet, &_writefd, &_exceptionfd, NULL);
    
    if (0 > ret) {
        _errno = TN_SOCKET_ERROR;
    }
    
    if (_autoclear){
        breaker().clear();
    }
    
    return ret;
}

int CTNSocketSelect::select(int msec) {
//    ASSERT(0 <= _msec);
    
    int sec = msec / 1000;
    int usec = (msec - sec * 1000) * 1000;
    timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    int ret = ::select(_maxsocket + 1, &_readSet, &_writefd, &_exceptionfd, &timeout);
    
    if (0 > ret){
        _errno = TN_SOCKET_ERROR;
    }
    
    if (_autoclear){
        breaker().clear();
    }
    
    return ret;
}

int CTNSocketSelect::select(int sec, int usec) {
//    ASSERT(0 <= _sec);
//    ASSERT(0 <= _usec);

    //timeval timeout = {sec, usec};
    timeval timeout;
    timeout.tv_sec = sec;
    timeout.tv_usec = usec;

    int ret = ::select(_maxsocket + 1, &_readSet, &_writefd, &_exceptionfd, &timeout);
    
    if (0 > ret){
        _errno = TN_SOCKET_ERROR;
    }
    
    if (_autoclear){
        breaker().clear();
    }
    
    return ret;
}

void CTNSocketSelect::read_FD_SET(socket_t socket) {
    _maxsocket = MAX(_maxsocket, socket);
    FD_SET(socket, &_readSet);
}

void CTNSocketSelect::write_FD_SET(socket_t socket) {
    _maxsocket = MAX(_maxsocket, socket);
    FD_SET(socket, &_writefd);
}

void CTNSocketSelect::exception_FD_SET(socket_t socket) {
    _maxsocket = MAX(_maxsocket, socket);
    FD_SET(socket, &_exceptionfd);
}

int CTNSocketSelect::read_FD_ISSET(socket_t socket) const {
    return FD_ISSET(socket, (fd_set*)&_readSet);
}

int CTNSocketSelect::write_FD_ISSET(socket_t socket) const {
    return FD_ISSET(socket, (fd_set*)&_writefd);
}

int CTNSocketSelect::exception_FD_ISSET(socket_t socket) const {
    return FD_ISSET(socket, (fd_set*)&_exceptionfd);
}

bool CTNSocketSelect::isException() const {
    return 0 != FD_ISSET(_breaker.breakerFD(), (fd_set*)&_exceptionfd);
}

bool CTNSocketSelect::isBreak() const {
    return 0 != FD_ISSET(_breaker.breakerFD(), (fd_set*)&_readSet);
}

CTNSocketBreaker& CTNSocketSelect::breaker() {
    return _breaker;
}

int CTNSocketSelect::getErrno() const {
    return _errno;
}
