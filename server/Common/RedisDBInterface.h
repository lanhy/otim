#ifndef __REDIS_DB_INTERFACE_IINCLUDE_H__
#define __REDIS_DB_INTERFACE_IINCLUDE_H__
#include <hiredis/hiredis.h>
#include <hiredis/hiredis_ssl.h>
#include "util/tc_config.h"
#include "Locker.h"


#define MAX_SCORE   "+inf"
#define MIN_SCORE   "-inf"


enum class EMRStatus
{
      EM_KVDB_SUCCESS = 0,
      EM_KVDB_RNULL = -1,
      EM_KVDB_ERROR = -2

};

typedef int INT32;
typedef int64_t INT64;
typedef uint64_t UINT64;
typedef unsigned int UINT32;
typedef unsigned char UINT8;

typedef std::pair<std::string, std::string> sspair;
typedef std::vector<sspair> vectsspair;

class CRedisDBInterface
{
public:

    CRedisDBInterface();
    CRedisDBInterface(const std::string& cfgFile);
    CRedisDBInterface(const std::string &strIp, INT32 nPort, const string& passwd)
        : m_pRedisDBContex(NULL),m_nDbIndex(0),m_isSSL(false)
    {
        ConnectDB(strIp, nPort, passwd);
    }

    std::string GenerateKey()
    {
        std::string strRet = "";
        FORMAT_STREAM(strRet, m_strIp << m_nPort);
        return strRet;
    }

	virtual ~CRedisDBInterface();

    virtual bool IsConnected();
	virtual bool ConnectDB(const std::string &strIp, INT32 nPort, const string& passwd);
    bool ConnectDB(const std::string& cfgFile);
    
	virtual bool ReConnectDB();
	virtual EMRStatus DelKey(const std::string &strKey);
	virtual EMRStatus IncrSeq(const std::string &strName, uint64_t &unItem);
	virtual EMRStatus IncrSeqBy(const std::string &strName, uint64_t &unItem, uint64_t nNum);
	virtual EMRStatus Keys(const std::string & pattern, std::list<std::string> &keyItems);
	virtual EMRStatus Expire(const std::string &strName, uint64_t nNum);
	//virtual EMRStatus Type(const std::string &strName, std::string &type);

	//-----------------队列操作------------------------------
	virtual EMRStatus QueRPop(const std::string &strName, std::string &strItem);
	virtual EMRStatus QueLPop(const std::string &strName, std::string &strItem);
	virtual EMRStatus QueLPush(const std::string &strName, const std::string &strItem);
	virtual EMRStatus QueRPush(const std::string &strName, const std::string &strItem);
	virtual EMRStatus QueIndexElement(const std::string &strName, INT32 nIndex, std::string &strItem);
	virtual EMRStatus QueSize(const std::string &strName, INT64& nRet);

	//------------------字符串操作--------------------------
    virtual EMRStatus IsStringKeyExits(const std::string &strKey, bool &bIsExit);
    virtual EMRStatus Set(const std::string &strKey, const std::string &strVal);
    virtual EMRStatus Get(const std::string &strKey, std::string &strVal);
    virtual EMRStatus SetEX(const std::string &strKey, const std::string &strVal, uint64_t nNum);
    virtual EMRStatus SetEXNX(const std::string &strKey, const std::string &strVal, int nExpireSeconds);

	//-----------------集合操作-----------------------------
	virtual EMRStatus GetSetItemCount(const std::string &strName, INT64& nRet);
	virtual EMRStatus GetSetItems(const std::string &strName, std::set<std::string> &rgItem);
	virtual EMRStatus IsItemInSet(const std::string &strName, const std::string &strItem, bool &bIsExit);
	virtual EMRStatus SetAdd(const std::string &strKey, const  std::string &strItem);
	virtual EMRStatus SetRemove(const std::string &strKey, const std::string &strItem);
	virtual EMRStatus ZSetAdd(const std::string &strKey, INT64 score, const std::string &strItem);
    //virtual EMRStatus ZSetAdd(const std::string &strKey, const std::vector<std::string> &scores, const std::vector<std::string> &strItems);

    virtual EMRStatus ZSetRemove(const std::string &strKey, const std::string &strItem);
	virtual EMRStatus ZScore(const std::string &strKey, const  std::string &strItem, INT64& nRet);
	virtual EMRStatus ZCount(const std::string &strKey, INT64 from, const char *to, INT64& nCount);
	virtual EMRStatus ZCard(const std::string &strKey, INT32& nLen);
    
