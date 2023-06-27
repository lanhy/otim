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


static std::string s_sessionId;
static int64_t s_seqId;
static int64_t s_timestamp;
static std::string s_packId;

class TestRServer:public testing::Test
{
public:
    tars::TC_TCPClient* _tcpClient;

    std::string _clientId;
    std::string _friendId;
    std::string _groupId;
    std::string _sessionId;

public:
    void SetUp()
    {
        _tcpClient = GlobalEnv::instance()->createTCPClient();
  
        _clientId = GlobalEnv::instance()->getFriendId();
        _friendId = GlobalEnv::instance()->getClientId();
        _groupId = GlobalEnv::instance()->getGroupId();
        _sessionId = "SC_"+_clientId+"_"+_friendId;
        
        login();
        
        MLOG("TestRServer SetUp");
    }
    
    void TearDown()
    {
        // 清除统计信息
        MLOG("TestRServer TearDown");
   }
    
    otim::OTIMPack login()
    {
        // test
        otim::LoginReq loginReq;
        loginReq.clientId = _clientId;
        loginReq.userName = "testUser1";
        loginReq.password = "testPwd1";
        loginReq.deviceType = otim::DEVICE_IOS;
        loginReq.deviceId = "2-28900-2891-2829333@test";
        loginReq.version = 1;
		loginReq.deviceName = "Apple iOS for lhy";

        otim::OTIMPack respPack = sendReq<otim::LoginReq>(_tcpClient, otim::PT_LOGIN, &loginReq);

		return respPack;
    }
    
    otim::OTIMPack logout()
    {
        // test
        otim::LoginReq loginReq;
        loginReq.clientId = _clientId;
        loginReq.userName = "testUser1";
        loginReq.password = "testPwd1";
        loginReq.deviceType = otim::DEVICE_IOS;
        loginReq.deviceId = "2-28900-2891-2829333@test";
        loginReq.version = 1;
        loginReq.deviceName = "Apple iOS for lhy";

        otim::OTIMPack respPack = sendReq<otim::LoginReq>(_tcpClient, otim::PT_LOGOUT, &loginReq);

        return respPack;
    }

 };


TEST_F(TestRServer, testHotSession)
{
    otim::HotSessionReq hotSessionReq;
    hotSessionReq.timestamp = 0;

    otim::OTIMPack respPack = sendReq<otim::HotSessionReq>(_tcpClient, otim::PT_HOTSESSION_SYNC, &hotSessionReq);

    ASSERT_EQ(respPack.header.type, otim::PT_HOTSESSION_SYNC);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::HotSessionResp resp;
    otim::unpackTars<otim::HotSessionResp>(respPack.payload, resp);
    MLOG("hotsession resp:"<<resp.writeToJsonString());
    ASSERT_EQ(resp.errorCode.code, 0);
    ASSERT_GE(resp.timestamp, 0);
    ASSERT_GE(resp.sessions.size(), 0u);
    for (auto item : resp.sessions){
        ASSERT_GE(item.readSeqId, 0);
        ASSERT_GE(item.unreadCount, 0);
        ASSERT_GE(item.lastMsgs.size(), 0u);
        MLOG("hot resp item:"<<item.writeToJsonString());
        if (s_sessionId.empty()){
            
            otim::MsgReq msgReq;
            otim::unpackTars<otim::MsgReq>(item.lastMsgs[0].payload, msgReq);

            s_seqId = msgReq.seqId;
            s_sessionId = msgReq.sessionId;
            s_packId = item.lastMsgs[0].header.packId;
        }
    }
    
    s_timestamp = resp.timestamp;
}

TEST_F(TestRServer, testReaded)
{
    otim::MsgReaded req;
    req.sessionId = s_sessionId;
    req.seqId = s_seqId;

    otim::OTIMPack respPack = sendReq<otim::MsgReaded>(_tcpClient, otim::PT_MSG_READ, &req);

    ASSERT_EQ(respPack.header.type, otim::PT_MSG_READ);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
 
    otim::CommonErrorCode resp;
    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
    ASSERT_EQ(resp.code, 0);
    MLOG("testReaded resp:"<<resp.writeToJsonString());
}

