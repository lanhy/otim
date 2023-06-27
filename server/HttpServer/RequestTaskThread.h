//
//  RequestTaskThread.h
//
//  Created by lanhy on 2023/2/9.
//  Copyright © 2023 lanhy. All rights reserved.
//

#ifndef RequestTaskThread_h
#define RequestTaskThread_h

#include <string>
#include <vector>
#include "util/tc_thread.h"
#include "util/tc_thread_queue.h"
#include "util/tc_singleton.h"
#include "Semaphore.h"
#include "HttpServant.h"

//500ms
const int TASK_DEFAULT_INTERVAL  = 500;

class RequestTaskThread : public tars::TC_Thread, public tars::TC_Singleton<RequestTaskThread>
{
public:
    RequestTaskThread();
    ~RequestTaskThread();
    
    
    void stop();
    
    void addTask(const otim::TaskQueueItem & item);

    void setInterval(int interval){
        //变成微秒
        _interval = interval*1000;
    }
    
 protected:
    virtual void run();
    
    bool pushTask(const std::string &req);
    bool popTask(std::string &req);

    int sendSinglechatReq(const otim::TaskQueueItem & item);
    int sendBizMsgReq(const otim::TaskQueueItem & item);
    
    // 发送sync command
    int sendSyncCmdReq(const otim::TaskQueueItem &item);
    
    
    int _interval;
    CSemaphoreSimple _semaphore;
};

#endif /* RequestTaskThread */
