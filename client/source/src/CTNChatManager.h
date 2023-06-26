//
//

#ifndef CTNChatManager_hpp
#define CTNChatManager_hpp

#include <stdio.h>

#include "OMTPResponse.h"
#include "CTNProcessThread.h"
#include "OMTPHostInfo.h"
#include "CLog.h"
#include <set>
#include <mutex>

#include "OMTPChatAPI.h"

namespace otim {
        
    
  
/*
 * 聊天管理类
 */
class CTNChatManager : public IOMTPResponse , public IOtimSdk
{
public:
    CTNChatManager();
    virtual ~CTNChatManager();
    static CTNChatManager* instance();
    
    /////// ITNMPResponse ////////////
    virtual void loginResp(int32_t code, const std::string &clientId, const std::string& extrData);
    virtual void kickOut();
    
    virtual void hotSessionResp(otim::HotSessionResp* hotSessions);

    virtual void msgAck(const string& packId, otim::MsgAck * ack);
    virtual void msgRecv(int32_t type, const std::string &packId,  otim::MsgReq * req);
    virtual void netStatusChanged(OMTPNetStatus status);


    /////// ITnImSdk ////////////
    virtual int32_t getNetStatus();
    virtual std::string getClientId(){
        return _clientInfo.clientId;
    }
    virtual int32_t sendMessage (int type, otim::MsgReq& req);

 
    /////// 功能接口 ////////////
    //初始化
    void addHostInfo(OMTPHostInfo &hostInfo);
    void setClientInfo(TNClientInfo &clientInfo);
    void addHostInfo(const std::string& host, int32_t port, bool isSSL);
    int32_t login(const std::string& username, const std::string& password);
    int32_t logout(bool notifyServer);
    virtual void setCallback(IOtimSdkCallback* callback){
        _callback = callback;
    }


protected:
    void createProcessThread ();
    void checkPorcessThread ();
    void addToDiedArray(CTNProcessThread* processThread);
    
    bool isInvalid(int32_t type, otim::MsgReq * req);
    bool checkMessageRepeat(const string& msgId);
private:
    static CTNChatManager*          _instance;
    CTNProcessThread*               _currProcess;
    TNClientInfo           _clientInfo;
    vector<OMTPHostInfo>            _serverVector;
    IOtimSdkCallback*               _callback;
    vector<string>                  _arrRecvMsgId;
    
    vector<CTNProcessThread*> _diedArray;
    std::mutex _diedArrayMutex;
    std::mutex _loginMutex;
    
   //内部接口
    int32_t login();
        
 
    bool isMySelfClientID(const string& clientID);
  
  
    void syncFriends();
    void syncFriendUserInfos();
    void syncGroupChat();
 
    
    void reqUserinfosFromServer(std::vector<std::string> userIds){
        
    }
};
    
    
    class CTNLog : public IOtimLog{
    public:
        static CTNLog* instance();
        
        virtual void writeLog(const std::string& logs);
        virtual const std::string& getLogFileName();
    private:
        static CTNLog* _instance;
    };

    
};
#endif /* CTNChatManager_h */

