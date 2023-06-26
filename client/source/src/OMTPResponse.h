//
//  OMTPResponse.h
//
//  Created by 兰怀玉 on 16/6/1.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef OMTPResponse_h
#define OMTPResponse_h

#include <string>
#include <map>
#include <vector>
#include <stdint.h>

#include "OMTPConst.h"

using namespace std;

namespace otim{
    class MsgReq;
    class MsgAck;
    class HotSessionResp;
}

class IOMTPResponse{
public:
    IOMTPResponse(){};
    virtual ~IOMTPResponse(){}
    
    virtual void loginResp(int code, const std::string &clientId, const std::string& extrData) = 0;
    virtual void hotSessionResp(otim::HotSessionResp* hotSessions) = 0;

    virtual void msgAck(const string& packId, otim::MsgAck * ack) = 0;
    virtual void msgRecv(int type, const std::string &packId, otim::MsgReq * req) = 0;
    virtual void netStatusChanged(OMTPNetStatus status) = 0;
   
};


#endif /* OMTPResponse_h */
