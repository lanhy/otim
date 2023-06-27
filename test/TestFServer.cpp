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


class TestFServer:public testing::Test
{
public:
    tars::TC_TCPClient* _tcpClient;

    std::string _clientId;
    std::string _friendId;
    std::string _sessionId;

public:
    void SetUp()
    {
        GlobalEnv *env = GlobalEnv::instance();

        _tcpClient = env->getTcpClient();
        _clientId = env->getClientId();
        _friendId = env->getFriendId();
        _sessionId = "SC_"+_clientId+"_"+_friendId;
//        MLOG("SetUp");
    }
    
    void TearDown()
    {
        // 清除统计信息
//          MLOG("TearDown");
   }
        
    otim::OTIMPack testLogin()
    {
        // test
        otim::LoginReq loginReq;
        loginReq.clientId = _clientId;
        loginReq.userName = "testUser";
        loginReq.password = "testPwd";
        loginReq.deviceType = otim::DEVICE_IOS;
        loginReq.deviceId = "1-28900-2891-2829333@test";
        loginReq.version = 1;
		loginReq.deviceName = "Apple iOS for lhy";

        otim::OTIMPack respPack = sendReq<otim::LoginReq>(_tcpClient, otim::PT_LOGIN, &loginReq);

		return respPack;
    }
    
    otim::OTIMPack testPing()
    {
        otim::OTIMPack respPack = sendReq<otim::OTIMPack>(_tcpClient, otim::PT_PING, nullptr);

        return respPack;
    }
    
    otim::OTIMPack testNotFriendSendSingleMsg()
    {
        // test
        otim::MsgReq msgReq;
        msgReq.from = _clientId;
        msgReq.to = _friendId;
        msgReq.sessionId = _sessionId;
        msgReq.contentType = 12;
        msgReq.content = "test single chat case for no friend";
   
        otim::OTIMPack respPack = sendReq<otim::MsgReq>(_tcpClient, otim::PT_MSG_SINGLE_CHAT, &msgReq);

        return respPack;
    }
    
 
    otim::OTIMPack testAddFriend()
    {
        otim::FriendAddReq friendReq;
        otim::FriendInfo fi;
        fi.userId = _clientId;
        fi.friendId = _friendId;
        
        friendReq.friends.push_back(fi);
        
        otim::OTIMPack respPack = sendReq<otim::FriendAddReq>(_tcpClient, otim::PT_FRIEND_ADD, &friendReq);

        return respPack;
    }
    
    otim::OTIMPack testSyncFriend()
    {
        otim::FriendSyncReq friendReq;
        friendReq.timestamp = 0;
        otim::OTIMPack respPack = sendReq<otim::FriendSyncReq>(_tcpClient, otim::PT_FRIEND_SYNC, &friendReq);

        return respPack;
    }
    
    otim::OTIMPack testDelFriend()
    {
        otim::FriendDelReq friendReq;
        otim::FriendInfo fi;
        fi.userId = _clientId;
        fi.friendId = _friendId;
        
        friendReq.friends.push_back(fi);
        otim::OTIMPack respPack = sendReq<otim::FriendDelReq>(_tcpClient, otim::PT_FRIEND_DEL, &friendReq);

        return respPack;
    }
    
