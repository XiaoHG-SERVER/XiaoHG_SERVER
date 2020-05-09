
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
    int LogInit();
    static void LogBasicStr();

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