TEST_F(TestRServer, testHotSessionAfterUnread)
{
    otim::HotSessionReq hotSessionReq;
    hotSessionReq.timestamp = s_timestamp;

    otim::OTIMPack respPack = sendReq<otim::HotSessionReq>(_tcpClient, otim::PT_HOTSESSION_SYNC, &hotSessionReq);

    ASSERT_EQ(respPack.header.type, otim::PT_HOTSESSION_SYNC);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::HotSessionResp resp;
    otim::unpackTars<otim::HotSessionResp>(respPack.payload, resp);
    MLOG("hotsession resp:"<<resp.writeToJsonString());
    ASSERT_EQ(resp.errorCode.code, 0);
    ASSERT_GE(resp.timestamp, 0);
    ASSERT_GE(resp.sessions.size(), 0u);
    for (auto item : resp.sessions){
        
        otim::MsgReq msgReq;
        otim::unpackTars<otim::MsgReq>(item.lastMsgs[0].payload, msgReq);
        if (msgReq.sessionId == s_sessionId){
            ASSERT_GE(item.readSeqId, 0);
            ASSERT_EQ(item.unreadCount, 0);
            ASSERT_GE(item.lastMsgs.size(), 0u);
        }
        else{
            ASSERT_GE(item.readSeqId, 0);
            ASSERT_GE(item.unreadCount, 0);
            ASSERT_GE(item.lastMsgs.size(), 0u);
        }

        MLOG("hot resp item:"<<msgReq.writeToJsonString());
    }
}

//enum MSG_CTL_CMD
//{
//    MC_REVOKE = 1,  // 撤回
//    MC_OVERRIDE = 2,  // 覆写
//    MC_DELETE = 3,  // 删除
//};
//
//struct MsgControl
//{
//    0 require int    command ;    //MSG_CTL_CMD枚举定义
//    1 require string sessionId ;    //会话Id
//    2 require string packId ;    //目标消息packId
//    3 require long  seqId ;    //目标消息seqId
//    4 optional string content; //覆写内容
//};
//TEST_F(TestRServer, testRevoke)
//{
//    otim::MsgControl req;
//    req.command = otim::MC_REVOKE;
//    req.sessionId = s_sessionId;
//    req.packId = s_packId;
//    req.seqId = s_seqId;
//
//    tars::TC_TCPClient *tcpClient = GlobalEnv::instance()->getTcpClient();
//    otim::OTIMPack respPack = sendReq<otim::MsgReaded>(tcpClient, otim::PT_MSG_CTRL, &req);
//
//    ASSERT_EQ(respPack.header.type, otim::PT_MSG_CTRL);
//    bool isAck = respPack.header.flags & otim::PF_ISACK;
//    ASSERT_EQ(isAck, true);
//
//    otim::CommonErrorCode resp;
//    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
//    ASSERT_EQ(resp.code, 0);
//    MLOG("testRevoke resp:"<<resp.writeToJsonString());
//}

TEST_F(TestRServer, testOverride)
{
    otim::MsgControl req;
    req.command = otim::MC_OVERRIDE;
    req.sessionId = s_sessionId;
    req.packId = s_packId;
    req.seqId = s_seqId;
    req.content = "testOverride";
    
    tars::TC_TCPClient *tcpClient = GlobalEnv::instance()->getTcpClient();
    otim::OTIMPack respPack = sendReq<otim::MsgControl>(tcpClient, otim::PT_MSG_CTRL, &req);

    ASSERT_EQ(respPack.header.type, otim::PT_MSG_CTRL);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
 
    otim::CommonErrorCode resp;
    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
    ASSERT_EQ(resp.code, 0);
    MLOG("testRevoke resp:"<<resp.writeToJsonString());
}



TEST_F(TestRServer, testRevoke)
{
    otim::MsgControl req;
    req.command = otim::MC_REVOKE;
    req.sessionId = s_sessionId;
    req.packId = s_packId;
    req.seqId = s_seqId;

    tars::TC_TCPClient *tcpClient = GlobalEnv::instance()->getTcpClient();
    otim::OTIMPack respPack = sendReq<otim::MsgControl>(tcpClient, otim::PT_MSG_CTRL, &req);

    ASSERT_EQ(respPack.header.type, otim::PT_MSG_CTRL);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
 
    otim::CommonErrorCode resp;
    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
    ASSERT_EQ(resp.code, 0);
    MLOG("testRevoke resp:"<<resp.writeToJsonString());
}

