//
//  CSSLSocket.cpp
//
//  Created by 兰怀玉 on 16/4/13.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#include "CSSLSocket.h"

#include <iostream>
#include <string.h>
#include <vector>
#include <openssl/err.h>
#include "CLog.h"
#include "CTNSocketSelect.h"

#include "CTNNetwork.h"


CSSLSocket::CSSLSocket()
{
    _ssl_ctx = nullptr;
    _ssl = nullptr;
 }

CSSLSocket::~CSSLSocket()
{
    OMTPLOG("CSSLSocket::~CSSLSocket ENTER:"<<m_socket);
    if (_ssl != nullptr){
        SSL_free(_ssl);
        _ssl = nullptr;
    }
    OMTPLOG("CSSLSocket::~CSSLSocket 2");

    if (_ssl_ctx != nullptr){
        OMTPLOG("CSSLSocket::~CSSLSocket 21");
        SSL_CTX_free(_ssl_ctx);
        _ssl_ctx = nullptr;
    }
    
    OMTPLOG("CSSLSocket::~CSSLSocket EXIT");
}

void CSSLSocket::initNet(){
    
    ERR_load_SSL_strings();
    OPENSSL_init_ssl(0, NULL);
}

void CSSLSocket::closeSocket(){
    CTcpSocket::closeSocket();
 }


int CSSLSocket::connectTLS(){
    OMTPLOG("ENTER connectTLS");
#if 0
    int ret = SSL_connect(_ssl);
    if(ret != 1){
        //    SSL_ERROR_NONE
        int errcode = SSL_get_error(_ssl, ret);
        OMTPLOG("connectTLS SSL_connect FAILED ret:"<<ret<<"  ERR_get_error():"<<errcode);
        COMPAT_CLOSE(m_socket);
        m_socket = INVALID_SOCKET;
        return OMTP_ERR_TLS;
    }
#else
    while (true)
    {
        int ret = SSL_connect(_ssl);
        if (ret == 1){
            OMTPLOG("SSL_connect success!");
            break;
        }
        
        CTNSocketSelect sel(_connectBreaker, true);
        sel.preSelect();
        
        switch(SSL_get_error(_ssl, ret))
        {
            case SSL_ERROR_WANT_WRITE:
                sel.write_FD_SET(this->socketFD());
                OMTPLOG("SSL_connect SSL_ERROR_WANT_WRITE!");
                break;
            case SSL_ERROR_WANT_READ:
                sel.read_FD_SET(this->socketFD());
                /* Wait for epoll/select to return */
                OMTPLOG("SSL_connect SSL_ERROR_WANT_READ!");
                break;
        }
     
        OMTPLOG("before CTNSocketSelect select:"<<ret);
        sel.exception_FD_SET(this->socketFD());
        ret = sel.select(5000);
        OMTPLOG("after CTNSocketSelect select:"<<ret);
        if (ret == 0){
            OMTPLOG("SSL_connect time out!");
            return OMTP_ERR_TLS;
        }
        if (sel.isException()) {
            return OMTP_ERR_TLS;
        }
        
        if (sel.exception_FD_ISSET(this->socketFD())) {
            return OMTP_ERR_TLS;
        }
        
        if (sel.read_FD_ISSET(this->socketFD())){
            int error;
            socklen_t len = sizeof(error);
#ifndef WIN32
            if (getsockopt(this->socketFD(), SOL_SOCKET, SO_ERROR, (void *) (&error), &len) < 0)
#else
            if (getsockopt(this->socketFD(), SOL_SOCKET, SO_ERROR, (char*)&error, &len) < 0)
#endif
           {
                return OMTP_ERR_TLS;
            }
        }
      
    }
#endif
    
    return OMTP_ERR_SUCCESS;
}

int CSSLSocket::serverCertificateVerify(int preverify_ok, X509_STORE_CTX *ctx)
{
    /* Preverify should have already checked expiry, revocation.
     * We need to verify the hostname. */
    
    /* Always reject if preverify_ok has failed. */
    //    if(!preverify_ok){
    //        return 0;
    //    }
    
    //SSL_CTX_set_cert_verify_callback;
#if 0
    char buf[256] = {0};
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
    X509_NAME_oneline(X509_get_subject_name(cert), buf, 256);
    cout<<"serverCertificateVerify:"<<buf<<endl;;
    
    X509_NAME_oneline(X509_get_issuer_name(ctx->current_cert), buf, 256);
    cout<<"serverCertificateVerify1:"<<buf<<endl;;
    
    STACK_OF(GENERAL_NAME) *san = (STACK_OF(GENERAL_NAME) *)X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
    if(san){
        for(int i=0; i<sk_GENERAL_NAME_num(san); i++){
            //            const GENERAL_NAME *nval = sk_GENERAL_NAME_value(san, i);
            //            cout<<"nval:"<<nval;
        }
    }
    //    SSL * ssl = (SSL*)X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    
    if(X509_STORE_CTX_get_error_depth(ctx) == 0){
        /* FIXME - use X509_check_host() etc. for sufficiently new openssl (>=1.1.x) */
        cert = X509_STORE_CTX_get_current_cert(ctx);
        /* This is the peer certificate, all others are upwards in the chain. */
        //            return _mosquitto_verify_certificate_hostname(cert, mosq->host);
        return 1;
    }else{
        return 1;
    }
#endif
    return 0;
}

