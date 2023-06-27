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
#include "JsonUtil.h"

std::string _host;

class TestHttpServer:public testing::Test
{
public:
 
    void SetUp()
    {
        _host = "http://10.250.0.112:9090";
        
        MLOG("TestHttpServer SetUp");
    }
    
    void TearDown()
    {
        // 清除统计信息
        MLOG("TestHttpServer TearDown");
   }

 };


TEST_F(TestHttpServer, testAddFriend)
{
    TC_HttpRequest stHttpReq;
    stHttpReq.setCacheControl("no-cache");
    stHttpReq.setUserAgent("Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; InfoPath.1; .NET CLR 1.1.4322)");
    std::string postData= "{\"traceId\":\"bdsc-test-001\", \"userId\":\"100\", \"friendId\":\"101\"}";
    
    std::string url = _host +"/friend/add";
    stHttpReq.setPostRequest(url, postData);
    
    string sSendBuffer = stHttpReq.encode();
    cout << sSendBuffer << endl;
    cout << "***********************************" << endl;

    TC_HttpResponse stHttpRep;
    stHttpReq.doRequest(stHttpRep, 3000);
   
    ASSERT_EQ(stHttpRep.getStatus(), 200);

    string content = stHttpRep.getContent();
    MLOG("testAddFriend resp content:"<< content);
    rapidjson::Document cBody;
    cBody.Parse(content.c_str());
    ASSERT_EQ(cBody.IsObject(), true);

    int code;
    JsonUtil::getInt(cBody, "code", code);
    ASSERT_EQ(code, 0);

}

TEST_F(TestHttpServer, testFriendGet)
{
    TC_HttpRequest stHttpReq;
    stHttpReq.setCacheControl("no-cache");
    stHttpReq.setUserAgent("Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; InfoPath.1; .NET CLR 1.1.4322)");
    std::string postData= "{\"traceId\":\"bdsc-test-002\", \"userId\":\"100\"}";

    std::string url = _host +"/friend/get";
    stHttpReq.setPostRequest(url, postData);
 
//    string sSendBuffer = stHttpReq.encode();
//    cout << sSendBuffer << endl;
//    cout << "***********************************" << endl;

    TC_HttpResponse stHttpRep;
    stHttpReq.doRequest(stHttpRep, 3000);
   
    ASSERT_EQ(stHttpRep.getStatus(), 200);

    string content = stHttpRep.getContent();
    MLOG("testFriendGet resp content:"<< content);
    rapidjson::Document cBody;
    cBody.Parse(content.c_str());
    ASSERT_EQ(cBody.IsObject(), true);

    int code;
    JsonUtil::getInt(cBody, "code", code);
    ASSERT_EQ(code, 0);
}

TEST_F(TestHttpServer, testFriendDel)
{
    TC_HttpRequest stHttpReq;
    stHttpReq.setCacheControl("no-cache");
    stHttpReq.setUserAgent("Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; InfoPath.1; .NET CLR 1.1.4322)");
    std::string postData= "{\"traceId\":\"bdsc-test-003\", \"userId\":\"100\", \"friendId\":\"101\"}";

    std::string url = _host +"/friend/del";
    stHttpReq.setPostRequest(url, postData);
 
//    string sSendBuffer = stHttpReq.encode();
//    cout << sSendBuffer << endl;
//    cout << "***********************************" << endl;

    TC_HttpResponse stHttpRep;
    stHttpReq.doRequest(stHttpRep, 3000);
   
    ASSERT_EQ(stHttpRep.getStatus(), 200);

    string content = stHttpRep.getContent();
    MLOG("testFriendDel resp content:"<< content);
    rapidjson::Document cBody;
    cBody.Parse(content.c_str());
    ASSERT_EQ(cBody.IsObject(), true);

    int code;
    JsonUtil::getInt(cBody, "code", code);
    ASSERT_EQ(code, 0);

}


TEST_F(TestHttpServer, testSendSimpleMsg)
{
    TC_HttpRequest stHttpReq;
    stHttpReq.setCacheControl("no-cache");
    stHttpReq.setUserAgent("Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; InfoPath.1; .NET CLR 1.1.4322)");
    
    std::map<std::string, std::string> mapData;
    mapData["traceId"] = "MSGT-8922-test-001";
    mapData["appId"] = "3200";
    mapData["from"] = "3200";
    mapData["to"] = "100";
    mapData["title"] = "Test title";
    mapData["summary"] = "this is a test for send simple msg";
    mapData["pushSummary"] = "push this is a test for simple";
    mapData["pushTitle"] = "Push Title";
    mapData["content"] = "this is a test for simple msg for content";

    std::string postData = JsonUtil::mapToJson(mapData);
    MLOG("testSendSimpleMsg postData:"<< postData);

    std::string url = _host +"/msg/sendsimplemsg";
    stHttpReq.setPostRequest(url, postData);
 
    TC_HttpResponse stHttpRep;
    stHttpReq.doRequest(stHttpRep, 3000);
   
    ASSERT_EQ(stHttpRep.getStatus(), 200);

    string content = stHttpRep.getContent();
    MLOG("testSendSimpleMsg resp content:"<< content);
    rapidjson::Document cBody;
    cBody.Parse(content.c_str());
    ASSERT_EQ(cBody.IsObject(), true);

    int code;
    JsonUtil::getInt(cBody, "code", code);
    ASSERT_EQ(code, 0);
}



TEST_F(TestHttpServer, testSendSyncMsg)
{
    TC_HttpRequest stHttpReq;
    stHttpReq.setCacheControl("no-cache");
    stHttpReq.setUserAgent("Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0; InfoPath.1; .NET CLR 1.1.4322)");
 
    std::string postData= "{\"appId\":\"3200\",\"from\":\"3200\",\"to\":\"100\",\"traceId\":\"SYNCMSG-8922-test-002\", \" command\":1, \"content\":\"this is a test for send sync msg\", \"users\":[\"100\",\"200\"]}";
    

    std::string url = _host +"/msg/sendsyncmsg";
    stHttpReq.setPostRequest(url, postData);
    
    TC_HttpResponse stHttpRep;
    stHttpReq.doRequest(stHttpRep, 3000);
   
    ASSERT_EQ(stHttpRep.getStatus(), 200);

    string content = stHttpRep.getContent();
    MLOG("testSendSyncMsg resp content:"<< content);
    rapidjson::Document cBody;
    cBody.Parse(content.c_str());
    ASSERT_EQ(cBody.IsObject(), true);

    int code;
    JsonUtil::getInt(cBody, "code", code);
    ASSERT_EQ(code, 0);
}

