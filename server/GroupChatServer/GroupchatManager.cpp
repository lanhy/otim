//
//  GroupchatManager.cpp
//
//

#include "GroupchatManager.h"
#include <map>
#include "otim_const.h"
#include "util/tc_common.h"

GroupchatManager::GroupchatManager(CRedisDBInterface *redis) {
    if (redis == nullptr) {
        return ;
    }
    _redis = redis;
}

int GroupchatManager::updateGroupCreator(const std::string &groupId, const std::string &userId) {
    MLOG_DEBUG("update group creator, groupId:" << groupId << " userId:" << userId);
    

    if (_redis == nullptr || groupId.empty() || userId.empty())
    {
        MLOG_ERROR("_redis or groupId or userId is null:" << groupId);
        return -1;
    }
    
    
    int64_t updateTime = TC_Common::now2ms();
    std::map<std::string, std::string>  infos;
    infos["creatorId"] = userId;
    infos["updateTime"] = std::to_string(updateTime);
    
    EMRStatus redisRet = _redis->MSetHashItem(otim::RKEY_GROUPINFO + groupId, infos);
    if (redisRet != EMRStatus::EM_KVDB_SUCCESS) {
        MLOG_ERROR("SERVICEALARM update group creator:" << groupId << " ret:" << (int)redisRet);
        return (int)redisRet;
    }
    
    return 0;
}

int GroupchatManager::getGroupInfo(const std::string &groupId, otim::GroupChatInfo &groupInfo) {
    if (_redis == nullptr || groupId.empty()) {
        MLOG_ERROR("getGroupInfo _redis or groupId is null:" << groupId);
        return -1;
    }
    
    std::map<std::string, std::string> mapInfo;
    EMRStatus redisRet = _redis->GetAllHashItem(otim::RKEY_GROUPINFO+ groupId, mapInfo);
    if (redisRet != EMRStatus::EM_KVDB_SUCCESS){
        MLOG_ERROR("getGroupInfo :" << groupId << " ret:" << (int)redisRet);
        return -1;
    }
    
    groupInfo.groupId = groupId;
    groupInfo.name = mapInfo["name"];
    groupInfo.avatar = mapInfo["avatar"];
    
    groupInfo.creatorId = mapInfo["creatorId"];
    
    groupInfo.desc = mapInfo["desc"];
    groupInfo.memberLimit = TC_Common::strto<size_t>(mapInfo["memberLimit"]);
    groupInfo.createTime = TC_Common::strto<int64_t>(mapInfo["createTime"]);
    groupInfo.updateTime = TC_Common::strto<int64_t>(mapInfo["updateTime"]);

    if (groupInfo.memberLimit == 0) {
        groupInfo.memberLimit = DEFAULT_MEMBER_LIMIT;
    }
    
    MLOG_INFO("groupId:" << groupInfo.groupId << " name:" << groupInfo.name);
    
    return 0;
}

int GroupchatManager::saveGroupInfo(const otim::GroupChatInfo &groupInfo) {
    if (_redis == nullptr || groupInfo.groupId.empty()) {
        MLOG_ERROR("getGroupInfo _redis or groupId is null:" << groupInfo.groupId);
        return -1;
    }
    
    int64_t updateTime = TC_Common::now2ms();

    std::map<std::string, std::string> mapGroupInfo;
 
    mapGroupInfo["creatorId"] = groupInfo.creatorId;
    mapGroupInfo["name"] = groupInfo.name;
    mapGroupInfo["desc"] = groupInfo.desc;
    mapGroupInfo["createTime"] = std::to_string(groupInfo.createTime);
    mapGroupInfo["updateTime"] = std::to_string(updateTime);
    mapGroupInfo["memberLimit"] = std::to_string(groupInfo.memberLimit);
    mapGroupInfo["avatar"] = groupInfo.avatar;
 
    EMRStatus redisRet = _redis->MSetHashItem(otim::RKEY_GROUPINFO+groupInfo.groupId, mapGroupInfo);
    if (redisRet != EMRStatus::EM_KVDB_SUCCESS) {
        MLOG_ERROR("SERVICEALARM saveGroupInfo :" << groupInfo.groupId << " ret:" << (int)redisRet);
        return (int)redisRet;
    }
    
    MLOG_INFO("groupId:" << groupInfo.groupId << " name:" << groupInfo.name << " memberLimit:" << groupInfo.memberLimit << " creatorId:" << groupInfo.creatorId);
    
    
    return (int)redisRet;
}

