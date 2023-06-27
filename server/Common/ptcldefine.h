//
//  ptcldefine.h
//
//  Created by lanhy on 2022/03/22.
//  Copyright © 2022 lanhy. All rights reserved.
//

#ifndef ptcldefine_h
#define ptcldefine_h
#include <vector>
#include "otim.h"
#include "log.h"
//ACK|DUP|COUNTER|OVERRIDE|REVOKE|HIGHPRJ|COMPRESS|CRYPTO|RESERVED

namespace otim
{
struct PT_FLAGS_BITS{
    unsigned int ack:1;
    unsigned int dup:1;
    unsigned int counter:1;
    unsigned int overwrite:1;
    unsigned int revoke:1;
    unsigned int highPRJ:1;
    unsigned int compress:3;
    unsigned int crypto:3;
    unsigned int reserved:20;
};

PT_FLAGS_BITS getHeaderFlagBits(uint32_t flags);//impl in common.cpp

//buf -> T
template<typename T>
bool unpackTars(const std::vector<char> data, T &t)
{
    if(data.empty())
    {
        return false;
    }
    
    try
    {
        tars::TarsInputStream<tars::BufferReader> is;
        
        is.setBuffer(data);
        
        t.readFrom(is);
        
        return true;
    }
    catch(std::exception & e)
    {
        MLOG_ERROR("unpackTars error:"<<e.what());
    }
    catch(...)
    {}
    
    return false;
}


//T -> buf
template<typename T>
bool packTars(const T & t, std::vector<char> &data)
{
    try
    {
        data.clear();
        
        tars::TarsOutputStream<tars::BufferWriter> os;
        
        t.writeTo(os);
        
        data = os.getByteBuffer();
        
        return true;
    }
    catch(std::exception & e)
    {
        MLOG_ERROR("packTars error:"<<e.what());
    }
    catch(...)
    {}
    
    return false;
}


template<typename T>
bool packTars(const T & t, std::string &data)
{
    try
    {
        data.clear();
        
        tars::TarsOutputStream<tars::BufferWriter> os;
        
        t.writeTo(os);
        
        data.assign(os.getBuffer(), os.getLength());
        
        return true;
    }
    catch(std::exception & e)
    {
        MLOG_ERROR("packTars error:"<<e.what());
    }
    catch(...)
    {}
    
    return false;
}

}

#endif /* ptcldefine_h */
