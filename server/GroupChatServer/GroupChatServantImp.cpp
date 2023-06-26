#include "GroupChatServantImp.h"
#include "servant/Application.h"
#include "otim.h"
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
#include "GroupchatManager.h"

using namespace std;

//////////////////////////////////////////////////////
void GroupChatServantImp::initialize()
{
    //initialize servant here:
    std::string myConfFile = ServerConfig::BasePath+ServerConfig::ServerName +".conf";
    TC_Config myConf;
    myConf.parseFile(myConfFile);
    _msgMaxLen = TC_Common::strto<int>(myConf.get("/otim<maxlen>", "65536"));
    MLOG_DEBUG("myconf:"<<myConfFile<<" _msgMaxLen:"<<_msgMaxLen);
}

//////////////////////////////////////////////////////
void GroupChatServantImp::destroy()
{
    //destroy servant here:
    //...
}

tars::Int32 GroupChatServantImp::request(const otim::ClientContext & clientContext,const otim::OTIMPack & req,otim::OTIMPack &resp,tars::TarsCurrentPtr current)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"req header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    switch(req.header.type){
        case otim::PT_MSG_GROUP_CHAT:
            this->sendMsg(clientContext, req, resp);
            break;
        case otim::PT_GROUPCHAT_SYNC:
            this->syncGroup(clientContext, req, resp);
            break;
        case otim::PT_GROUPCHAT_CREATE:
            this->createGroup(clientContext, req, resp);
            break;
        case otim::PT_GROUPCHAT_JION:
            this->joinGroup(clientContext, req, resp);
            break;
        case otim::PT_GROUPCHAT_QUIT:
            this->quitGroup(clientContext, req, resp);
            break;
        case otim::PT_GROUPCHAT_DISMISS:
            this->dismissGroup(clientContext, req, resp);
            break;
        case otim::PT_GROUPCHAT_UPDATE_CREATOR:
            this->updateGroupCreator(clientContext, req, resp);
            break;
        case otim::PT_GROUPCHAT_INFO_UPDATE:
            this->updateGroupInfo(clientContext, req, resp);
            break;
        case otim::PT_GROUPCHAT_MEMBERS_GET:
            this->getGroupMember(clientContext, req, resp);
            break;
        default:
            MLOG_DEBUG("the type is  invalid:"<<otim::etos((otim::PACK_TYPE)req.header.type));
            return otim::EC_PROTOCOL;
    }
    
    scopelogger<<"resp header:"<<resp.header.writeToJsonString();

    return otim::EC_SUCCESS;
}

int GroupChatServantImp::syncGroup(const otim::ClientContext & clientContext, const otim::OTIMPack & req,  otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_GROUPCHAT_SYNC){
        MLOG_DEBUG("the type is not PT_GROUPCHAT_SYNC");
        return otim::EC_PROTOCOL;
    }
    
    otim::GroupChatSyncReq groupChatSyncReq;
    otim::unpackTars<otim::GroupChatSyncReq>(req.payload, groupChatSyncReq);
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    GroupchatManager gmIns(redis.get());
    
    std::vector<std::string> vctGroupId;
    gmIns.getMyGroup(clientContext.clientId, vctGroupId);
    
    otim::GroupChatSyncResp groupSyncResp;
    
    for (auto groupId : vctGroupId) {
        otim::GroupChatInfo groupInfo;
        int ret = gmIns.getGroupInfo(groupId, groupInfo);
        if (ret != 0 || groupInfo.groupId.empty()) {
            MLOG_WARN("get group info failed, groupId:" << groupId);
            continue;
        }
        
        // 全量id返回列表
        groupSyncResp.groupIds.push_back(groupId);
        
        groupSyncResp.timestamp = std::max(groupSyncResp.timestamp, groupInfo.updateTime);
        if (groupInfo.updateTime < groupChatSyncReq.timestamp) {
            MLOG_DEBUG("will give up groupId:" <<groupId << " updateTime:" << groupInfo.updateTime);
            continue;
        }
        
        groupSyncResp.groupChats.push_back(groupInfo);
    }
    
    groupSyncResp.errorCode.code = otim::EC_SUCCESS;
    otim::packTars<otim::GroupChatSyncResp>(groupSyncResp, resp.payload);
    
    MLOG_DEBUG("clientId:" << clientContext.clientId << " groupSyncResp:" << groupSyncResp.writeToJsonString());
    
    return otim::EC_SUCCESS;
}