void GroupchatManager::refreshUpdateTime(const std::string &groupId) {
    if (_redis == nullptr || groupId.empty()) {
         MLOG_ERROR("getGroupInfo _redis or groupId is null:" << groupId);
         return;
     }
     
    int64_t updateTime = TC_Common::now2ms();

    EMRStatus ret = _redis->SetHashItem(otim::RKEY_GROUPINFO + groupId, "updateTime", std::to_string(updateTime));
    if (ret != EMRStatus::EM_KVDB_SUCCESS) {
        MLOG_ERROR("refreshUpdateTime :" << groupId << " ret:" << (int)ret);
    }
}

int GroupchatManager::removeGroup(const std::string &groupId) {
    if (_redis == nullptr || groupId.empty()) {
        MLOG_ERROR("isGroupExist _redis or groupId is null:" << groupId);
        return -1;
    }
    
    EMRStatus ret = _redis->DelKey(otim::RKEY_GROUPINFO+groupId);
    MLOG_DEBUG("remove group info, groupId:" << groupId << " ret:" << (int)ret);
    
    ret = _redis->DelKey(otim::RKEY_GROUPMEMBER+groupId);
    MLOG_DEBUG("remove group member, groupId:" << groupId << " ret:" << (int)ret);

    return 0;
}

bool GroupchatManager::isGroupExist(const std::string &groupId) {
    if (_redis == nullptr || groupId.empty()) {
        MLOG_ERROR("isGroupExist _redis or groupId is null:" << groupId);
        return false;
    }
    
    bool exist = false;
    std::string key = otim::RKEY_GROUPINFO + groupId;
    if (EMRStatus::EM_KVDB_SUCCESS == _redis->IsStringKeyExits(key, exist)) {
        return exist;
    }
    
    MLOG_DEBUG("isGroupExist check error:" << groupId);
    return exist;
}

size_t GroupchatManager::getMemberCount(const std::string &groupId) {
    if (_redis == nullptr || groupId.empty()) {
        MLOG_WARN("_redis or groupId is null:" << groupId);
        return 0;
    }
    
    INT64 count = 0;
    _redis->HLen(otim::RKEY_GROUPMEMBER+groupId,count);
    MLOG_DEBUG("groupId:" << groupId << " count:" << count);
    
    return (size_t)count;
}


int GroupchatManager::getAllMembers(const std::string &groupId, std::vector<std::string> &userIds) {
    if (_redis == nullptr || groupId.empty()) {
        MLOG_WARN("_redis or groupId is null:" << groupId);
        return -1;
    }
    
    userIds.clear();
    std::map<std::string, std::string> mapAllValue;
    EMRStatus redisRet = _redis->GetAllHashItem(otim::RKEY_GROUPMEMBER+groupId, mapAllValue);
    if (redisRet != EMRStatus::EM_KVDB_SUCCESS) {
        MLOG_WARN("_redis or groupId is null:" << groupId<<" ret:"<<(int)redisRet);
        return -1;
    }
    
    for (auto &it :mapAllValue) {
        userIds.push_back(it.first);
    }
    
    return 0;
}

int GroupchatManager::getMyGroup(const std::string &userId, std::vector<std::string> &groupIds) {
    if (_redis == nullptr || userId.empty()) {
        MLOG_WARN("_redis or userId is null:" << userId);
        return -1;
    }
    
    groupIds.clear();
    std::vector<std::string> scores;
    EMRStatus redisRet = _redis->ZRangeWithScore(otim::RKEY_MYGROUP + userId, groupIds, 0, -1, scores);
    if (redisRet != EMRStatus::EM_KVDB_SUCCESS) {
        MLOG_WARN("_redis or userId is null, get my group failed:" << userId);
        return -1;
    }

    return groupIds.size();
}

bool GroupchatManager::isExistMember(const std::string &groupId, const std::string &userId) {
    std::string value;
    if (EMRStatus::EM_KVDB_SUCCESS == _redis->GetHashItem(otim::RKEY_GROUPMEMBER + groupId, userId, value)) {
        MLOG_DEBUG("check is exist member, groupId:" << groupId << " userId:" << userId << " result:" << !(value.empty()));
        return !(value.empty());
    }
    return false;
}


