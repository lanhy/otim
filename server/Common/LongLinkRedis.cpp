//
//  LongLinkRedis.cpp
//  otim
//
//  Created by lanhy on 2022/3/22.
//

#include "LongLinkRedis.h"
#include "log.h"
#include "ptcldefine.h"
#include "RedisPool.h"
#include "otim_const.h"
#include "tup/TarsJson.h"

std::string LongLink::toJson()
{
    tars::JsonValueObjPtr p = new tars::JsonValueObj();
    p->value["clientId"] = tars::JsonOutput::writeJson(context.clientId);
    p->value["deviceType"] = tars::JsonOutput::writeJson(context.deviceType);
    p->value["deviceId"] = tars::JsonOutput::writeJson(context.deviceId);
    p->value["uid"] = tars::JsonOutput::writeJson(context.uid);
    p->value["ptclVersion"] = tars::JsonOutput::writeJson(ptclVersion);
    p->value["loginTime"] = tars::JsonOutput::writeJson(loginTime);
    p->value["brokerId"] = tars::JsonOutput::writeJson(context.brokerId);
   
    return tars::TC_Json::writeValue(p);
}

void LongLink::fromJson(const std::string &json)
{
    tars::JsonValuePtr  p = tars::TC_Json::getValue(json);
    if (NULL == p.get() || p->getType() != tars::eJsonTypeObj)
    {
         MLOG_ERROR("LongLink Json read 'struct' type mismatch, get type:"<<json);
         return;
    }

    tars::JsonValueObjPtr pObj=tars::JsonValueObjPtr::dynamicCast(p);
    tars::JsonInput::readJson(context.clientId,pObj->value["clientId"], true);
    tars::JsonInput::readJson(context.deviceType,pObj->value["deviceType"], true);
    tars::JsonInput::readJson(context.deviceId,pObj->value["deviceId"], true);
    tars::JsonInput::readJson(context.uid,pObj->value["uid"], true);
    tars::JsonInput::readJson(ptclVersion,pObj->value["ptclVersion"], true);
    tars::JsonInput::readJson(loginTime,pObj->value["loginTime"], true);
    tars::JsonInput::readJson(context.brokerId,pObj->value["brokerId"], true);

    MLOG_DEBUG("uid:"<<context.uid<<" clientId:"<<context.clientId<<" deviceId:"<<context.deviceId<<" loginTime:"<<loginTime);
}

/**
 LongLinkRedis
 */

void LongLinkRedis::remove(const std::string &clientId, int deviceType)
{
    //remove from redis
    std::string key = otim::RKEY_CONN + clientId;
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    assert(redis.get() != nullptr);
    EMRStatus ret = redis->DelHashField(key, std::to_string(deviceType));
    MLOG_DEBUG("DelHashField key :"<<key<<" deviceType:"<<deviceType<<" ret:"<<(int)ret);
}

void LongLinkRedis::add(LongLinkPtr link)
{
    if (link.get() == nullptr){
        MLOG_ERROR("longlink is null");
        return;
    }
    std::string json = link->toJson();
    MLOG_DEBUG("uid:"<<link->context.uid<<" clientId:"<<link->context.clientId<<" json:"<<json);

    otim::RedisConnPtr redis(otim::RedisPool::instance());
    assert(redis.get() != nullptr);

    std::string key = otim::RKEY_CONN + link->context.clientId;
    EMRStatus ret = redis->SetHashItem(key, std::to_string(link->context.deviceType), json);
    MLOG_DEBUG("uid:"<<link->context.uid<<" key:"<<key<<" json:"<<json<<" ret:"<<(int)ret);
}


std::vector<LongLinkPtr> LongLinkRedis::getLongLinkByClientId(const std::string &clientId)
{
    otim::RedisConnPtr redis(otim::RedisPool::instance());
    assert(redis.get() != nullptr);

    std::map<std::string, std::string> mapItems;
    std::string key = otim::RKEY_CONN + clientId;
    EMRStatus ret = redis->GetAllHashItem(key, mapItems);
    MLOG_DEBUG("key:"<<key<<" cout:"<<mapItems.size()<<" ret:"<<(int)ret);

    std::vector<LongLinkPtr> vctRet;
    for (auto item : mapItems){
        MLOG_DEBUG("deviceType:"<<item.first<<" json:"<<item.second);
        LongLinkPtr pTmp(new LongLink());
        pTmp->fromJson(item.second);
        if (!pTmp->isValid()){
            MLOG_DEBUG("curr item is invalid:"<<item.second);
            continue;
        }
        vctRet.push_back(pTmp);
        MLOG_DEBUG("clientId:"<<pTmp->context.clientId);
    }
    
    return vctRet;
}