    //非加密函数
    virtual EMRStatus ZRangeWithScore(const std::string &strKey, std::vector<std::string> &topicItem, INT64 from, INT64 to, std::vector<std::string> &scoreItem);
    virtual EMRStatus ZRangeByScore(const std::string &strKey, INT64 from, const char *to, std::vector<std::string> &memberItem);
    virtual EMRStatus ZRangeByScoreWithScores(const std::string &strKey, INT64 from, const char *to, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem);
    
    virtual EMRStatus ZRangeByScoreAndLimit(const std::string &strKey, INT64 nStart, INT64 nLimit, std::vector<std::string> &memberItem);
    
    virtual EMRStatus ZRangeByScoreAndLimitWithScore(const std::string &strKey, INT64 nStart, INT64 nEnd, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem);
    virtual EMRStatus ZRangeByScoreAndLimitWithScore(const std::string &strKey, INT64 nStart, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem);

    virtual EMRStatus ZReverRangeByScoreAndLimit(const std::string &strKey, INT64 nStart, INT64 nLimit, std::vector<std::string> &memberItem);
    virtual EMRStatus ZReverRangeByScoreAndLimitWithScores(const std::string &strKey, INT64 nStart, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem);
    virtual EMRStatus ZReverRangeByScoreAndLimitWithScores(const std::string &strKey, const char * strStart, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem);
    
    
    
	virtual EMRStatus ZSetGetPeerMsgs(const std::string &strKey, std::vector<std::string> &msgs, INT32& nTotalSize);	// range scope 0 -1
	virtual EMRStatus ZSetRemoveByScore(const std::string &strKey, INT64 scorefrom, INT64 scoreto);
	virtual EMRStatus ZSetRemoveByScore(const std::string &strKey, INT64 scorefrom, const std::string &scoreto);
    virtual EMRStatus ZSetIncrby(const std::string &strKey, const std::string &strScore, const std::string &strMember, INT64& nRet);
	
	//-----------------哈希操作---key field value---------
	virtual EMRStatus DelHashKey(const std::string &strKey);
	virtual EMRStatus SetHashItem(const std::string &strKey, const std::string &strField, const std::string &strItem);

    virtual EMRStatus MSetHashItem(const std::string &strKey, const std::map<std::string, std::string> &mapItems);
//    virtual EMRStatus MSetHashItem(const std::string &strHashTableName,const std::string &strHashItemName,const std::vector<pair<std::string,std::string> > &hashItemsPair);
    virtual EMRStatus MGetHashItem(const std::string &strKey,const std::vector<std::string> &hashItemFields,std::vector<std::string> &hashItemValues);
	virtual EMRStatus DelHashField(const std::string &strKey, const std::string &strField);
	virtual EMRStatus GetHashItem(const std::string &strKey,  const std::string &strField, std::string &strItem);
    virtual EMRStatus GetAllHashItem(const std::string &strKey, std::map<std::string, std::string> &mapFileValue);
	virtual EMRStatus GetSeqID(const std::string &strKey, INT64& nRet);
	virtual EMRStatus HIncrby(const std::string &strKey, const std::string &strField, INT64 &unItem, INT64 val = 1);
//	virtual EMRStatus HScan(const std::string &strHashTableName, const std::string &strHashItemMatch, int page_size, int& page_nums, std::list<std::string>& hashItems, std::list<std::string>& hashItemsValue);
	
    EMRStatus HLen(const std::string & key, INT64 & count);
    EMRStatus HSet(const std::string & key, const std::string & field, const std::string & value);
    EMRStatus HGet(const std::string & key, const std::string & field, std::string & value);
    EMRStatus HDel(const std::string & key, const std::string & field);
    EMRStatus HGetAll(const std::string & key, vectsspair & result);
    EMRStatus HKeys(const std::string & key, std::vector<std::string> & result);
    EMRStatus HVals(const std::string & key, std::vector<std::string> & result);

	virtual EMRStatus Scan(const std::string &match, int count, uint64_t cursor, uint64_t &newCursor, std::vector<std::string>& keys);
    virtual EMRStatus ScanPrivate(const std::string &match, int count, uint64_t cursor, uint64_t &newCursor, std::vector<std::string>& keys, const std::string &strFunName);