TEST_F(TestRServer, testCheckHistoryMsgForRevoke)
{
    otim::HistoryMsgPullReq req;
    req.sessionId = s_sessionId;
    req.seqId = s_seqId+1;
    req.count = 10;

    otim::OTIMPack respPack = sendReq<otim::HistoryMsgPullReq>(_tcpClient, otim::PT_HISTORY_MSG_PULL, &req);
    MLOG("testCheckHistoryMsg req:"<<req.writeToJsonString()<<" respPack:"<<respPack.header.writeToJsonString());

    ASSERT_EQ(respPack.header.type, otim::PT_HISTORY_MSG_PULL);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::HistoryMsgPullResp resp;
    otim::unpackTars<otim::HistoryMsgPullResp>(respPack.payload, resp);
    //MLOG("resp:"<<resp.writeToJsonString());
    ASSERT_EQ(resp.errorCode.code, 0);
    ASSERT_EQ(resp.sessionId, req.sessionId);
    ASSERT_GE(resp.msgs.size(), 0u);
    for (auto item : resp.msgs){
        MLOG("history item:"<<item.header.writeToJsonString());
        otim::MsgReq msgReq;
        otim::unpackTars<otim::MsgReq>(item.payload, msgReq);
        if (msgReq.sessionId == s_sessionId){
            bool revoke = item.header.flags & otim::PF_REVOKE;
            ASSERT_EQ(revoke, true);
            bool overRide = item.header.flags & otim::PF_OVERRIDE;
            ASSERT_EQ(overRide, true);
            
            ASSERT_EQ(msgReq.content, "testOverride");
            
            break;
        }
    }
}

TEST_F(TestRServer, testDelete)
{
    otim::MsgControl req;
    req.command = otim::MC_DELETE;
    req.sessionId = s_sessionId;
    req.packId = s_packId;
    req.seqId = s_seqId;

    tars::TC_TCPClient *tcpClient = GlobalEnv::instance()->getTcpClient();
    otim::OTIMPack respPack = sendReq<otim::MsgControl>(tcpClient, otim::PT_MSG_CTRL, &req);

    ASSERT_EQ(respPack.header.type, otim::PT_MSG_CTRL);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
 
    otim::CommonErrorCode resp;
    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
    ASSERT_EQ(resp.code, 0);
    MLOG("testRevoke resp:"<<resp.writeToJsonString());
}

TEST_F(TestRServer, testCheckHistoryMsgForDel)
{
    otim::HistoryMsgPullReq req;
    req.sessionId = s_sessionId;
    req.seqId = s_seqId+1;
    req.count = 10;

    otim::OTIMPack respPack = sendReq<otim::HistoryMsgPullReq>(_tcpClient, otim::PT_HISTORY_MSG_PULL, &req);
    MLOG("testCheckHistoryMsg req:"<<req.writeToJsonString()<<" respPack:"<<respPack.header.writeToJsonString());

    ASSERT_EQ(respPack.header.type, otim::PT_HISTORY_MSG_PULL);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::HistoryMsgPullResp resp;
    otim::unpackTars<otim::HistoryMsgPullResp>(respPack.payload, resp);
    //MLOG("testCheckHistoryMsgForDel resp:"<<resp.writeToJsonString());
    ASSERT_EQ(resp.errorCode.code, 0);
    ASSERT_EQ(resp.sessionId, req.sessionId);
    ASSERT_GE(resp.msgs.size(), 0u);
    for (auto item : resp.msgs){
        MLOG("history item:"<<item.header.writeToJsonString());
        otim::MsgReq msgReq;
        otim::unpackTars<otim::MsgReq>(item.payload, msgReq);
    	ASSERT_NE(item.header.packId, s_packId);
		ASSERT_NE(msgReq.seqId, s_seqId); 
	}
}


TEST_F(TestRServer, testCheckHistoryProrityMsg)
{
    otim::MsgHighPrioritySyncReq req;
    req.seqId = 0;
    req.count = 100;

    otim::OTIMPack respPack = sendReq<otim::MsgHighPrioritySyncReq>(_tcpClient, otim::PT_HIGH_PRIOR_MSG_SYNC, &req);
    MLOG("testCheckHistoryMsg req:"<<req.writeToJsonString()<<" respPack:"<<respPack.header.writeToJsonString());

    ASSERT_EQ(respPack.header.type, otim::PT_HIGH_PRIOR_MSG_SYNC);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::MsgHighPrioritySyncResp resp;
    otim::unpackTars<otim::MsgHighPrioritySyncResp>(respPack.payload, resp);
    MLOG("testCheckHistoryProityMsg resp:"<<resp.writeToJsonString());
    ASSERT_EQ(resp.errorCode.code, 0);
    ASSERT_GE(resp.lastSeqId, 0);
    ASSERT_GE(resp.msgs.size(), 0u);

    for (auto item : resp.msgs){
        MLOG("history item:"<<item.header.writeToJsonString());
	}
}
