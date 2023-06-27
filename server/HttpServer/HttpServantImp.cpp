#include "HttpServantImp.h"
#include "servant/Application.h"
#include "JsonUtil.h"

#include "otim.h"
#include "otim_err.h"
#include "otim_const.h"
#include "ptcldefine.h"
#include "log.h"
#include "ScopeLogger.h"
#include "RedisPool.h"
#include "Common.h"
#include "msgcontent.h"
#include "HttpServant.h"
#include "RequestTaskThread.h"
#include "UserFriendServant.h"

using namespace std;

//////////////////////////////////////////////////////
void HttpServantImp::initialize()
{
    //initialize servant here:
    addProcessFunction("/msg/sendsyncmsg", &HttpServantImp::doSendSyncCmd);
    addProcessFunction("/msg/sendsimplemsg", &HttpServantImp::doSendSimpleMsgCmd);
  
    addProcessFunction("/friend/add", &HttpServantImp::doAddFriend);
    addProcessFunction("/friend/del", &HttpServantImp::doDelFriend);
    addProcessFunction("/friend/get", &HttpServantImp::doGetFriends);

}

//////////////////////////////////////////////////////
void HttpServantImp::destroy()
{
    //destroy servant here:
    //...
}

int HttpServantImp::doRequest(TarsCurrentPtr current, vector<char> &buffer)
{
    TC_HttpRequest req;
    TC_HttpResponse rsp;

	// parse request header
    vector<char> v = current->getRequestBuffer();
    string sBuf;
    sBuf.assign(v.data(), v.size());
    req.decode(sBuf);

    int ret = doRequest(req, rsp);

    rsp.encode(buffer);

    return ret;
}

int HttpServantImp::doRequest(const TC_HttpRequest &req, TC_HttpResponse &rsp)
{
    int status = 200;
    string about = "OK";
    string result = "";

    try
    {
        MLOG_DEBUG("http request url:" << req.getOriginRequest());
        auto itFind = _mapHttpProcess.find(req.getRequestUrl());
        if (itFind !=  _mapHttpProcess.end()){
            result = (this->*(itFind->second))(req);
        }
        else{
            MLOG_DEBUG("404 NOT FOUND :" << req.getOriginRequest());
            about = "NOT FOUND";
            status = 404;
        }
        
        MLOG_DEBUG("result:" << result);

    }
    catch(exception &ex)
    {
        status = 500;
//        bClose = true;
        about = "Internal Server Error";
        MLOG_DEBUG( "doRequest ex:" << ex.what());

    }

    rsp.setHeader("Access-Control-Allow-Origin", "*");
    rsp.setContentType("application/json");
    rsp.setResponse(status, about, result);
//    rsp.setResponse(msg.c_str(), msg.size());
    
//    if(bClose) {
//        //必须要在发送数据前设置好Connection:Close标志
//        rsp.setConnection("close");
//    }
  

    return 0;   
}

