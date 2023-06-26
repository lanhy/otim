//
//  CTNNetwork.h
//
//  Created by lanhy on 2018/4/17.
//  Copyright © 2018年. All rights reserved.
//

#ifndef CTNNetwork_h
#define CTNNetwork_h

#ifdef WIN32

#include <WinSock2.h>
#include <Ws2ipdef.h>
#include <Ws2tcpip.h>

#include "win32/CTNSocketBreaker.h"

#define ssize_t size_t
#define socket_t SOCKET

#define TN_SOCKET_CLOSE(a)         closesocket(a)
#define TN_SOCKET_ERROR       WSAGetLastError()
#define getsockopt(s, level, optname, optval, optlen) getsockopt(s, level, optname, (char*)optval, optlen)

#define TN_ECONNRESET       WSAECONNRESET
#define TN_EINPROGRESS      WSAEINPROGRESS
#define TN_EWOULDBLOCK      WSAEWOULDBLOCK
#define TN_ENETDOWN         WSAENETDOWN
#define TN_EISCONN          WSAEISCONN
#define TN_EINTR            WSAEINTR
#define TN_EAGAIN           WSAEWOULDBLOCK


#else

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include "unix/CTNSocketBreaker.h"


#define socket_t int

#define TN_SOCKET_CLOSE(a)     close(a)
#define TN_SOCKET_ERROR    errno


#define TN_ECONNRESET   ECONNRESET
#define TN_EINPROGRESS  EINPROGRESS
#define TN_EWOULDBLOCK  EWOULDBLOCK
#define TN_ENETDOWN     ENETDOWN
#define TN_EISCONN      EISCONN
#define TN_EINTR        EINTR
#define TN_EAGAIN        EAGAIN



#endif


#ifndef INVALID_SOCKET

#define INVALID_SOCKET -1

#endif

int setNonBlock(socket_t sock);

#endif /* CTNNetwork_h */