	virtual EMRStatus ZSetScan(const std::string &key, int count, uint64_t cursor, uint64_t &newCursor, std::vector<std::string>& values, std::vector<std::string>& scores);
    virtual EMRStatus ZSetScanPrivate(const std::string &key, int count, uint64_t cursor, uint64_t &newCursor, std::vector<std::string>& values, std::vector<std::string>& scores, const std::string &strFunName);

protected:
    std::vector<std::string> m_redisServerList;
    CMutexSem m_cMutex;
    redisContext *m_pRedisDBContex;
    std::string m_strIp;
    INT32 m_nPort;
    std::string m_strPwd;
    int   m_nDbIndex;
    bool m_isSSL;

    bool TryConnectDB();

    bool SelectDB(int index);

private:
	EMRStatus CheckReply(const std::string & strFunName, redisReply * pReply);
    EMRStatus IsKeyExits(const std::string &strKey, bool &bIsExit, const std::string &strFunName);

    EMRStatus IncrSeqPrivate(const std::string &strName, uint64_t &unItem, const std::string &strFunName);
    EMRStatus IncrSeqByPrivate(const std::string &strName, uint64_t &unItem, uint64_t nNum, const std::string &strFunName);
    EMRStatus KeysPrivate(const std::string & pattern, std::list<std::string> &keyItems, const std::string &strFunName);
    EMRStatus ExpirePrivate(const std::string &strName, uint64_t nNum, const std::string &strFunName);
    EMRStatus TypePrivate(const std::string &strName, std::string &type, const std::string &strFunName);

	//队列操作
    EMRStatus QueRPopPrivate(const std::string &strName, std::string &strItem, const std::string &strFunName);
    EMRStatus QueLPopPrivate(const std::string &strName, std::string &strItem, const std::string &strFunName);
    EMRStatus QueLPushPrivate(const std::string &strName, const std::string &strItem, const std::string &strFunName);
    EMRStatus QueIndexElementPrivate(const std::string &strName, INT32 nIndex, std::string &strItem, const std::string &strFunName);
    EMRStatus QueRPushPrivate(const std::string &strName, const std::string &strItem, const std::string &strFunName);
    EMRStatus QueSizePrivate(const std::string &strName, INT64& nRet, const std::string &strFunName);
    EMRStatus SetPrivate(const std::string &strKey, const std::string &strVal, const std::string &strFunName);
    EMRStatus GetPrivate(const std::string &strKey, std::string &strVal, const std::string &strFunName);
    EMRStatus SetEXPrivate(const std::string &strKey, const std::string &strVal, uint64_t nNum, const std::string &strFunName);
    EMRStatus SetEXNXPrivate(const std::string &strKey, const std::string &strVal, int nExpireSeconds, const std::string &strFunName);

	
    EMRStatus GetSetItemCountPrivate(const std::string &strName, INT64& nRet, const std::string &strFunName);
    EMRStatus GetSetItemsPrivate(const std::string &strName, std::set<std::string> &rgItem, const std::string &strFunName);
    EMRStatus IsItemInSetPrivate(const std::string &strName, const std::string &strItem, bool &bIsExit, const std::string &strFunName);
	

    EMRStatus SetAddPrivate(const std::string &strKey, const  std::string &strItem, const std::string &strFunName);
    EMRStatus SetRemovePrivate(const std::string &strKey, const std::string &strItem, const std::string &strFunName);
    EMRStatus ZSetAddPrivate(const std::string &strKey, INT64 score, const  std::string &strItem, const std::string &strFunName);
    EMRStatus ZSetAddPrivate(const std::string &strKey, const std::vector<std::string> &scores, const std::vector<std::string> &strItems, const std::string &strFunName);
    EMRStatus ZSetRemovePrivate(const std::string &strKey, const std::string &strItem, const std::string &strFunName);
    EMRStatus ZScorePrivate(const std::string &strKey, const  std::string &strItem, INT64& nRet, const std::string &strFunName);
	EMRStatus ZCountPrivate(const std::string &strKey, INT64 from, const char *to, INT64& nCount, const std::string &strFunName);
    EMRStatus ZCardPrivate(const std::string &strKey, INT32& nLen, const std::string &strFunName);


    // !!! 注意：加解密相关信息尽量不能写入代码中，仅能写到注释中。###此处statflag参数实际为加解密是否可用信息
    EMRStatus ZRangeWithScorePrivate(const std::string &strKey, std::vector<std::string> &topicItem, INT64 from, INT64 to, std::vector<std::string> &scoreItem, const std::string &strFunName, bool statflag);
    EMRStatus ZRangeByScorePrivate(const std::string &strKey, INT64 from, const char *to, std::vector<std::string> &memberItem, const std::string &strFunName);

