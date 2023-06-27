//
//  UserInfoManager.hpp
//
//  Created by lanhy on 2020/3/6.
//  Copyright © 2020 lanhy. All rights reserved.
//

#ifndef UserInfoManager_hpp
#define UserInfoManager_hpp

#include <stdio.h>
#include <string>
#include <map>
#include "otim.h"
#include "RedisDBInterface.h"
#include "RedisPool.h"

class UserInfoManager
{
public:
    UserInfoManager(CRedisDBInterface *redis = nullptr);
    ~UserInfoManager();
    
    bool getUserInfo(const std::string &userId, otim::UserInfo & userinfo);
    bool getUserInfo(const std::string &userId, std::map<std::string, std::string> & mapUserInfo);
    void saveUserInfo(const otim::UserInfo & userinfo);
    bool getUserRemark(const std::string &userId, const std::string &friendId, std::string & remark);
    
    void addFriend(const std::string& userId, const std::string& friendId);
    void removeFriend(const std::string& userId, const std::string& friendId);
    int getFriends(const std::string& userId, int64_t timestamp, std::vector<otim::FriendInfo> &vctFriend);
    bool checkUserFriend(const std::string& userId, const std::string& friendId);

    std::string getUserAttribute(const std::string &userId, const std::string &friendId, const std::string &attrField);
    int32_t getUserAttribute(const std::string &userId, const std::string &friendId, std::map<std::string, std::string> &attrAll);
    void setUserAttribute(const std::string &userId, const std::string &friendId, const std::string &attrField, const std::string& value);
    
    void setUserAttribute(otim::UserAttribute &userAttribute);

    void setSessionAttribute(otim::SessionAttrSetReq & sessionAttr);

private:
    CRedisDBInterface *_redis;
};
#endif /* UserInfoManager_hpp */
