//
//  CSocketSelect.h
//
//  Created by lanhy on 2018/4/12.
//  Copyright © 2018年 lanhy. All rights reserved.
//

#ifndef CTNSocketSelect_h
#define CTNSocketSelect_h

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#define SOCKET int
#endif

class CTNSocketBreaker;

class CTNSocketSelect {
public:
    CTNSocketSelect(CTNSocketBreaker& breaker, bool autoclear = false);
    ~CTNSocketSelect();
    
    void preSelect();
    void read_FD_SET(SOCKET socket);
    void write_FD_SET(SOCKET socket);
    void exception_FD_SET(SOCKET socket);
    int select();
    int select(int msec);
    int select(int sec, int usec);
    
    int getErrno() const;
    
    int read_FD_ISSET(SOCKET socket) const;
    int write_FD_ISSET(SOCKET socket) const;
    int exception_FD_ISSET(SOCKET socket) const;
    
    bool isBreak() const;
    bool isException() const;
    
    CTNSocketBreaker& breaker();
    
private:
    CTNSocketSelect(const CTNSocketSelect&);
    CTNSocketSelect& operator=(const CTNSocketSelect&);
    
private:
    CTNSocketBreaker& _breaker;
    SOCKET _maxsocket;
    
    fd_set _readSet;
    fd_set _writefd;
    fd_set _exceptionfd;
    
    int _errno;
    bool _autoclear;
};
#endif /* CSocketSelect_hpp */
