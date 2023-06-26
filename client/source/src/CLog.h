//
//  CLog.hpp
//
//  Created by 兰怀玉 on 16/5/6.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef CLog_hpp
#define CLog_hpp

#ifdef WIN32
#if _MSC_VER >= 1600
#pragma execution_character_set("utf-8")
#endif
#endif

#include <stdio.h>
#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <sstream>
#include "CSafeQueue.h"
#include "CThreadEx.h"


using namespace std;

//class ITNMPResponse;

/**
 * 用于输出log文件的类.
 */
class CLog : public CThreadEx
{
private:
    
public:
    CLog();
    ~CLog();
    
    static CLog* instance();
    static string getTimeStr();
    static unsigned long getCurrentThreadID();
    

    void setPath(string path);
    void writeLog(stringstream& logstream);
   
    virtual void run();
    string getFileName();

private:
    mutex m_mutex;
    string _appPath;
    
    static CLog* m_instance;
    FILE *_file;
    
    CSafeQueue<string> _logQueue;

    bool open();
    void close();
    
    void clearOldFile();

};


#define TNMPINS     CLog::instance()


#define OMTPLOG(logstr) try{   std::stringstream stream; stream <<" ["<<CLog::getCurrentThreadID()<<"]"<<CLog::getTimeStr()<<logstr; CLog::instance()->writeLog(stream); }catch(...){std::cout<<"++++Excecption occur on LOG++++++++"<<std::endl;}



//#define TNMPLOG(logstr)

#endif /* CLog_hpp */
