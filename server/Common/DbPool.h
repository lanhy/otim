#pragma once

#include "util/tc_mysql.h"
#include "log.h"
#include "ConnPool.h"

using namespace tars;

using namespace otim
{

typedef ConnPool<TC_Mysql*> DbPool;
typedef ConnPtr<TC_Mysql*>  DbConnPtr;

int initDbPools(int count, const std::string &confFile);

};