int GroupChatServantImp::createGroup(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    if (req.header.type != otim::PT_GROUPCHAT_CREATE){
        MLOG_DEBUG("the type is not PT_GROUPCHAT_CREATE");
        return otim::EC_PROTOCOL;
    }
    
    otim::GroupChatCreateReq groupCreateReq;
    otim::unpackTars<otim::GroupChatCreateReq>(req.payload, groupCreateReq);
    
    std::string groupId = "GC_" + std::to_string(TC_Common::now2ms());
    groupCreateReq.groupInfo.groupId = groupId;
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    
    GroupchatManager gcIns(redis.get());
    gcIns.saveGroupInfo(groupCreateReq.groupInfo);
    
    MLOG_DEBUG("groupInfo:"<<groupCreateReq.writeToJsonString());
    gcIns.addMember(groupId, groupCreateReq.memberIds);
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    otim::GroupChatCreateResp groupResp;
    groupResp.errorCode.code = otim::EC_SUCCESS;
    groupResp.groupId = groupId;
    otim::packTars<otim::GroupChatCreateResp>(groupResp, resp.payload);
    
    //send event async
    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_GROUP, groupId, groupCreateReq.memberIds);
    
    return otim::EC_SUCCESS;
}

int GroupChatServantImp::dismissGroup(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_GROUPCHAT_DISMISS){
        MLOG_DEBUG("the type is not PT_GROUPCHAT_DISMISS");
        return otim::EC_PROTOCOL;
    }
    
    otim::CommonErrorCode errorCode;
    otim::GroupChatDismissReq dismissReq;
    otim::unpackTars<otim::GroupChatDismissReq>(req.payload, dismissReq);
    if (dismissReq.groupId.empty()){
        MLOG_DEBUG("the groupId is empty!");
        errorCode.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_PARAM;
    }
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    GroupchatManager gcIns(redis.get());
    otim::GroupChatInfo groupInfo;
    int ret = gcIns.getGroupInfo(dismissReq.groupId, groupInfo);
    if (ret < 0){
        errorCode.code = otim::EC_GROUP_NOT_EXIST;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_GROUP_NOT_EXIST;
    }
    
    if (dismissReq.operatorId != groupInfo.creatorId){
        MLOG_DEBUG("Forbidden, the operatorId is not creatorId old:"<<groupInfo.creatorId<<" current:"<<dismissReq.operatorId);
        errorCode.code = otim::EC_GROUP_CREAGOR_ERROR;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_GROUP_CREAGOR_ERROR;
    }
    
    std::vector<std::string> userIds;
    gcIns.getAllMembers(dismissReq.groupId, userIds);
 
    gcIns.removeGroup(dismissReq.groupId);
    
    gcIns.delMember(dismissReq.groupId, userIds);

    errorCode.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
    
    //send event async
    //notify to the member
    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_GROUP, "", userIds);

    
    return otim::EC_SUCCESS;
}

