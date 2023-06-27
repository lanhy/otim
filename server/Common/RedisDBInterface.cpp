#include "RedisDBInterface.h"
#include <assert.h>
#include "log.h"
#include <string.h>
#include "strutil.h"
#include <stdlib.h>
#include <algorithm>
#include "ScopeLogger.h"
#include "util/tc_common.h"

#define  MAX_RECONNEC_TIMES 3

#define REDIS_REPLY_STRING_TYPE		0x01
#define REDIS_REPLY_ARRAY_TYPE		0x02
#define REDIS_REPLY_INTEGER_TYPE	0x04
#define REDIS_REPLY_NIL_TYPE		0x08
#define REDIS_REPLY_STATUS_TYPE	    0x10
#define REDIS_REPLY_ERROR_TYPE		0x20

const int nREDISDB_TRANSFER_SECONDS = 500;



#define DO_REDIS_PUB_FUN(__FUN__, __FUNERROR_NAME__){	\
	int nCount = 0; \
	EMRStatus emRest = EMRStatus::EM_KVDB_ERROR;  \
	while (true)\
	{\
        if (!IsConnected())\
        {\
            emRest = EMRStatus::EM_KVDB_ERROR;\
        }\
        else\
        {\
            CScopeMutexLocker cScopeLock(m_cMutex); \
            emRest = __FUN__; \
            if(m_pRedisDBContex && m_pRedisDBContex->err)   \
            {   \
                MLOG_ERROR(__FUNCTION__ << " OLD RedisDBInterface, errno:" << m_pRedisDBContex->err << " error:" << m_pRedisDBContex->errstr << " cmd:" << __FUNERROR_NAME__);    \
            }   \
        }\
		if ((EMRStatus::EM_KVDB_ERROR == emRest) && !IsConnected())\
		{\
            if (nCount < MAX_RECONNEC_TIMES)\
            {\
                if(!ReConnectDB())\
                {\
                    struct timeval now;\
                    gettimeofday(&now, NULL);\
                    uint32_t seed = now.tv_sec*1000000+now.tv_usec;\
                    srandom(seed);\
                    usleep(random() % 700001);\
                }\
                nCount++; \
            }\
            else\
            {\
                MLOG_ERROR("redisCommand exec '" << __FUNERROR_NAME__ << "' failed,redisDB disconnect."); \
                break;\
            }\
		}\
		else{\
			break; \
		}\
	}\
	return emRest; \
}


#define REDISCOMMAND_BEGIN()    redisReply * reply = NULL;  \
for(int rcidx = 0; rcidx < 3; ++rcidx) \
{   \

#define REDISCOMMAND_END()  if(!reply)   \
    {   \
        MLOG_ERROR(__FUNCTION__ << " failed, [" << rcidx << "] reply null errno:" << m_pRedisDBContex->err << " error:" << m_pRedisDBContex->errstr << " key:" << key);    \
        redisFree(m_pRedisDBContex);    \
        m_pRedisDBContex = NULL;    \
        if(!TryConnectDB()) \
        {   \
            return ret; \
        }   \
        continue;   \
    }   \
    break;  \
}



static std::string get_safe_string(const char *p, int len) {
    return p ? std::string(p, p+len) : "";
}

CRedisDBInterface::CRedisDBInterface()
    : m_pRedisDBContex(NULL),m_nDbIndex(0),m_isSSL(false)
{

}

CRedisDBInterface::CRedisDBInterface(const std::string& cfgFile)
    : m_pRedisDBContex(NULL),m_nDbIndex(0),m_isSSL(false)
{
    ConnectDB(cfgFile);
}

CRedisDBInterface::~CRedisDBInterface()
{
	if (NULL != m_pRedisDBContex)
	{
		redisFree(m_pRedisDBContex);
		m_pRedisDBContex = NULL;
	}
}

bool CRedisDBInterface::ConnectDB(const std::string &strIp, INT32 nPort, const string& passwd)
{
    m_strPwd = passwd;
    CScopeMutexLocker cScopeLock(m_cMutex);
    return ConnectDBPrivate(strIp,nPort);
}

/**
 <otim>
 <redis>
 dbindex=0
 dbpasswd=
 host=127.0.01
 </redis>
 </otim>
 
 */
bool CRedisDBInterface::ConnectDB(const std::string& cfgFile)
{
    tars::TC_Config cTcCfg;
    cTcCfg.parseFile(cfgFile);
    m_nDbIndex = TC_Common::strto<int>(cTcCfg.get("/otim/redis<dbindex>"));
    m_strPwd = cTcCfg.get("/otim/redis<dbpasswd>");
    std::string host = cTcCfg.get("/otim/redis<host>");
    if (host.empty()){
        MLOG_ERROR("redis host is empty!!!");
        return false;
    }
    m_redisServerList.push_back(host);
    
    int isSSL = TC_Common::strto<int>(cTcCfg.get("/otim/redis<ssl>"));
    m_isSSL = isSSL == 0?false:true;

    CScopeMutexLocker cScopeLock(m_cMutex);
    return  ConnectDBPrivate();
}

bool CRedisDBInterface::TryConnectDB()
{
    if(!m_pRedisDBContex)
    {
        MLOG_DEBUG("m_strIp:"<<m_strIp<<" nPort:"<<m_nPort);
     
        ConnectDBPrivate(m_strIp, m_nPort);

        if(!m_pRedisDBContex)
        {
            MLOG_ERROR(__FUNCTION__ << " failed " << m_strIp << " " << m_nPort);
        }
    }

    return !!m_pRedisDBContex;
}

bool CRedisDBInterface::SelectDB(int index)
{
    bool ret = false;

    if(index >= 0 && m_pRedisDBContex)
    {
        redisReply * reply = (redisReply*)redisCommand(m_pRedisDBContex, "SELECT %d", index);

        if(reply)
        {
            if(reply->type == REDIS_REPLY_STATUS && !strncmp(reply->str, "OK", 2))
            {
                ret = true;
            }
            else
            {
                MLOG_DEBUG("SELECT DB failed, index:" << index);
            }

            freeReplyObject(reply);
        }
        else
        {
            MLOG_ERROR(__FUNCTION__ << " failed, reply null errno:" << m_pRedisDBContex->err << " error:" << m_pRedisDBContex->errstr << " dbindex:" << index);
            redisFree(m_pRedisDBContex);
            m_pRedisDBContex = NULL;
        }
    }

	return ret;
}

bool CRedisDBInterface::ReConnectDB()
{
    ScopeLogger scopelogger("CRedisDBInterface::ReConnectDB");
    CScopeMutexLocker cScopeLock(m_cMutex);
    MLOG_DEBUG("m_strIp:"<<m_strIp<<" nPort:"<<m_nPort);

    return ConnectDBPrivate(m_strIp, m_nPort);
}

bool CRedisDBInterface::getRedisServer(std::string &address)
{
    if(m_redisServerList.empty()) {
        MLOG_ERROR("m_redisServerList is empty");
        return false;
    }
    std::random_shuffle(m_redisServerList.begin(), m_redisServerList.end());
    address.assign(m_redisServerList.back().c_str());
    return true;
}

EMRStatus CRedisDBInterface::DelKey(const std::string &strKey)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "DEL" << strKey);
    DO_REDIS_PUB_FUN(DelKeyPrivate(strKey, strFunName), strFunName);
}


