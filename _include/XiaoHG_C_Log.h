
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

/* We divided the log into eight levels [level from high to low, the level 
 * with the smallest number is the highest, 
 * the level with the largest number is the lowest] */
#define LOG_LEVEL_STDERR      0    /* Console error [stderr]: highest level log */
#define LOG_LEVEL_EMERG       1    /* Urgent [emerg] */
#define LOG_LEVEL_ALERT       2    /* Alert [alert] */
#define LOG_LEVEL_CRIT        3    /* Serious [crit] */
#define LOG_LEVEL_ERR         4    /* Error [error]: belongs to the common level */
#define LOG_LEVEL_WARN        5    /* Warning [warn]: belongs to the common level */
#define LOG_LEVEL_NOTICE      6    /* Note [notice] */
#define LOG_LEVEL_INFO        7    /* Information [info] */
#define LOG_LEVEL_DEBUG       8    /* Debug [debug] */
#define LOG_LEVEL_TRACK       9    /* Track [track]: function track */

/* Define the default log storage path and file name */
#define DEFAULT_LOG_PATH  "logs/XiaoHG_error.log"

/* Log struct */
typedef struct log_s
{
	int iLogLevel;
	int iLogFd;
}LOG_T;

class CLog
{
private:
    CLog();
public:
    ~CLog();

private:
    static CLog *m_Instance;

private:
    static LOG_T m_stLog;
    static char *m_pLogBuff;
    static int m_iLogStrPoint;
    static int m_iLogStrLen;

private:
    static pthread_mutex_t m_LogMutex;

private:
    static CConfig *m_pConfig;

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
    int Init();
    static bool LogBasicStr(int iLevel);
    static void WriteLog(const char* &fmt, va_list &stArgs, int iStdFlag = 0);

public:
    int SetLogFile(const char *pLogFile);
    void SetLogLevel(int iLevel);

public:
    static void Log(const char *fmt, ...);
    static void Log(int iLevel, const char *fmt, ...);
    static void Log(int iLevel, int iErrno, const char *fmt, ...);
    static void Log(int iLevel, const char *pFileName, int iLine, const char *fmt, ...);
    static void Log(int iLevel, int iErrno, const char *pFileName, int iLine, const char *fmt, ...);
};

#endif //!__XiaoHG_LOG_H__