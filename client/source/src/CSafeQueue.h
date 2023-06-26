//
//  CSafeQueue.h
//
//  Created by 兰怀玉 on 16/4/15.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef CSafeQueue_hpp
#define CSafeQueue_hpp

#include <stdio.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <list>

#define SFQUEUE_OK          0
#define SFQUEUE_TIMEOUT     -200
#define SFQUEUE_EMPTY       -201

template <typename T>
class CSafeQueue {
    std::queue<T> m_queue;
    
    std::condition_variable m_condition;
    std::mutex m_mutex;
    bool _needCond;
public:
    CSafeQueue(bool needCond = true){
        _needCond = needCond;
    }
    
    
    void push(const T& t){
        std::unique_lock<std::mutex> unilock(m_mutex);
        m_queue.push(t);
        if (_needCond){
            m_condition.notify_all();
        }
    }

    void weakup(){
        std::unique_lock<std::mutex> unilock(m_mutex);
        m_condition.notify_all();
    }

    size_t size(){
        std::unique_lock<std::mutex> unilock(m_mutex);
        return m_queue.size();
    }
    
    bool empty(){
        std::unique_lock<std::mutex> unilock(m_mutex);
        return m_queue.empty();
    }
   

    //return 0 ok, return <0 timeout
     int pop(T &t,int timeout){
        std::unique_lock<std::mutex> unilock(m_mutex);
        if (_needCond && m_queue.empty()) {
            if  (m_condition.wait_for(unilock,std::chrono::seconds(timeout)) == std::cv_status::timeout){
                return SFQUEUE_TIMEOUT;
            }
            else{
                if (m_queue.empty())
                {
//                    std::cout <<"empty retun NULL"<<std::endl;
                    return SFQUEUE_EMPTY;
                }
            }
        }
        
        
         t = m_queue.front();
         m_queue.pop();
         return SFQUEUE_OK;
    }


};


#endif /* CSafeQueue_hpp */