EMRStatus CRedisDBInterface::IncrSeq(const std::string &strName, uint64_t &unItem)
{
    std::string strFunName = "INCR " + strName;
    DO_REDIS_PUB_FUN(IncrSeqPrivate(strName, unItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::Keys(const std::string & pattern, std::list<std::string> &keyItems)
{
    std::string strFunName = "KEYS " + pattern;
    DO_REDIS_PUB_FUN(KeysPrivate(pattern, keyItems, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::QueRPop(const std::string &strName, std::string &strItem)
{
    std::string strFunName = "RPOP " + strName;
    DO_REDIS_PUB_FUN(QueRPopPrivate(strName, strItem, strFunName), strFunName);
}


EMRStatus CRedisDBInterface::QueLPop(const std::string &strName, std::string &strItem)
{
    std::string strFunName = "LPOP " +  strName;
    DO_REDIS_PUB_FUN(QueLPopPrivate( strName, strItem, strFunName), strFunName);
}


EMRStatus CRedisDBInterface::QueLPush(const std::string &strName, const std::string &strItem)
{
    std::string strFunName = "LPUSH " +  strName;
    DO_REDIS_PUB_FUN(QueLPushPrivate(strName, strItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::QueRPush(const std::string &strName, const std::string &strItem)
{
    std::string strFunName = "RPUSH " + strName;
    DO_REDIS_PUB_FUN(QueRPushPrivate( strName, strItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::QueSize(const std::string &strName, INT64& nRet)
{
    std::string strFunName = "LLEN " +  strName;
    DO_REDIS_PUB_FUN(QueSizePrivate(strName, nRet, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::IsStringKeyExits(const std::string &strKey, bool &bIsExit)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "EXISTS " << strKey);
    DO_REDIS_PUB_FUN(IsKeyExits(strKey, bIsExit, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::Set(const std::string &strKey, const std::string &strVal)
{
    std::string strFunName = "SET " + strKey;
    DO_REDIS_PUB_FUN(SetPrivate(strKey, strVal, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::Get(const std::string &strKey, std::string &strVal)
{
    std::string strFunName = "GET " + strKey;
    DO_REDIS_PUB_FUN(GetPrivate(strKey, strVal, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::GetSetItemCount(const std::string &strName, INT64& nRet)
{
    std::string strFunName = "SCARD " + strName;
    DO_REDIS_PUB_FUN(GetSetItemCountPrivate(strName, nRet, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::GetSetItems(const std::string &strName, std::set<std::string> &rgItem)
{
    std::string strFunName = "SMEMBERS " + strName;
    DO_REDIS_PUB_FUN(GetSetItemsPrivate(strName, rgItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::IsItemInSet(const std::string &strName, const std::string &strItem, bool &bIsExit)
{
    std::string strFunName = "SISMEMBER " + strName + " " + strItem;
    DO_REDIS_PUB_FUN(IsItemInSetPrivate( strName, strItem, bIsExit, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::SetAdd(const std::string &strKey, const std::string &strItem)
{
    std::string strFunName = "SADD "  + strKey;
    DO_REDIS_PUB_FUN(SetAddPrivate(strKey, strItem, strFunName), strFunName);
}


EMRStatus CRedisDBInterface::ZSetAdd(const std::string &strKey, INT64 score, const  std::string &strItem)
{
    std::string strFunName = "ZADD " + strKey;
    DO_REDIS_PUB_FUN(ZSetAddPrivate(strKey, score, strItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::SetRemove(const std::string &strKey, const std::string &strItem)
{
    std::string strFunName = "SREM " + strKey;
    DO_REDIS_PUB_FUN(SetRemovePrivate(strKey, strItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ZSetRemove(const std::string &strKey, const std::string &strItem)
{
    std::string strFunName = "ZREM " + strKey;
    DO_REDIS_PUB_FUN(ZSetRemovePrivate(strKey, strItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ZScore(const std::string &strKey, const  std::string &strItem, INT64& nRet)
{
    std::string strFunName = "ZSCORE " + strKey;
    DO_REDIS_PUB_FUN(ZScorePrivate(strKey, strItem, nRet, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ZCount(const std::string &strKey, INT64 from, const char *to, INT64& nRet)
{
    std::string strFunName = "ZCOUNT" + strKey;
    DO_REDIS_PUB_FUN(ZCountPrivate(strKey, from, to, nRet, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ZCard(const std::string &strKey, INT32& nLen)
{
    std::string strFunName = "ZCARD " + strKey;
    DO_REDIS_PUB_FUN(ZCardPrivate(strKey, nLen, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ZRangeWithScore(const std::string &strKey, std::vector<std::string> &topicItem, INT64 from, INT64 to, std::vector<std::string> &scoreItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZRANGE " << strKey << " " << from << " " << to);
    DO_REDIS_PUB_FUN(ZRangeWithScorePrivate(strKey, topicItem, from, to, scoreItem, strFunName, false), strFunName);
}


EMRStatus CRedisDBInterface::ZRangeByScore(const std::string &strKey, INT64 from, const char *to, std::vector<std::string> &memberItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZRANGEBYSCORE " << strKey << " " << from << " " << to);
    DO_REDIS_PUB_FUN(ZRangeByScorePrivate(strKey, from, to, memberItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ZRangeByScoreWithScores(const std::string &strKey, INT64 from, const char *to, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZRANGEBYSCORE " << strKey << " " << from << " " << to);
    DO_REDIS_PUB_FUN(ZRangeByScoreWithScoresPrivate(strKey, from, to, memberItem, scoreItem, strFunName, false), strFunName);
}


EMRStatus CRedisDBInterface::ZRangeByScoreAndLimit(const std::string &strKey, INT64 nStart, INT64 nLimit, std::vector<std::string> &memberItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZRANGEBYSCOREANDLIMITWITHSCORE " << strKey << " " << nStart << " " << nLimit);
    DO_REDIS_PUB_FUN(ZRangeByScoreAndLimitPrivate(strKey, TC_Common::tostr(nStart), MAX_SCORE, nLimit, memberItem, strFunName, false), strFunName);
}

EMRStatus CRedisDBInterface::ZRangeByScoreAndLimitWithScore(const std::string &strKey, INT64 nStart, INT64 nEnd, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZRANGEBYSCOREANDLIMITWITHSCORE " << strKey << " " << nStart << " " << nEnd << " " << nLimit);
    DO_REDIS_PUB_FUN(ZRangeByScoreAndLimitWithScorePrivate(strKey, TC_Common::tostr(nStart), TC_Common::tostr(nEnd), nLimit, memberItem, scoreItem, strFunName, false), strFunName);
}

EMRStatus CRedisDBInterface::ZRangeByScoreAndLimitWithScore(const std::string &strKey, INT64 nStart, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZRANGEBYSCOREANDLIMITWITHSCORE " << strKey << " " << nStart << " " << MAX_SCORE << " " << nLimit);
    DO_REDIS_PUB_FUN(ZRangeByScoreAndLimitWithScorePrivate(strKey, TC_Common::tostr(nStart), MAX_SCORE, nLimit, memberItem, scoreItem, strFunName, false), strFunName);
}

EMRStatus CRedisDBInterface::ZReverRangeByScoreAndLimitWithScores(const std::string &strKey, INT64 nStart, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZREVERRANGEBYSCOREANDLIMITWITHSCORES " << strKey << " " << nStart << " " << nLimit);
    DO_REDIS_PUB_FUN(ZReverRangeByScoreAndLimitWithScoresPrivate(strKey, nStart, nLimit, memberItem, scoreItem, strFunName, false), strFunName);
}

EMRStatus CRedisDBInterface::ZReverRangeByScoreAndLimitWithScores(const std::string &strKey, const char * strStart, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZREVERRANGEBYSCOREANDLIMITWITHSCORES " << strKey << " " << strStart << " " << nLimit);
    DO_REDIS_PUB_FUN(ZReverRangeByScoreAndStartEndLimitWithScorePrivate(strKey, strStart, MIN_SCORE, nLimit, memberItem, scoreItem, strFunName, false), strFunName);
}


EMRStatus CRedisDBInterface::ZReverRangeByScoreAndLimit(const std::string &strKey, INT64 nStart, INT64 nLimit, std::vector<std::string> &memberItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZREVERRANGEBYSCOREANDLIMIT" << strKey << " " << nStart << " " << nLimit);
    DO_REDIS_PUB_FUN(ZReverRangeByScoreAndLimitPrivate(strKey, TC_Common::tostr(nStart), MIN_SCORE, nLimit, memberItem, strFunName, false), strFunName);
}


EMRStatus CRedisDBInterface::ZSetGetPeerMsgs(const std::string &strKey, std::vector<std::string> &msgs, INT32& nTotalSize)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZRANGE " << strKey << " 0 -1 WITHSCORES");
    DO_REDIS_PUB_FUN(ZSetGetPeerMsgsPrivate(strKey, msgs, nTotalSize, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ZSetRemoveByScore(const std::string &strKey, INT64 scorefrom, INT64 scoreto)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZREMRANGEBYSCORE " << strKey << " " << scorefrom << " " << scorefrom);
    DO_REDIS_PUB_FUN(ZSetRemoveByScorePrivate(strKey, scorefrom, scoreto, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ZSetRemoveByScore(const std::string &strKey, INT64 scorefrom, const std::string &scoreto)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZREMRANGEBYSCORE " << strKey << " " << scorefrom << " " << scorefrom);
    DO_REDIS_PUB_FUN(ZSetRemoveByScorePrivate(strKey, scorefrom, scoreto, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ZSetIncrby(const std::string &strKey, const std::string &strScore, const std::string &strMember, INT64& nRet)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "ZINCRBY " << strKey << " " << strScore << " " << strMember);
    DO_REDIS_PUB_FUN(ZSetIncrbyPrivate(strKey, strScore, strMember, nRet, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::DelHashKey(const std::string &strKey)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "DEL" << strKey);
    DO_REDIS_PUB_FUN(DelKeyPrivate(strKey, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::SetHashItem(const std::string &strKey, const std::string &strField, const std::string &strItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "HSET " << strKey << " " << strField);
    DO_REDIS_PUB_FUN(SetHashItemPrivate( strKey, strField, strItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::MSetHashItem(const std::string &key, const std::map<std::string,std::string> &mapItems)
{
    std::string strFunName;
    FORMAT_STREAM(strFunName,"HMSET"<< key<< "" );
    DO_REDIS_PUB_FUN(MSetHashItemPrivate(key, mapItems,strFunName),strFunName);
}

//EMRStatus CRedisDBInterface::MSetHashItem(const std::string &strHashTableName,const std::string &strHashItemName, const std::vector<pair<std::string,std::string> >&hashItemsPair)
//{	
//    std::string strFunName;
//    FORMAT_STREAM(strFunName,"HMSET"<< strHashTableName <<strHashItemName << "" );
//    DO_REDIS_PUB_FUN(MSetHashItemPrivate( strHashTableName+strHashItemName,hashItemsPair,strFunName),strFunName);
//}

EMRStatus CRedisDBInterface::MGetHashItem(const std::string &strKey, const std::vector<std::string> &hashItemFields,std::vector<std::string> &hashItemValues)
{
    std::string strFunName;
    FORMAT_STREAM(strFunName,"HMGET"<< strKey << "" );
    DO_REDIS_PUB_FUN(MGetHashItemPrivate( strKey,hashItemFields,hashItemValues,strFunName),strFunName);
}

EMRStatus CRedisDBInterface::HIncrby(const std::string &strKey, const std::string &strField, INT64 &unItem, INT64 val)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "HINCRBY "  << strKey << " " << strField);
    DO_REDIS_PUB_FUN(HIncrbyPrivate(strKey , strField, unItem, val, strFunName), strFunName);
}

//EMRStatus CRedisDBInterface::HScan(const std::string &strHashTableName, const std::string &strHashItemMatch, int page_size, int& page_nums, std::list<std::string>& hashItems, std::list<std::string>& hashItemsValue)
//{
//    std::string strFunName; FORMAT_STREAM(strFunName, "HSCAN " << strHashTableName << " " << page_nums << " MATCH " << strHashItemMatch << " COUNT " << page_size);
//    DO_REDIS_PUB_FUN(HScanPrivate(strHashTableName, strHashItemMatch, page_size, page_nums, hashItems, hashItemsValue, strFunName), strFunName);
//}

EMRStatus CRedisDBInterface::Scan(const std::string &match, int count, uint64_t cursor, uint64_t &newCursor, std::vector<std::string>& keys) {
    std::string strFunName; 
    FORMAT_STREAM(strFunName, "SCAN " << " " << cursor << " MATCH " << match << " COUNT " << count);
    DO_REDIS_PUB_FUN(ScanPrivate(match, count, cursor, newCursor, keys, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ScanPrivate(const std::string &match, int count, uint64_t cursor, uint64_t &newCursor, std::vector<std::string>& keys, const std::string &strFunName)
{
    redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "SCAN %u MATCH %s COUNT %lld", cursor, match.c_str(), count);
    EMRStatus emRet = CheckReply(strFunName, pReply);
    if (EMRStatus::EM_KVDB_SUCCESS == emRet)
    {
        unsigned int nIndex = 0;
        newCursor = atoi(pReply->element[0]->str);
        for (nIndex = 0 ; nIndex< pReply->element[1]->elements ;nIndex++)
        {
            std::string str;
            str.append(pReply->element[1]->element[nIndex]->str, pReply->element[1]->element[nIndex]->len);
            keys.push_back(str);
        }
    }

	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::ZSetScan(const std::string &key, int count, uint64_t cursor, uint64_t &newCursor, std::vector<std::string>& values, std::vector<std::string>& scores) {
    std::string strFunName; 
    FORMAT_STREAM(strFunName, "ZSCAN " << key << " " << cursor << " COUNT " << count);
    DO_REDIS_PUB_FUN(ZSetScanPrivate(key, count, cursor, newCursor, values, scores, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::ZSetScanPrivate(const std::string &key, int count, uint64_t cursor, uint64_t &newCursor, std::vector<std::string>& values, std::vector<std::string>& scores, const std::string &strFunName) {
    redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZSCAN %s %u COUNT %lld", key.c_str(), cursor, count);
    EMRStatus emRet = CheckReply(strFunName, pReply);
    if (EMRStatus::EM_KVDB_SUCCESS == emRet)
    {
        unsigned int nIndex = 0;
        newCursor = atoi(pReply->element[0]->str);
        for (nIndex = 0 ; nIndex< pReply->element[1]->elements ;nIndex++)
        {
            std::string str1;
            str1.append(pReply->element[1]->element[nIndex]->str, pReply->element[1]->element[nIndex]->len);
            values.push_back(str1);

            nIndex++;
            std::string str2;
            str2.append(pReply->element[1]->element[nIndex]->str, pReply->element[1]->element[nIndex]->len);
            scores.push_back(str2);
        }
    }

	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::HLen(const std::string & key, INT64 & count)
{
    EMRStatus ret = EMRStatus::EM_KVDB_ERROR;

    if(key.size())
    {
        CScopeMutexLocker cScopeLock(m_cMutex);

        if(!TryConnectDB())
        {
            return ret;
        }

        REDISCOMMAND_BEGIN();
        reply = (redisReply*)redisCommand(m_pRedisDBContex, "HLEN %s", key.c_str());
        REDISCOMMAND_END();

        if(reply)
        {
            if(reply->type == REDIS_REPLY_INTEGER)
            {
                count = reply->integer;
                ret = EMRStatus::EM_KVDB_SUCCESS;
            }
            else
            {
                MLOG_DEBUG("HLEN failed, key:" << key << " reply.type:" << reply->type << " reply.integer:" << reply->integer);
            }

            freeReplyObject(reply);
        }
    }

	return ret;
}
    
EMRStatus CRedisDBInterface::HSet(const std::string & key, const std::string & field, const std::string & value)
{
    EMRStatus ret = EMRStatus::EM_KVDB_ERROR;

    if(key.size() && field.size() && value.size())
    {
        CScopeMutexLocker cScopeLock(m_cMutex);

        if(!TryConnectDB())
        {
            return ret;
        }

        REDISCOMMAND_BEGIN();
        reply = (redisReply*)redisCommand(m_pRedisDBContex, "HSET %s %s %b", key.c_str(), field.c_str(), value.c_str(), value.size());
        if(!reply)
        {
            MLOG_ERROR(__FUNCTION__ << " failed, field:" << field);
        }
        REDISCOMMAND_END();

        if(reply)
        {
            if(reply->type == REDIS_REPLY_INTEGER)
            {
                ret = EMRStatus::EM_KVDB_SUCCESS;
            }
            else
            {
                MLOG_DEBUG("HSET failed, key:" << key << " reply.type:" << reply->type << " reply.integer:" << reply->integer << " field:" << field);
            }

            freeReplyObject(reply);
        }
    }

	return ret;
}
    
EMRStatus CRedisDBInterface::HGet(const std::string & key, const std::string & field, std::string & value)
{
    EMRStatus ret = EMRStatus::EM_KVDB_ERROR;

    if(key.size() && field.size())
    {
        CScopeMutexLocker cScopeLock(m_cMutex);

        if(!TryConnectDB())
        {
            return ret;
        }

        REDISCOMMAND_BEGIN();
        reply = (redisReply*)redisCommand(m_pRedisDBContex, "HGET %s %s", key.c_str(), field.c_str());
        if(!reply)
        {
            MLOG_ERROR(__FUNCTION__ << " failed, field:" << field);
        }
        REDISCOMMAND_END();

        if(reply)
        {
            if(reply->type == REDIS_REPLY_STRING)
            {
                value.assign(reply->str, reply->len);

                ret = EMRStatus::EM_KVDB_SUCCESS;
            }
            else
            {
                MLOG_DEBUG("HGET failed, key:" << key << " reply.type:" << reply->type << " reply.str:" << reply->str << " field:" << field);
            }

            freeReplyObject(reply);
        }
    }

	return ret;
}
    
EMRStatus CRedisDBInterface::HDel(const std::string & key, const std::string & field)
{
    EMRStatus ret = EMRStatus::EM_KVDB_ERROR;

    if(key.size() && field.size())
    {
        CScopeMutexLocker cScopeLock(m_cMutex);

        if(!TryConnectDB())
        {
            return ret;
        }

        REDISCOMMAND_BEGIN();
        reply = (redisReply*)redisCommand(m_pRedisDBContex, "HDEL %s %s", key.c_str(), field.c_str());
        if(!reply)
        {
            MLOG_ERROR(__FUNCTION__ << " failed, field:" << field);
        }
        REDISCOMMAND_END();

        if(reply)
        {
            if(reply->type == REDIS_REPLY_INTEGER)
            {
                ret = EMRStatus::EM_KVDB_SUCCESS;
            }
            else
            {
                MLOG_DEBUG("HDEL failed, key:" << key << " reply.type:" << reply->type << " reply.integer:" << reply->integer << " field:" << field);
            }

            freeReplyObject(reply);
        }
    }

	return ret;
}
    
EMRStatus CRedisDBInterface::HGetAll(const std::string & key, vectsspair & result)
{
    EMRStatus ret = EMRStatus::EM_KVDB_ERROR;

    if(key.size())
    {
        CScopeMutexLocker cScopeLock(m_cMutex);

        if(!TryConnectDB())
        {
            return ret;
        }

        REDISCOMMAND_BEGIN();
        reply = (redisReply*)redisCommand(m_pRedisDBContex, "HGETALL %s", key.c_str());
        REDISCOMMAND_END();

        if(reply)
        {
            result.resize(reply->elements);

            for(size_t i = 0; i < reply->elements; ++i)
            {
                sspair & p = result[i];

                p.first.assign(reply->element[i]->str, reply->element[i]->len);

                ++i;

                p.second.assign(reply->element[i]->str, reply->element[i]->len);
            }

            freeReplyObject(reply);

            ret = EMRStatus::EM_KVDB_SUCCESS;
        }
    }

	return ret;
}
    
EMRStatus CRedisDBInterface::HKeys(const std::string & key, std::vector<std::string> & result)
{
    EMRStatus ret = EMRStatus::EM_KVDB_ERROR;

    if(key.size())
    {
        CScopeMutexLocker cScopeLock(m_cMutex);

        if(!TryConnectDB())
        {
            return ret;
        }

        REDISCOMMAND_BEGIN();
        reply = (redisReply*)redisCommand(m_pRedisDBContex, "HKEYS %s", key.c_str());
        REDISCOMMAND_END();

        if(reply)
        {
            result.resize(reply->elements);

            for(size_t i = 0; i < reply->elements; ++i)
            {
                result[i].assign(reply->element[i]->str, reply->element[i]->len);
            }

            freeReplyObject(reply);

            ret = EMRStatus::EM_KVDB_SUCCESS;
        }
    }

	return ret;
}
    
EMRStatus CRedisDBInterface::HVals(const std::string & key, std::vector<std::string> & result)
{
    EMRStatus ret = EMRStatus::EM_KVDB_ERROR;

    if(key.size())
    {
        CScopeMutexLocker cScopeLock(m_cMutex);

        if(!TryConnectDB())
        {
            return ret;
        }

        REDISCOMMAND_BEGIN();
        reply = (redisReply*)redisCommand(m_pRedisDBContex, "HVALS %s", key.c_str());
        REDISCOMMAND_END();

        if(reply)
        {
            result.resize(reply->elements);

            for(size_t i = 0; i < reply->elements; ++i)
            {
                result[i].assign(reply->element[i]->str, reply->element[i]->len);
            }

            freeReplyObject(reply);

            ret = EMRStatus::EM_KVDB_SUCCESS;
        }
    }

    return ret;
}

EMRStatus CRedisDBInterface::DelHashField(const std::string &strKey, const std::string &strField)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "HDEL "  << strKey << " " << strField);
    DO_REDIS_PUB_FUN(DelHashFieldPrivate(strKey, strField, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::GetHashItem(const std::string &strKey, const std::string &strField, std::string &strItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "HGET  " << strKey << " " << strField);
    DO_REDIS_PUB_FUN(GetHashItemPrivate(strKey , strField, strItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::GetAllHashItem(const std::string &strKey, std::map<std::string, std::string> &mapFileValue)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "HGETALL  " << strKey );
    DO_REDIS_PUB_FUN(GetAllHashItemPrivate(strKey, mapFileValue, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::GetSeqID(const std::string &strKey, INT64& nRet)
{
    std::string strFunName = "INCR " + strKey;
    DO_REDIS_PUB_FUN(GetSeqIDPrivate(strKey, nRet, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::QueIndexElement(const std::string &strName, INT32 nIndex, std::string &strItem)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "LINDEX " << strName << " " << nIndex);
    DO_REDIS_PUB_FUN(QueIndexElementPrivate(strName, nIndex, strItem, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::Expire(const std::string &strName, uint64_t nNum)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "EXPIRE " << strName << " " << nNum);
    DO_REDIS_PUB_FUN(ExpirePrivate(strName, nNum, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::IncrSeqBy(const std::string &strName, uint64_t &unItem, uint64_t nNum)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "INCRBY " << strName << nNum);
    DO_REDIS_PUB_FUN(IncrSeqByPrivate(strName, unItem, nNum, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::SetEX(const std::string &strKey, const std::string &strVal, uint64_t nNum)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "SETEX " << strKey << " " << strVal << " " << nNum);
    DO_REDIS_PUB_FUN(SetEXPrivate(strKey, strVal, nNum, strFunName), strFunName);
}


EMRStatus CRedisDBInterface::SetEXNX(const std::string &strKey, const std::string &strVal, int nExpireSeconds)
{
    std::string strFunName; FORMAT_STREAM(strFunName, "SET " << strKey << " " << strVal << " EX " << nExpireSeconds << " NX ");
    DO_REDIS_PUB_FUN(SetEXNXPrivate(strKey, strVal, nExpireSeconds, strFunName), strFunName);
}

EMRStatus CRedisDBInterface::IncrSeqPrivate(const std::string &strName, uint64_t &unItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "INCR %s", strName.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		unItem = pReply->integer;
	}
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::KeysPrivate(const std::string & pattern, std::list<std::string> &keyItems, const std::string &strFunName)
{
 	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "KEYS %s", pattern.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		keyItems.clear();
		unsigned int nIndex = 0;
		for(nIndex = 0 ; nIndex< pReply->elements;nIndex++)
		{
			std::string strKey;
			strKey.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
			keyItems.push_front(strKey);
		}
	}
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::QueRPopPrivate(const std::string &strName, std::string &strItem, const std::string &strFunName)
{

	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "RPOP %s", strName.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		strItem.append(pReply->str, pReply->len);
	} 

	freeReplyObject(pReply);
	return emRet;	
}


EMRStatus CRedisDBInterface::QueLPopPrivate(const std::string &strName, std::string &strItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "LPOP %s", strName.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		strItem.append(pReply->str, pReply->len);
	}

	freeReplyObject(pReply);
	return emRet;
}


EMRStatus CRedisDBInterface::QueLPushPrivate(const std::string &strName, const std::string &strItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "LPUSH  %s %b", strName.c_str(), strItem.c_str(), strItem.size());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::QueRPushPrivate(const std::string &strName, const std::string &strItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "RPUSH %s %b", strName.c_str(), strItem.c_str(), strItem.size());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::QueSizePrivate(const std::string &strName, INT64& nRet, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "LLEN %s", strName.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{	
		nRet = pReply->integer;
	}
	freeReplyObject(pReply);
	return emRet;
}


EMRStatus CRedisDBInterface::GetSeqIDPrivate(const std::string &strKey, INT64& nRet, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "INCR %s", strKey.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		nRet = pReply->integer;
	}
	freeReplyObject(pReply);

	return emRet;
}

EMRStatus CRedisDBInterface::SetPrivate(const std::string &strKey, const std::string &strVal, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "SET %s %b", strKey.c_str(), strVal.c_str(), strVal.size());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::GetPrivate(const std::string &strKey, std::string &strVal, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "GET %s", strKey.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		strVal.append(pReply->str, pReply->len);
	}
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::GetSetItemCountPrivate(const std::string &strName, INT64& nRet, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "SCARD %s", strName.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		nRet = pReply->integer;
	} 
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::GetSetItemsPrivate(const std::string &strName, std::set<std::string> &rgItem, const std::string &strFunName)
{
	rgItem.clear();
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "SMEMBERS %s", strName.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		for (unsigned nIndex = 0; nIndex < pReply->elements; nIndex++)
		{
			redisReply *pReplyTmp = *(pReply->element+nIndex);
			std::string strTmp;
			strTmp.append(pReplyTmp->str, pReplyTmp->len);
			rgItem.insert(strTmp);
		}
	}
	freeReplyObject(pReply);
	return emRet;

}

EMRStatus CRedisDBInterface::IsItemInSetPrivate(const std::string &strName, const std::string &strItem, bool &bIsExit, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "SISMEMBER  %s %s", strName.c_str(), strItem.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		if (0 < pReply->integer)
		{
			bIsExit = true;
		}
		else
		{
			bIsExit = false;
		}
	}
	freeReplyObject(pReply);
	return emRet;
}



EMRStatus CRedisDBInterface::SetAddPrivate(const std::string &strKey, const  std::string &strItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "SADD %s %b", strKey.c_str(), strItem.c_str(), strItem.size());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::ZSetAddPrivate(const std::string &strKey, INT64 score, const  std::string &strItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZADD %s %lld %b", strKey.c_str(), score, strItem.c_str(), strItem.size());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::SetRemovePrivate(const std::string &strKey, const std::string &strItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "SREM %s %b", strKey.c_str(), strItem.c_str(), strItem.size());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::ZSetRemovePrivate(const std::string &strKey, const std::string &strItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZREM %s %b", strKey.c_str(), strItem.c_str(), strItem.size());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::ZScorePrivate(const std::string &strKey, const  std::string &strItem, INT64& nRet, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZSCORE %s %s", strKey.c_str(), strItem.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if(EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		nRet = atoll(pReply->str);
	}

	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::ZCountPrivate(const std::string &strKey, INT64 from, const char *to, INT64& nRet, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZCOUNT %s %lld %s", strKey.c_str(), from, to);
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if(EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		nRet = pReply->integer;
	}

	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::ZCardPrivate(const std::string &strKey, INT32& nLen, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZCARD %s", strKey.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		nLen = pReply->integer;
	}
	freeReplyObject(pReply);
	return emRet;
}



/*
* from = 0, to = -1 :从第一个到最后一个
* 
*/
EMRStatus CRedisDBInterface::ZRangeWithScorePrivate(const std::string &strKey, std::vector<std::string> &rgTopicItem, INT64 from, INT64 to, std::vector<std::string> &rgScoreItem, const std::string &strFunName, bool statflag)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZRANGE %s %lld %lld WITHSCORES", strKey.c_str(), from, to);
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		rgTopicItem.clear();
		rgScoreItem.clear();
		unsigned int nIndex = 0;
		for(nIndex = 0 ; nIndex< pReply->elements;nIndex++)
		{
			std::string strele;
            strele.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
			rgTopicItem.push_back(strele);
			nIndex++;
			rgScoreItem.push_back(get_safe_string(pReply->element[nIndex]->str, pReply->element[nIndex]->len));
		}
	}
	freeReplyObject(pReply);

	return emRet;
}

/*
ZRANGEBYSCORE 
* from: from score
* to  : to score
*/
EMRStatus CRedisDBInterface::ZRangeByScorePrivate(const std::string &strKey, INT64 from, const char *to, std::vector<std::string> &rgMemberItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZRANGEBYSCORE %s %lld %s", strKey.c_str(), from, to);
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		rgMemberItem.clear();
		unsigned int nIndex = 0;
		for(nIndex = 0 ; nIndex< pReply->elements;nIndex++)
		{
			std::string strele;
			strele.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
			rgMemberItem.push_back(strele);
		}
	}

	freeReplyObject(pReply);
	return emRet;
}


/*
ZRANGEBYSCOREWITHSCORES 
* from: from score
* to  : to score
*/
EMRStatus CRedisDBInterface::ZRangeByScoreWithScoresPrivate(const std::string &strKey, INT64 from, const char *to, std::vector<std::string> &rgMemberItem, std::vector<std::string> &rgScoreItem, const std::string &strFunName, bool statflag)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZRANGEBYSCORE %s %lld %s WITHSCORES", strKey.c_str(), from, to);
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		rgMemberItem.clear();
        rgScoreItem.clear();
		unsigned int nIndex = 0;
		for(nIndex = 0 ; nIndex< pReply->elements;nIndex++)
		{
			std::string strele;
        
            strele.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
			rgMemberItem.push_back(strele);
            nIndex++;
            rgScoreItem.push_back(get_safe_string(pReply->element[nIndex]->str, pReply->element[nIndex]->len));
		}
	}

	freeReplyObject(pReply);
	return emRet;
}


EMRStatus CRedisDBInterface::ZRangeByScoreAndLimitPrivate(const std::string &strKey, const std::string &strStart, const std::string &strEnd, INT64 nLimit, std::vector<std::string> &memberItem, const std::string &strFunName, bool statflag)
{
    memberItem.clear();
    std::string strCmd;
    FORMAT_STREAM(strCmd, "ZRANGEBYSCORE " << strKey << " " << strStart << " " << strEnd << " limit 0 " << nLimit);
    redisReply* pReply = (redisReply*)redisCommand(m_pRedisDBContex, strCmd.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
    if (EMRStatus::EM_KVDB_SUCCESS == emRet) {
        unsigned int nIndex = 0;
        for (nIndex = 0; nIndex < pReply->elements; nIndex++)
        {
            std::string strele;
            strele.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
            memberItem.push_back(strele);
        }
    }

    freeReplyObject(pReply);
    return emRet;
}

EMRStatus CRedisDBInterface::ZRangeByScoreAndLimitWithScorePrivate(const std::string &strKey, const std::string &strStart, const std::string &strEnd, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem, const std::string &strFunName, bool statflag)
{
    memberItem.clear();
    scoreItem.clear();
    std::string strCmd;
    FORMAT_STREAM(strCmd, "ZRANGEBYSCORE " << strKey << " " << strStart << " " << strEnd << " limit 0 " << nLimit << " WITHSCORES");
    redisReply* pReply = (redisReply*)redisCommand(m_pRedisDBContex, strCmd.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
    if (EMRStatus::EM_KVDB_SUCCESS == emRet) {
        unsigned int nIndex = 0;
        for (nIndex = 0; nIndex < pReply->elements; nIndex++)
        {
            std::string strele;
            strele.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
            memberItem.push_back(strele);
            nIndex++;
            scoreItem.push_back(get_safe_string(pReply->element[nIndex]->str, pReply->element[nIndex]->len));
        }
    }

    freeReplyObject(pReply);
    return emRet;
}


EMRStatus CRedisDBInterface::ZReverRangeByScoreAndLimitWithScoresPrivate(const std::string &strKey, INT64 nStart, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem, const std::string &strFunName, bool statflag) {
    memberItem.clear();
    scoreItem.clear();
    redisReply* pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZREVRANGEBYSCORE %s %lld -inf limit 0 %lld WITHSCORES", strKey.c_str(), nStart, nLimit);
    EMRStatus emRet = CheckReply(strFunName, pReply);
    if (EMRStatus::EM_KVDB_SUCCESS == emRet) {
        unsigned int nIndex = 0;
        for(nIndex = 0 ; nIndex< pReply->elements;nIndex++)
        {
            std::string strele;
            strele.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
            memberItem.push_back(strele);
            nIndex++;
            scoreItem.push_back(get_safe_string(pReply->element[nIndex]->str, pReply->element[nIndex]->len));
        }
    }

    freeReplyObject(pReply);
    return emRet;
}

EMRStatus CRedisDBInterface::ZReverRangeByScoreAndStartEndLimitWithScorePrivate(const std::string &strKey, const std::string &strStart, const std::string &strEnd, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem, const std::string &strFunName, bool statflag) {
    memberItem.clear();
    scoreItem.clear();
    std::string strCmd;
    FORMAT_STREAM(strCmd, "ZREVRANGEBYSCORE " << strKey << " " << strStart << " " << strEnd << " limit 0 " << nLimit << " WITHSCORES");
    redisReply* pReply = (redisReply*)redisCommand(m_pRedisDBContex, strCmd.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
    if (EMRStatus::EM_KVDB_SUCCESS == emRet) {
        unsigned int nIndex = 0;
        for(nIndex = 0 ; nIndex< pReply->elements;nIndex++)
        {
            std::string strele;
            strele.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
            memberItem.push_back(strele);
            nIndex++;
            scoreItem.push_back(get_safe_string(pReply->element[nIndex]->str, pReply->element[nIndex]->len));
        }
    }

    freeReplyObject(pReply);
    return emRet;
}

EMRStatus CRedisDBInterface::ZReverRangeByScoreAndLimitPrivate(const std::string &strKey, const std::string &strStart, const std::string &strEnd, INT64 nLimit, std::vector<std::string> &memberItem, const std::string &strFunName, bool statflag) {
    memberItem.clear();
    std::string strCmd;
    FORMAT_STREAM(strCmd, "ZREVRANGEBYSCORE " << strKey << " " << strStart << " " << strEnd << " limit 0 " << nLimit);
    redisReply* pReply = (redisReply*)redisCommand(m_pRedisDBContex, strCmd.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
    if (EMRStatus::EM_KVDB_SUCCESS == emRet) {
        unsigned int nIndex = 0;
        for(nIndex = 0 ; nIndex< pReply->elements;nIndex++)
        {
            std::string strele;
            strele.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
            memberItem.push_back(strele);
        }
    }

    freeReplyObject(pReply);
    return emRet;
}

EMRStatus CRedisDBInterface::ZSetGetPeerMsgsPrivate(const std::string &strKey, std::vector<std::string> &msgs, INT32& nTotalSize, const std::string &strFunName)
{
	nTotalSize = 0;
	msgs.clear();
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZRANGE %s 0 -1 WITHSCORES", strKey.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		unsigned int nIndex = 0;
		for(nIndex = 0 ; nIndex< pReply->elements;nIndex++)
		{
			msgs.push_back(get_safe_string(pReply->element[nIndex]->str, pReply->element[nIndex]->len));
			nTotalSize += pReply->element[nIndex]->len;
			nIndex++;
		}
	}

	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::ZSetRemoveByScorePrivate(const std::string &strKey, INT64 scorefrom, INT64 scoreto, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZREMRANGEBYSCORE  %s %lld %lld", strKey.c_str(), scorefrom, scoreto);
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::ZSetRemoveByScorePrivate(const std::string &strKey, INT64 scorefrom, const std::string &scoreto, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZREMRANGEBYSCORE  %s %lld %s", strKey.c_str(), scorefrom, scoreto.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::ZSetIncrbyPrivate(const std::string &strKey, const std::string &strScore, const std::string &strMember, INT64& nRet, const std::string &strFunName)
{
    redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "ZINCRBY %s %s %s", strKey.c_str(), strScore.c_str(), strMember.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
    if (EMRStatus::EM_KVDB_SUCCESS == emRet)
    {
        nRet = atoll(pReply->str);
    }

    freeReplyObject(pReply);
    return emRet;
}

EMRStatus CRedisDBInterface::CheckReply(const std::string & strFunName, redisReply * pReply)
{
	EMRStatus nRet = EMRStatus::EM_KVDB_ERROR;
	if (NULL == pReply)
	{
		//连接被断开，如何进行进一步处理，需要知道
		redisFree(m_pRedisDBContex);
		m_pRedisDBContex = NULL;
		nRet = EMRStatus::EM_KVDB_ERROR;
		return nRet;
	}

	if (REDIS_REPLY_ERROR == pReply->type)
	{
		MLOG_ERROR("redisCommand exec '" << strFunName << "' failed, pReply->type = " << pReply->type << ", pReply->str == " << pReply->str);
		nRet = EMRStatus::EM_KVDB_ERROR;
	}
	else if (REDIS_REPLY_NIL == pReply->type)
	{
//		MLOG_DEBUG("redisCommand exec '" << strFunName << "' failed, pReply->type = " << pReply->type << " :REDIS_REPLY_NIL");
		nRet = EMRStatus::EM_KVDB_RNULL;
	}
	else
	{
		nRet = EMRStatus::EM_KVDB_SUCCESS;
	}

	return nRet;
}

bool CRedisDBInterface::IsConnected()
{
    CScopeMutexLocker cScopeLock(m_cMutex);
	return NULL != m_pRedisDBContex;
}

EMRStatus CRedisDBInterface::SetHashItemPrivate(const std::string &strHashItemName, const std::string &strField, const std::string &strItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "HSET %s %s %b", strHashItemName.c_str(), strField.c_str(), strItem.c_str(), strItem.size());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::Commandargv(const std::vector<string> &vData,const std::string &strFunName)
{
     vector<const char *>argv( vData.size() );
     vector<size_t> argvlen( vData.size() ); 
     unsigned int i = 0;
     for ( vector<string>::const_iterator iter = vData.begin(); iter != vData.end(); ++iter, ++i )
    {
         argv[i] = iter->c_str(), argvlen[i] = iter->size();
    }
    //调用redis命令执行方法实现数组命令的执行
    redisReply *pReply = (redisReply*)redisCommandArgv(m_pRedisDBContex,argv.size(),&(argv[0]),&(argvlen[0]));
    EMRStatus emRet = CheckReply(strFunName,pReply);
    freeReplyObject(pReply);
    return emRet;
}
    

//add by yangliang 2017/02/28
//实现hash 的多个filed的存储
EMRStatus CRedisDBInterface::MSetHashItemPrivate(const std::string &strHashItemName,const std::map<std::string,std::string> &mapItems,const std::string &strFunName)
{	
    if (mapItems.size() == 0)
    {
        MLOG_WARN("redisCommand exec '" << strFunName << "' failed, reason: hashItemsPair.size==0, strHashItemName:" << strHashItemName);
        return EMRStatus::EM_KVDB_ERROR;
    }

    std::vector<std::string> vCmdData;
    vCmdData.clear();
    vCmdData.push_back("HMSET");
    vCmdData.push_back(strHashItemName);
    for (auto item : mapItems)
    {		
        vCmdData.push_back(item.first);
        vCmdData.push_back(item.second);
    }	
    
    return Commandargv(vCmdData,strFunName);

}

EMRStatus CRedisDBInterface::MGetHashItemPrivate(const std::string &strHashItemName,const std::vector<std::string> &hashItemFields,std::vector<std::string> &hashItemValues,const std::string &strFunName)
{
    std::vector<std::string> vCmdData;
    vCmdData.clear();
    vCmdData.push_back("HMGET");
    vCmdData.push_back(strHashItemName);
    std::vector<std::string>::const_iterator iter = hashItemFields.begin();
    for (;iter != hashItemFields.end();iter++)	
    {		
        vCmdData.push_back(*iter);
    }	
    return CommandargvArray(vCmdData,hashItemValues,strFunName);
}
EMRStatus CRedisDBInterface::CommandargvArray(const std::vector<string> &vData,std::vector<string> &vDataValues,const std::string &strFunName)
{
    vector<const char *>argv( vData.size() );
    vector<size_t> argvlen( vData.size() );
    unsigned int i = 0;
    for ( vector<string>::const_iterator iter = vData.begin(); iter != vData.end(); ++iter, ++i )
    {
         argv[i] = iter->c_str(), argvlen[i] = iter->size();
    }
    redisReply *pReply = (redisReply*)redisCommandArgv(m_pRedisDBContex,argv.size(),&(argv[0]),&(argvlen[0]));
    EMRStatus emRet = CheckReply(strFunName,pReply);
    if (EMRStatus::EM_KVDB_SUCCESS == emRet){
        unsigned int nIndex = 0;
        for (nIndex = 0; nIndex < pReply->elements; nIndex++){
            vDataValues.push_back(get_safe_string(pReply->element[nIndex]->str, pReply->element[nIndex]->len));
        }
    }
    freeReplyObject(pReply);
    return emRet;
}
    

EMRStatus CRedisDBInterface::HIncrbyPrivate(const std::string &strHashItemName, const std::string &strField, INT64 &unItem, INT64 val, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "HINCRBY %s %s %lld", strHashItemName.c_str(), strField.c_str(), val);
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet) {
		unItem = pReply->integer;
	}
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::HScanPrivate(const std::string &strHashTableName, const std::string &strHashItemMatch, int page_size, int& page_nums, std::list<std::string>& hashItems, std::list<std::string>& hashItemsValue, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "HSCAN %s %u MATCH %s COUNT %lld", strHashTableName.c_str(), page_nums, strHashItemMatch.c_str(), page_size);
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		unsigned int nIndex = 0;
    page_nums = atoi(pReply->element[0]->str);
    for(nIndex = 0 ; nIndex< pReply->element[1]->elements ;nIndex++)
    {
      std::string strele;
      strele.append(pReply->element[1]->element[nIndex]->str, pReply->element[1]->element[nIndex]->len);
      hashItems.push_front(strele);
      nIndex++;
      hashItemsValue.push_front(get_safe_string(pReply->element[1]->element[nIndex]->str, pReply->element[1]->element[nIndex]->len));
    }
  }

	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::DelHashFieldPrivate(const std::string &strHashItemName, const std::string &strField, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "HDEL %s %s", strHashItemName.c_str(), strField.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}


EMRStatus CRedisDBInterface::GetHashItemPrivate(const std::string &strHashItemName, const std::string &strField, std::string &strItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "HGET %s %s", strHashItemName.c_str(), strField.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		strItem.append(pReply->str, pReply->len);
	}

	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::GetAllHashItemPrivate(const std::string &strHashItemName, std::map<std::string, std::string> &mapFileValue, const std::string &strFunName)
{
    redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "HGETALL %s", strHashItemName.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
    if (EMRStatus::EM_KVDB_SUCCESS == emRet)
    {
        mapFileValue.clear();
        unsigned int nIndex = 0;
        for (nIndex = 0; nIndex < pReply->elements; nIndex++)
        {
            std::string strField;
            strField.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
            nIndex++;
            std::string strValue;
            strValue.append(pReply->element[nIndex]->str, pReply->element[nIndex]->len);
            mapFileValue.insert(std::pair<std::string, std::string>(strField, strValue));
        }
    }
    freeReplyObject(pReply);
    return emRet;
}

EMRStatus CRedisDBInterface::DelKeyPrivate(const std::string &strKeyName, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "DEL %s", strKeyName.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	freeReplyObject(pReply);
	return emRet;
}


EMRStatus CRedisDBInterface::IsKeyExits(const std::string &strKey, bool &bIsExit, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "EXISTS  %s", strKey.c_str());
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		bIsExit = pReply->integer;
	}

	freeReplyObject(pReply);
	return emRet;
}


EMRStatus CRedisDBInterface::QueIndexElementPrivate(const std::string &strName, INT32 nIndex, std::string &strItem, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "LINDEX  %s %d", strName.c_str(), nIndex);
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		strItem.append(pReply->str, pReply->len);
	}
	freeReplyObject(pReply);
	return emRet;
}



EMRStatus CRedisDBInterface::IncrSeqByPrivate(const std::string &strName, uint64_t &unItem, uint64_t nNum, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "INCRBY %s %lld", strName.c_str(), nNum);
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		unItem = pReply->integer;
	}
	freeReplyObject(pReply);
	return emRet;
}



EMRStatus CRedisDBInterface::ExpirePrivate(const std::string &strName, uint64_t nNum, const std::string &strFunName)
{
	redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "EXPIRE %s %lld", strName.c_str(), nNum);
    EMRStatus emRet = CheckReply(strFunName, pReply);
	if (EMRStatus::EM_KVDB_SUCCESS == emRet)
	{
		if(pReply->integer == 1) 
        {
			emRet = EMRStatus::EM_KVDB_SUCCESS;
        }
        else 
        {
			emRet = EMRStatus::EM_KVDB_RNULL;
        }
	}
	freeReplyObject(pReply);
	return emRet;
}

EMRStatus CRedisDBInterface::SetEXPrivate(const std::string &strKey, const std::string &strVal, uint64_t nNum, const std::string &strFunName)
{
    redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "SETEX %s %lld %b", strKey.c_str(), nNum, strVal.c_str(), strVal.size());
    EMRStatus emRet = CheckReply(strFunName, pReply);
    freeReplyObject(pReply);
    return emRet;
}


EMRStatus CRedisDBInterface::SetEXNXPrivate(const std::string &strKey, const std::string &strVal, int nExpireSeconds, const std::string &strFunName)
{
    redisReply *pReply = (redisReply*)redisCommand(m_pRedisDBContex, "SET %s %s EX %d NX", strKey.c_str(), strVal.c_str(), nExpireSeconds);
    EMRStatus emRet = CheckReply(strFunName, pReply);
    freeReplyObject(pReply);
    return emRet;
}

bool CRedisDBInterface::ConnectDBPrivate(const std::string &strIp, INT32 nPort)
{
    if (NULL != m_pRedisDBContex)
    {
        redisFree(m_pRedisDBContex);
        m_pRedisDBContex = NULL;
    }
    
    if (strIp.empty()){
        MLOG_ERROR("SERVICEALARM strIp is empty " << strIp << ", m_strIp:" << m_strIp);
        return false;
    }
   
    MLOG_DEBUG("strIp:"<<strIp<<" nPort:"<<nPort);
    m_strIp = strIp;
    m_nPort = nPort;
    m_pRedisDBContex = redisConnect(strIp.c_str(), nPort);
    if (NULL == m_pRedisDBContex)
    {
        MLOG_ERROR("SERVICEALARM CRedisDBInterface::ConnectDB failed strIp == " << strIp << ", nPort== " << nPort);
        return false;
    }
    else
    {
        if (0 != m_pRedisDBContex->err)
        {
            MLOG_ERROR("CRedisDBInterface::ConnectDB failed strIp == " << strIp << ", nPort== " << nPort << ", errorCode==" << m_pRedisDBContex->err << " error:" << m_pRedisDBContex->errstr);
            redisFree(m_pRedisDBContex);
            m_pRedisDBContex = NULL;
            return false;
        }

        if (m_strPwd.size() > 0)
        {
            MLOG_DEBUG("CRedisDBInterface::ConnectDB send Auth ...");
            string cmd = "AUTH " + m_strPwd;
            redisReply *reply = (redisReply*)redisCommand(m_pRedisDBContex, cmd.c_str());
            if (CheckReply("AUTH", reply) != EMRStatus::EM_KVDB_SUCCESS)
            {
                freeReplyObject(reply);
                return false;
            }
            freeReplyObject(reply);
        }

        MLOG_DEBUG("CRedisDBInterface::ConnectDB success! " << strIp << " " << nPort);

        if (m_nDbIndex > 0 && !SelectDB(m_nDbIndex))
        {
            redisFree(m_pRedisDBContex);
            m_pRedisDBContex = NULL;

            MLOG_ERROR(__FUNCTION__ << " select db failed, DBIDNEX:" << m_nDbIndex);
            return false;
        }

        MLOG_DEBUG("CRedisDBInterface::ConnectDB success strIp == " << strIp << ", nPort== " << nPort);
        return true;
    }
}

bool CRedisDBInterface::ConnectDBPrivate()
{
    std::string avaliable_addr;
    if (getRedisServer(avaliable_addr)) {
        std::vector<std::string> addr = hcommon::string_split(avaliable_addr, ':');
        if (addr.size() < 2) {
            //地址中有无效数据,非IP地址格式
            MLOG_ERROR("SERVICEALARM CRedisDBInterface::ConnectDB codis proxy address wrong:" << avaliable_addr << " size:" << addr.size() << addr[0]);
            return false;
        }
        MLOG_DEBUG("avaliable_addr:"<<avaliable_addr<<" addr[0]:"<<addr[0]<<" addr[1]:"<<addr[1]);
        bool success = false;
        if (m_isSSL){
            success = ConnectDBPrivateWithSSL(addr[0], TC_Common::strto<int>(addr[1]));
        }
        else{
            success = ConnectDBPrivate(addr[0], TC_Common::strto<int>(addr[1]));
        }
        if (!success) {
            //删除连接不上的地址
            MLOG_ERROR("SERVICEALARM CRedisDBInterface::ConnectDB remove codis proxy:" << avaliable_addr);
            m_redisServerList.pop_back();
            return false;
        }
        return true;
    }
    MLOG_ERROR("SERVICEALARM CRedisDBInterface::ConnectDB failed, No avaliable server");
    return false;
}


bool CRedisDBInterface::ConnectDBPrivateWithSSL(const std::string &strIp, INT32 nPort)
{
    if (NULL != m_pRedisDBContex)
    {
        redisFree(m_pRedisDBContex);
        m_pRedisDBContex = NULL;
    }
    MLOG_DEBUG("ConnectDBPrivateWithSSL strIp:"<<strIp<<" nPort:"<<nPort);

    /* An Hiredis SSL context. It holds SSL configuration and can be reused across
     * many contexts.
     */
    redisSSLContext *ssl_context;

    /* An error variable to indicate what went wrong, if the context fails to
     * initialize.
     */
    redisSSLContextError ssl_error;

    /* Initialize global OpenSSL state.
     *
     * You should call this only once when your app initializes, and only if
     * you don't explicitly or implicitly initialize OpenSSL it elsewhere.
     */
    redisInitOpenSSL();

    /* Create SSL context */
//    ssl_context = redisCreateSSLContext(
//        "cacertbundle.crt",     /* File name of trusted CA/ca bundle file, optional */
//        "/path/to/certs",       /* Path of trusted certificates, optional */
//        "client_cert.pem",      /* File name of client certificate file, optional */
//        "client_key.pem",       /* File name of client private key, optional */
//        "redis.mydomain.com",   /* Server name to request (SNI), optional */
//        &ssl_error);

    ssl_context = redisCreateSSLContext(
                                        NULL,     /* File name of trusted CA/ca bundle file, optional */
                                        NULL,       /* Path of trusted certificates, optional */
                                        NULL,      /* File name of client certificate file, optional */
                                        NULL,       /* File name of client private key, optional */
                                        NULL,   /* Server name to request (SNI), optional */
        &ssl_error);
    if(ssl_context == NULL || ssl_error != 0) {
        /* Handle error and abort... */
        MLOG_ERROR("SSL error:"<<ssl_error<<" desc:"<<redisSSLContextGetError(ssl_error));
        return false;
    }

    m_strIp = strIp;
    m_nPort = nPort;
    m_pRedisDBContex = redisConnect(strIp.c_str(), nPort);
    if (NULL == m_pRedisDBContex)
    {
        MLOG_ERROR("SERVICEALARM CRedisDBInterface::ConnectDB failed strIp == " << strIp << ", nPort== " << nPort);
        return false;
    }
   
    if (0 != m_pRedisDBContex->err)
    {
        MLOG_ERROR("CRedisDBInterface::ConnectDB failed strIp == " << strIp << ", nPort== " << nPort << ", errorCode==" << m_pRedisDBContex->err << " error:" << m_pRedisDBContex->errstr);
        redisFree(m_pRedisDBContex);
        m_pRedisDBContex = NULL;
        return false;
    }
    
    /* Negotiate SSL/TLS */
    if (redisInitiateSSLWithContext(m_pRedisDBContex, ssl_context) != REDIS_OK) {
        /* Handle error, in c->err / c->errstr */
        MLOG_ERROR("redisInitiateSSLWithContext failed strIp == " << strIp << ", nPort== " << nPort << ", errorCode==" << m_pRedisDBContex->err << " error:" << m_pRedisDBContex->errstr);
    }

    if (m_strPwd.size() > 0)
    {
        MLOG_DEBUG("CRedisDBInterface::ConnectDB send Auth ...");
        string cmd = "AUTH " + m_strPwd;
        redisReply *reply = (redisReply*)redisCommand(m_pRedisDBContex, cmd.c_str());
        if (CheckReply("AUTH", reply) != EMRStatus::EM_KVDB_SUCCESS)
        {
            freeReplyObject(reply);
            return false;
        }
        freeReplyObject(reply);
    }

    MLOG_DEBUG("CRedisDBInterface::ConnectDB success! " << strIp << " " << nPort);

    if(m_nDbIndex > 0 && !SelectDB(m_nDbIndex))
    {
        redisFree(m_pRedisDBContex);
        m_pRedisDBContex = NULL;

        MLOG_ERROR(__FUNCTION__ << " select db failed, DBIDNEX:" << m_nDbIndex);
        return false;
    }

    MLOG_DEBUG("CRedisDBInterface::ConnectDB success strIp == " << strIp << ", nPort== " << nPort);
    return true;
}






