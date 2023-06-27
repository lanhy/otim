#include <gtest/gtest.h>
#include <iostream>
#include <time.h>
#include <thread>
#include <string>
#include "ptcldefine.h"
#include "RedisPool.h"
#include "otim_const.h"
#include "otim.h"
#include "otim_err.h"
#include "Common.h"
#include "ptcldefine.h"
#include "util/tc_option.h"
#include "util/tc_clientsocket.h"
#include "GlobalEnv.h"


class TestGServer:public testing::Test
{
public:
    tars::TC_TCPClient* _tcpClient;

    std::string _clientId;
    std::string _friendId;
    std::string _groupId;

public:
    void SetUp()
    {
        GlobalEnv *env = GlobalEnv::instance();

        _tcpClient = env->getTcpClient();
        _clientId = env->getClientId();
        _friendId = env->getFriendId();
        _groupId = env->getGroupId();
        MLOG("TestGServer SetUp");
    }
    
    void TearDown()
    {
        // 清除统计信息
        MLOG("TestGServer TearDown");
   }
    
    otim::OTIMPack testLogin()
    {
        // test
        otim::LoginReq loginReq;
        loginReq.clientId = _friendId;
        loginReq.userName = "testUser1";
        loginReq.password = "testPwd1";
        loginReq.deviceType = otim::DEVICE_IOS;
        loginReq.deviceId = "2-28900-2891-2829333@test";
        loginReq.version = 1;
		loginReq.deviceName = "Apple iOS for lhy";

        otim::OTIMPack respPack = sendReq<otim::LoginReq>(_tcpClient, otim::PT_LOGIN, &loginReq);

		return respPack;
    }
    
    
    otim::OTIMPack testCreateGroup()
    {
        // test
        otim::GroupChatCreateReq groupReq;
        groupReq.groupInfo.name = "TestGroup";
        groupReq.groupInfo.memberLimit = 20;
        groupReq.groupInfo.creatorId = _clientId;
        groupReq.memberIds.push_back(_clientId);
        groupReq.memberIds.push_back(_friendId);

        otim::OTIMPack respPack = sendReq<otim::GroupChatCreateReq>(_tcpClient, otim::PT_GROUPCHAT_CREATE, &groupReq);

        return respPack;
    }
    
    otim::OTIMPack testSyncGroup()
    {
        otim::GroupChatSyncReq groupReq;
        groupReq.timestamp = 0;
        otim::OTIMPack respPack = sendReq<otim::GroupChatSyncReq>(_tcpClient, otim::PT_GROUPCHAT_SYNC, &groupReq);

        return respPack;
    }
    
    otim::OTIMPack testAddMember()
    {
        otim::GroupChatJoinQuitReq groupReq;
        groupReq.groupId = _groupId;
   
        groupReq.memberIds.push_back("100097");
        groupReq.memberIds.push_back("100098");
        groupReq.memberIds.push_back("100099");

        otim::OTIMPack respPack = sendReq<otim::GroupChatJoinQuitReq>(_tcpClient, otim::PT_GROUPCHAT_JION, &groupReq);

        return respPack;
    }
    
    otim::OTIMPack testGetGroupMember()
    {
        otim::GroupChatMemberGetReq groupReq;
        groupReq.groupId = _groupId;
        otim::OTIMPack respPack = sendReq<otim::GroupChatMemberGetReq>(_tcpClient, otim::PT_GROUPCHAT_MEMBERS_GET, &groupReq);

        return respPack;
    }

    otim::OTIMPack testDelMember()
    {
        otim::GroupChatJoinQuitReq groupReq;
        groupReq.groupId = _groupId;
   
        groupReq.memberIds.push_back("100097");
        groupReq.memberIds.push_back("100098");

        otim::OTIMPack respPack = sendReq<otim::GroupChatJoinQuitReq>(_tcpClient, otim::PT_GROUPCHAT_QUIT, &groupReq);

        return respPack;
    }
    
