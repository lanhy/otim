#include "UserFriendServantImp.h"
#include "servant/Application.h"
#include "otim_err.h"
#include "ScopeLogger.h"
#include "LongLinkRedis.h"
#include "log.h"
#include "ptcldefine.h"
#include "BrokerPushServant.h"
#include "OlapServant.h"
#include "Common.h"
#include "RedisPool.h"
#include "otim_const.h"
#include "UserInfoManager.h"

using namespace std;

//////////////////////////////////////////////////////
void UserFriendServantImp::initialize()
{
    //initialize servant here:
    //...
}

//////////////////////////////////////////////////////
void UserFriendServantImp::destroy()
{
    //destroy servant here:
    //...
}

tars::Int32 UserFriendServantImp::request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr current)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    switch(req.header.type){
        case otim::PT_FRIEND_ADD:
            this->addFriend(clientContext, req, resp);
            break;
        case otim::PT_FRIEND_DEL:
            this->removeFriend(clientContext, req, resp);
            break;
        case otim::PT_FRIEND_SYNC:
            this->syncFriends(clientContext, req, resp);
            break;
        case otim::PT_USERINFO_GET:
            this->getUserInfo(clientContext, req, resp);
            break;
        case otim::PT_USERINFO_UPDATE:
            this->updateUserInfo(clientContext, req, resp);
            break;
        case otim::PT_USERATTRIBUTE_SET:
            this->setUserAttribute(clientContext, req, resp);
            break;
        case otim::PT_USERATTRIBUTE_GET:
            this->getUserAttribute(clientContext, req, resp);
            break;
        case otim::PT_SESSIONATTRIBUTE_SET:
            this->setSessionAttribute(clientContext, req, resp);
            break;
        default:
            MLOG_DEBUG("the type is  invalid:"<<otim::etos((otim::PACK_TYPE)req.header.type));
            return otim::EC_PROTOCOL;
    }
    
    
    return otim::EC_SUCCESS;
}


int UserFriendServantImp::syncFriends(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_FRIEND_SYNC){
        MLOG_DEBUG("the type is not PT_FRIEND_SYNC");
        return otim::EC_PROTOCOL;
    }
    
    otim::FriendSyncResp friendResp;
    otim::FriendSyncReq friendReq;
    otim::unpackTars<otim::FriendSyncReq>(req.payload, friendReq);
  
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());
    std::vector<otim::FriendInfo> vctFriend;
    umIns.getFriends(clientContext.clientId, friendReq.timestamp, friendResp.friends);
    
    friendResp.errorCode.code = otim::EC_SUCCESS;
    otim::packTars<otim::FriendSyncResp>(friendResp, resp.payload);

    return otim::EC_SUCCESS;
}

int UserFriendServantImp::addFriend(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_FRIEND_ADD){
        MLOG_DEBUG("the type is not PT_FRIEND_ADD");
        return otim::EC_PROTOCOL;
    }
    
    otim::CommonErrorCode friendResp;
    otim::FriendAddReq friendReq;
    otim::unpackTars<otim::FriendAddReq>(req.payload, friendReq);
    if (friendReq.friends.empty()){
        MLOG_DEBUG("the friends is empty!");
        friendResp.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(friendResp, resp.payload);
        return otim::EC_PARAM;
    }
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());

    std::vector<std::string> vctFriendId;
    for (otim::FriendInfo fi : friendReq.friends){
        umIns.addFriend(fi.userId, fi.friendId);
        vctFriendId.push_back(fi.friendId);
    }
  
    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_FRIEND, "", vctFriendId);

    friendResp.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(friendResp, resp.payload);

    return otim::EC_SUCCESS;
}

int UserFriendServantImp::removeFriend(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp)
{
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_FRIEND_DEL){
        MLOG_DEBUG("the type is not PT_FRIEND_DEL");
        return otim::EC_PROTOCOL;
    }
    
    otim::CommonErrorCode friendResp;
    otim::FriendDelReq friendReq;
    otim::unpackTars<otim::FriendDelReq>(req.payload, friendReq);
    if (friendReq.friends.empty()){
        MLOG_DEBUG("the friends is empty!");
        friendResp.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(friendResp, resp.payload);
        return otim::EC_PARAM;
    }

    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());

    std::vector<std::string> vctFriendId;
    for (otim::FriendInfo fi : friendReq.friends){
        umIns.removeFriend(fi.userId, fi.friendId);
        vctFriendId.push_back(fi.friendId);
    }
  
    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_FRIEND, "", vctFriendId);

    friendResp.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(friendResp, resp.payload);
    return otim::EC_SUCCESS;
}