    otim::OTIMPack testSendSingleMsg()
    {
        // test
        otim::MsgReq msgReq;
        msgReq.from = _clientId;
        msgReq.to = _friendId;
        msgReq.sessionId = _sessionId;
        msgReq.contentType = 12;
        msgReq.content = "test single chat case for friend";
   
        otim::OTIMPack respPack = sendReq<otim::MsgReq>(_tcpClient, otim::PT_MSG_SINGLE_CHAT, &msgReq);

        return respPack;
    }
//
//    otim::OTIMPack testSendNotifyMsg()
//    {
//        // test
//        otim::MsgReq msgReq;
//        msgReq.from = _clientId;
//        msgReq.to = _friendId;
//        msgReq.sessionId = "SC_1106_"+_clientId;
//        msgReq.contentType = 12;
//        msgReq.content = "test single chat case for friend";
//
//        otim::OTIMPack respPack = sendReq<otim::MsgReq>(otim::PT_MSG_SINGLE_CHAT, &msgReq);
//
//        return respPack;
//    }
//
    otim::OTIMPack testUpdateUserInfo()
    {
        otim::UserInfoUpdateReq userInfoReq;
        userInfoReq.userInfo.userId = _clientId;
        userInfoReq.userInfo.name = "User Test";
        userInfoReq.userInfo.avatar = "https://profile.csdnimg.cn/F/7/7/3_u012516571";
        userInfoReq.userInfo.mobile = "18600201111";
        userInfoReq.userInfo.birthday = 1888288388833;

        otim::OTIMPack respPack = sendReq<otim::UserInfoUpdateReq>(_tcpClient, otim::PT_USERINFO_UPDATE, &userInfoReq);

        return respPack;
    }
    
    otim::OTIMPack testGetUserInfo()
    {
        otim::UserInfoGetReq userInfoReq;
        userInfoReq.userIds.push_back(_clientId);
        otim::OTIMPack respPack = sendReq<otim::UserInfoGetReq>(_tcpClient, otim::PT_USERINFO_GET, &userInfoReq);

        return respPack;
    }
    
    otim::OTIMPack testSetUserAttr()
    {
        otim::UserAttrSetReq userInfoReq;
        userInfoReq.attribute.userId = _clientId;
        userInfoReq.attribute.friendId = _friendId;
        userInfoReq.attribute.attrName = "REMARK";
        userInfoReq.attribute.attrValue = "FRIEND";

        otim::OTIMPack respPack = sendReq<otim::UserAttrSetReq>(_tcpClient, otim::PT_USERATTRIBUTE_SET, &userInfoReq);
       
        otim::UserAttrSetReq userInfoReq1;
        userInfoReq1.attribute.userId = _clientId;
        userInfoReq1.attribute.friendId = _friendId;
        userInfoReq1.attribute.attrName = "TestName";
        userInfoReq1.attribute.attrValue = "testValue";

        respPack = sendReq<otim::UserAttrSetReq>(_tcpClient, otim::PT_USERATTRIBUTE_SET, &userInfoReq1);

        return respPack;
    }
    
    
    otim::OTIMPack testGetUserAttrRemark()
    {
        otim::UserAttribute userInfoReq;
        userInfoReq.userId = _clientId;
        userInfoReq.friendId = _friendId;
        userInfoReq.attrName = "REMARK";

        otim::OTIMPack respPack = sendReq<otim::UserAttribute>(_tcpClient, otim::PT_USERATTRIBUTE_GET, &userInfoReq);

        return respPack;
    }
    
    otim::OTIMPack testGetUserAttrAll()
    {
        otim::UserAttribute userInfoReq;
        userInfoReq.userId = _clientId;
        userInfoReq.friendId = _friendId;

        otim::OTIMPack respPack = sendReq<otim::UserAttribute>(_tcpClient, otim::PT_USERATTRIBUTE_GET, &userInfoReq);

        return respPack;
    }
   
    otim::OTIMPack testSetSessionAttr()
    {
        otim::SessionAttrSetReq userInfoReq;
        userInfoReq.userId = _clientId;
        userInfoReq.sessionId = _sessionId;
        userInfoReq.attrName = "TOP";
        userInfoReq.attrValue = "1";

        otim::OTIMPack respPack = sendReq<otim::SessionAttrSetReq>(_tcpClient, otim::PT_SESSIONATTRIBUTE_SET, &userInfoReq);

        return respPack;
    }
    
    otim::OTIMPack testHistoryMsgSingle()
    {
        otim::HistoryMsgPullReq req;
        req.sessionId = _sessionId;
        req.seqId = 0;

        otim::OTIMPack respPack = sendReq<otim::HistoryMsgPullReq>(_tcpClient, otim::PT_HISTORY_MSG_PULL, &req);


        return respPack;
    }
    
    
};


