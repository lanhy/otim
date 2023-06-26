//
//  CTNSocketBreaker.hpp
//
//  Created by lanhy on 2018/4/12.
//  Copyright © 2018年 lanhy. All rights reserved.
//

#ifndef CTNSocketBreaker_h
#define CTNSocketBreaker_h
#include <mutex>

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
    int  breakerFD() const;
    
private:
    CTNSocketBreaker(const CTNSocketBreaker&);
    CTNSocketBreaker& operator=(const CTNSocketBreaker&);
    
private:
    int   _pipes[2];
    bool  _create_success;
    bool  _broken;
    std::mutex _mutex;
};


#endif /* CTNSocketBreaker_hpp */
