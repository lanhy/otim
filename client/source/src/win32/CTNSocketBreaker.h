// Tencent is pleased to support the open source community by making Mars available.
// Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.

// Licensed under the MIT License (the "License"); you may not use this file except in 
// compliance with the License. You may obtain a copy of the License at
// http://opensource.org/licenses/MIT

// Unless required by applicable law or agreed to in writing, software distributed under the License is
// distributed on an "AS IS" basis, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
// either express or implied. See the License for the specific language governing permissions and
// limitations under the License.



#ifndef _SOCKSTSELECT_
#define _SOCKSTSELECT_

#include <winsock2.h>
#include <mutex>
//#include "thread/lock.h"

class CTNSocketBreaker {
public:
    CTNSocketBreaker();
    ~CTNSocketBreaker();
    
    bool isCreateSuc() const;
    bool reCreate();
    void close();
    
    bool fireon();
    bool clear();
    
    bool isBreak() const;
    SOCKET  breakerFD() const;
    
private:
    CTNSocketBreaker(const CTNSocketBreaker&);
    CTNSocketBreaker& operator=(const CTNSocketBreaker&);
    
private:
    SOCKET _socket_w;
    SOCKET _socket_r;
    struct sockaddr _sendin;
    int _sendinlen;
    bool  _create_success;
    bool  _broken;
    std::mutex _mutex;
};

#endif
