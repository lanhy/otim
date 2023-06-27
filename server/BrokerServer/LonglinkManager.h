//
//  LonglinkManager.hpp
//  tarsim
//
//  Created by lanhy on 2022/3/22.
//

#ifndef LonglinkManager_hpp
#define LonglinkManager_hpp

#include <stdio.h>
#include <map>
#include <mutex>
#include "util/tc_singleton.h"
#include "servant/Application.h"
#include "util/tc_common.h"
#include "otim.h"
#include "LongLinkRedis.h"


class LongLinkManager : public tars::TC_Singleton<LongLinkManager>
{
public:
    void remove(uint64_t uid);
    LongLinkPtr update(uint64_t uid, const otim::OTIMHeader & header);
    LongLinkPtr update(uint64_t uid, const otim::ClientContext &context, tars::TarsCurrentPtr current);
    
    std::vector<LongLinkPtr> getLongLinkByClientId(const std::string &clientId);
    LongLinkPtr getLonglinkByUId(uint64_t uid);
private:
    LongLinkPtr get(uint64_t uid);
    void updateRedis(LongLinkPtr link);
    
    std::mutex _mutex;
    std::map<uint64_t, LongLinkPtr> _mapLonglink;
    std::map<std::string, std::set<uint64_t>> _mapClientIdUid;

};


#endif /* LonglinkManager_hpp */
