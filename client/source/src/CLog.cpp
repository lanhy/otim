//
//  CLog.cpp
//  OMTP
//
//  Created by 兰怀玉 on 16/5/6.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#include "CLog.h"
#include <iostream>
//#include "OMTPResponse.h"
#include <chrono>
using namespace std::chrono;

#ifdef WIN32
#include <Windows.h>
#include <ctime>
#elif ANDROID
#include <unistd.h>
#include <jni.h>
#undef printf
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG,"native-tnmp",__VA_ARGS__)

#else
 #include <sys/types.h>
#endif



#define LOG_FILENAME(path, tm) (path+"/imlog"+tm+".txt")

CLog* CLog::m_instance = NULL;

CLog* CLog::instance(){
    if (m_instance == NULL) {
        m_instance = new CLog();
        m_instance->start();
    }
    
    return m_instance;
}

CLog::CLog()
{
    _file = nullptr;
}

CLog::~CLog()
{
}

bool CLog::open()
{
    if (_file != nullptr) {
        fclose(_file);
        _file = nullptr;
    }
    
    string sFileName = this->getFileName();// LOG_FILENAME(_appPath);

    _file = fopen(sFileName.c_str(), "a");
    
    if (nullptr == _file) {
        std::cout<<"can't open the file:"<<sFileName<<std::endl;
        return false;
    }
    
    return true;
}

void CLog::close()
{
    if (_file != nullptr) {
        fclose(_file);
        _file = nullptr;
    }
}


void CLog::setPath(string path){
    if (path.length() == 0){
        return;
    }
    
    _appPath = path;
}

void CLog::clearOldFile(){
    
    system_clock::time_point now = system_clock::now();
    for (int i = 0; i < 10; i++){
        std::time_t tt = system_clock::to_time_t(now - std::chrono::hours(24*(i+7)));
        struct tm* ptm = localtime(&tt);
        char strDateTime[128] = {0};
        strftime(strDateTime, 30, "%Y-%m-%d", ptm);
        string sFileName = LOG_FILENAME(_appPath, strDateTime);
        if (remove(sFileName.c_str()) == 0) {
            OMTPLOG("Succeded Remove log file:"<<sFileName);
        }else{
            OMTPLOG("Fail to Remove log file:"<<sFileName);
        }
    
        
    }
}

string CLog::getFileName()
{
    time_t tNowTime;
    time(&tNowTime);
    
    tm* tLocalTime = localtime(&tNowTime);
    
    //"2011-07-18 23:03:01 ";
    char strDateTime[64] = {0};
    strftime(strDateTime, 30, "%Y-%m-%d", tLocalTime);
    string sFileName = LOG_FILENAME(_appPath, strDateTime);

    return sFileName;
}


unsigned long CLog::getCurrentThreadID()
{
#ifdef WIN32
    return (unsigned long )GetCurrentThreadId();
#elif __APPLE__
    return (unsigned long )pthread_mach_thread_np(pthread_self());
//#elif defined(ANDROID)
//    return gettid();
#else
    return (unsigned long)pthread_self();
//    return (unsigned long )pthread_mach_thread_np(pthread_self());
#endif
}




void CLog::writeLog(stringstream& logstream)
{
    //    logstream<<" thread:"<<getCurrentThreadID();
    string log = logstream.str();
    _logQueue.push(log);
}

//得到当前时间的字符串
string CLog::getTimeStr()
{
    static std::chrono::steady_clock::time_point startup = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    
    auto d = std::chrono::duration_cast<std::chrono::milliseconds>(now - startup);
    auto tt = std::chrono::system_clock::to_time_t
    (std::chrono::system_clock::now());
    struct tm* ptm = localtime(&tt);
    char date[128] = {0};
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d.%lld  ",
            (int)ptm->tm_year + 1900,(int)ptm->tm_mon + 1,(int)ptm->tm_mday,
            (int)ptm->tm_hour,(int)ptm->tm_min,(int)ptm->tm_sec, d.count());
    return std::string(date);
}



void CLog::run()
{
    printf("CLog::run()");
    this->open();
    
    this->clearOldFile();
    
    int ret = 0;
    while (this->isRunning()) {
        string logStr;
        ret = _logQueue.pop(logStr, 60000);
        if (ret != SFQUEUE_OK || logStr.length() == 0 ){
            continue;
        }
        
        if (nullptr == _file) {
            this->open();
        }
        
        if (_file != nullptr) {
            fwrite(logStr.c_str(), sizeof(char), logStr.length(), _file);
            fwrite("\r\n", 1, 2, _file);
            fflush(_file);
        }
        
        printf("%s\n", logStr.c_str());
    }

    this->close();
}
