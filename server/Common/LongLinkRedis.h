//
//  LonglinkRedis_h
//  tarsim
//
//  Created by lanhy on 2022/3/22.
//

#ifndef LonglinkRedis_h
#define LonglinkRedis_h

#include <stdio.h>
#include <map>
#include <mutex>
#include "util/tc_singleton.h"
#include "servant/Application.h"
#include "util/tc_common.h"
#include "otim.h"

struct LongLink{
    otim::ClientContext context;
    tars::TarsCurrentPtr current;
    int ptclVersion;
    int crypto;          //加密算法
    int compress;        //压缩算法
    int64_t loginTime;
    
    LongLink(){
        ptclVersion = 1;
        crypto = 0;
        compress = 0;
        loginTime = TC_Common::now2ms();
    }
    
    bool isValid(){
        if (context.clientId.empty() || context.deviceId.empty()){
            return false;
        }
        
        return true;
    }
    
    std::map<std::string, std::string> getMap(){
        std::map<std::string, std::string> mapValue;
        mapValue["clientId"] = context.clientId;
        mapValue["deviceType"] = std::to_string(context.deviceType);
        mapValue["deviceId"] = context.deviceId;
        mapValue["uid"] = std::to_string(context.uid);
        mapValue["ptclVersion"] = std::to_string(ptclVersion);
        mapValue["loginTime"] = std::to_string(loginTime);
        mapValue["brokerId"] = context.brokerId;

        return mapValue;
   }
    
    std::string toJson();
    void fromJson(const std::string &json);
};

typedef shared_ptr<LongLink> LongLinkPtr;

class LongLinkRedis : public tars::TC_Singleton<LongLinkRedis>
{
public:
    void remove(const std::string &clientId, int deviceType);
    void add(LongLinkPtr p);
    std::vector<LongLinkPtr> getLongLinkByClientId(const std::string &clientId);
 };


#endif /* LonglinkManager_hpp */
