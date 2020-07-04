
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> 
#include <stdarg.h> 
#include <unistd.h> 
#include <sys/time.h> 
#include <time.h> 
#include <fcntl.h> 
#include <errno.h> 
#include <pthread.h>
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_Global.h"
#include "XiaoHG_C_Log.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_LockMutex.h"
#include "XiaoHG_C_Log.h"
#include "XiaoHG_error.h"

#define __THIS_FILE__ "XiaoHG_C_Log.cxx"

//(such as)2020/04/09 20:32:40 [alert] [pid: 30112] [errno: 98, Address already in use] [XiaoHG]: ProcessGetStatus() pid = 30113 exited on signal 11!

/* level print */
static u_char g_ErrLevel[][20]  = 
{
    {"stderr"},     /* 0 */
    {"emerg"},      /* 1 */
    {"alert"},     /* 2 */
    {"serious"},    /* 3 */
    {"error"},     /* 4 */
    {"warn"},      /* 5 */
    {"notice"},    /* 6 */
    {"info"},      /* 7 */
    {"debug"},      /* 8 */
    {"track"}       /* 9 */
};

/* Max log string length */
#define MAX_LOG_STRLEN 2048
/* Log string basic length */
#define BASIC_LOG_STRLEN 512

pthread_mutex_t CLog::m_LogMutex = PTHREAD_MUTEX_INITIALIZER;
LOG_T CLog::m_stLog = {-1, LOG_LEVEL_ERR};
CLog *CLog::m_Instance = NULL;
char *CLog::m_pLogBuff = NULL;
uint16_t CLog::m_LogStrPoint = 0;
uint16_t CLog::m_LogStrLen = 0;

CLog::CLog()
{
    Init();
}

CLog::~CLog()
{
    /* close log file */
    if(m_stLog.logFd != -1)  
    {
        close(m_stLog.logFd);
        m_stLog.logFd = -1;
    }
    if (m_pLogBuff != NULL)
    {
        delete m_pLogBuff;
    }
    /* release log mutex */
    pthread_mutex_destroy(&m_LogMutex);
}

void CLog::Init()
{
    /* Alloc memory for log buffer */
    m_pLogBuff = new char[MAX_LOG_STRLEN];
    memset(m_pLogBuff, 0, MAX_LOG_STRLEN);
    /* Init the log mutex  */
    if(pthread_mutex_init(&m_LogMutex, NULL) != 0)
    {
        exit(0);
    }
    SetLogLevel(CConfig::GetInstance()->GetIntDefault("LogLevel", LOG_LEVEL_ERR));
    SetLogFile(CConfig::GetInstance()->GetString("Log"));
}

int CLog::SetLogFile(const char *pLogFile)
{
    if (pLogFile == NULL)
    {
        pLogFile = DEFAULT_LOG_PATH;
    }
    m_stLog.logFd = open(pLogFile, O_WRONLY|O_APPEND|O_CREAT, 0644);  
    if (m_stLog.logFd == -1)
    {
        return XiaoHG_ERROR;
    }

    return XiaoHG_SUCCESS;
}

void CLog::SetLogLevel(int logLevel)
{
    m_stLog.logLevel = logLevel;
}

//(such as)2020/04/09 20:32:40 [alert] [pid: 30112] [errno: 98, Address already in use] [XiaoHG]: ProcessGetStatus() pid = 30113 exited on signal 11!
bool CLog::LogBasicStr(int logLevel = LOG_LEVEL_TRACK)
{
    char pStrLogInfo[BASIC_LOG_STRLEN] = { 0 };
    struct tm stTM = { 0 };
    time_t time_seconds = time(0);

    /* Init */
    memset(m_pLogBuff, 0, MAX_LOG_STRLEN);
    m_LogStrLen = 0;
    m_LogStrPoint = 0;
    
    /* Check log file id is value */
    if (m_stLog.logFd == -1)
    {
        return false;
    }

    /* Log level control */
    if (logLevel > m_stLog.logLevel)
    {
        return false;
    }
    
    /* add date: (such: 2020/04/09 20:32:40) and Loglevel, and pid ...*/
    /* Convert the time_t of parameter 1 to local time, 
     * save it to parameter 2, the one with _r is the 
     * thread-safe version, try to use 
     * */
    localtime_r(&time_seconds, &stTM);
    m_LogStrPoint = snprintf(m_pLogBuff + m_LogStrLen, BASIC_LOG_STRLEN, 
                            "%4d/%02d/%02d %02d:%02d:%02d [XiaoHG][pid:%d]", 
                            stTM.tm_year + 1900, stTM.tm_mon + 1,
                            stTM.tm_mday, stTM.tm_hour,
                            stTM.tm_min, stTM.tm_sec, getpid());
    m_LogStrLen += m_LogStrPoint;

    return true;
}

