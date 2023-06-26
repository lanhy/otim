//
//  CThreadEx.h
//
//  Created by lanhy on 2017/2/16.
//  Copyright © 2017年 兰怀玉. All rights reserved.
//

#ifndef CThreadWin_hpp
#define CThreadWin_hpp

//#include "CThread.h"
#include <thread>
#include <mutex>

using namespace std;

class CThreadEx //: public CThread
{
public:
    CThreadEx();
    virtual ~CThreadEx();
    
    virtual int start();
    virtual void join();
    virtual int detach();
   
    bool isDied(){
        return m_bIsDied;
    }
    
    virtual void stop();
    bool isRunning();

    unsigned int getId();
    
    virtual void run()= 0;
private:
    void callback();
    
protected:
    std::thread  *m_thread;
 
    mutex m_mutexRunning;
    bool        m_running;
//    int        m_detached;
    unsigned int m_threadId;
    bool m_bIsDied;

   
};

#endif /* CThreadWin_hpp */