int GroupChatServantImp::updateGroupCreator(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_GROUPCHAT_UPDATE_CREATOR){
        MLOG_DEBUG("the type is not PT_GROUPCHAT_UPDATE_CREATOR");
        return otim::EC_PROTOCOL;
    }
    
    otim::CommonErrorCode errorCode;
    otim::GroupChatCreatorUpdateReq creatorUpdateReq;
    otim::unpackTars<otim::GroupChatCreatorUpdateReq>(req.payload, creatorUpdateReq);
    if (creatorUpdateReq.groupId.empty()
        || creatorUpdateReq.operatorId.empty()
        || creatorUpdateReq.newCreatorId.empty()){
        MLOG_DEBUG("the groupId is empty!");
        errorCode.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_PARAM;
    }
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    GroupchatManager gcIns(redis.get());
    otim::GroupChatInfo groupInfo;
    int ret = gcIns.getGroupInfo(creatorUpdateReq.groupId, groupInfo);
    if (ret < 0){
        errorCode.code = otim::EC_GROUP_NOT_EXIST;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_GROUP_NOT_EXIST;
    }
    
    if (creatorUpdateReq.operatorId != groupInfo.creatorId){
        MLOG_DEBUG("Forbidden, the operatorId is not creatorId!");
        errorCode.code = otim::EC_GROUP_CREAGOR_ERROR;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_GROUP_CREAGOR_ERROR;
    }
    
    gcIns.addMember(creatorUpdateReq.groupId, creatorUpdateReq.newCreatorId);
    gcIns.delMember(creatorUpdateReq.groupId, creatorUpdateReq.operatorId);
    gcIns.updateGroupCreator(creatorUpdateReq.groupId, creatorUpdateReq.newCreatorId);

    errorCode.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
    
    //send event async
    std::vector<std::string> userIds;
    gcIns.getAllMembers(creatorUpdateReq.groupId, userIds);
    userIds.push_back(creatorUpdateReq.operatorId);
    
    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_GROUPINFO, "",  userIds);

    return otim::EC_SUCCESS;
}

int GroupChatServantImp::updateGroupInfo(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_GROUPCHAT_INFO_UPDATE){
        MLOG_DEBUG("the type is not PT_GROUPCHAT_INFO_UPDATE");
        return otim::EC_PROTOCOL;
    }
    
    otim::CommonErrorCode errorCode;
    otim::GroupChatInfoUpdateReq groupInfoUpdateReq;
    otim::unpackTars<otim::GroupChatInfoUpdateReq>(req.payload, groupInfoUpdateReq);
    if (groupInfoUpdateReq.groupInfo.groupId.empty() || groupInfoUpdateReq.operatorId.empty()){
        MLOG_DEBUG("the groupId or operatorId is empty:"<<groupInfoUpdateReq.writeToJsonString());
        errorCode.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_PARAM;
    }
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    GroupchatManager gcIns(redis.get());
    otim::GroupChatInfo groupInfo;
    int ret = gcIns.getGroupInfo(groupInfoUpdateReq.groupInfo.groupId, groupInfo);
    if (ret < 0){
        errorCode.code = otim::EC_GROUP_NOT_EXIST;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_GROUP_NOT_EXIST;
    }
    
    if (groupInfoUpdateReq.operatorId != groupInfo.creatorId){
        MLOG_DEBUG("Forbidden, the operatorId is not creatorId!");
        errorCode.code = otim::EC_GROUP_CREAGOR_ERROR;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_GROUP_CREAGOR_ERROR;
    }
    
    if (!groupInfoUpdateReq.groupInfo.name.empty()){
        groupInfo.name = groupInfoUpdateReq.groupInfo.name;
    }
    if (!groupInfoUpdateReq.groupInfo.avatar.empty()){
        groupInfo.avatar = groupInfoUpdateReq.groupInfo.avatar;
    }
    
    gcIns.saveGroupInfo(groupInfo);
    
    errorCode.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
    
    //send event async
    std::vector<std::string> userIds;
    gcIns.getAllMembers(groupInfoUpdateReq.groupInfo.groupId, userIds);
    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_GROUPINFO,"", userIds);

    
    return otim::EC_SUCCESS;
}

int GroupChatServantImp::joinGroup(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_GROUPCHAT_JION){
        MLOG_DEBUG("the type is not PT_GROUPCHAT_JION");
        return otim::EC_PROTOCOL;
    }
    
    otim::CommonErrorCode errorCode;
    otim::GroupChatJoinQuitReq joinReq;
    otim::unpackTars<otim::GroupChatJoinQuitReq>(req.payload, joinReq);
    if (joinReq.groupId.empty()
        || joinReq.memberIds.size() == 0){
        MLOG_DEBUG("the groupId is empty!");
        errorCode.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_PARAM;
    }
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    GroupchatManager gcIns(redis.get());
    otim::GroupChatInfo groupInfo;
    int ret = gcIns.getGroupInfo(joinReq.groupId, groupInfo);
    if (ret < 0){
        errorCode.code = otim::EC_GROUP_NOT_EXIST;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_GROUP_NOT_EXIST;
    }
    
    MLOG_DEBUG("groupInfo:"<<joinReq.writeToJsonString());
    gcIns.addMember(joinReq.groupId, joinReq.memberIds);
    
    errorCode.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
    
    //send event async
    std::vector<std::string> userIds;
    gcIns.getAllMembers(joinReq.groupId, userIds);
    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_GROUPMENBER, "", userIds);

    //新加入的要同步群聊
    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_GROUP, "", joinReq.memberIds);

    return otim::EC_SUCCESS;
}