    EMRStatus ZRangeByScoreWithScoresPrivate(const std::string &strKey, INT64 from, const char *to, std::vector<std::string> &rgMemberItem, std::vector<std::string> &rgScoreItem, const std::string &strFunName, bool statflag);
   
    EMRStatus ZRangeByScoreAndLimitPrivate(const std::string &strKey, const std::string &strStart, const std::string &strEnd, INT64 nLimit, std::vector<std::string> &memberItem, const std::string &strFunName, bool statflag);
    EMRStatus ZRangeByScoreAndLimitWithScorePrivate(const std::string &strKey, const std::string &strStart, const std::string &strEnd, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem, const std::string &strFunName, bool statflag);

    EMRStatus ZReverRangeByScoreAndLimitWithScoresPrivate(const std::string &strKey, INT64 nStart, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem, const std::string &strFunName, bool statflag);
    EMRStatus ZReverRangeByScoreAndStartEndLimitWithScorePrivate(const std::string &strKey, const std::string &strStart, const std::string &strEnd, INT64 nLimit, std::vector<std::string> &memberItem, std::vector<std::string> &scoreItem, const std::string &strFunName, bool statflag);
    EMRStatus ZReverRangeByScoreAndLimitPrivate(const std::string &strKey, const std::string &strStart, const std::string &strEnd, INT64 nLimit, std::vector<std::string> &memberItem, const std::string &strFunName, bool statflag);

    EMRStatus ZSetGetPeerMsgsPrivate(const std::string &strKey, std::vector<std::string> &msgs, INT32& nTotalSize, const std::string &strFunName);
    EMRStatus ZSetRemoveByScorePrivate(const std::string &strKey, INT64 scorefrom, INT64 scoreto, const std::string &strFunName);
    EMRStatus ZSetRemoveByScorePrivate(const std::string &strKey, INT64 scorefrom, const std::string &scoreto, const std::string &strFunName);
    EMRStatus ZSetIncrbyPrivate(const std::string &strKey, const std::string &strScore, const std::string &strMember, INT64& bRet, const std::string &strFunName);

	//-----------------哈希操作----------------------------
    EMRStatus DelKeyPrivate(const std::string &strKeyName, const std::string &strFunName);
    EMRStatus SetHashItemPrivate(const std::string &strKey, const std::string &strField, const std::string &strItem, const std::string &strFunName);
    EMRStatus MSetHashItemPrivate(const std::string &strKey,const std::map<std::string,std::string> &mapItems,const std::string &strFunName);
    EMRStatus MGetHashItemPrivate(const std::string &strKey,const std::vector<std::string> &hashItemFields,std::vector<std::string> &hashItemValues,const std::string &strFunName);
    EMRStatus Commandargv(const std::vector<string> &vData,const std::string &strFunName);
    EMRStatus CommandargvArray(const std::vector<string> &vData,std::vector<string> &vDataValues,const std::string &strFunName);
    EMRStatus DelHashFieldPrivate(const std::string &strHashItemName, const std::string &strField, const std::string &strFunName);
    EMRStatus GetHashItemPrivate(const std::string &strHashItemName, const std::string &strField, std::string &strItem, const std::string &strFunName);
    EMRStatus GetAllHashItemPrivate(const std::string &strHashItemName, std::map<std::string, std::string> &mapFileValue, const std::string &strFunName);
    EMRStatus GetSeqIDPrivate(const std::string &strKey, INT64& nRet, const std::string &strFunName);
    EMRStatus HIncrbyPrivate(const std::string &strHashItemName, const std::string &strField, INT64 &unItem, INT64 val, const std::string &strFunName);
    EMRStatus HScanPrivate(const std::string &strHashTableName, const std::string &strHashItemMatch, int page_size, int& page_nums, std::list<std::string>& hashItems, std::list<std::string>& hashItemsValue, const std::string &strFunName);

    EMRStatus MSetItemPrivate(const std::vector<pair<std::string,std::string>> &strItemsPair, const std::string &strFunName);
    EMRStatus DelItemPrivate(const std::vector<std::string> &strItems,const std::string &strFunName);

    bool getRedisServer(std::string &address);

    bool ConnectDBPrivate();
    bool ConnectDBPrivate(const std::string &strIp, INT32 nPort);
    bool ConnectDBPrivateWithSSL(const std::string &strIp, INT32 nPort);

 };

#endif