int UserFriendServantImp::updateUserInfo(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp)
{
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_USERINFO_UPDATE){
        MLOG_DEBUG("the type is not PT_USERINFO_UPDATE");
        return otim::EC_PROTOCOL;
    }
    
    otim::CommonErrorCode userInfoResp;
    otim::UserInfoUpdateReq userInfoReq;
    otim::unpackTars<otim::UserInfoUpdateReq>(req.payload, userInfoReq);
    if (userInfoReq.userInfo.userId.empty()){
        MLOG_DEBUG("the friends is empty!");
        userInfoResp.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(userInfoResp, resp.payload);
        return otim::EC_PARAM;
    }

    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());
    umIns.saveUserInfo(userInfoReq.userInfo);

    userInfoResp.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(userInfoResp, resp.payload);
    
    return otim::EC_SUCCESS;
}

int UserFriendServantImp::getUserInfo(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp)
{
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_USERINFO_GET){
        MLOG_DEBUG("the type is not PT_USERINFO_GET");
        return otim::EC_PROTOCOL;
    }
    
    otim::UserInfoGetResp userInfoResp;
    otim::UserInfoGetReq userInfoReq;
    otim::unpackTars<otim::UserInfoGetReq>(req.payload, userInfoReq);
    if (userInfoReq.userIds.empty()){
        MLOG_DEBUG("the friends is empty!");
        userInfoResp.errorCode.code = otim::EC_PARAM;
        otim::packTars<otim::UserInfoGetResp>(userInfoResp, resp.payload);
        return otim::EC_PARAM;
    }

    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());
    
    for (std::string userId : userInfoReq.userIds){
        otim::UserInfo userinfo;
        
        umIns.getUserInfo(userId, userinfo);

        userInfoResp.userInfos.push_back(userinfo);
    }

    userInfoResp.errorCode.code = otim::EC_SUCCESS;
    otim::packTars<otim::UserInfoGetResp>(userInfoResp, resp.payload);
  
    
    return otim::EC_SUCCESS;
}


int UserFriendServantImp::setUserAttribute(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp)
{
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_USERATTRIBUTE_SET){
        MLOG_DEBUG("the type is not PT_USERATTRIBUTE_SET");
        return otim::EC_PROTOCOL;
    }
    
//    struct UserAttribute
//    {
//        0 require string userId;
//        1 require string friendId;
//        2 require string attrName;
//        3 require string attrValue;
//    };
    
    otim::CommonErrorCode userAttrResp;
    otim::UserAttrSetReq userAttrReq;
    otim::unpackTars<otim::UserAttrSetReq>(req.payload, userAttrReq);
    if (userAttrReq.attribute.userId.empty()
        || userAttrReq.attribute.friendId.empty()
        || userAttrReq.attribute.attrName.empty()
        || userAttrReq.attribute.attrValue.empty()){
        MLOG_DEBUG("the attribute is empty:"<<userAttrReq.writeToJsonString());
        userAttrResp.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(userAttrResp, resp.payload);
        return otim::EC_PARAM;
    }

    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());
    
    umIns.setUserAttribute(userAttrReq.attribute);
 
    userAttrResp.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(userAttrResp, resp.payload);

    return otim::EC_SUCCESS;
}

int UserFriendServantImp::getUserAttribute(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp)
{
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_USERATTRIBUTE_GET){
        MLOG_DEBUG("the type is not PT_USERATTRIBUTE_GET");
        return otim::EC_PROTOCOL;
    }
    
