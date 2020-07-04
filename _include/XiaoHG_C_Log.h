
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_C_LOG_H__
#define __XiaoHG_C_LOG_H__

#include <unistd.h>
#include <stdarg.h>
#include "XiaoHG_Global.h"
#include "XiaoHG_Macro.h"

class CConfig;

/* Define the default log storage path and file name */
#define DEFAULT_LOG_PATH  "logs/XiaoHG_error.log"

/* We divided the log into eight levels [level from high to low, the level 
 * with the smallest number is the highest, 
 * the level with the largest number is the lowest] */
enum eLogLevel
{
    LOG_LEVEL_STDERR = 0,    /* Console error [stderr]: highest level log */
    LOG_LEVEL_EMERG = 1,    /* Urgent [emerg] */
    LOG_LEVEL_ALERT = 2,    /* Alert [alert] */
    LOG_LEVEL_CRIT = 3,    /* Serious [crit] */
    LOG_LEVEL_ERR = 4,    /* Error [error]: belongs to the common level */
    LOG_LEVEL_WARN = 5,    /* Warning [warn]: belongs to the common level */
    LOG_LEVEL_NOTICE = 6,    /* Note [notice] */
    LOG_LEVEL_INFO = 7,    /* Information [info] */
    LOG_LEVEL_DEBUG = 8,    /* Debug [debug] */
    LOG_LEVEL_TRACK = 9    /* Track [track]: function track */
};

/* Log struct */
typedef struct stLog
{
	int logLevel;
	int logFd;
}LOG_T;

class CLog
{
private:
    CLog();
public:
    ~CLog();

private:
    static LOG_T m_stLog;
    static char *m_pLogBuff;
    static uint16_t m_LogStrPoint;
    static uint16_t m_LogStrLen;

private:
    static pthread_mutex_t m_LogMutex;

private:
    static CLog *m_Instance;
/* Singleton implementation */
public:
    static CLog* GetInstance()
    {
        if (m_Instance == NULL)
        {
            if (m_Instance == NULL)
            {
                m_Instance = new CLog();
                static CDeleteInstance ci;
            }
        }
        return m_Instance;
    }
    class CDeleteInstance
    {
    public:
        ~CDeleteInstance()
        {
            if (CLog::m_Instance != NULL)
            {
                delete CLog::m_Instance;
                CLog::m_Instance = NULL;
            }
        }
    };

private:
    void Init();
    static bool LogBasicStr(int logLevel);
    static void WriteLog(const char* &fmt, va_list &stArgs, int stdFlag = 0);

public:
    int SetLogFile(const char *pLogFile);
    void SetLogLevel(int logLevel);

public:
    static void Log(const char *fmt, ...);
    static void Log(int logLevel, const char *fmt, ...);
    static void Log(int logLevel, int errNo, const char *fmt, ...);
    static void Log(int logLevel, const char *pFileName, int lineNo, const char *fmt, ...);
    static void Log(int logLevel, int errNo, const char *pFileName, int lineNo, const char *fmt, ...);
};

#endif //!__XiaoHG_LOG_H__