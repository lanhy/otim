//
//  CSSLSocket.h
//
//  Created by 兰怀玉 on 16/4/13.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef CSSLSocket_hpp
#define CSSLSocket_hpp

#include <stdio.h>
#include <string>
#include <openssl/ssl.h>
#include <mutex>

#include "CTcpSocket.h"
#include "OMTPConst.h"

class CSSLSocket : public CTcpSocket{
    
protected:

    virtual void initNet();
    
    static int serverCertificateVerify(int preverify_ok, X509_STORE_CTX *ctx);
    int connectTLS();
    
    SSL_CTX* _ssl_ctx;
    SSL* _ssl;
public:
    CSSLSocket();

    virtual ~CSSLSocket();
    virtual int connectToHost(const char* host, short port);
    virtual void closeSocket();
    
    virtual int sendData(const char *buf, size_t len);
    virtual int recvData(char *buf, size_t len);
        
};

#endif /* CTcpSocket_hpp */