TEST_F(TestFServer, testLogin)
{
    otim::OTIMPack resp = testLogin();
    ASSERT_EQ(resp.header.type, otim::PT_LOGIN);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);

    otim::LoginResp loginResp;
    otim::unpackTars<otim::LoginResp>(resp.payload, loginResp);
    ASSERT_EQ(loginResp.errorCode.code, 0);
    MLOG("loginResp:"<<loginResp.writeToJsonString());
}

TEST_F(TestFServer, testPing)
{
    otim::OTIMPack resp = testPing();
    ASSERT_EQ(resp.header.type, otim::PT_PING);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
}

TEST_F(TestFServer, testNotFriendSendSingleMsg)
{
    otim::OTIMPack resp = testNotFriendSendSingleMsg();
    ASSERT_EQ(resp.header.type, otim::PT_MSG_SINGLE_CHAT);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::MsgAck msgAck;
    otim::unpackTars<otim::MsgAck>(resp.payload, msgAck);
    ASSERT_EQ(msgAck.errorCode.code, otim::EC_USER_RELATION_INVALID);
    MLOG("msgAck:"<<msgAck.writeToJsonString());

}


TEST_F(TestFServer, testAddFriend)
{
    otim::OTIMPack resp = testAddFriend();
    ASSERT_EQ(resp.header.type, otim::PT_FRIEND_ADD);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::CommonErrorCode friendResp;
    otim::unpackTars<otim::CommonErrorCode>(resp.payload, friendResp);
    ASSERT_EQ(friendResp.code, 0);
    MLOG("friendResp:"<<friendResp.writeToJsonString());
}


TEST_F(TestFServer, testSyncFriend)
{
    otim::OTIMPack resp = testSyncFriend();
    ASSERT_EQ(resp.header.type, otim::PT_FRIEND_SYNC);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::FriendSyncResp friendResp;
    otim::unpackTars<otim::FriendSyncResp>(resp.payload, friendResp);
    ASSERT_EQ(friendResp.errorCode.code, 0);
    ASSERT_EQ(friendResp.friends.size(), 1u);
    
    otim::FriendInfo fi = friendResp.friends.at(0);
    ASSERT_EQ(fi.friendId, "100087");
  
    MLOG("friendResp:"<<friendResp.writeToJsonString());
}


TEST_F(TestFServer, testSendSingleMsg)
{
    otim::OTIMPack resp = testSendSingleMsg();
    ASSERT_EQ(resp.header.type, otim::PT_MSG_SINGLE_CHAT);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::MsgAck msgAck;
    otim::unpackTars<otim::MsgAck>(resp.payload, msgAck);
    ASSERT_EQ(msgAck.errorCode.code, otim::EC_SUCCESS);
    ASSERT_GE(msgAck.seqId, 0);

    MLOG("msgAck:"<<msgAck.writeToJsonString());

}

TEST_F(TestFServer, testDelFriend)
{
    otim::OTIMPack resp = testDelFriend();
    ASSERT_EQ(resp.header.type, otim::PT_FRIEND_DEL);
    bool isAck = resp.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::CommonErrorCode friendResp;
    otim::unpackTars<otim::CommonErrorCode>(resp.payload, friendResp);
    ASSERT_EQ(friendResp.code, 0);
    MLOG("friendResp:"<<friendResp.writeToJsonString());
}

TEST_F(TestFServer, testUpdateUserInfo)
{
    otim::OTIMPack respPack = testUpdateUserInfo();
    ASSERT_EQ(respPack.header.type, otim::PT_USERINFO_UPDATE);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::CommonErrorCode resp;
    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
    ASSERT_EQ(resp.code, 0);
    MLOG("resp:"<<resp.writeToJsonString());
}

