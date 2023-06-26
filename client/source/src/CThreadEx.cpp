//
//  CThreadWin.cpp
//
//  Created by lanhy on 2017/2/16.
//  Copyright © 2017年 兰怀玉. All rights reserved.
//

#include "CThreadEx.h"
#include <iostream>
#include "CLog.h"

static int s_threadId = 1;

CThreadEx::CThreadEx()
{
    m_thread = nullptr;
    m_bIsDied = false;
    m_running = false;
    
    m_threadId = s_threadId++;

}

CThreadEx::~CThreadEx()
{
    try {
        if (m_thread  != nullptr){
            if (m_thread->joinable()){
                m_thread->join();
            }
            delete m_thread;
        }
    } catch (...) {
        std::cout<<"exception:CThreadEx"<<std::endl;
    }
}

//bool型变量自动置位类
class CBoolGuard{
public:
    CBoolGuard(bool *value){
        _value = value;
        if (value != nullptr){
            *_value = false;
        }
    }
    
    ~CBoolGuard(){
        *_value = true;
    }
private:
    bool *_value;
};


void CThreadEx::callback(){
    
    CBoolGuard boolGuard(&m_bIsDied);
    
    this->run();
    
    OMTPLOG("Thread will died:"<<this->getId());
}

int CThreadEx::start()
{
	unique_lock<mutex> unilock(m_mutexRunning);
	m_running = true;
	m_thread = new std::thread(&CThreadEx::callback, this);
     
    return 0;
}

void CThreadEx::join()
{
    if (m_thread != nullptr && m_thread->joinable()){
        m_thread->join();
    }
}

int CThreadEx::detach()
{
    if (m_thread != nullptr){
      m_thread->detach();
    }
    
    return 0;
}

unsigned int CThreadEx::getId() {
    return m_threadId;
}

void CThreadEx::stop()
{
    unique_lock<mutex> unilock(m_mutexRunning);
    m_running = false;
}

bool CThreadEx::isRunning(){
    unique_lock<mutex> unilock(m_mutexRunning);
    return m_running;
}