    otim::OTIMPack testUpdateGroupInfo()
    {
        otim::GroupChatInfoUpdateReq groupReq;
        groupReq.operatorId = _clientId;
        groupReq.groupInfo.groupId = _groupId;
        groupReq.groupInfo.avatar = "https://profile.csdnimg.cn/F/7/7/3_u012516571";
        groupReq.groupInfo.name = "Test Group M";
        
        otim::OTIMPack respPack = sendReq<otim::GroupChatInfoUpdateReq>(_tcpClient, otim::PT_GROUPCHAT_INFO_UPDATE, &groupReq);

        return respPack;
    }
    
    otim::OTIMPack testUpdateGroupCreator()
    {
        otim::GroupChatCreatorUpdateReq groupReq;
        groupReq.groupId = _groupId;
        groupReq.operatorId = _clientId;
        groupReq.newCreatorId = _friendId;
        
        otim::OTIMPack respPack = sendReq<otim::GroupChatCreatorUpdateReq>(_tcpClient, otim::PT_GROUPCHAT_UPDATE_CREATOR, &groupReq);

        return respPack;
    }
    
    otim::OTIMPack testDismissGroup()
    {
        otim::GroupChatDismissReq groupReq;
        groupReq.groupId = _groupId;
        groupReq.operatorId = _friendId;
   
        otim::OTIMPack respPack = sendReq<otim::GroupChatDismissReq>(_tcpClient, otim::PT_GROUPCHAT_DISMISS, &groupReq);

        return respPack;
    }
    
  
    otim::OTIMPack testSendGroupChatMsg()
    {
        // test
        otim::MsgReq msgReq;
        msgReq.from = _clientId;
        msgReq.to = _groupId;
        msgReq.sessionId = _groupId;
        msgReq.contentType = 12;
        msgReq.content = "test single chat case for friend";
   
        otim::OTIMPack respPack = sendReq<otim::MsgReq>(_tcpClient, otim::PT_MSG_GROUP_CHAT, &msgReq);

        return respPack;
    }

 
 };


//TEST_F(TestGServer, testLogin)
//{
//    otim::OTIMPack resp = testLogin();
//    ASSERT_EQ(resp.header.type, otim::PT_LOGIN);
//    bool isAck = resp.header.flags & otim::PF_ISACK;
//    ASSERT_EQ(isAck, true);
//
//    otim::LoginResp loginResp;
//    otim::unpackTars<otim::LoginResp>(resp.payload, loginResp);
//    ASSERT_EQ(loginResp.errorCode.code, 0);
//    MLOG("loginResp:"<<loginResp.writeToJsonString());
//}

TEST_F(TestGServer, testCreateGroup)
{
    otim::OTIMPack resp = testCreateGroup();
    ASSERT_EQ(resp.header.type, otim::PT_GROUPCHAT_CREATE);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::GroupChatCreateResp groupResp;
    otim::unpackTars<otim::GroupChatCreateResp>(resp.payload, groupResp);
    ASSERT_EQ(groupResp.errorCode.code, 0);
    MLOG("groupResp:"<<groupResp.writeToJsonString());
    
    GlobalEnv::instance()->setGroupId(groupResp.groupId);

}

TEST_F(TestGServer, testSyncGroup)
{
    otim::OTIMPack resp = testSyncGroup();
    ASSERT_EQ(resp.header.type, otim::PT_GROUPCHAT_SYNC);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::GroupChatSyncResp groupResp;
    otim::unpackTars<otim::GroupChatSyncResp>(resp.payload, groupResp);
    ASSERT_EQ(groupResp.errorCode.code, otim::EC_SUCCESS);
    ASSERT_EQ(groupResp.groupChats.size(), 1u);
    ASSERT_EQ(groupResp.groupIds.size(), 1u);
    MLOG("groupResp:"<<groupResp.writeToJsonString());

}


TEST_F(TestGServer, testAddMember)
{
    otim::OTIMPack resp = testAddMember();
    ASSERT_EQ(resp.header.type, otim::PT_GROUPCHAT_JION);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::CommonErrorCode groupResp;
    otim::unpackTars<otim::CommonErrorCode>(resp.payload, groupResp);
    ASSERT_EQ(groupResp.code, 0);
    MLOG("resp:"<<groupResp.writeToJsonString());
}


