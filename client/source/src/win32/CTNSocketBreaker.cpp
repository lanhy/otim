/*  Copyright (c) 2013-2015 Tencent. All rights reserved.  */
#include "CTNSocketBreaker.h"
#include <ws2tcpip.h>
#include "../CTNNetwork.h"


CTNSocketBreaker::CTNSocketBreaker()
    : _sendinlen(sizeof(_sendin))
    , _socket_w(INVALID_SOCKET)
    , _socket_r(INVALID_SOCKET)
    , _broken(false)
    , _create_success(true) {
    this->reCreate();
}

CTNSocketBreaker::~CTNSocketBreaker() {
    close();
}

bool CTNSocketBreaker::isCreateSuc() const {
    return _create_success;
}

bool CTNSocketBreaker::reCreate() {
    this->close();

    // initial pipes
    int Ret;
    _socket_w = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (INVALID_SOCKET == _socket_w) {
//        xerror2("INVALID_SOCKET with  _socket_w=%d", WSAGetLastError());
        _create_success = false;
        return _create_success;
    }

    _socket_r = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (INVALID_SOCKET == _socket_r) {
//        xerror2("INVALID_SOCKET with _socket_r=%d", WSAGetLastError());
        closesocket(_socket_w);
        _create_success = false;
        return _create_success;
    }

    Ret = setNonBlock(_socket_w);

    if (SOCKET_ERROR == Ret) {
//        xerror2("ioctlsocket failed with _socket_w: %d\n", WSAGetLastError());
        closesocket(_socket_r);
        closesocket(_socket_w);
        _create_success = false;
        return _create_success;
    }

    Ret = setNonBlock(_socket_r);

    if (SOCKET_ERROR == Ret) {
//        xerror2("ioctlsocket failed with _socket_r: %d\n", WSAGetLastError());
        closesocket(_socket_r);
        closesocket(_socket_w);
        _create_success = false;
        return _create_success;
    }

    struct sockaddr_in local_b;

    local_b.sin_family = AF_INET;

    local_b.sin_port = htons(0);

    local_b.sin_addr.S_un.S_addr = htonl(INADDR_LOOPBACK);

    Ret = bind(_socket_w, (struct sockaddr*)&local_b, sizeof(local_b));

    if (SOCKET_ERROR == Ret) {
//        xerror2("bind failed with _socket_w: %d\n", WSAGetLastError());
        closesocket(_socket_r);
        closesocket(_socket_w);
        _create_success = false;
        return _create_success;
    }

    Ret = bind(_socket_r, (struct sockaddr*)&local_b, sizeof(local_b));

    if (SOCKET_ERROR == Ret) {
//        xerror2("bind failed with _socket_r: %d\n", WSAGetLastError());
        closesocket(_socket_r);
        closesocket(_socket_w);
        _create_success = false;
        return _create_success;
    }

    Ret = getsockname(_socket_r, &_sendin, &_sendinlen);

    if (SOCKET_ERROR == Ret) {
//        xerror2("getsockname failed %d\n", WSAGetLastError());
        closesocket(_socket_r);
        closesocket(_socket_w);
        _create_success = false;
        return _create_success;
    }

    _create_success = true;
    return _create_success;
}

bool CTNSocketBreaker::isBreak() const {
    return _broken;
}

bool CTNSocketBreaker::fireon() {
    std::unique_lock<std::mutex> unilock(_mutex);
    
    if (_broken) return true;

    char dummy[] = "1";

    //int sendto(SOCKET,const char *,int,int,const sockaddr *,int)
    int ret = sendto(_socket_w, (const char*)&dummy, strlen(dummy), 0, (sockaddr*)&_sendin, _sendinlen);
    _broken = true;

    if (ret < 0 || ret != strlen(dummy)) {
//        xerror2(TSF"sendto Ret:%_, errno:(%_, %_)", ret, errno, WSAGetLastError());
        _broken =  false;
        this->reCreate();
    }

    // Ret = WSAGetLastError();
    return _broken;
}



bool CTNSocketBreaker::clear() {
    std::unique_lock<std::mutex> unilock(_mutex);
    
    if (!_broken){
        return true;
    }

    char buf[128];
    struct sockaddr src = {0};
    int len = sizeof(src);
    int ret = recvfrom(_socket_r, buf, sizeof(buf), 0, &src, &len);
    _broken = false;

    if (ret < 0) {
//        xerror2(TSF"recvfrom Ret:%_, errno:(%_, %_)", ret, errno, WSAGetLastError());
        reCreate();
        return false;
    }

    // Ret = WSAGetLastError();
    return true;
}

void CTNSocketBreaker::close() {
    if (_socket_w != INVALID_SOCKET)
        closesocket(_socket_w);

    if (_socket_r != INVALID_SOCKET)
        closesocket(_socket_r);

    _socket_w = INVALID_SOCKET;
    _socket_r = INVALID_SOCKET;
}

SOCKET CTNSocketBreaker::breakerFD() const {
    return _socket_r;
}

