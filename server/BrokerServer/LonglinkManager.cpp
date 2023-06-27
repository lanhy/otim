//
//  LonglinkManager.cpp
//  tarsim
//
//  Created by lanhy on 2022/3/22.
//

#include "LonglinkManager.h"
#include "log.h"
#include "ptcldefine.h"


void LongLinkManager::remove(uint64_t uid)
{
    std::lock_guard<std::mutex> guard(_mutex);
    auto itFind = _mapLonglink.find(uid);
    if (itFind != _mapLonglink.end()){
        
        std::string clientId = itFind->second->context.clientId;
        MLOG_DEBUG("DEL CONN uid:"<<uid<<" clientId:"<<clientId<<" deviceType:"<<itFind->second->context.deviceType);

        //remove from redis
        LongLinkRedis::getInstance()->remove(clientId, itFind->second->context.deviceType);
        _mapLonglink.erase(itFind);
        
        auto itFind2 = _mapClientIdUid.find(clientId);
        if (itFind2 == _mapClientIdUid.end()){
            return;
        }
        
        if (itFind2->second.size() > 1){
            itFind2->second.erase(uid);
        }
        else{
            _mapClientIdUid.erase(itFind2);
        }
    }
}

LongLinkPtr LongLinkManager::get(uint64_t uid)
{
    auto itFind = _mapLonglink.find(uid);
    if (itFind != _mapLonglink.end()){
        MLOG_DEBUG("uid:"<<uid<<" clientId:"<<itFind->second->context.clientId);
        return itFind->second;
    }
    
//    add new
    LongLinkPtr pTmp(new LongLink());
    pTmp->context.uid = uid;
    _mapLonglink[uid] = pTmp;

    return pTmp;
}

LongLinkPtr LongLinkManager::update(uint64_t uid, const otim::OTIMHeader & header)
{
    std::lock_guard<std::mutex> guard(_mutex);
//    flags
    otim::PT_FLAGS_BITS flagBits = otim::getHeaderFlagBits(header.flags);
    
    LongLinkPtr longlink = this->get(uid);
    longlink->ptclVersion = header.version;
    longlink->crypto = flagBits.crypto;
    longlink->compress = flagBits.compress;
    
    MLOG_DEBUG("uid:"<<uid<<" ptclVersion:"<<longlink->ptclVersion<<" crypto:"<<longlink->crypto<<" compress:"<<longlink->compress);

    return longlink;
}

LongLinkPtr LongLinkManager::update(uint64_t uid, const otim::ClientContext &context, tars::TarsCurrentPtr current)
{
    std::lock_guard<std::mutex> guard(_mutex);

    LongLinkPtr longlink = this->get(uid);
    longlink->context = context;
    longlink->current = current;

    //update _mapClientIdUid
    if (!context.clientId.empty()){
        _mapClientIdUid[longlink->context.clientId].insert(uid);
    }
    MLOG_DEBUG("uid:"<<uid<<" context:"<<longlink->context.writeToJsonString());
    LongLinkRedis::getInstance()->add(longlink);

    return longlink;
}

std::vector<LongLinkPtr> LongLinkManager::getLongLinkByClientId(const std::string &clientId)
{
    std::lock_guard<std::mutex> guard(_mutex);
    std::vector<LongLinkPtr> vctRet;

    auto itFind = _mapClientIdUid.find(clientId);
    if (itFind == _mapClientIdUid.end()){
        return vctRet;
    }

    for (auto uid : itFind->second){
        auto itFind = _mapLonglink.find(uid);
        if (itFind == _mapLonglink.end()){
            continue;
        }
        
        vctRet.push_back(itFind->second);
        MLOG_DEBUG("uid:"<<uid<<" clientId:"<<itFind->second->context.clientId);
    }
    
    return vctRet;
}

LongLinkPtr LongLinkManager::getLonglinkByUId(uint64_t uid)
{
    std::lock_guard<std::mutex> guard(_mutex);
    auto itFind = _mapLonglink.find(uid);
    if (itFind != _mapLonglink.end()){
        MLOG_DEBUG("uid:"<<uid<<" clientId:"<<itFind->second->context.clientId);
        return itFind->second;
    }

    return nullptr;
}
