//
//  CTNChatManager.cpp
//
//

#include "CTNChatManager.h"
#include "CLog.h"
#include <sstream>
#include <algorithm>
#include <regex>
#include "otim_err.h"
#include "OMTPConfig.h"

#ifdef WIN32
#include <objbase.h>
#elif defined(ANDROID)
#else
#include <uuid/uuid.h>
#endif
//using namespace rapidjson;

namespace otim {


void initLog(const std::string& appPath){
    if (appPath.empty()){
        return;
    }
    
    CLog::instance()->setPath(appPath);
}

IOtimSdk* initIm(TNClientInfo &clientInfo) {
    CTNChatManager::instance()->setClientInfo(clientInfo);
  
    return CTNChatManager::instance();
}


IOtimSdk* getImSDK(){
    //        assert(CTNChatManager::_instance !== nullptr);
    return CTNChatManager::instance();
}


std::string generateSessionId(int32_t type, std::string from, std::string to, bool isMyself)
{
    std::string sessionId;
    if (type == otim::PT_MSG_SINGLE_CHAT){
        //单聊，比较from和to的大小，大的在前，中间以 _ 相隔； wxy 兼容之前版本，小的在前
        std::ostringstream s;
        s<<"LP_G1_";
        if (from.compare(to) > 0) {
            s << to << "_" << from;
        } else {
            s << from << "_" << to;
        }
        sessionId = s.str();
    } else if (type == otim::PT_MSG_BIZ_NOTIFY){
        if (isMyself) {
            sessionId = to;
        } else {
            sessionId = from;
        }
    } else if (type == otim::PT_MSG_GROUP_CHAT) {
        sessionId = to;
    } else {
        std::ostringstream s;
        s << "otrherSessionType_" << type;
        sessionId = s.str();
    }
    
    return sessionId;
}

std::string generateMsgId()
{
    char str[64];
    
#ifdef WIN32
    GUID guid;
    ::CoCreateGuid(&guid);
    _snprintf_s(
        str,
        sizeof(str),
        "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        guid.Data1, guid.Data2, guid.Data3,
        guid.Data4[0], guid.Data4[1],
        guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5],
        guid.Data4[6], guid.Data4[7]);
#elif defined(ANDROID)
    time_t temp = time(NULL);
    sprintf(str,"%ld",temp);
#else
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse(uuid, str);
#endif
    
    return string(str);
}


//checkcode
CTNChatManager* CTNChatManager::_instance = nullptr;

CTNChatManager::CTNChatManager () {
    _currProcess = nullptr;
    _callback = nullptr;
}

CTNChatManager::~CTNChatManager () {
}

CTNChatManager* CTNChatManager::instance(){
    if (_instance == nullptr){
        _instance = new CTNChatManager();
    }
    
    return _instance;
}


/////// IOMTPResponse ////////////
//2、登录结果
void CTNChatManager::loginResp(int32_t code,  const std::string &clientId, const std::string& extrData) {
    OMTPLOG("loginResp:"<<code<<" clientId:"<<clientId<<"  extra:"<<extrData);
    
    //process others
    switch (code) {
        case otim::EC_SUCCESS:
        {
            if (!clientId.empty()){
                _clientInfo.clientId = clientId;
                //init the db;
            }
            
            if (_callback != nullptr){
                _callback->loginResp(0, clientId);
                
            } else {
                OMTPLOG("_callback is null loginResp CONNECT_RESULT_SUCCESS");
            }
            break;
        }
        case otim::EC_SERVICE_UNAVAILABLE:
            break;
        case otim::EC_PROTOCOL:
        case otim::EC_CONN_INDICATE:
        case otim::EC_CONN_USERNAME_OR_PASSWORD:
        case otim::EC_CONN_KICKOUT:
//        case otim::EC_CONN_OTHER:
        default:
            OMTPLOG("Kickout:"<< _clientInfo.clientId);
            this->kickOut();
            break;
    }
}

