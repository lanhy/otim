#ifndef _UserFriendServantImp_H_
#define _UserFriendServantImp_H_

#include "servant/Application.h"
#include "UserFriendServant.h"
#include "baseservant.h"
#include "otim.h"

/**
 *UserFriendServantImp
 *
 */
class UserFriendServantImp : public otim::BaseServant
{
public:
    /**
     *
     */
    virtual ~UserFriendServantImp() {}

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();

    /**
     *
     */
    tars::Int32 request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr current);
    
private:
    int syncFriends(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int addFriend(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int removeFriend(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    
    int updateUserInfo(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int getUserInfo(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    
    int setUserAttribute(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int getUserAttribute(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int setSessionAttribute(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);

};
/////////////////////////////////////////////////////
///
//////////////

class UserFriendRPCServantImp : public otim::UserFriendServant
{
public:
    virtual ~UserFriendRPCServantImp() {}

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();

    virtual tars::Int32 addFriend(const vector<otim::FriendInfo> & friends,tars::TarsCurrentPtr _current_);
    virtual tars::Int32 delFriend(const vector<otim::FriendInfo> & friends,tars::TarsCurrentPtr _current_);
    virtual tars::Int32 getFriend(const std::string & userId,vector<otim::FriendInfo> &friends,tars::TarsCurrentPtr _current_);
};

#endif
