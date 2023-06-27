#include "RedisPool.h"
#include "log.h"
#include "util/tc_config.h"

namespace otim{


    inline TC_Mysql *open_mysql(const std::string &confFile) {
        TC_Mysql *pDb = new TC_Mysql();
        try {
            TC_Config conf;
            conf.parseFile(confFile);
           
            MLOG_DEBUG("open_mysql connecting:"<<conf.tostr());

            TC_DBConf dbConf = conf.loadFromMap(conf.getDomainMap("/db"));
            pDb->init(dbConf);
            pDb->connect();
        } catch (TC_Mysql_Exception &e) {
            MLOG_ERROR("open_mysql failed, e: " << e.what());
            delete pDb;
            pDb = nullptr;
        }
        MLOG_DEBUG("open_mysql pDb 0x" << pDb);
        return pDb;
    }


    inline void close_mysql(TC_Mysql *pMySql) {
        MLOG_DEBUG("close_mysql");
        if (pMySql != nullptr) {
            pMySql->disconnect();
            delete pMySql;
        }
        MLOG_DEBUG("close_mysql ok");
    }


    int initDbPools(int nPoolSize, const std::string &confFile);
    {
        if (nPoolSize < 5){
            nPoolSize = 5;
        }
        if (!DbPool::instance()->open(nPoolSize, std::bind(open_mysql, confFile), close_mysql)) {
            MLOG_ERROR("open db failed.");
        }
    
        return 0;	
    }
    
}
