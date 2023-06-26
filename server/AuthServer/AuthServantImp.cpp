#include "AuthServantImp.h"
#include "servant/Application.h"
#include "otim.h"
#include "log.h"

using namespace std;

//////////////////////////////////////////////////////
void AuthServantImp::initialize()
{
    //initialize servant here:
    //...
}

//////////////////////////////////////////////////////
void AuthServantImp::destroy()
{
    //destroy servant here:
    //...
}

tars::Int32 AuthServantImp::auth(const otim::ClientContext & clientContext,const otim::AuthParam & authParam,std::string &extraData,tars::TarsCurrentPtr _current_)
{
	MLOG_DEBUG("context:"<<clientContext.writeToJsonString()<<" authParam:"<<authParam.writeToJsonString());
    //实现认证代码添加到此处
    
	return 0;
}
