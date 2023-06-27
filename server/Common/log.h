#ifndef __APPLOG__H__
#define __APPLOG__H__

/*------------------------------- HEADER FILE INCLUDES --------------------------------*/
#include <stdint.h>
#include <assert.h>
#include <string>
#include "servant/TarsLogger.h"
#include "servant/Application.h"

using namespace tars;
/*-------------------------------------- CONSTANTS ------------------------------------*/

#define	FORMAT_STREAM(__STROUPUT__, __INPUT__)\
{\
	try	\
{	\
	std::ostringstream  strSourceStream;	\
	strSourceStream << __INPUT__;	\
	__STROUPUT__ = strSourceStream.str();	\
}	\
	catch (exception* e)	\
{	\
	MLOG_ERROR("FORMAT_STREAM ERROR, file:" << __FILE__ << ", line:" << __LINE__ << ", reason:" << e->what());	\
}	\
}


#define LOG_BASE_INFO   __FILE__ << ": " << __LINE__ << "|" << __FUNCTION__ << "; "
//#define DLOG_BASE       syscall(SYS_gettid)<<"|"<<LOG_BASE_INFO
#define DLOG_BASE       LOG_BASE_INFO

//#define REMOTE_LOGMSG(level, msg...) do{ if(LOG->IsNeedLog(level)) DLOG<<msg;}while(0)
#define REMOTE_LOGMSG(level,msg...)

#define MLOG_INFO(d)     {TLOGINFO(LOG_BASE_INFO << d << endl);  REMOTE_LOGMSG(TarsRollLogger::INFO_LOG,"INFO|"<<DLOG_BASE<< d <<endl ); }
#define MLOG_DEBUG(d)    {TLOGDEBUG(LOG_BASE_INFO << d << endl); REMOTE_LOGMSG(TarsRollLogger::DEBUG_LOG,"DEBUG|"<<DLOG_BASE<< d <<endl ); }
#define MLOG_WARN(d)     {TLOGWARN(LOG_BASE_INFO << d << endl);  REMOTE_LOGMSG(TarsRollLogger::WARN_LOG,"WARN|"<<DLOG_BASE<< d <<endl); }
#define MLOG_ERROR(d)    {TLOGERROR(LOG_BASE_INFO << d << endl); REMOTE_LOGMSG(TarsRollLogger::ERROR_LOG,"ERROR|"<<DLOG_BASE<< d <<endl);}


#endif /*__APPLOG__H__*/
