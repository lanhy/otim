#ifndef _GroupChatRPCServantImp_H_
#define _GroupChatRPCServantImp_H_

#include "servant/Application.h"
#include "GroupChatRPCServant.h"
#include "otim.h"

/**
 *
 *
 */
class GroupChatRPCServantImp : public otim::GroupChatRPCServant
{
public:
    /**
     *
     */
    virtual ~GroupChatRPCServantImp() {}

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
    virtual tars::Int32 getGroupMember(const std::string & groupId,vector<std::string> &memberIds,tars::TarsCurrentPtr _current_);

    virtual tars::Bool isGroupMember(const std::string & groupId,const std::string & memberId,tars::TarsCurrentPtr _current_);

};
/////////////////////////////////////////////////////
#endif
