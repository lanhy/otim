#ifndef _GroupChatServantImp_H_
#define _GroupChatServantImp_H_

#include "servant/Application.h"
#include "otim.h"
#include "baseservant.h"

/**
 *
 *
 */
class GroupChatServantImp : public otim::BaseServant
{
public:
    /**
     *
     */
    virtual ~GroupChatServantImp() {}

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
	virtual tars::Int32 request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr current);

private:
    size_t _msgMaxLen;
    
    int syncGroup(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int createGroup(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int dismissGroup(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int updateGroupCreator(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int updateGroupInfo(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int joinGroup(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int quitGroup(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);
    int getGroupMember(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp);

    
    int sendMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::OTIMPack & resp);
    int saveMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req,const std::string & sessionId,tars::Int64 seqId);
    int updateSession(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::MsgReq &msgReq);
    int dispatchGroupMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req, const std::string &groupId);
    
};
/////////////////////////////////////////////////////
#endif