void CTNChatManager::kickOut() {
    
    if (_callback != nullptr){
        _callback->kickOut();
    } else {
        OMTPLOG("_callback is null kickOut");
    }
}

void CTNChatManager::hotSessionResp(otim::HotSessionResp* hotSessions)
{
    OMTPLOG("CTNChatManager::hotSessionResp");
    if (_callback != nullptr){
        _callback->hotSessionResp(hotSessions);
    }
    else {
        OMTPLOG("_callback is null msgRecv");
    }

}

void CTNChatManager::msgAck(const string& packId, otim::MsgAck * ack)
{
    if (_callback != nullptr){
        _callback->msgAck(packId, ack);
    }
    else {
        OMTPLOG("_callback is null msgRecv");
    }

}


void CTNChatManager::msgRecv(int32_t type, const std::string &packId, otim::MsgReq * req) {
    if (isInvalid(type, req)) {
        OMTPLOG("invalid message msgRecv");
        return;
    }
    
    std::string content(req->content.begin(), req->content.end());
    OMTPLOG("msgRecv type:"<<type<<" msg:"<<req->writeToJsonString()<<" content:"<<content);
    if (_callback != nullptr){
        _callback->msgRecv(type, packId, req);
    }
    else {
        OMTPLOG("_callback is null msgRecv");
    }
}

void CTNChatManager::netStatusChanged(OMTPNetStatus status) {
    OMTPLOG("netStatusChanged:"<<status);
    if (_callback != nullptr){
        _callback->netStatusChanged(status);
    } else {
        OMTPLOG("_callback is null netStatusChanged");
    }
    
}



/////// ITnImSdk ////////////


int32_t CTNChatManager::getNetStatus()
{
    if (_currProcess == nullptr){
        return OMTPNetNone;
    }
    
    return _currProcess->getNetStatus();
}


void CTNChatManager::addHostInfo(const std::string& host, int32_t port, bool isSSL)
{
    if (host.empty()){
        return;
    }
    
    OMTPHostInfo hostInfo;
    hostInfo.host = host;
    hostInfo.port = port;
    hostInfo.isSSL = isSSL;
    
    vector<OMTPHostInfo>::iterator iter = find(_serverVector.begin(), _serverVector.end(), hostInfo);
    
    if (iter == _serverVector.end()) {
        _serverVector.push_back(hostInfo);
    }
}

//1、登录 外部接口
int32_t CTNChatManager::login(const std::string& username, const std::string& password) {
    if (password.empty()){
        OMTPLOG("password can't be empty!!");
        return -1;
    }
    
    
    OMTPLOG("TNIMClientOMTP login ENTER  tk:" << password);
    if (_serverVector.size() == 0
//        || _clientInfo.clientId.length() == 0
        ) {
        OMTPLOG("IM Server is not init!");
        return -1;
    }
    
    //save the username password
    _clientInfo.setUsername(username);
    _clientInfo.setPassword(password);
    
    return this->login();
}

int32_t CTNChatManager::login()
{
    std::unique_lock<std::mutex> unilock(_loginMutex);
    if (this->getNetStatus() == OMTPNetConnecting
        || this->getNetStatus() == OMTPNetConnected
        || this->getNetStatus() == OMTPNetRecving){
        OMTPLOG("Netstatus is ok!"<<this->getNetStatus());
        return -1;
    }
    
    if (_clientInfo.password.length() == 0){
        OMTPLOG("CTNChatManager::login token is EMPTY FAILED!!!");
        return -1;
    }
    
    std::vector<OMTPHostInfo> vctHostInfo = _serverVector;
    
    OMTPLOG("otim appType:"<<_clientInfo.appType<<" version:"<<_clientInfo.version);
    this->createProcessThread();
    if (_currProcess == nullptr){
        OMTPLOG("createProcessThread FAILED!!!");
        return -1;
    }
    
    _currProcess->setClientInfo(vctHostInfo, _clientInfo.clientId.c_str(), _clientInfo.appType, _clientInfo.version.c_str(), _clientInfo.deviceType,0, 0);
    _currProcess->setUserInfo(_clientInfo.username.c_str(), _clientInfo.password.c_str(), _clientInfo.deviceId.c_str(), "");
    _currProcess->setAppPath(_clientInfo.appPath.c_str());
    _currProcess->start();
    
    return  0;
}