std::string HttpServantImp::getJsonResp(int code, const std::string &msg)
{
    if (code != 0){
        MLOG_WARN("failed:"<<code<<" msg:"<<msg);
    }
    
    rapidjson::Document root(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& alloc= root.GetAllocator();
    root.AddMember("code", code, alloc);
    root.AddMember("success", code == otim::EC_SUCCESS, alloc);
    root.AddMember("message", msg, alloc);
    
    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    return sb.GetString();
}


std::string HttpServantImp::doSendSimpleMsgCmd(const TC_HttpRequest &cRequest)
{
    SCOPELOGGER(logger);
    logger << "request json:" << cRequest.getContent().c_str();
    std::string strRet = "success";
    int nStatus = 0;
    try
    {
        rapidjson::Document doc;
        doc.Parse(cRequest.getContent().c_str());
        if ( doc.HasParseError() || (!doc.IsObject()))
        {
            strRet = "rapidjson::Document has parse error, or param is invalid";
            MLOG_DEBUG("json error:"<<strRet)
            return getJsonResp(otim::EC_PARAM, strRet);
        }

        int contentType = 0;
        std::string traceId, appId, sessionId,from, to, content,pushTitle, pushSummary;
        
        JsonUtil::getStr(doc, "traceId", traceId);
        JsonUtil::getStr(doc, "appId", appId);
        JsonUtil::getStr(doc, "sessionId", sessionId);
        JsonUtil::getStr(doc, "from", from);
        JsonUtil::getStr(doc, "to", to);
        JsonUtil::getInt(doc, "contentType", contentType);
        JsonUtil::getStr(doc, "content", content);
        JsonUtil::getStr(doc, "pushTitle", pushTitle);
        JsonUtil::getStr(doc, "pushSummary", pushSummary);
  
        MLOG_DEBUG("traceId:"<<traceId<<" appId:"<<appId<<" to:"<<to<<" pushTitle:"<<pushTitle<<" pushSummary:"<<pushSummary);
        if (traceId.empty() || appId.empty() || from.empty()  || to.empty() || content.empty())
        {
            MLOG_DEBUG("traceId,appId, to,title,title or summary is  empty!");
            return getJsonResp(otim::EC_PARAM, "param is empty!");
        }
        
        if (sessionId.empty()){
            sessionId = "BM_" + appId + "_" + to;
        }
       
        otim::MsgReq msgReq;
        msgReq.sessionId = sessionId;
        msgReq.from = from;
        msgReq.to = to;
        msgReq.status = 0;
        msgReq.timestamp = time(NULL);
        msgReq.seqId = otim::genSeqId();
        msgReq.contentType = contentType;
     
        otim::PushInfo pushInfo;
        pushInfo.title = pushTitle;
        pushInfo.summary = pushSummary;
        msgReq.pushInfo = pushInfo;
        
        otim::OTIMPack pack;
        pack.header.type = otim::PT_MSG_BIZ_NOTIFY;
        pack.header.packId = traceId;
        pack.header.flags = otim::PF_COUNTER;

        otim::packTars<otim::MsgReq>(msgReq, pack.payload);
        
        otim::TaskQueueItem item;
        item.traceId = traceId;
        item.taskType = otim::TASK_TYPE_BIZMSG;
        item.pack = pack;
        item.userIds.push_back(to);
        RequestTaskThread::getInstance()->addTask(item);
    }
    catch (std::exception& e)
    {
        MLOG_ERROR("SERVICEALARM doSendMsgCmd error:" << e.what());
        nStatus = otim::EC_SERVER_EXCEPTION;
        strRet = "Fail to connect target Server";
    }
    catch (...)
    {
        MLOG_ERROR("SERVICEALARM doSendMsgCmd unknown exception");
        nStatus = otim::EC_SERVER_EXCEPTION;
        strRet = "Fail to connect target Server";
    }

    return getJsonResp(nStatus, "success");
}


std::string HttpServantImp::doSendSyncCmd(const TC_HttpRequest & cRequest)
{
    SCOPELOGGER(logger);

    logger << "request json:" << cRequest.getContent().c_str();
	rapidjson::Document doc;
    doc.Parse(cRequest.getContent().c_str());
    if ( doc.HasParseError() || (!doc.IsObject()))
    {
        std::string strRet = "rapidjson::Document has parse error, or param is invalid";
        MLOG_DEBUG("json error:"<<strRet)
        return getJsonResp(otim::EC_PARAM, strRet);
    }

    int command = 0;
    std::string traceId, appId, content;
    JsonUtil::getStr(doc, "traceId", traceId);
    JsonUtil::getStr(doc, "appId", appId);
    JsonUtil::getInt(doc, "command", command);
    JsonUtil::getStr(doc, "content", content);
    
    std::vector<std::string> vctUserId;
    JsonUtil::getStrs(doc, "users", vctUserId);
    
    MLOG_DEBUG("traceId:"<<traceId<<" appId:"<<appId<<" command:"<<command<<" content:"<<content);
    if (traceId.empty() || appId.empty() || vctUserId.empty())
    {
        MLOG_DEBUG("traceId,appId, to,title,title or summary is  empty!");
        return getJsonResp(otim::EC_PARAM, "param is empty!");
    }

    otim::SyncDataReq syncReq;
    syncReq.command = command;
    syncReq.content = content;
 
    otim::OTIMPack pack;
    pack.header.type = otim::PT_SYNC_DATA_CMD;
    pack.header.packId = traceId;
    pack.header.version = 1;

    otim::packTars<otim::SyncDataReq>(syncReq, pack.payload);
    
    otim::TaskQueueItem item;
    item.traceId = traceId;
    item.taskType = otim::TASK_TYPE_SYNCCMD;
    item.pack = pack;
    item.userIds = vctUserId;
    
    RequestTaskThread::getInstance()->addTask(item);

    rapidjson::Document root(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& alloc = root.GetAllocator();
    root.AddMember("code", 0, alloc);
    root.AddMember("message", "success", alloc);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);
    return sb.GetString();
}


std::string  HttpServantImp::doAddFriend(const TC_HttpRequest &request)
{
    std::string strBodyData = request.getContent();
    if(strBodyData.empty()){
        return getJsonResp(otim::EC_PARAM, "the request param is invalid");
    }

    MLOG_DEBUG("strBodyData:"<<strBodyData);
    rapidjson::Document cBody;
    cBody.Parse(strBodyData.c_str());

    if(!cBody.IsObject()){
        MLOG_DEBUG("Not json data");
       return getJsonResp(otim::EC_PARAM, "the request param is invalid");
    }

    std::string traceId, userId, friendId;
    JsonUtil::getStr(cBody, "traceId", traceId);
    JsonUtil::getStr(cBody, "userId", userId);
    JsonUtil::getStr(cBody, "friendId",friendId);
    MLOG_DEBUG("traceId:"<<traceId<<" userId:"<<userId<<":"<<friendId);

    if (traceId.empty() || userId.empty() || friendId.empty()){
        MLOG_DEBUG("traceId, userId or friendId is empty");
        return getJsonResp(otim::EC_PARAM, "the request param is invalid");
    }


    std::string desc;
    int code = 0;
    try{
  
        otim::FriendInfo fi;
        fi.userId = userId;
        fi.friendId = friendId;
        
        std::vector<otim::FriendInfo> vctFriend;
        vctFriend.push_back(fi);
        
        otim::UserFriendServantPrx proxy = otim::getServantPrx<otim::UserFriendServantPrx>(PRXSTR_USERFRIEND_RPC);
        code = proxy->addFriend(vctFriend);
        MLOG_INFO("traceId:"<<traceId<<" code:"<<code);
    }
    catch (std::exception& e) {
        MLOG_ERROR("FriendRelationPrx error:" << e.what());
        code = otim::EC_SERVER_EXCEPTION;
        desc = "FriendRelation server process failed!";
    }
    catch (...) {
        MLOG_ERROR("FriendRelationPrx  unknown exception");
        code = otim::EC_SERVER_EXCEPTION;
        desc = "FriendRelation server process failed!";
    }

    rapidjson::Document root(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& alloc= root.GetAllocator();

    root.AddMember("success", true, alloc);
    root.AddMember("code", code, alloc);
    root.AddMember("message", desc, alloc);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);

    return sb.GetString();
}


std::string  HttpServantImp::doDelFriend(const TC_HttpRequest &request)
{
    std::string strBodyData = request.getContent();
    if(strBodyData.empty()){
        return getJsonResp(otim::EC_PARAM, "the request param is invalid");
    }

    MLOG_DEBUG("strBodyData:"<<strBodyData);
    rapidjson::Document cBody;
    cBody.Parse(strBodyData.c_str());

    if(!cBody.IsObject()){
        MLOG_DEBUG("Not json data");
       return getJsonResp(otim::EC_PARAM, "the request param is invalid");
    }


    std::string traceId, userId, friendId;
    JsonUtil::getStr(cBody, "traceId", traceId);
	JsonUtil::getStr(cBody, "userId", userId);
	JsonUtil::getStr(cBody, "friendId",friendId);
    MLOG_DEBUG("traceId:"<<traceId<<" userId:"<<userId<<":"<<friendId);

    if (traceId.empty() || userId.empty() || friendId.empty()){
        MLOG_DEBUG("traceId, userId or friendId is empty");
        return getJsonResp(otim::EC_PARAM, "the request param is invalid");
    }

 
    std::string desc;
    int code = 0;
    try{
        MLOG_INFO("traceId:"<<traceId);
        otim::FriendInfo fi;
        fi.userId = userId;
        fi.friendId = friendId;
        
        std::vector<otim::FriendInfo> vctFriend;
        vctFriend.push_back(fi);
        
        otim::UserFriendServantPrx proxy = otim::getServantPrx<otim::UserFriendServantPrx>(PRXSTR_USERFRIEND_RPC);
        code = proxy->delFriend(vctFriend);
        MLOG_INFO("traceId:"<<traceId<<" code:"<<code);
   }
    catch (std::exception& e) {
        MLOG_ERROR("UserFriendServantPrx error:" << e.what());
        code = otim::EC_SERVER_EXCEPTION;
        desc = "server process failed!";
    }
    catch (...) {
        MLOG_ERROR("UserFriendServantPrx  unknown exception");
        code = otim::EC_SERVER_EXCEPTION;
        desc = "server process failed!";
    }

    rapidjson::Document root(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& alloc= root.GetAllocator();

    root.AddMember("success", true, alloc);
    root.AddMember("code", code, alloc);
    root.AddMember("message", desc, alloc);

    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);

    return sb.GetString();
}

std::string  HttpServantImp::doGetFriends(const TC_HttpRequest &request)
{
    std::string strBodyData = request.getContent();
    if(strBodyData.empty()){
        return getJsonResp(otim::EC_PARAM, "the request param is invalid");
    }

    MLOG_DEBUG("body:"<<strBodyData);
    rapidjson::Document cBody;
    cBody.Parse(strBodyData.c_str());
    if(!cBody.IsObject()){
        MLOG_DEBUG("Not json data:"<<strBodyData);
       return getJsonResp(otim::EC_PARAM, "the request param is invalid");
    }

  
    std::string traceId, userId, friendId;
    JsonUtil::getStr(cBody, "traceId", traceId);
    JsonUtil::getStr(cBody, "userId", userId);
    MLOG_DEBUG("traceId:"<<traceId<<" userId:"<<userId);

    if (traceId.empty() || userId.empty()){
        MLOG_DEBUG("traceId, userId is empty");
        return getJsonResp(otim::EC_PARAM, "the request param is invalid");
    }

    std::string desc;
    int code = 0;
    std::vector<otim::FriendInfo> vctFriend;
    try{
        MLOG_INFO("traceId:"<<traceId);
 
        otim::UserFriendServantPrx proxy = otim::getServantPrx<otim::UserFriendServantPrx>(PRXSTR_USERFRIEND_RPC);
        code = proxy->getFriend(userId, vctFriend);
        MLOG_INFO("traceId:"<<traceId<<" code:"<<code);
   }
    catch (std::exception& e) {
        MLOG_ERROR("UserFriendServantPrx error:" << e.what());
        code = otim::EC_SERVER_EXCEPTION;
        desc = "server process failed!";
    }
    catch (...) {
        MLOG_ERROR("UserFriendServantPrx  unknown exception");
        code = otim::EC_SERVER_EXCEPTION;
        desc = "server process failed!";
    }

    rapidjson::Document root(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& alloc= root.GetAllocator();
    rapidjson::Value arrFriend(rapidjson::kArrayType);

    for (auto item : vctFriend){
        rapidjson::Value itemValue(rapidjson::kObjectType);
        itemValue.AddMember("userId", item.userId, alloc);
        itemValue.AddMember("friendId", item.friendId, alloc);
        arrFriend.PushBack(itemValue, alloc);
    }

    root.AddMember("success", true, alloc);
    root.AddMember("code", code, alloc);
    root.AddMember("message", "", alloc);
    root.AddMember("data", arrFriend, alloc);


    rapidjson::StringBuffer sb;
    rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
    root.Accept(writer);

    return sb.GetString();
}
