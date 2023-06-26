//
//  CTNPacket_h
//
//  Created by 兰怀玉 on 16/4/13.
//  Copyright © 2016年 兰怀玉. All rights reserved.
//

#ifndef CTNPacket_h
#define CTNPacket_h

#include <stdio.h>

#ifdef WIN32

#include <Winsock2.h>
typedef __int32 int32;
typedef __int16 int16;

#else

#include <sys/types.h>

typedef __int32_t int32;
typedef __int16_t int16;

#endif

#define NEW_PACKET(len) ((TNPacketBuffer *)new char[len])

#define DELETE_PACKET(p) if (p != NULL) {delete [] (char*)p; p = NULL;}

const int PTCL_VERSION = 1;

const uint32_t PACK_MIN_SIZE = 4;//length size
#pragma pack(push)
#pragma pack(1)

struct TNPacketBuffer{
    uint32_t length;//pack size (include the size of length)
    char payload[0];
    
    TNPacketBuffer(){
        length = 0;
    }
    
    void decode(){
        length = ntohl(length);
    }
    
    void encode(){
        length = htonl(length);
    }
};

#pragma pack(pop)



#endif /* CTNPacket_h */
