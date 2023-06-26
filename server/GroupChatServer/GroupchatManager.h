//
//  GroupchatManager.h
//
//

#ifndef GroupchatManager_h
#define GroupchatManager_h

#include <stdio.h>
#include <string>
#include "RedisDBInterface.h"
#include "otim.h"
#include <map>

const int DEFAULT_MEMBER_LIMIT = 1000;


class GroupchatManager {
public:
    GroupchatManager(CRedisDBInterface *redis);
    ~GroupchatManager(){}

    int getMyGroup(const std::string &userId, std::vector<std::string> &groups);

    int getGroupInfo(const std::string &groupId, otim::GroupChatInfo &groupInfo);

    int saveGroupInfo(const otim::GroupChatInfo &groupInfo);

    int removeGroup(const std::string &groupId);

    void refreshUpdateTime(const std::string &groupId);

    bool isGroupExist(const std::string &groupId);
    
    size_t getMemberCount(const std::string & groupId);

    int  updateGroupCreator(const std::string &groupId,const std::string &userId);
    
    int getAllMembers(const std::string &groupId, std::vector<std::string> &userids);

    bool isExistMember(const std::string &groupId, const std::string &userId);

    bool addMember(const std::string &groupId, const std::string &userId);
    bool delMember(const std::string &groupId, const std::string &userId);
    
    bool addMember(const std::string &groupId, const std::vector<std::string> &userIds);
    bool delMember(const std::string &groupId, const std::vector<std::string> &userIds);


private:
    CRedisDBInterface *_redis;
    
};

#endif /* GroupchatManager_h */