bool GroupchatManager::addMember(const std::string &groupId, const std::string &userId) {
    if (_redis == nullptr || userId.empty()) {
        MLOG_WARN("_redis or userId is null:" << userId);
        return false;
    }
  
    int64_t updateTime = TC_Common::now2ms();
    
    EMRStatus ret = _redis->HSet(otim::RKEY_GROUPMEMBER + groupId, userId, "ok");
    if (EMRStatus::EM_KVDB_SUCCESS != ret){
        MLOG_DEBUG("addMember RKEY_GROUPMEMBER, sessionid:" << groupId << " ret:" << userId<<" ret:"<<(int)ret);
    }
    ret = _redis->ZSetAdd(otim::RKEY_MYGROUP + userId, updateTime, groupId);
    if (EMRStatus::EM_KVDB_SUCCESS != ret){
        MLOG_DEBUG("addMember RKEY_MYGROUP, sessionid:" << groupId << " userid:" << userId<<" ret:"<<(int)ret);
    }

    this->refreshUpdateTime(groupId);

    return true;
}

bool GroupchatManager::addMember(const std::string &groupId, const std::vector<std::string> &userIds)
{
    if (_redis == nullptr || userIds.empty()) {
        MLOG_WARN("_redis or userId is null:" << userIds.size());
        return false;
    }
  
    int64_t updateTime = TC_Common::now2ms();
    
    for (std::string userId : userIds){
        EMRStatus ret = _redis->HSet(otim::RKEY_GROUPMEMBER + groupId, userId, "ok");
        if (EMRStatus::EM_KVDB_SUCCESS != ret){
            MLOG_DEBUG("addMember RKEY_GROUPMEMBER, sessionid:" << groupId << " ret:" << userId<<" ret:"<<(int)ret);
        }
        ret = _redis->ZSetAdd(otim::RKEY_MYGROUP + userId, updateTime, groupId);
        if (EMRStatus::EM_KVDB_SUCCESS != ret){
            MLOG_DEBUG("addMember RKEY_MYGROUP, sessionid:" << groupId << " userid:" << userId<<" ret:"<<(int)ret);
        }
    }

    this->refreshUpdateTime(groupId);

	return 0;
}

bool GroupchatManager::delMember(const std::string &groupId, const std::string &userId) {
    if (_redis == nullptr || userId.empty() || groupId.empty()) {
        MLOG_WARN("_redis or userId is null:" << userId);
        return false;
    }
            
    EMRStatus ret = _redis->HDel(otim::RKEY_GROUPMEMBER + groupId, userId);
    if (EMRStatus::EM_KVDB_SUCCESS != ret){
        MLOG_ERROR("delMember RKEY_GROUPMEMBER, groupId:" << groupId << " userId:" << userId<<" ret:"<<(int)ret);
    }
    ret = _redis->ZSetRemove(otim::RKEY_MYGROUP + userId, groupId);
    if (EMRStatus::EM_KVDB_SUCCESS != ret){
        MLOG_ERROR("delMember RKEY_MYGROUP, groupId:" << groupId << " userId:" << userId<<" ret:"<<(int)ret);
    }
    this->refreshUpdateTime(groupId);

    return true;
}

bool GroupchatManager::delMember(const std::string &groupId, const std::vector<std::string> &userIds) {
    if (_redis == nullptr || userIds.empty() || groupId.empty()) {
        MLOG_WARN("_redis or userId is empty:" << userIds.size());
        return false;
    }
            
    for (std::string userId : userIds){
        EMRStatus ret = _redis->HDel(otim::RKEY_GROUPMEMBER + groupId, userId);
        if (EMRStatus::EM_KVDB_SUCCESS != ret){
            MLOG_ERROR("delMember RKEY_GROUPMEMBER, groupId:" << groupId << " userId:" << userId<<" ret:"<<(int)ret);
        }
        ret = _redis->ZSetRemove(otim::RKEY_MYGROUP + userId, groupId);
        if (EMRStatus::EM_KVDB_SUCCESS != ret){
            MLOG_ERROR("delMember RKEY_MYGROUP, groupId:" << groupId << " userId:" << userId<<" ret:"<<(int)ret);
        }
    }
    
    this->refreshUpdateTime(groupId);

    return true;
}
