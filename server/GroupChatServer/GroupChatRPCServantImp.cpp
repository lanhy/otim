#include "GroupChatRPCServantImp.h"
#include "servant/Application.h"
#include "otim.h"
#include "otim_err.h"
#include "ScopeLogger.h"
#include "LongLinkRedis.h"
#include "log.h"
#include "ptcldefine.h"
#include "Common.h"
#include "RedisPool.h"
#include "otim_const.h"
#include "GroupchatManager.h"

using namespace std;

//////////////////////////////////////////////////////
void GroupChatRPCServantImp::initialize()
{
    //initialize servant here:
//    std::string myConfFile = ServerConfig::BasePath+ServerConfig::ServerName +".conf";
//    TC_Config myConf;
//    myConf.parseFile(myConfFile);
//    _msgMaxLen = TC_Common::strto<int>(myConf.get("/otim<maxlen>", "65536"));
//    MLOG_DEBUG("myconf:"<<myConfFile<<" _msgMaxLen:"<<_msgMaxLen);
}

//////////////////////////////////////////////////////
void GroupChatRPCServantImp::destroy()
{
    //destroy servant here:
    //...
}


tars::Int32 GroupChatRPCServantImp::getGroupMember(const std::string & groupId,vector<std::string> &memberIds,tars::TarsCurrentPtr _current_)
{
    if (groupId.empty()){
        return otim::EC_PARAM;
    }
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    GroupchatManager gcIns(redis.get());

    if (!gcIns.isGroupExist(groupId)){
        MLOG_DEBUG("group is not exist:"<<groupId);
        return otim::EC_GROUP_NOT_EXIST;
    }
    
    gcIns.getAllMembers(groupId, memberIds);
   
    MLOG_DEBUG("memberIds size:"<<memberIds.size());

    
    return otim::EC_SUCCESS;
}

tars::Bool  GroupChatRPCServantImp::isGroupMember(const std::string & groupId,const std::string & memberId,tars::TarsCurrentPtr _current_)
{
    if (groupId.empty() || memberId.empty()){
        return  false;
    }
   
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    GroupchatManager gcIns(redis.get());
    
    return gcIns.isExistMember(groupId, memberId);
    
 
}