int GroupChatServantImp::quitGroup(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_GROUPCHAT_QUIT){
        MLOG_DEBUG("the type is not PT_GROUPCHAT_QUIT");
        return otim::EC_PROTOCOL;
    }
    
    otim::CommonErrorCode errorCode;
    otim::GroupChatJoinQuitReq quitReq;
    otim::unpackTars<otim::GroupChatJoinQuitReq>(req.payload, quitReq);
    if (quitReq.groupId.empty()
        || quitReq.memberIds.size() == 0){
        MLOG_DEBUG("the groupId is empty!");
        errorCode.code = otim::EC_PARAM;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_PARAM;
    }
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    GroupchatManager gcIns(redis.get());
    otim::GroupChatInfo groupInfo;
    int ret = gcIns.getGroupInfo(quitReq.groupId, groupInfo);
    if (ret < 0){
        errorCode.code = otim::EC_GROUP_NOT_EXIST;
        otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
        return otim::EC_GROUP_NOT_EXIST;
    }
    
    
    MLOG_DEBUG("groupInfo:"<<quitReq.writeToJsonString());
    gcIns.delMember(quitReq.groupId, quitReq.memberIds);
    
    errorCode.code = otim::EC_SUCCESS;
    otim::packTars<otim::CommonErrorCode>(errorCode, resp.payload);
    
    //send event async
    std::vector<std::string> userIds;
    gcIns.getAllMembers(quitReq.groupId, userIds);
    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_GROUPMENBER, "", userIds);

    otim::sendSyncMsg(clientContext, otim::SYNC_CMD_GROUP, "", quitReq.memberIds);

    return otim::EC_SUCCESS;
}

int GroupChatServantImp::getGroupMember(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    if (req.header.type != otim::PT_GROUPCHAT_MEMBERS_GET){
        MLOG_DEBUG("the type is not PT_GROUPCHAT_MEMBERS_GET");
        return otim::EC_PROTOCOL;
    }
    
    otim::GroupChatMemberGetResp memberGetResp;
    otim::GroupChatMemberGetReq memberGetReq;
    otim::unpackTars<otim::GroupChatMemberGetReq>(req.payload, memberGetReq);
    if (memberGetReq.groupId.empty()){
        MLOG_DEBUG("the groupId is empty!");
        memberGetResp.errorCode.code = otim::EC_PARAM;
        otim::packTars<otim::GroupChatMemberGetResp>(memberGetResp, resp.payload);
        return otim::EC_PARAM;
    }
    
    memberGetResp.groupId = memberGetReq.groupId;

    otim::RedisConnPtr redis(otim::RedisPool::instance());
    GroupchatManager gcIns(redis.get());
    otim::GroupChatInfo groupInfo;
    int ret = gcIns.getGroupInfo(memberGetReq.groupId, groupInfo);
    if (ret < 0){
        memberGetResp.errorCode.code = otim::EC_GROUP_NOT_EXIST;
        otim::packTars<otim::GroupChatMemberGetResp>(memberGetResp, resp.payload);
        return otim::EC_GROUP_NOT_EXIST;
    }
    
    
    gcIns.getAllMembers(memberGetReq.groupId, memberGetResp.memberIds);
   
    MLOG_DEBUG("memberGetResp:"<<memberGetResp.writeToJsonString());

    memberGetResp.errorCode.code = otim::EC_SUCCESS;
    otim::packTars<otim::GroupChatMemberGetResp>(memberGetResp, resp.payload);
    return otim::EC_SUCCESS;
}