int32_t CTNChatManager::logout(bool notifyServer) {
    OMTPLOG("TNIMClientOMTP logout ENTER notifyServer:" << notifyServer);
    
    if (_currProcess == nullptr) {
        OMTPLOG("logout _process is nullptr");
        return -1;
    }
    
    try {
        _currProcess->logout(notifyServer);
        if (notifyServer){
            //退出登录后清空上一用户缓存
            _arrRecvMsgId.clear();
        }
        OMTPLOG("after logout");
    } catch (...) {
        OMTPLOG("exception occur logout");
    }
    
    OMTPLOG("TNIMClientOMTP logout EXIT");
    
    return 0;
}

void CTNChatManager::syncFriends()
{
    if (_currProcess == nullptr) {
        OMTPLOG("reqFriends _currProcess is nullptr");
        return ;
    }
    
//    try {
//        otim::GetFriendsReq req;
//        req.relation = (otim::EFriendRelation)0;
//        req.userIds.push_back(_clientInfo.clientId);
//
//        _currProcess->reqFriends(req);
//        OMTPLOG("after reqFriends");
//    } catch (...) {
//        OMTPLOG("exception occur reqFriends");
//    }
}


void CTNChatManager::syncFriendUserInfos()
{
    if (_currProcess == nullptr) {
        OMTPLOG("reqFriends _currProcess is nullptr");
        return ;
    }
    
    try {
    
        
//        this->reqUserinfosFromServer(vctSyncUserId);
        OMTPLOG("after syncFriendUserInfos");
    } catch (...) {
        OMTPLOG("exception occur syncFriendUserInfos");
    }
}

void CTNChatManager::syncGroupChat()
{
    if (_currProcess == nullptr) {
        OMTPLOG("reqFriends _currProcess is nullptr");
        return ;
    }
    
//    try {
//        otim::SyncMyGroupChatsReq req;
//        req.timestamp = 1592988468420867;
//        
//        _currProcess->reqMyGroupchat(req);
//        OMTPLOG("after reqFriends");
//    } catch (...) {
//        OMTPLOG("exception occur reqFriends");
//    }
}



//6、发送消息
int32_t CTNChatManager::sendMessage(int type, otim::MsgReq &req) {
    
    if (_currProcess == nullptr) {
        OMTPLOG("sendMessage _process is nullptr will login!");
        //修改无网重连后消息不能自动发出问题	创建currProcess
        this->login();
    }
    
    if (req.sessionId.empty()){
        req.sessionId = generateSessionId(type, req.from, req.to, true);
    }
//    if (req->msg_id.empty()){
//        req->msg_id = bfim::generateMsgId();
//    }
//     if(req->fromClientId.length() <= 0) {
//         req->fromClientId = _clientInfo.clientId;
//    }
    
    _currProcess->sendMsgReq(type, req, true);
   
    
//    OMTPLOG("seqId:" << req->seq_id << "\tSEND\tLOG\t" << req->msg_id);
    //LOG_LEVEL_DEBUG 新接口没有定义topic参数
    
    return 0;
}

/////// 功能接口 ////////////
void CTNChatManager::setClientInfo(TNClientInfo &clientInfo){
    OMTPLOG("setClientInfo clientId:" << clientInfo.clientId<<" appPath:"<<clientInfo.appPath);
    
    //此处有多线程问题，@lll
    _clientInfo = clientInfo;
}


