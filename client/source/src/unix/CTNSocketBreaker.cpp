//
//  CTNCTNSocketBreaker.cpp
//
//  Created by lanhy on 2018/4/12.
//  Copyright © 2018年 lanhy. All rights reserved.
//

#include "CTNSocketBreaker.h"
#include <fcntl.h>
#include <unistd.h>

CTNSocketBreaker::CTNSocketBreaker()
: _create_success(true),
_broken(false)
{
    reCreate();
}

CTNSocketBreaker::~CTNSocketBreaker()
{
    close();
}

bool CTNSocketBreaker::isCreateSuc() const
{
    return _create_success;
}

bool CTNSocketBreaker::reCreate()
{
    _pipes[0] = -1;
    _pipes[1] = -1;
    
    int Ret;
    Ret = pipe(_pipes);
//    xassert2(-1 != Ret, "pipe errno=%d", errno);
    
    if (Ret == -1)
    {
        _pipes[0] = -1;
        _pipes[1] = -1;
        _create_success = false;
        return _create_success;
    }
    
    long flags0 = fcntl(_pipes[0], F_GETFL, 0);
    long flags1 = fcntl(_pipes[1], F_GETFL, 0);
    
    if (flags0 < 0 || flags1 < 0) {
//        xerror2(TSF"get old flags error");
        ::close(_pipes[0]);
        ::close(_pipes[1]);
        _pipes[0] = -1;
        _pipes[1] = -1;
        _create_success = false;
        return _create_success;
    }
    
    flags0 |= O_NONBLOCK;
    flags1 |= O_NONBLOCK;
    int ret0 = fcntl(_pipes[0], F_SETFL, flags0);
    int ret1 = fcntl(_pipes[1], F_SETFL, flags1);
    
    if ((-1 == ret1) || (-1 == ret0)) {
//        xerror2(TSF"fcntl error");
        ::close(_pipes[0]);
        ::close(_pipes[1]);
        _pipes[0] = -1;
        _pipes[1] = -1;
        _create_success = false;
        return _create_success;
    }
    
    _create_success = true;
    return _create_success;
}

bool CTNSocketBreaker::fireon()
{
    std::unique_lock<std::mutex> unilock(_mutex);
    if (_broken){
        return true;
    }
    
    const char dummy = '1';
    int ret = (int)write(_pipes[1], &dummy, sizeof(dummy));
    _broken = true;
    
    if (ret < 0 || ret != (int)sizeof(dummy))
    {
//        xerror2(TSF"Ret:%_, errno:(%_, %_)", ret, errno, strerror(errno));
        _broken =  false;
    }
    
    return _broken;
}

bool CTNSocketBreaker::clear()
{
    std::unique_lock<std::mutex> unilock(_mutex);
    char dummy[128];
    int ret = (int)read(_pipes[0], dummy, sizeof(dummy));
    
    if (ret < 0)
    {
//        xverbose2(TSF"Ret=%0", ret);
        return false;
    }
    
    _broken =  false;
    return true;
}

void CTNSocketBreaker::close()
{
    _broken =  true;
    if(_pipes[1] >= 0)
        ::close(_pipes[1]);
    if(_pipes[0] >= 0)
        ::close(_pipes[0]);
}

int CTNSocketBreaker::breakerFD() const
{
    return _pipes[0];
}

bool CTNSocketBreaker::isBreak() const
{
    return _broken;
}
