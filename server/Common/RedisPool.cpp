#include "RedisPool.h"
#include "log.h"

namespace otim{
    int initRedisPool(int nPoolSize, const std::string &confFile)
    {
        if (nPoolSize < 5){
            nPoolSize = 5;
        }
        
        //RedisPool::create(5, nPoolSize, std::bind(open_redis, confFile), close_redis);
        RedisPool::instance()->open(5, nPoolSize, std::bind(open_redis, confFile), close_redis);

        return 0;	
    }
    
}
