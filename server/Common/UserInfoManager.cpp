//
//  UserInfoManager.cpp
//
//  Created by lanhy on 2020/3/6.
//  Copyright © 2020 lanhy. All rights reserved.
//

#include "UserInfoManager.h"
#include <map>
#include "log.h"
#include "otim_const.h"
#include "ScopeLogger.h"

UserInfoManager::UserInfoManager(CRedisDBInterface* redis)
{    
    _redis = redis;
}

UserInfoManager::~UserInfoManager()
{

}

bool UserInfoManager::getUserInfo(const std::string &userId, otim::UserInfo & userInfo)
{
    std::map<std::string, std::string> mapUserinfo;

    this->getUserInfo(userId, mapUserinfo);

    userInfo.userId = userId;
    userInfo.name = mapUserinfo["name"];
    userInfo.mobile = mapUserinfo["mobile"];
    userInfo.avatar = mapUserinfo["avatar"];
    userInfo.birthday = TC_Common::strto<long>(mapUserinfo["birthday"]);
   
    MLOG_INFO("userId:"<<userInfo.userId<<" name:"<<userInfo.name<<" avatar:"<<userInfo.avatar);

    return true;
}

bool UserInfoManager::getUserInfo(const std::string &userId, std::map<std::string, std::string> & mapUserInfo)
{
    if (_redis == nullptr){
        return false;
    }
    
    mapUserInfo.clear();
    
    _redis->GetAllHashItem(otim::RKEY_USERINFO + userId, mapUserInfo);

    return true;
}

void UserInfoManager::saveUserInfo(const otim::UserInfo & userinfo)
{
    if (_redis == nullptr){
        return;
    }

    std::map<std::string, std::string> mapUserinfo;
    mapUserinfo["userId"] = userinfo.userId;
    mapUserinfo["name"] = userinfo.name;
    mapUserinfo["mobile"] = userinfo.mobile;
    mapUserinfo["avatar"] = userinfo.avatar;
    mapUserinfo["birthday"] = userinfo.birthday;


    EMRStatus redisRet = _redis->MSetHashItem(otim::RKEY_USERINFO+userinfo.userId, mapUserinfo);
    if (redisRet != EMRStatus::EM_KVDB_SUCCESS){
        MLOG_ERROR("saveUserInfo :"<<userinfo.userId<<" ret:"<<(int)redisRet);
    }
    
    MLOG_DEBUG("userId:"<<userinfo.userId<<" name:"<<mapUserinfo["name"]);
}

bool UserInfoManager::getUserRemark(const std::string &userId, const std::string &friendId, std::string & remark)
{
    if (_redis == nullptr){
        return false;
    }
 
    _redis->GetHashItem(otim::RKEY_USERINFO+userId, "REMARK", remark);
    MLOG_DEBUG("userId:"<<userId<<" friendId:"<<friendId<<" remark:"<<remark);
    if (remark.size() == 0) {
        return false;
    }
    return true;
}


void UserInfoManager::addFriend(const std::string& userId, const std::string& friendId)
{
    if (_redis == nullptr){
        return;
    }
 
    if (userId.empty() || friendId.empty()){
        MLOG_DEBUG("userId or  friendId is empty:"<<userId<<" friendId:"<<friendId);
        return;
    }
    
    //增加分布式锁，过期时间10s
//    std::string key = "DislockFriend:"+imid+"_"+friendId;
//    CDisLockObj lockObj(redis0.get(), key);
//    DisLockGuard<CDisLockObj> guard(&lockObj, 10);

    int64_t updateTime = TC_Common::now2ms();

    EMRStatus ret = _redis->ZSetAdd(otim::RKEY_FRIEND+userId,updateTime, friendId);
    ret = _redis->ZSetAdd(otim::RKEY_FRIEND+friendId,updateTime, userId);
    MLOG_DEBUG("userId:"<<userId<<" friendId:"<<friendId<<" updateTime:"<<updateTime<<" ret:"<<(int)ret);
}

void UserInfoManager::removeFriend(const std::string& userId, const std::string& friendId)
{
    if (_redis == nullptr){
        return ;
    }

    
    if (userId.empty() || friendId.empty()){
        MLOG_DEBUG("userId or  friendId is empty:"<<userId<<" friendId:"<<friendId);
        return;
    }

    //增加分布式锁，过期时间10s
//    std::string key = "DislockFriend:"+imid+"_"+friendId;
//    CDisLockObj lockObj(redis0.get(), key);
//    DisLockGuard<CDisLockObj> guard(&lockObj, 10);

    EMRStatus ret = _redis->ZSetRemove(otim::RKEY_FRIEND + userId, friendId);
    ret = _redis->ZSetRemove(otim::RKEY_FRIEND + friendId, userId);
    MLOG_INFO("del key:"<<otim::RKEY_FRIEND+userId+"."+friendId<<" ret:"<<(int)ret);

    //清除备注关系属性
    ret = _redis->DelKey(otim::RKEY_USERATTR+userId+"."+friendId);
    MLOG_INFO("del key:"<<otim::RKEY_USERATTR+userId+"."+friendId<<" ret:"<<(int)ret);
    
    ret = _redis->DelKey(otim::RKEY_USERATTR+friendId+"."+userId);
    MLOG_INFO("del key:"<<otim::RKEY_USERATTR+friendId+"."+userId<<" ret:"<<(int)ret);
}