int CSSLSocket::connectToHost(const char *host, short port)
{
    OMTPLOG("ENTER CSSLSocket::connectToHost:"<<host<<" port:"<<port);
    
    //socket connect retry 3times;
    int ret = 0;
    ret = CTcpSocket::connectToHost(host, port);
    if (ret != 0){
        OMTPLOG("CTcpSocket::connectToHost failed:"<<ret);
        return ret;
    }
    
    if (_ssl != nullptr){
        SSL_free(_ssl);
        _ssl = nullptr;
    }
    
    if (_ssl_ctx != nullptr){
        SSL_CTX_free(_ssl_ctx);
        _ssl_ctx = nullptr;
    }
    
    _ssl_ctx = SSL_CTX_new(SSLv23_client_method());
//     _ssl_ctx = SSL_CTX_new(SSLv23_method());
    SSL_CTX_set_verify(_ssl_ctx, SSL_VERIFY_NONE, NULL);
    //    SSL_CTX_set_verify(_ssl_ctx, SSL_VERIFY_PEER, &CSSLSocket::serverCertificateVerify);
    //    SSL_CTX_set_verify(_ssl_ctx, SSL_VERIFY_CLIENT_ONCE, &CSSLSocket::serverCertificateVerify);
    //    ret = SSL_CTX_use_certificate_chain_file(_ssl_ctx, "/code/imca.cer");
    //    ret = SSL_CTX_load_verify_locations(_ssl_ctx, "/code/imca.cer", NULL);
    //    if (ret == 0){
    //        ERR_print_errors_fp(stderr);
    //        return OMTP_ERR_TLS;
    //    }
//    SSL_CTX_set_options(_ssl_ctx, SSL_OP_ALL | SSL_OP_NO_TLSv1_2 | SSL_OP_NO_TLSv1_1 | SSL_OP_NO_TLSv1);
    SSL_CTX_set_options(_ssl_ctx, SSL_OP_ALL | SSL_OP_NO_SSLv2);
//    SSL_CTX_set_options(_ssl_ctx, SSL_OP_ALL);
    SSL_CTX_set_mode(_ssl_ctx, SSL_MODE_AUTO_RETRY);
    SSL_CTX_set_mode(_ssl_ctx, SSL_MODE_RELEASE_BUFFERS);
    
    _ssl = SSL_new(_ssl_ctx);
    if (_ssl == NULL){
        OMTPLOG("Can't new SSL");
        return OMTP_ERR_TLS;
    }
//    SSL_set_tlsext_host_name(_ssl, host);

    //SSL_set_ex_data(_ssl, tls_ex_index_mosq, mosq);
    BIO *bio = BIO_new_socket(m_socket, BIO_NOCLOSE);
    if(!bio){
        OMTPLOG("Can't new bio");
        TN_SOCKET_CLOSE(m_socket);
        return OMTP_ERR_TLS;
    }
    
    SSL_set_bio(_ssl, bio, bio);
    
    ret = this->connectTLS();
    OMTPLOG("Exit connectToHost::connectToHost:"<<ret);
    
    return ret;
}


int CSSLSocket::sendData(const char *buf, size_t len)
{
    if (m_socket == INVALID_SOCKET || _ssl == NULL) {
        return -1;
    }
    
    int ret = SSL_write(_ssl, buf, (int)len);
    if(ret <= 0){
        int  err = SSL_get_error(_ssl, ret);
        if (err == SSL_ERROR_WANT_READ
            || err == SSL_ERROR_WANT_WRITE){
            ret = OMTP_ERR_SUCCESS;
        }
        else {
            OMTPLOG("SSL_write error ret:"<<ret<<" err:"<<err);
            ret = OMTP_ERR_SOCK_CLOSE;
        }
    }
    
    return ret;
}

int CSSLSocket::recvData(char *buf, size_t len)
{
    if (m_socket == INVALID_SOCKET || _ssl == NULL) {
        return -1;
    }
    
    int ret = OMTP_ERR_SUCCESS;
    ret = SSL_read(_ssl, buf, (int)len);
    if(ret <= 0){
        int  err = SSL_get_error(_ssl, ret);
        if (err == SSL_ERROR_WANT_READ
            || err == SSL_ERROR_NONE
            || err == SSL_ERROR_WANT_WRITE){
            ret = OMTP_ERR_SUCCESS;
        }
        else {
            OMTPLOG("SSL_read error ret:"<<ret<<" err:"<<err);
            ret = OMTP_ERR_SOCK_CLOSE;
        }
    }
    
    return (int)ret;
}