int GroupChatServantImp::sendMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::OTIMPack & resp)
{
    SCOPELOGGER(scopelogger);
    scopelogger<<"header:"<<req.header.writeToJsonString();
    
    if (req.header.type != otim::PT_MSG_GROUP_CHAT){
        MLOG_DEBUG("the type is not PT_MSG_GROUP_CHAT:"<<req.header.type);
        return otim::EC_PROTOCOL;
    }
    
    
    resp.header = req.header;
    resp.header.flags |= otim::PF_ISACK;
    
    otim::MsgAck msgAck;
    msgAck.timestamp = TC_Common::now2ms();
    msgAck.errorCode.code = otim::EC_SUCCESS;
    if (req.payload.size() < 4){
        msgAck.errorCode.code = otim::EC_MSG_TOO_SHORT;
        otim::packTars<otim::MsgAck>(msgAck, resp.payload);
        return otim::EC_SUCCESS;
    }
    
    if (req.payload.size() > _msgMaxLen){
        msgAck.errorCode.code = otim::EC_MSG_TOO_LONG;
        otim::packTars<otim::MsgAck>(msgAck, resp.payload);
        return otim::EC_SUCCESS;
    }
    
    otim::MsgReq msgReq;
    otim::unpackTars<otim::MsgReq>(req.payload, msgReq);
    if (msgReq.from.empty() || msgReq.to.empty() || msgReq.sessionId.empty()){
        MLOG_ERROR("msg from , to or sessionId is empty, packId:"<<req.header.packId<<" from:" <<msgReq.from<<" to:"<<msgReq.to<<" sessionId:"<<msgReq.sessionId);
        msgAck.errorCode.code = otim::EC_PARAM;
        
        otim::packTars<otim::MsgAck>(msgAck, resp.payload);
        return otim::EC_SUCCESS;
    }
    
    //check repeat
    otim::PT_FLAGS_BITS flagbits = otim::getHeaderFlagBits(req.header.flags);
    if (flagbits.dup){
        otim::RedisConnPtr redis(otim::RedisPool::instance());
        bool isReapted = otim::isRepeatedPackId(redis.get(), req.header.packId, msgAck.seqId);
        if (isReapted){
            msgAck.errorCode.code = otim::EC_MSG_REPEATED;
            
            otim::packTars<otim::MsgAck>(msgAck, resp.payload);
            MLOG_DEBUG("Msg Repeated seqId:"<<msgReq.seqId<<" packId:"<<req.header.packId);
            
            return otim::EC_MSG_REPEATED;
        }
    }
    
    
    //get the seqId
    msgReq.timestamp = msgAck.timestamp;
    msgReq.seqId = otim::genSeqId();
    msgAck.seqId = msgReq.seqId;
    msgAck.sessionId = msgReq.sessionId;
    
    otim::OTIMPack pack;
    pack.header = req.header;
    otim::packTars<otim::MsgReq>(msgReq, pack.payload);
    
    MLOG_DEBUG("Msg seqId:"<<msgReq.seqId<<" packId:"<<pack.header.packId);
    //check sensi word
    
    //save
    saveMsg(clientContext, pack, msgReq.sessionId, msgReq.seqId);
    
    if (flagbits.highPRJ){
        otim::RedisConnPtr redis(otim::RedisPool::instance());

        otim::savePriorityMsg(redis.get(), pack, msgReq.to, msgReq.seqId);
    }
    
    //update the session
    updateSession(clientContext, pack, msgReq);
    
    //dispatch
    dispatchGroupMsg(clientContext, pack, msgReq.to);
    
    //发送给其他端
    otim::sendPackToMySelf(clientContext, pack);
    
    
    otim::packTars<otim::MsgAck>(msgAck, resp.payload);
    
    return otim::EC_SUCCESS;
}


class OlapServantCallback : public otim::OlapServantPrxCallback{
public:
    OlapServantCallback(const std::string & packId, const std::string & sessionId,tars::Int64 seqId){
        _packId = packId;
        _sessionId = sessionId;
        _seqId = seqId;
    }
    