int UserInfoManager::getFriends(const std::string& userId,int64_t timestamp, std::vector<otim::FriendInfo> &vctFriend)
{
    if (_redis == nullptr){
        return -1;
    }

    vctFriend.clear();

    if (userId.empty()){
        MLOG_DEBUG("userId is empty:"<<userId);
        return 0;
    }
    
    std::vector<std::string> keys;
    std::vector<std::string> scores;
    EMRStatus ret = _redis->ZRangeByScoreWithScores(otim::RKEY_FRIEND + userId, timestamp, MAX_SCORE, keys, scores);
    if (EMRStatus::EM_KVDB_SUCCESS != ret){
        MLOG_DEBUG("read DB error:" << userId << " ret: " << (int)ret);
        return 0;
    }
    
    int count = keys.size();
    if (count == 0){
        MLOG_DEBUG("have not friend:" << userId << " ret: " << (int)ret);
        return 0;
    }
    
    MLOG_DEBUG("userId:" << userId << " keep friends: " << count);
    for (int i = 0; i < count; i++){
        otim::FriendInfo friendInfo;
        friendInfo.userId = userId;
        friendInfo.friendId = keys[i];
        friendInfo.remark = getUserAttribute(userId,  friendInfo.friendId, "REMARK");
     
        MLOG_INFO("userId:"<<friendInfo.userId<<" friend:"<<friendInfo.friendId<<" remark:"<<friendInfo.remark)
        vctFriend.push_back(friendInfo);
    }
    
    return 0;
}

bool UserInfoManager::checkUserFriend(const std::string& userId, const std::string& friendId)
{
    if (_redis == nullptr){
        return false;
    }

    INT64 score = 0;
    EMRStatus ret = _redis->ZScore(otim::RKEY_FRIEND + userId, friendId, score);
    MLOG_DEBUG("key:"<<otim::RKEY_FRIEND << userId<<" friendId:"<<friendId<<" score:"<<score<<" ret:"<<(int)ret);

    return score > 0;
}


std::string UserInfoManager::getUserAttribute(const std::string &userId, const std::string &friendId, const std::string &attrField)
{   
    if (_redis == nullptr){
        return "";
    }

    std::string value;
    EMRStatus ret = _redis->GetHashItem(otim::RKEY_USERATTR+ userId + "." + friendId, attrField, value);

    MLOG_DEBUG("userId:"<<userId<<" friendId:"<<friendId<<" attrField:"<<attrField<<" value:"<<value<<" ret:"<<(int)ret);

    return value;
}

int32_t UserInfoManager::getUserAttribute(const std::string &userId, const std::string &friendId, std::map<std::string, std::string> &attrAll)
{   
    if (_redis == nullptr){
        return -1;
    }


    EMRStatus ret = _redis->GetAllHashItem(otim::RKEY_USERATTR+userId + "." + friendId, attrAll);
    MLOG_DEBUG("userId:" << userId << " friendId:" << friendId<<" ret:"<<(int)ret);

    return 0;
}


void UserInfoManager::setUserAttribute(const std::string &userId, const std::string &friendId, const std::string &attrField, const std::string& value)
{
    if (_redis == nullptr){
        return ;
    }

    EMRStatus ret = _redis->SetHashItem(otim::RKEY_USERATTR + userId + "." + friendId, attrField, value);
    MLOG_DEBUG("userId:"<<userId<<" friendId:"<<friendId<<" attrField:"<<attrField<<" value:"<<value<<" ret:"<<(int)ret);
}


void UserInfoManager::setUserAttribute(otim::UserAttribute &userAttribute)
{
    setUserAttribute(userAttribute.userId, userAttribute.friendId, userAttribute.attrName, userAttribute.attrValue);
}

void UserInfoManager::setSessionAttribute(otim::SessionAttrSetReq & sessionAttr)
{
    if (_redis == nullptr){
        return;
    }

    EMRStatus ret = _redis->SetHashItem(otim::RKEY_SESSIONATTR + sessionAttr.userId + "." + sessionAttr.sessionId, sessionAttr.attrName, sessionAttr.attrValue);
    MLOG_DEBUG("sessionAttr:"<<sessionAttr.writeToJsonString()<<" ret:"<<(int)ret);
}
