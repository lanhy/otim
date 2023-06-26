//
//  OMTPConst.h
//
//  Created by 兰怀玉 on 16/4/15.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef OMTPConst_h
#define OMTPConst_h

//网络协议
//OMTPNetConnected此状态可以自动重连
//OMTPNetLogout此状态不能自动重连,自己退出或者被踢掉
typedef enum  _OMTPNetStatus{
    OMTPNetNone = 0,
    OMTPNetConnecting = 1,
    OMTPNetConnected = 2,
    OMTPNetLogout =  3,
    OMTPNetRecving = 4,
    OMTPNetLogouting = 5
} OMTPNetStatus;


//#define DEVICE_TYPE_IOS      1
//#define DEVICE_TYPE_ANDROID  2
//#define DEVICE_TYPE_WINDOWS  3
//#define DEVICE_TYPE_MAC      4


#ifdef WIN32
#include <stdint.h>
//#include <windows.h>

#define tn_msleep(t)   Sleep(t)

#else
#include <unistd.h>

#define tn_msleep(t)   usleep(t*1000)

#endif






#endif /* OMTPConst_h */
