#pragma once
#include <string>
#include <sstream>
#include <chrono>

#define SCOPELOGGER( value ) ScopeLogger value(__FILE__, __LINE__, __FUNCTION__)
#define SCOPELOGGERWIHDELAY( value, delay ) ScopeLogger value(__FILE__, __LINE__, __FUNCTION__, delay)

enum EmLogLevel
{
    DBG_L = 0,
    INFO_L = 1,
    WARN_L = 2,
    ERROR_L = 3
};

class ScopeLogger
{
public:
    //默认告警延迟200毫秒
    ScopeLogger(const std::string &log, int warn_delay = 200);
    ScopeLogger(const std::string &filename, int lineno, const std::string& functioname, int warn_delay = 200);
    ~ScopeLogger();
    
    void appendLog(const char *format, ...);
    void appendLog(const std::string &log);
    void setWarnDelay(int warn_delay);
    void setLogLevel(EmLogLevel emLevel)
    {
        if (m_emLevel < emLevel)
        {
            m_emLevel = emLevel;
        }
    }

    void end();

    void PrintErrorLog(std::chrono::milliseconds  during);

    //获取时间消耗长度，单位毫秒
    int getTimeusing()
    {
        auto during = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_begin_tp);
        return during.count();
    }

    /**
     * 重载 <<
     */
    template <typename P>
    std::stringstream& operator << (const P &t)
    {
        m_logdata << t;
        return m_logdata;
    }

    /**
     * endl, flush等函数
     */
    typedef std::ostream& (*F)(std::ostream& os);
    std::stringstream& operator << (F f)
    {
        (f)(m_logdata);
        return m_logdata;
    }

private:
    std::stringstream m_logdata;
    std::chrono::steady_clock::time_point m_begin_tp;
    int m_warn_delay;
    bool m_isEnd;
    EmLogLevel m_emLevel;
};