void CLog::Log(const char *fmt, ...)
{
    /* m_LogMutex lock */
    CLock lock(&m_LogMutex);

    /* first print log basic string */
    if (!LogBasicStr())
    {
        return;
    }
    
    m_LogStrPoint = snprintf(m_pLogBuff + m_LogStrLen, BASIC_LOG_STRLEN - m_LogStrLen, "[std] ");
    m_LogStrLen += m_LogStrPoint;
    
    va_list stArgs;
    va_start(stArgs, fmt);
    /* Write the log */
    WriteLog(fmt, stArgs, STDERR_FILENO);
    va_end(stArgs);

    return;
}

void CLog::Log(int logLevel, const char *fmt, ...)
{
    /* m_LogMutex lock */
    CLock lock(&m_LogMutex);

    /* first print log basic string */
    if (!LogBasicStr(logLevel))
    {
        return;
    }

    m_LogStrPoint = snprintf(m_pLogBuff + m_LogStrLen, BASIC_LOG_STRLEN - m_LogStrLen, "[%s] ", g_ErrLevel[logLevel]);
    m_LogStrLen += m_LogStrPoint;
    
    va_list stArgs;
    va_start(stArgs, fmt);
    /* Write the log */
    WriteLog(fmt, stArgs);
    va_end(stArgs);

    return;
}

void CLog::Log(int logLevel, int errNo, const char *fmt, ...)
{
    /* m_LogMutex lock */
    CLock lock(&m_LogMutex);
    
    /* first print log basic string */
    if (!LogBasicStr(logLevel))
    {
        return;
    }

    m_LogStrPoint = snprintf(m_pLogBuff + m_LogStrLen, BASIC_LOG_STRLEN - m_LogStrLen, "[%s][errno:%d, %s] ",
                                                                    g_ErrLevel[logLevel], errNo, strerror(errNo));
    m_LogStrLen += m_LogStrPoint;
    
    va_list stArgs;
    va_start(stArgs, fmt);
    /* Write the log */
    WriteLog(fmt, stArgs);
    va_end(stArgs);
    
    return;
}

void CLog::Log(int logLevel, const char *pFileName, int lineNo, const char *fmt, ...)
{
    /* m_LogMutex lock */
    CLock lock(&m_LogMutex);

    /* first print log basic string */
    if (!LogBasicStr(logLevel))
    {
        return;
    }

    m_LogStrPoint = snprintf(m_pLogBuff + m_LogStrLen, BASIC_LOG_STRLEN - m_LogStrLen, "[%s][%s: %d] ",
                                                                        g_ErrLevel[logLevel], pFileName, lineNo);
    m_LogStrLen += m_LogStrPoint;
    
    va_list stArgs;
    va_start(stArgs, fmt);
    /* Write the log */
    WriteLog(fmt, stArgs);
    va_end(stArgs);
    
    return;
}

void CLog::Log(int logLevel, int errNo, const char *pFileName, int lineNo, const char *fmt, ...)
{
    /* m_LogMutex lock */
    CLock lock(&m_LogMutex);

    /* first print log basic string */
    if (!LogBasicStr(logLevel))
    {
        return;
    }

    m_LogStrPoint = snprintf(m_pLogBuff + m_LogStrLen, BASIC_LOG_STRLEN - m_LogStrLen, "[%s][%s: %d][errno:%d, %s] ",
                                                            g_ErrLevel[logLevel], pFileName, lineNo, errNo, strerror(errNo));
    m_LogStrLen += m_LogStrPoint;

    va_list stArgs;
    va_start(stArgs, fmt);
    /* Write the log */
    WriteLog(fmt, stArgs);
    va_end(stArgs);
    
    return;
}

void CLog::WriteLog(const char* &fmt, va_list &stArgs, int stdFlag)
{
    /* Log to file */
	m_LogStrPoint = vsnprintf(m_pLogBuff + m_LogStrLen, MAX_LOG_STRLEN - m_LogStrLen, fmt, stArgs);
    m_LogStrLen += m_LogStrPoint;
    *(m_pLogBuff + strlen(m_pLogBuff)) = '\n';
    
    switch (stdFlag)
    {
    case STDERR_FILENO:
        write(STDERR_FILENO, m_pLogBuff, m_LogStrLen + 1);
        break;
    default:
        write(m_stLog.logFd, m_pLogBuff, m_LogStrLen + 1);
        break;
    }

    return;
}
