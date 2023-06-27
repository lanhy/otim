#pragma once

#include "RedisDBInterface.h"
#include "log.h"
#include "ConnPool.h"

namespace otim
{
    inline CRedisDBInterface *open_redis(const std::string &config) {
        CRedisDBInterface *pRedis = nullptr;
        try {
            pRedis = new CRedisDBInterface(config);
        } catch (std::exception &e) {
            delete pRedis;
            return nullptr;
        }
        return pRedis;
    }
    
    inline void close_redis(CRedisDBInterface *pRedis) {
        if (pRedis) {
            delete pRedis;
        }
    }
    
    typedef ConnPool<CRedisDBInterface*> RedisPool;
    typedef ConnPtr<CRedisDBInterface*>  RedisConnPtr;
    
    int initRedisPool(int nPoolSize, const std::string &confFile);
/**
 usage:
 RedisConnPtr redis(RedisPool::instance());
 assert(redis.get() != nullptr);
 EMRStatus ret = redis->ZSetAdd(hhchat::HPMSG + sessionId, seqId, msgWrite);

 */

}