    virtual void callback_saveMsg(tars::Int32 ret)
    {
        MLOG_DEBUG("save ok ret:"<<ret<<" _packId:"<<_packId<<" sessionId:"<<_sessionId<<" seqId:"<<_seqId);
    }
    virtual void callback_saveMsg_exception(tars::Int32 ret)
    {
        MLOG_DEBUG("save exception ret:"<<ret<<" _packId:"<<_packId<<" sessionId:"<<_sessionId<<" seqId:"<<_seqId);
    }
    
private:
    std::string _packId;
    std::string _sessionId;
    tars::Int64 _seqId;
};

int GroupChatServantImp::saveMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req,const std::string & sessionId,tars::Int64 seqId)
{
    SCOPELOGGER(scopelogger);
    int ret = otim::EC_SUCCESS;
    try
    {
        otim::OlapServantPrx servantPrx = otim::getServantPrx<otim::OlapServantPrx>(PRXSTR_OLAP);
        
        tars::TC_AutoPtr<OlapServantCallback> srvCB = new OlapServantCallback(req.header.packId, sessionId, seqId);
        
        servantPrx->async_saveMsg(srvCB, clientContext, req, sessionId, seqId);
    }
    catch (std::exception& e)
    {
        ret = otim::EC_SERVICE_UNAVAILABLE;
        MLOG_ERROR("SERVICEALARM servantPrx->async_saveMsg error:" << e.what());
    }
    catch (...)
    {
        ret = otim::EC_SERVICE_UNAVAILABLE;
        MLOG_ERROR("SERVICEALARM servantPrx->async_saveMsg unknown exception");
    }
    
    
    return ret;
}


int GroupChatServantImp::dispatchGroupMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req, const std::string &groupId)
{
    std::vector<std::string> vctMemberId;
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    GroupchatManager gcIns(redis.get());
    gcIns.getAllMembers(groupId,vctMemberId);
   
    MLOG_DEBUG("member size:"<<vctMemberId.size());
    
    for (auto userId : vctMemberId){
        if (userId == clientContext.clientId){
            MLOG_DEBUG("can not send msg to myself:"<<userId);
            continue;
        }
        MLOG_DEBUG("send group msg to userId:"<<userId);
        otim::dispatchMsg(clientContext, req, userId);
    }

    return otim::EC_SUCCESS;
}


int GroupChatServantImp::updateSession(const otim::ClientContext & clientContext, const otim::OTIMPack & req, otim::MsgReq &msgReq)
{
    MLOG_DEBUG("packId:" << req.header.packId <<" to:" << msgReq.to << " sessionId:" << msgReq.sessionId<<" seqId:"<<msgReq.seqId);
    if (msgReq.sessionId.empty()){
        MLOG_ERROR("_updateSessionListofUser sessionId is empty!!!!!!");
        return otim::EC_SUCCESS;
    }
    
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    assert(redis.get() != nullptr);
    
    EMRStatus ret = redis->ZSetAdd(otim::RKEY_HOTSESSION + msgReq.to, msgReq.seqId, msgReq.sessionId);
    if (ret != EMRStatus::EM_KVDB_SUCCESS){
        MLOG_ERROR("update HOTSESSION fail, to" << msgReq.to << ", " << msgReq.sessionId);
    }
    
    ret = redis->ZSetAdd(otim::RKEY_HOTSESSION + msgReq.from, msgReq.seqId, msgReq.sessionId);
    if (ret != EMRStatus::EM_KVDB_SUCCESS){
        MLOG_ERROR("update HOTSESSION fail, from" << msgReq.from << ", " << msgReq.sessionId);
    }
    
    //save unread count
    std::string key = otim::RKEY_UNREADCOUNT + msgReq.to + "." + msgReq.sessionId;
    ret = redis->ZSetAdd(key, msgReq.seqId, req.header.packId);
    if(ret != EMRStatus::EM_KVDB_SUCCESS)
    {
        MLOG_ERROR("update RKEY_UNREADCOUNT fail, key" << key << " packId:"<< req.header.packId);
    }
    
    return otim::EC_SUCCESS;
}
