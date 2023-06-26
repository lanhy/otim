//
//  CTNSendRecvThread.h
//
//  Created by 兰怀玉 on 16/4/15.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef CTNSendRecvThread_hpp
#define CTNSendRecvThread_hpp

#include "OMTPConst.h"

#include "CThreadEx.h"
#include "otim.h"
#include "CSafeQueue.h"
#include "CTNPacket.h"
#include "CTcpSocket.h"
#include "CBasicQueue.h"
#include "OMTPConfig.h"
#ifdef WIN32
#include "win32/CTNSocketBreaker.h"
#else
#include "unix/CTNSocketBreaker.h"
#endif

#if SSLSocketBuild
#include "CTcpSSLSocket.h"

#else
#endif


const int RECONN_MAX = 6;
const int RECONN_STEP[RECONN_MAX] = {0,1,2,4,8,16};


class CTNProcessThread;

class CTNSendRecvThread : public CThreadEx
{
public:
    CTNSendRecvThread(CTNProcessThread* processThread);
    ~CTNSendRecvThread();
    
    void run();
    
    void sendPacket(otim::OTIMPack& packet);
    
    virtual void stop();

    void setHostInfo(OMTPHostInfo &hostInfo){
        _hostInfo = hostInfo;
    }
    
    const OMTPHostInfo& hostInfo(){
        return _hostInfo;
    }
    
protected:
    void connect();
    
    void processRecvData();
    
    void postRecvPacketBuffer(const TNPacketBuffer * packetBuffer);
    
    void socketClosed();
    
private:
    CBasicQueue<char> _recvByteQueue;
    CSafeQueue<otim::OTIMPack> _queuePacket;
   
    CTNProcessThread* _processThread;
    
    OMTPHostInfo _hostInfo;
    CTcpSocket* _pSocket;
    
    CTNSocketBreaker _rwBreaker;

    int _reconnectTimes;// = 0;
 };

#endif /* CTNSendRecvThread_hpp */
