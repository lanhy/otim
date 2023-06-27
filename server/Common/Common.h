//
//  Common.hpp
//  otim
//
//  Created by lanhy on 2022/12/1.
//

#ifndef Common_hpp
#define Common_hpp

#include <string>
#include "servant/Application.h"
#include "RedisDBInterface.h"
#include "otim.h"

const std::string PRXSTR_BROKER = "otim.BrokerServer.BrokerPushServantObj";
const std::string PRXSTR_AUTH = "otim.AuthServer.AuthServantObj";
const std::string PRXSTR_BIZMSG = "otim.BizMsgServer.BizMsgServantObj";
const std::string PRXSTR_SINGLE_CHAT = "otim.SingleChatServer.SingleChatServantObj";
const std::string PRXSTR_GROUP_CHAT = "otim.GroupChatServer.GroupChatServantObj";
const std::string PRXSTR_OLAP = "otim.OlapServer.OlapServantObj";
const std::string PRXSTR_GROUP_CHAT_RPC = "otim.GroupChatServer.GroupChatRPCServantObj";
const std::string PRXSTR_USERFRIEND_RPC = "otim.UserFriendServer.UserFriendRPCServantObj";

namespace otim{

    template<typename T>
    T getServantPrx(const std::string& endpoint)
    {
        //    TC_ThreadLock::Lock sync(*this);
        T servantPrx;
        Application::getCommunicator()->stringToProxy(endpoint, servantPrx);
        return servantPrx;
    }


    tars::Int64 genSeqId();
    std::string genUUID();

    void saveRepeatedPackId(CRedisDBInterface *redis, const std::string & packId, tars::Int64 seqId, int lifeTime);
    bool isRepeatedPackId(CRedisDBInterface *redis, const std::string & packId,  tars::Int64 & seqId);

    int savePriorityMsg(CRedisDBInterface *redis, const otim::OTIMPack & pack,const std::string & to,tars::Int64 seqId);

    int dispatchMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req, const std::string &to);

    int sendSyncMsg(const otim::ClientContext & clientContext, int command, const std::string &content, const std::vector<std::string>& userIds);

    int sendSyncMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & pack, const std::vector<std::string>& userIds);

    int sendPackToMySelf(const otim::ClientContext & clientContext, const otim::OTIMPack & req);


};

#endif /* Common_hpp */
