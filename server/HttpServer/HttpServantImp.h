#ifndef _HttpServantImp_H_
#define _HttpServantImp_H_

#include "servant/Application.h"

/**
 *
 *
 */


class HttpServantImp;

typedef std::string (HttpServantImp::*PFN_HTTP_PROCESS)(const TC_HttpRequest &request);
class HttpServantImp : public Servant
{
public:
    /**
     *
     */
    virtual ~HttpServantImp() {}

    /**
     *
     */
    virtual void initialize();

    /**
     *
     */
    virtual void destroy();

    /**
     *
     */
	int doRequest(TarsCurrentPtr current, vector<char> &buffer);

private:
	int doRequest(const TC_HttpRequest &req, TC_HttpResponse &rsp);
   
    void addProcessFunction(const std::string& path, PFN_HTTP_PROCESS pFunction)
    {
        _mapHttpProcess[path] = pFunction;
    }
    
    std::string getJsonResp(int code, const std::string &msg);

    std::string  doSendSimpleMsgCmd(const TC_HttpRequest &cRequest);
    std::string  doSendSyncCmd(const TC_HttpRequest & cRequest);
    std::string  doAddFriend(const TC_HttpRequest &request);
    std::string  doDelFriend(const TC_HttpRequest &request);
    std::string  doGetFriends(const TC_HttpRequest &request);

private:
    std::map<std::string, PFN_HTTP_PROCESS>  _mapHttpProcess;
};
/////////////////////////////////////////////////////
#endif
