//
//  RequestTaskThread.cpp
//
//  Created by lanhy on 2023/2/9.
//  Copyright © 2023 lanhy. All rights reserved.
//

#include "RequestTaskThread.h"
#include "ptcldefine.h"
#include "RedisPool.h"
#include "Common.h"
#include "otim_const.h"
#include "baseservant.h"
#include "otim_err.h"

RequestTaskThread::RequestTaskThread()
{
    _interval = TASK_DEFAULT_INTERVAL * 1000;
}

RequestTaskThread::~RequestTaskThread()
{
}

bool RequestTaskThread::pushTask(const std::string &req)
{
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    if (EMRStatus::EM_KVDB_SUCCESS != redis->QueLPush(otim::RKEY_TASKCACHE, req)){
        MLOG_WARN("Can't queue value for:" << otim::RKEY_TASKCACHE);
        return false;
    }
    return true;
}

bool RequestTaskThread::popTask(std::string &req)
{
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    if (EMRStatus::EM_KVDB_SUCCESS != redis->QueRPop(otim::RKEY_TASKCACHE, req)){
        //        LOG_WARN("Can't pop list value for:" << Toon::groupTaskCache);
        return false;
    }
    
    return true;
}


void RequestTaskThread::stop()
{
    TC_ThreadLock::Lock sync(_lock);
    _running = false;
}

void RequestTaskThread::addTask(const otim::TaskQueueItem & item)
{
    MLOG_DEBUG("traceId:"<<item.traceId<<" taskType:"<<item.taskType<<" pack:"<<item.pack.header.writeToJsonString());
 
    std::string pack;
    otim::packTars<otim::TaskQueueItem>(item, pack);

    this->pushTask(pack);
    _semaphore.signal();
}

void RequestTaskThread::run()
{
    MLOG_DEBUG("RequestTaskThread start sleep interval:"<<_interval);
 
    int ret = 0;
    while (this->isAlive())
    {
        usleep(_interval);
        MLOG_INFO("next task sleep");

        std::string pack;
        bool ok = this->popTask(pack);
        if (!ok || pack.empty())
        {
            //获取不到任务，等待60s
            MLOG_INFO("no task wait 60s");
            _semaphore.wait(60);
            continue;
        }
        
		std::vector<char> data(pack.begin(), pack.end());

        otim::TaskQueueItem item;
        otim::unpackTars<otim::TaskQueueItem>(data, item);

        MLOG_DEBUG("traceId:"<<item.traceId<<" taskType:"<<item.taskType<<" req pack:"<<item.pack.header.writeToJsonString());
     
        if (item.taskType == otim::TASK_TYPE_BIZMSG)
        {
            ret = this->sendBizMsgReq(item);
        }
        else if (item.taskType == otim::TASK_TYPE_SINGLE)
        {
            ret = this->sendSinglechatReq(item);
        }
        else if (item.taskType == otim::TASK_TYPE_SYNCCMD)
        {
            ret = this->sendSyncCmdReq(item);
        }
        else
        {
            MLOG_ERROR("taskType error:"<<item.taskType<<" traceId:"<<item.traceId);
        }
        
        //发送失败，重新加入队列，再重试 暂时不加,需要处理groupchat返回值后区别对待，否则容易无限执行无效指令
        if (ret <= TARSSERVERQUEUETIMEOUT)
        {
            MLOG_ERROR("task RPC error:" << ret<<" taskType:"<<item.taskType<<" traceId:"<<item.traceId);
        }
        else if (ret != 0)
        {
            MLOG_WARN("Send failed:"<<ret<<" traceId:"<<item.traceId<<" taskType:"<<item.taskType);
        }
 
    }
     
    MLOG_DEBUG("RequestTaskThread end");
}


int RequestTaskThread::sendSinglechatReq(const otim::TaskQueueItem & item)
{
    int ret = 0;
    try {
        MLOG_INFO("traceId:"<<item.traceId);
        otim::BaseServantPrx proxy = otim::getServantPrx<otim::BaseServantPrx>(PRXSTR_SINGLE_CHAT);

        otim::ClientContext context;
        context.clientId = "http";
        context.brokerId = "httpAPI";
		otim::OTIMPack resp;
        ret = proxy->request(context, item.pack, resp);
    }
    catch (std::exception& e) {
        MLOG_ERROR("task sendSinglechatReq error:" << e.what());
        ret = otim::EC_SERVICE_UNAVAILABLE;
    }
    catch (...) {
        MLOG_ERROR("task sendSinglechatReq unknown exception");
        ret = otim::EC_SERVICE_UNAVAILABLE;
    }
    
    return ret;
}

int RequestTaskThread::sendBizMsgReq(const otim::TaskQueueItem & item)
{
    int ret = 0;
    try {
        MLOG_INFO("traceId:"<<item.traceId);

        otim::BaseServantPrx proxy = otim::getServantPrx<otim::BaseServantPrx>(PRXSTR_BIZMSG);

        otim::ClientContext context;
        context.clientId = "http";
        context.brokerId = "httpAPI";
		otim::OTIMPack resp;
        ret = proxy->request(context, item.pack, resp);
    }
    catch (std::exception& e) {
        MLOG_ERROR("task biz sendBizMsgReq error:" << e.what());
        ret = otim::EC_SERVICE_UNAVAILABLE;
    }
    catch (...) {
        MLOG_ERROR("task biz sendBizMsgReq unknown exception");
        ret = otim::EC_SERVICE_UNAVAILABLE;
    }
    
    return ret;
}


// 发送sync
int RequestTaskThread::sendSyncCmdReq(const otim::TaskQueueItem &item)
{
    int ret = 0;
    try {
        
        
        MLOG_INFO("Send sync cmd traceId:"<<item.traceId);
        otim::ClientContext context;
        context.clientId = "http";
        context.brokerId = "httpAPI";
        otim::sendSyncMsg(context, item.pack, item.userIds);
    }
    catch (std::exception &e) {
        MLOG_ERROR("sendSyncCmdReq request error: " << e.what());
        ret = otim::EC_SERVICE_UNAVAILABLE;
    }
    catch (...) {
        MLOG_ERROR("sendSyncCmdReq request unknown exception");
        ret = otim::EC_SERVICE_UNAVAILABLE;
    }
    
    return ret;
}
