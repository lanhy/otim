//
//  OMTPHostInfo.h
//
//  Created by lanhy on 2016/12/15.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef OMTPHostInfo_h
#define OMTPHostInfo_h

#include <string>

struct TNClientInfo {
    TNClientInfo(){
        appType = 0;
        deviceType =  1;
    }
    
    int32_t appType;
    int32_t deviceType;
    std::string username;
    std::string password;
    std::string clientId;
    std::string deviceId;
    std::string version;
    std::string appPath;

    void setUsername(const std::string& value){
        if (value.empty()){
            return;
        }
        
        this->username = value;
    }
    
    void setPassword(const std::string& value){
        if (value.empty()){
            return;
        }
        
        this->password = value;
    }
    
    
    bool isValid(){
        if (clientId.length() == 0 /*|| feedList.size() == 0*/){
            return false;
        }
        
        return true;
    }
};


class OMTPHostInfo{
public:
    std::string host;
    std::string ip;
    int32_t port;
    bool isSSL;
    
    OMTPHostInfo()
    {
        port = 0;
        isSSL = false;
    }

    OMTPHostInfo& operator = (const OMTPHostInfo& other){
        if (this == &other){
            return *this;
        }


        this->host = other.host;
        this->ip = other.ip;
        this->port = other.port;
        this->isSSL = other.isSSL;
       
       return *this;
   }

   bool operator == (const OMTPHostInfo& other){
       if (this == &other){
           return true;
       }

       if (this->host == other.host && this->port == other.port){
           return true;
       }

       return false;
   }
};


#endif /* OMTPHostInfo */
