#include "OlapServantImp.h"
#include "servant/Application.h"
#include "ptcldefine.h"
#include "otim.h"
#include "otim_err.h"
#include "RedisPool.h"
#include "otim_const.h"
#include "log.h"

using namespace std;

//////////////////////////////////////////////////////
void OlapServantImp::initialize()
{
    //initialize servant here:
    //...
}

//////////////////////////////////////////////////////
void OlapServantImp::destroy()
{
    //destroy servant here:
    //...
}

tars::Int32 OlapServantImp::saveMsg(const otim::ClientContext & clientContext,const otim::OTIMPack & pack,const std::string & sessionId,tars::Int64 seqId,tars::TarsCurrentPtr _current_)
{
    
    //save to redis
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    assert(redis.get() != nullptr);
    
    std::string msg;
    otim::packTars<otim::OTIMPack>(pack, msg);
    MLOG_DEBUG("packId:"<<pack.header.writeToJsonString()<<" msgsize:"<<msg.size());
    EMRStatus ret = redis->ZSetAdd(otim::RKEY_MSG + sessionId, seqId, msg);
    if ( EMRStatus::EM_KVDB_SUCCESS != ret )
    {
        MLOG_ERROR("save msg fail:"<<(int)ret);
        return otim::EC_REDIS_ERROR;
    }
   
     
    //save to db
    saveToMysql(clientContext, pack, sessionId, seqId);
    
    return otim::EC_SUCCESS;
}

tars::Int32 OlapServantImp::saveToMysql(const otim::ClientContext & clientContext,const otim::OTIMPack & pack,const std::string & sessionId,tars::Int64 seqId)
{
    return otim::EC_SUCCESS;
}