TEST_F(TestGServer, testGetGroupMember)
{
    otim::OTIMPack resp = testGetGroupMember();
    ASSERT_EQ(resp.header.type, otim::PT_GROUPCHAT_MEMBERS_GET);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::GroupChatMemberGetResp groupResp;
    otim::unpackTars<otim::GroupChatMemberGetResp>(resp.payload, groupResp);
    ASSERT_EQ(groupResp.groupId, _groupId);
    ASSERT_EQ(groupResp.errorCode.code, 0);
    ASSERT_EQ(groupResp.memberIds.size(), 5u);
    MLOG("groupResp:"<<groupResp.writeToJsonString());
}

TEST_F(TestGServer, testSendGroupChatMsg)
{
    otim::OTIMPack resp = testSendGroupChatMsg();
    ASSERT_EQ(resp.header.type, otim::PT_MSG_GROUP_CHAT);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);

    otim::MsgAck msgAck;
    otim::unpackTars<otim::MsgAck>(resp.payload, msgAck);
    ASSERT_EQ(msgAck.errorCode.code, otim::EC_SUCCESS);
    ASSERT_GE(msgAck.seqId, 0);

    MLOG("Group msgAck:"<<msgAck.writeToJsonString());

}


TEST_F(TestGServer, testDelMember)
{
    otim::OTIMPack resp = testDelMember();
    ASSERT_EQ(resp.header.type, otim::PT_GROUPCHAT_QUIT);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::CommonErrorCode groupResp;
    otim::unpackTars<otim::CommonErrorCode>(resp.payload, groupResp);
    ASSERT_EQ(groupResp.code, 0);
    MLOG("DelMemberResp:"<<groupResp.writeToJsonString());
}

TEST_F(TestGServer, testUpdateGroupInfo)
{
    otim::OTIMPack respPack = testUpdateGroupInfo();
    ASSERT_EQ(respPack.header.type, otim::PT_GROUPCHAT_INFO_UPDATE);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::CommonErrorCode resp;
    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
    ASSERT_EQ(resp.code, 0);
    MLOG("resp:"<<resp.writeToJsonString());
}

TEST_F(TestGServer, testUpdateGroupCreator)
{
    otim::OTIMPack respPack = testUpdateGroupCreator();
    ASSERT_EQ(respPack.header.type, otim::PT_GROUPCHAT_UPDATE_CREATOR);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::CommonErrorCode resp;
    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
    ASSERT_EQ(resp.code, 0);
    MLOG("resp:"<<resp.writeToJsonString());
}

TEST_F(TestGServer, testHistoryMsgGroupchat)
{
    otim::HistoryMsgPullReq req;
    req.sessionId = _groupId;
    req.seqId = 0;

    otim::OTIMPack respPack = sendReq<otim::HistoryMsgPullReq>(_tcpClient, otim::PT_HISTORY_MSG_PULL, &req);

    ASSERT_EQ(respPack.header.type, otim::PT_HISTORY_MSG_PULL);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::HistoryMsgPullResp resp;
    otim::unpackTars<otim::HistoryMsgPullResp>(respPack.payload, resp);
    MLOG("resp:"<<resp.writeToJsonString());
    ASSERT_EQ(resp.errorCode.code, 0);
    ASSERT_EQ(resp.sessionId, req.sessionId);
    ASSERT_GE(resp.msgs.size(), 0u);
    for (auto item : resp.msgs){
        ASSERT_EQ(item.header.type, otim::PT_MSG_GROUP_CHAT);
        MLOG("history group resp item:"<<item.header.writeToJsonString());
    }
    
}

TEST_F(TestGServer, testDismissGroup)
{
    otim::OTIMPack respPack = testDismissGroup();
    ASSERT_EQ(respPack.header.type, otim::PT_GROUPCHAT_DISMISS);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::CommonErrorCode resp;
    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
    ASSERT_EQ(resp.code, 0);
    MLOG("resp:"<<resp.writeToJsonString());
}