TEST_F(TestFServer, testGetUserInfo)
{
    otim::OTIMPack respPack = testGetUserInfo();
    ASSERT_EQ(respPack.header.type, otim::PT_USERINFO_GET);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::UserInfoGetResp resp;
    otim::unpackTars<otim::UserInfoGetResp>(respPack.payload, resp);
    ASSERT_EQ(resp.errorCode.code, 0);
    ASSERT_EQ(resp.userInfos.size(), 1u);
    MLOG("resp:"<<resp.writeToJsonString());
}


TEST_F(TestFServer, testSetUserAttr)
{
    otim::OTIMPack respPack = testSetUserAttr();
    ASSERT_EQ(respPack.header.type, otim::PT_USERATTRIBUTE_SET);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::CommonErrorCode resp;
    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
    ASSERT_EQ(resp.code, 0);
    MLOG("resp:"<<resp.writeToJsonString());
}


TEST_F(TestFServer, testGetUserAttrRemark)
{
    otim::OTIMPack respPack = testGetUserAttrRemark();
    ASSERT_EQ(respPack.header.type, otim::PT_USERATTRIBUTE_GET);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::UserAttrGetResp resp;
    otim::unpackTars<otim::UserAttrGetResp>(respPack.payload, resp);
    ASSERT_EQ(resp.errorCode.code, 0);
    ASSERT_EQ(resp.attributes.size(), 1u);
    
    otim::UserAttribute attr = resp.attributes[0];
    ASSERT_EQ(attr.attrName, "REMARK");
    ASSERT_EQ(attr.attrValue, "FRIEND");

    MLOG("resp:"<<resp.writeToJsonString());
}


TEST_F(TestFServer, testGetUserAttrAll)
{
    otim::OTIMPack respPack = testGetUserAttrAll();
    ASSERT_EQ(respPack.header.type, otim::PT_USERATTRIBUTE_GET);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::UserAttrGetResp resp;
    otim::unpackTars<otim::UserAttrGetResp>(respPack.payload, resp);
    ASSERT_EQ(resp.errorCode.code, 0);
    ASSERT_EQ(resp.attributes.size(), 2u);
    
    MLOG("resp:"<<resp.writeToJsonString());
}



TEST_F(TestFServer, testSetSessionAttr)
{
    otim::OTIMPack respPack = testSetSessionAttr();
    ASSERT_EQ(respPack.header.type, otim::PT_SESSIONATTRIBUTE_SET);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::CommonErrorCode resp;
    otim::unpackTars<otim::CommonErrorCode>(respPack.payload, resp);
    ASSERT_EQ(resp.code, 0);
    MLOG("resp:"<<resp.writeToJsonString());
}


TEST_F(TestFServer, testHistoryMsgSingle)
{
//    struct HistoryMsgPullReq
//    {
//        0 optional string sessionId;
//        1 require long seqId; //起始seqId
//        2 require int count = 200;//获取数量
//    };
//
//    struct HistoryMsgPullResp
//    {
//        0 require CommonErrorCode errorCode;
//        1 require string sessionId; //起始sessionId
//        2 require vector<OTIMPack> msgs;
//    };
//
    otim::OTIMPack respPack = testHistoryMsgSingle();
    ASSERT_EQ(respPack.header.type, otim::PT_HISTORY_MSG_PULL);
    bool isAck = respPack.header.flags & otim::PF_ISACK;
    ASSERT_EQ(isAck, true);
    
    otim::HistoryMsgPullResp resp;
    otim::unpackTars<otim::HistoryMsgPullResp>(respPack.payload, resp);
    MLOG("resp:"<<resp.writeToJsonString());
    ASSERT_EQ(resp.errorCode.code, 0);
    ASSERT_EQ(resp.sessionId, _sessionId);
    ASSERT_GE(resp.msgs.size(), 0u);
    for (auto item : resp.msgs){

        ASSERT_EQ(item.header.type, otim::PT_MSG_SINGLE_CHAT);
        MLOG("history Single resp item:"<<item.header.writeToJsonString());
    }
}

