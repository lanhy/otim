//
//  OMTPConfig.h
//
//  Created by 兰怀玉 on 16/4/18.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef OMTPConfig_hpp
#define OMTPConfig_hpp

#include <string>
#include <vector>
#include <mutex>
#include <map>
#include "OMTPConst.h"
#include "OMTPHostInfo.h"

using namespace std;



#define CONFIG  OMTPConfig::instance()

class OMTPConfig
{
public:
    ~OMTPConfig();

    static OMTPConfig* instance();
    
    void appPath(string appPath);
    
    string appPath(){
        return m_appPath;
    }
    
    void setHostInfos(std::vector<OMTPHostInfo> & vctHostInfo){
        m_vctHostInfo = vctHostInfo;
    }
    
    OMTPHostInfo hostInfo();
        
        
    void clientId(const char*clientId);
    
    string clientId(){
        return m_clientId;
    }
    
    void deviceId(const char* deviceId){
        if (deviceId != NULL) {
            m_deviceId = deviceId;
        }
    }
    
    string deviceId(){
        return m_deviceId;
    }

    
    void username(const char* username){
        if (username != NULL && strlen(username)) {
            m_username = username;
        }
    }
    
    string username(){
        return m_username;
    }
    

    void password(const char* password){
        if (password != NULL && strlen(password) > 0) {
            m_password = password;
        }
    }
    
    string password(){
        return m_password;
    }
    
    
    void keepAlive(int keepAlive){
        m_keepAlive = keepAlive;
    }
    int keepAlive(){
        return m_keepAlive;
    }
  
    void appType(int appType){
        m_appType = appType;
    }
    int appType(){
        return m_appType;
    }
  
    void userType(int userType){
        m_userType = userType;
    }
    int userType(){
        return m_userType;
    }
 
    void authType(int authType){
        m_authType = authType;
    }
    int authType(){
        return m_authType;
    }

    
    
    void deviceType(int appType){
        m_deviceType = appType;
    }
    int deviceType(){
        return m_deviceType;
    }
    
  
    void version(const char* version){
        if (version != NULL) {
            m_version = version;
        }
    }
    
    string version(){
        return m_version;
    }
    
    
    void resetCurrHostIndex(){
        m_currHostIndex = 0;
    }
    
    
    int64_t hsTimestamp();
    
    void setHSTimestamp(int64_t hsTimestamp);
    
    string getIpCache(const char* host);
    void removeIpCache(const char* host);
    
private:
    OMTPConfig();
    
    void loadConfig();
    void loadIpCache();
    void saveIpCache();

    static OMTPConfig* m_instance;
    
    std::vector<OMTPHostInfo> m_vctHostInfo;
    //host->ip
    std::map<string, string> _mapHosts;
    mutex _mutexHosts;

    int m_currHostIndex;
    int m_deviceType;
    int m_appType;
    int m_userType;
    int m_authType;
    
    string m_appPath;
    string m_version;
    string m_deviceId;
    string m_clientId;
    string m_username;
    string m_password;
    
    
    //重连次数
    int m_keepAlive;// = 60 * 1000;
        
    std::map<std::string, int64_t> _mapClientTimestamp;
    
   
};


#endif /* OMTPConfig */
