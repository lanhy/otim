//
//  CTNInflightVector.cpp
//  OMTP
//
//  Created by lanhy on 16/11/3.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#include "CTNInflightVector.h"
#include "CLog.h"
#include "OMTPHostInfo.h"


CTNInflightVector* CTNInflightVector::m_instance = NULL;

CTNInflightVector::CTNInflightVector()
{
    
}

CTNInflightVector::~CTNInflightVector()
{
}

void CTNInflightVector::addToInflight(InflightMessage* req)
{
    if (req == NULL) {
        return;
    }
    
    std::unique_lock<std::mutex> unilock(m_mutexInflight);
    m_vctInflight.push_back(req);
}

int CTNInflightVector::removeInflightMutex(std::string & msgId)
{
    std::unique_lock<std::mutex> unilock(m_mutexInflight);
    OMTPLOG("removeInflightMutex:"<<msgId);
    
    return removeInflight(msgId);
}

int CTNInflightVector::removeInflight(std::string & msgId)
{
    int size = 0;
    OMTPLOG("before removed inflight:"<<m_vctInflight.size());
    std::vector<InflightMessage*>::iterator itTmp = m_vctInflight.begin();
    while (itTmp != m_vctInflight.end()) {
        //must define tmpMsg;
        InflightMessage* tmpMsg = *itTmp;
        if (tmpMsg->req.header.packId == msgId){
           itTmp = m_vctInflight.erase(itTmp);

            size = tmpMsg->size;
            delete tmpMsg;

            OMTPLOG("find msg and removed:"<<msgId);
            break;
        }
        itTmp ++;
    }
    
    OMTPLOG("after removed inflight:"<<m_vctInflight.size());
    return size;
}

bool CTNInflightVector::inflightIsEmpty()
{
    std::unique_lock<std::mutex> unilock(m_mutexInflight);
    if (m_vctInflight.size() == 0) {
        return true;
    }
    
    return false;
}

void CTNInflightVector::forceResend(std::function<void(InflightMessage* msg)> callback)
{
//    std::cout<<"forceResend enter:"<<endl;
    std::unique_lock<std::mutex> unilock(m_mutexInflight);
//    std::cout<<"forceResend enter1:"<<m_vctInflight.size()<<std::endl;
    
    std::vector<InflightMessage*>::iterator itTmp = m_vctInflight.begin();
    while (itTmp != m_vctInflight.end()) {
//        std::cout<<"forceResend will callback:"<<(*itTmp)->req->msg_id<<std::endl;
        
        callback(*itTmp);
        itTmp ++;
    }
    
//    std::cout<<"forceResend exit:"<<endl;
}


void CTNInflightVector::checkInflight(bool isOffline, std::function<void(bool isResend, InflightMessage*)> callback)
{
    std::unique_lock<std::mutex> unilock(m_mutexInflight);
    if (m_vctInflight.empty()) {
        return;
    }
    
    long now = time(NULL);
    
    std::vector<string> vctRemoved;
    
    std::vector<InflightMessage*>::iterator itTmp = m_vctInflight.begin();
    while (itTmp != m_vctInflight.end()) {
        //次数超过，或者时间超过都失败
        if ((*itTmp)->retryTimes >= RETRY_MAXTIMES) {
            vctRemoved.push_back((*itTmp)->req.header.packId);
            
            OMTPLOG("Send failed:"<<(*itTmp)->req.header.packId);
            callback(false, *itTmp);
        }
        else if (isOffline){
//            cout <<"don't check inflight"<<endl;
            continue;
        }
        else if (now - (*itTmp)->timestamp > RESEND_TIMER){
//            cout<<"checkInflight:"<<now<<" timestamp:"<<(*itTmp)->req->timestamp<<endl;;
            (*itTmp)->timestamp = now;
            (*itTmp)->retryTimes ++;
            
            OMTPLOG("Resend msg:"<<(*itTmp)->req.header.packId<<" retrytimes:"<<(*itTmp)->retryTimes);
            callback(true, *itTmp);
        }
        itTmp ++;
    }
    
    //remove failed sucess;
    for (int i = 0; i < vctRemoved.size(); i++){
        removeInflight(vctRemoved.at(i));
    }
    
}