//    struct UserAttribute
//    {
//        0 require string userId;
//        1 require string friendId;
//        2 require string attrName;
//        3 require string attrValue;
//    };
    
    otim::UserAttrGetResp userAttrResp;
    otim::UserAttribute userAttrReq;
    otim::unpackTars<otim::UserAttribute>(req.payload, userAttrReq);
    if (userAttrReq.userId.empty()
        || userAttrReq.friendId.empty()){
        MLOG_DEBUG("the attribute is empty:"<<userAttrReq.writeToJsonString());
        userAttrResp.errorCode.code = otim::EC_PARAM;
        otim::packTars<otim::UserAttrGetResp>(userAttrResp, resp.payload);
        return otim::EC_PARAM;
    }

    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());
    
    std::map<std::string, std::string> mapAttr;
    if (!userAttrReq.attrName.empty()){
        mapAttr[userAttrReq.attrName] = umIns.getUserAttribute(userAttrReq.userId, userAttrReq.friendId, userAttrReq.attrName);
    }
    else{
        umIns.getUserAttribute(userAttrReq.userId, userAttrReq.friendId, mapAttr);
    }
    
    for (auto iter : mapAttr){
        otim::UserAttribute userAttr = userAttrReq;
        userAttr.attrName = iter.first ;
        userAttr.attrValue = iter.second;
        
        MLOG_INFO("user attr:"<<userAttrReq.writeToJsonString());
        userAttrResp.attributes.push_back(userAttr);
    }
    
    userAttrResp.errorCode.code = otim::EC_SUCCESS;
    otim::packTars<otim::UserAttrGetResp>(userAttrResp, resp.payload);

    return otim::EC_SUCCESS;
}

int UserFriendServantImp::setSessionAttribute(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp)
{
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_SESSIONATTRIBUTE_SET){
        MLOG_DEBUG("the type is not PT_SESSIONATTRIBUTE_SET");
        return otim::EC_PROTOCOL;
    }
    
//    struct SessionAttrSetReq
//    {
//        0 require string userId;
//        1 require string sessionId;
//        2 require string attrName;
//        3 require string attrValue;
//    };
    
    otim::CommonErrorCode userAttrResp;
    otim::SessionAttrSetReq userAttrReq;
    otim::unpackTars<otim::SessionAttrSetReq>(req.payload, userAttrReq);
    if (userAttrReq.userId.empty()
        || userAttrReq.sessionId.empty()
        || userAttrReq.attrName.empty()
        || userAttrReq.attrValue.empty()){
        MLOG_DEBUG("the attribute is empty:"<<userAttrReq.writeToJsonString());
        userAttrResp.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(userAttrResp, resp.payload);
        return otim::EC_PARAM;
    }

    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());
    
    umIns.setSessionAttribute(userAttrReq);
 
    userAttrResp.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(userAttrResp, resp.payload);


    return otim::EC_SUCCESS;
}



/**
 UserFriendRPCServantImp
 */

void UserFriendRPCServantImp::initialize()
{
    
}

void UserFriendRPCServantImp::destroy()
{
    
}

tars::Int32 UserFriendRPCServantImp::addFriend(const vector<otim::FriendInfo> & friends,tars::TarsCurrentPtr _current_)
{
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());

    std::vector<std::string> vctFriendId;
    for (otim::FriendInfo fi : friends){
        umIns.addFriend(fi.userId, fi.friendId);
        vctFriendId.push_back(fi.friendId);
        MLOG_DEBUG("add friend userId:"<<fi.userId<<" friend:"<<fi.friendId);
    }
  
    otim::ClientContext clientContext;
    clientContext.clientId = "RPC";
    clientContext.brokerId = "";
    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_FRIEND, "", vctFriendId);

    
    return otim::EC_SUCCESS;
}

tars::Int32 UserFriendRPCServantImp::delFriend(const vector<otim::FriendInfo> & friends,tars::TarsCurrentPtr _current_)
{
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());

    std::vector<std::string> vctFriendId;
    for (otim::FriendInfo fi : friends){
        umIns.removeFriend(fi.userId, fi.friendId);
        vctFriendId.push_back(fi.friendId);
       
        MLOG_DEBUG("remove friend userId:"<<fi.userId<<" friend:"<<fi.friendId);
    }
    
    otim::ClientContext clientContext;
    clientContext.clientId = "RPC";
    clientContext.brokerId = "";

    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_FRIEND, "", vctFriendId);


    return otim::EC_SUCCESS;
}

tars::Int32 UserFriendRPCServantImp::getFriend(const std::string & userId,vector<otim::FriendInfo> &friends,tars::TarsCurrentPtr _current_)
{
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    UserInfoManager umIns(redis.get());

    umIns.getFriends(userId, 0, friends);
    MLOG_DEBUG("userId:"<<userId<<" friend count:"<<friends.size());

    return otim::EC_SUCCESS;
}