//////////////////////////////////////////////////
bool CTNChatManager::isInvalid(int32_t type, otim::MsgReq * req){
    if (req == nullptr) {
        OMTPLOG("req is nullptr!");
        return true;
    }
    
    if (req->from.length() == 0 || req->to.length() == 0 || req->content.size() == 0) {
        OMTPLOG("Message is empty, msgId:" << req->sessionId << " to:" << req->to << " from:" << req->from);
        
        return true;
    }
    
    return false;
}


void CTNChatManager::createProcessThread () {
    try {
        if(_currProcess != nullptr) {
            OMTPLOG("createProcessThread Will logout process thread:" <<_currProcess->getId());
            _currProcess->logout(false);
            this->addToDiedArray(_currProcess);
            _currProcess = nullptr;
        }
        
        this->checkPorcessThread();
        
        _currProcess = new CTNProcessThread(this);
        if (_currProcess == nullptr){
            OMTPLOG("!!!!!!!_currProcess CAN'T NEW !!!!!!!!");
            return;
        }
        
        OMTPLOG("The current Process thread:"<< _currProcess->getId());
    } catch (exception e) {
        OMTPLOG("exception occur createOMTPThread :" << e.what());
    }
    OMTPLOG("createProcessThread ok:" << _currProcess->getId());
}

void CTNChatManager::addToDiedArray(CTNProcessThread* processThread){
    std::unique_lock<std::mutex> unilock(_diedArrayMutex);
    _diedArray.push_back(processThread);
}

void CTNChatManager::checkPorcessThread () {
    std::unique_lock<std::mutex> unilock(_diedArrayMutex);
    if (_diedArray.empty()){
        return;
    }
    
    OMTPLOG("checkPorcessThread:"<<_diedArray.size());
    for (int32_t i = _diedArray.size()-1; i >= 0; i--){
        CTNProcessThread* process = _diedArray.at(i);
        if (process == nullptr) {
            _diedArray.erase(_diedArray.begin()+i);
        }
        else if (process->isDied())
        {
            delete process;
            _diedArray.erase(_diedArray.begin()+i);
        }
        else{
            OMTPLOG("checkPorcessThread:"<<_diedArray.size()<<" thread:"<< process->getId());
            process->logout(false);
            process->stop();
        }
    }
}

bool CTNChatManager::isMySelfClientID(const string& clientID) {
    
    if (_clientInfo.clientId == clientID) {
        return true;
    }
    
    return false;
}



bool CTNChatManager::checkMessageRepeat(const string& msgId) {
    if (msgId.length() == 0) {
        OMTPLOG("checkMessageRepeat: error params!");
        return true;
    }
    
    vector<string>::iterator iter = find(_arrRecvMsgId.begin(),_arrRecvMsgId.end(),msgId);
    
    if (iter!=_arrRecvMsgId.end())
    {
        //            OMTPLOG("repeat message give up it:"<<msgId<<" find:"<<*iter);//LOG_LEVEL_NOTICE
        return true;
        
    }
    
    if (_arrRecvMsgId.size() > 100) {
        _arrRecvMsgId.pop_back();
    }
    
    //add to first position
    _arrRecvMsgId.insert(_arrRecvMsgId.begin(), msgId);
    //        _arrRecvMsgId.push_back(msgId);
    
    return false;
}



IOtimLog* getLogInstance(){
    return CTNLog::instance();
}
CTNLog* CTNLog::_instance = nullptr;

CTNLog* CTNLog::instance(){
    if (_instance == nullptr){
        _instance = new CTNLog();
    }
    
    return _instance;
}

const std::string& CTNLog::getLogFileName(){
    
    std::string fileName = CLog::instance()->getFileName();
    return fileName;
}

void CTNLog::writeLog(const std::string&  logs)
{
    if (logs.length() == 0){
        return;
    }
    
    OMTPLOG(logs);
}
}






