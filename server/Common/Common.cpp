//
//  Common.cpp
//  otim
//
//  Created by lanhy on 2022/12/1.
//

#include "Common.h"
#include "ptcldefine.h"
#include "util/tc_common.h"
#include "RedisDBInterface.h"
#include "log.h"
#include "otim.h"
#include "otim_const.h"
#include "otim_err.h"
#include "BrokerPushServant.h"
#include "LongLinkRedis.h"
#include <uuid/uuid.h>

namespace otim
{
    PT_FLAGS_BITS getHeaderFlagBits(uint32_t flags)
    {
        PT_FLAGS_BITS flagBits = {0};
        memcpy(&flagBits, &flags, sizeof(PT_FLAGS_BITS));
        return flagBits;
    }

    tars::Int64 genSeqId(){
        //    struct timeval tv;
        //    gettimeofday(&tv, NULL);
        //    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
        return TC_Common::now2us();
    }



    std::string genUUID()
    {
        char uuid_string[37] = {0};
        uuid_t uu;
        uuid_generate(uu);
        uuid_unparse(uu, uuid_string);
        return std::string(uuid_string);
    }

    void saveRepeatedPackId(CRedisDBInterface *redis, const std::string & packId, tars::Int64 seqId, int lifeTime)
    {
        if (redis == nullptr) {
            return ;
        }
        MLOG_DEBUG("saveRepeatedPackId, packId:"<<packId);
        if (lifeTime == 0){
            return;
        }
        
        std::string strSeqId = TC_Common::tostr(seqId);
        redis->SetEXNX(otim::RKEY_REPEATPACKID + packId, strSeqId, lifeTime);
    }

    bool isRepeatedPackId(CRedisDBInterface *redis, const std::string & packId,  tars::Int64 & seqId)
    {
        if (redis == nullptr) {
            return false;
        }

        std::string strSeqId;
        if (EMRStatus::EM_KVDB_SUCCESS == redis->Get(otim::RKEY_REPEATPACKID + packId, strSeqId))
        {
            seqId = TC_Common::strto<tars::Int64>(strSeqId);
            MLOG_DEBUG("recv repeat msg, msgId:" << packId << ", strSeqId:" << strSeqId);
            return true;
        }
        
        MLOG_DEBUG("recv not repeat msg, msgId:" << packId);
        return false;
    }


    int savePriorityMsg(CRedisDBInterface *redis, const otim::OTIMPack & pack,const std::string & to,tars::Int64 seqId)
    {
        if (redis == nullptr){
            MLOG_DEBUG("redis is null!");
            return -1;
        }
        
        MLOG_DEBUG("PriorityMsg to: " << to <<" seqId:"<<seqId<<" packId:"<<pack.header.packId);
        std::string msg;
        otim::packTars<otim::OTIMPack>(pack, msg);
     
        EMRStatus ret = redis->ZSetAdd(otim::RKEY_MSG_HIGHPRJ + to, seqId, msg);
        if (ret != EMRStatus::EM_KVDB_SUCCESS)
        {
            MLOG_ERROR("Priority redis error or userId:"<<to<<" ret:"<<(int)ret);
            return otim::EC_REDIS_ERROR;
        }
        //设置过期时间 一个月，30*86400秒
        redis->Expire(otim::RKEY_MSG_HIGHPRJ + to, 2592000);
        
        return otim::EC_SUCCESS;
    }

    int dispatchMsg(const otim::ClientContext & clientContext, const otim::OTIMPack & req, const std::string &to)
    {
        std::vector<LongLinkPtr> vctLink = LongLinkRedis::getInstance()->getLongLinkByClientId(to);
        if (vctLink.size() == 0){
            //need push
            MLOG_DEBUG("the user offline ,will push msg:"<<to<<" packId:"<<req.header.packId);
            return otim::EC_SUCCESS;
        }
        
        for (auto item : vctLink){
            try
            {
                if (item->context.brokerId.empty()){
                    MLOG_ERROR("brokerId is empty uid: " << item->context.uid);
                    continue;
                }
                
                otim::BrokerPushServantPrx brokerPrx = otim::getServantPrx<otim::BrokerPushServantPrx>(item->context.brokerId);
//                brokerPrx->async_push(NULL, item->context.uid, req);
                int ret = brokerPrx->push(item->context.uid, req);
                
                MLOG_DEBUG("USERONLINE push uid:" << item->context.uid << " userId:" << to << " packId:" << req.header.packId << " size: " << req.payload.size()<<" ret:"<<ret);
            }
            catch (std::exception& e)
            {
                MLOG_ERROR("USERONLINE push error: " << e.what());
                continue;
            }
            catch (...)
            {
                MLOG_ERROR("USERONLINE push unknown exception");
            }
        }
        
        return otim::EC_SUCCESS;
    }

    int sendSyncMsg(const otim::ClientContext & clientContext, int command, const std::string &content, const std::vector<std::string>& userIds)
    {
        
        otim::OTIMPack pack;
        pack.header.type = otim::PT_SYNC_DATA_CMD;
        pack.header.packId = otim::genUUID();
        
        otim::SyncDataReq syncReq;
        syncReq.command = command;
        syncReq.content = content;
      
        otim::packTars<otim::SyncDataReq>(syncReq, pack.payload);

        sendSyncMsg(clientContext, pack, userIds);
        
    	return otim::EC_SUCCESS;
	}

    int sendSyncMsg(const otim::ClientContext & clientContext, const otim::OTIMPack &pack, const std::vector<std::string>& userIds)
    {
        for (std::string userId : userIds){
            if (clientContext.clientId == userId){
                continue;
            }
            
            dispatchMsg(clientContext, pack, userId);
        }

        return otim::EC_SUCCESS;
    }


    int sendPackToMySelf(const otim::ClientContext & clientContext, const otim::OTIMPack & req)
    {
        std::vector<LongLinkPtr> vctLink = LongLinkRedis::getInstance()->getLongLinkByClientId(clientContext.clientId);
        if (vctLink.size() <= 1){
            //need push
            MLOG_DEBUG("the user offline, only one app is online,will push msg  packId:"<<req.header.packId);
            return otim::EC_SUCCESS;
        }
        
        for (auto item : vctLink){
            try
            {
                if (item->context.brokerId.empty()){
                    MLOG_ERROR("brokerId is empty uid: " << item->context.uid);
                    continue;
                }
                if (clientContext.uid == item->context.uid){
                    MLOG_DEBUG("current link do not push uid:"<<clientContext.uid);
                    continue;
                }
      
                otim::BrokerPushServantPrx brokerPrx = otim::getServantPrx<otim::BrokerPushServantPrx>(item->context.brokerId);
    //            brokerPrx->async_push(NULL, item->context.uid, req);
                int ret = brokerPrx->syncMsg(clientContext, req);

                MLOG_DEBUG("MYSELF ONLINE syncMsg uid:" << item->context.uid << " userId:" << clientContext.clientId << " packId:" << req.header.packId << " size: " << req.payload.size()<<" ret:"<<ret);
            }
            catch (std::exception& e)
            {
                MLOG_ERROR("MYSELF push error: " << e.what());
                continue;
            }
            catch (...)
            {
                MLOG_ERROR("MYSELF push unknown exception");
            }
        }
        
        return otim::EC_SUCCESS;
    }
}
