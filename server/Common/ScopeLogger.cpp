#include "ScopeLogger.h"
#include "log.h"

ScopeLogger::ScopeLogger(const std::string &log, int warn_delay)
: m_begin_tp(std::chrono::steady_clock::now())
, m_warn_delay(warn_delay)
, m_isEnd(false)
, m_emLevel(DBG_L)
{
    m_logdata << LOG_BASE_INFO << log;
    MLOG_DEBUG(m_logdata.str() << " begin");
}

ScopeLogger::ScopeLogger(const std::string &filename, int lineno, const std::string& functioname, int warn_delay)
: m_begin_tp(std::chrono::steady_clock::now())
, m_warn_delay(warn_delay)
, m_isEnd(false)
, m_emLevel(DBG_L)
{
    m_logdata << filename << " :" << lineno << "|" << functioname << "; ";
    MLOG_DEBUG(m_logdata.str()<< " begin");
}

ScopeLogger::~ScopeLogger()
{
    end();
}

void ScopeLogger::end()
{
    if(m_isEnd){
        return;
    }

    auto during = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_begin_tp);
    //用时超过告警值就打印WARN级别的log
    if (during.count() >= m_warn_delay && m_emLevel < WARN_L) {
        m_emLevel = WARN_L;
    }

    switch (m_emLevel)
    {
    case INFO_L:
    case DBG_L:
       MLOG_DEBUG(m_logdata.str() << " [takes " << during.count() << "ms]");
        break;
    case WARN_L:
        MLOG_WARN(m_logdata.str() << " [takes " << during.count() << "ms]");
        break;
    case ERROR_L:
        {
             PrintErrorLog(during);
        }
        break;
    default:
        {
             PrintErrorLog(during);
        }
        break;
    }

    m_isEnd = true;
}

void ScopeLogger::appendLog(const char *format, ...)
{
    char buf[1024];
    va_list   pArgList;
    va_start(pArgList, format);
    vsnprintf(buf, 1024, format, pArgList);
    va_end(pArgList);
    m_logdata << buf;
}

void ScopeLogger::appendLog(const std::string &log)
{
    m_logdata << log;
}

void ScopeLogger::setWarnDelay(int warn_delay)
{
    m_warn_delay = warn_delay;
}

void ScopeLogger::PrintErrorLog(std::chrono::milliseconds during)
{
    MLOG_ERROR(m_logdata.str() << " [takes " << during.count() << "ms]");
}



