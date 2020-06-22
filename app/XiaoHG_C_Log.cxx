
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
int CLog::m_iLogStrPoint = 0;
int CLog::m_iLogStrLen = 0;
CConfig* CLog::m_pConfig = CConfig::GetInstance();

CLog::CLog()
{
    Init();
}

CLog::~CLog()
{
    /* close log file */
    if(m_stLog.iLogFd != -1)  
    {
        close(m_stLog.iLogFd);
        m_stLog.iLogFd = -1;
    }
    if (m_pLogBuff != NULL)
    {
        delete m_pLogBuff;
    }
    /* release log mutex */
    pthread_mutex_destroy(&m_LogMutex);
}

int CLog::Init()
{
    /* Alloc memory for log buffer */
    m_pLogBuff = new char[MAX_LOG_STRLEN];
    memset(m_pLogBuff, 0, MAX_LOG_STRLEN);
    /* Init the log mutex  */
    if(pthread_mutex_init(&m_LogMutex, NULL) != 0)
    {
        return XiaoHG_ERROR;
    }

    SetLogLevel(m_pConfig->GetIntDefault("LogLevel", LOG_LEVEL_ERR));
    SetLogFile(m_pConfig->GetString("Log"));

    return XiaoHG_SUCCESS;
}

int CLog::SetLogFile(const char *pLogFile)
{
    if (pLogFile == NULL)
    {
        pLogFile = DEFAULT_LOG_PATH;
    }
    m_stLog.iLogFd = open(pLogFile, O_WRONLY|O_APPEND|O_CREAT, 0644);  
    if (m_stLog.iLogFd == -1)
    {
        return XiaoHG_ERROR;
    }

    return XiaoHG_SUCCESS;
}

void CLog::SetLogLevel(int iLevel)
{
    m_stLog.iLogLevel = iLevel;
    return;
}

//(such as)2020/04/09 20:32:40 [alert] [pid: 30112] [errno: 98, Address already in use] [XiaoHG]: ProcessGetStatus() pid = 30113 exited on signal 11!
bool CLog::LogBasicStr(int iLevel = LOG_LEVEL_TRACK)
{
    char pStrLogInfo[BASIC_LOG_STRLEN] = { 0 };
    struct tm stTM = { 0 };
    time_t time_seconds = time(0);

    /* Initialization */
    memset(m_pLogBuff, 0, MAX_LOG_STRLEN);
    m_iLogStrLen = 0;
    m_iLogStrPoint = 0;
    
    /* Check log file id is value */
    if (m_stLog.iLogFd == -1)
    {
        return false;
    }

    /* Log level control */
    if (iLevel > m_stLog.iLogLevel)
    {
        return false;
    }
    
    /* add date: (such: 2020/04/09 20:32:40) and Loglevel, and pid ...*/
    /* Convert the time_t of parameter 1 to local time, 
     * save it to parameter 2, the one with _r is the 
     * thread-safe version, try to use 
     * */
    localtime_r(&time_seconds, &stTM);
    m_iLogStrPoint = snprintf(m_pLogBuff + m_iLogStrLen, BASIC_LOG_STRLEN, 
                            "%4d/%02d/%02d %02d:%02d:%02d [XiaoHG][pid:%d]", 
                            stTM.tm_year + 1900, stTM.tm_mon + 1,
                            stTM.tm_mday, stTM.tm_hour,
                            stTM.tm_min, stTM.tm_sec, getpid());
    m_iLogStrLen += m_iLogStrPoint;

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
    
    m_iLogStrPoint = snprintf(m_pLogBuff + m_iLogStrLen, BASIC_LOG_STRLEN - m_iLogStrLen, "[std] ");
    m_iLogStrLen += m_iLogStrPoint;
    
    va_list stArgs;
    va_start(stArgs, fmt);
    /* Write the log */
    WriteLog(fmt, stArgs, STDERR_FILENO);
    va_end(stArgs);

    return;
}

void CLog::Log(int iLevel, const char *fmt, ...)
{
    /* m_LogMutex lock */
    CLock lock(&m_LogMutex);

    /* first print log basic string */
    if (!LogBasicStr(iLevel))
    {
        return;
    }

    m_iLogStrPoint = snprintf(m_pLogBuff + m_iLogStrLen, BASIC_LOG_STRLEN - m_iLogStrLen, "[%s] ", g_ErrLevel[iLevel]);
    m_iLogStrLen += m_iLogStrPoint;
    
    va_list stArgs;
    va_start(stArgs, fmt);
    /* Write the log */
    WriteLog(fmt, stArgs);
    va_end(stArgs);

    return;
}

void CLog::Log(int iLevel, int iErrno, const char *fmt, ...)
{
    /* m_LogMutex lock */
    CLock lock(&m_LogMutex);
    
    /* first print log basic string */
    if (!LogBasicStr(iLevel))
    {
        return;
    }

    m_iLogStrPoint = snprintf(m_pLogBuff + m_iLogStrLen, BASIC_LOG_STRLEN - m_iLogStrLen, "[%s][errno:%d, %s] ",
                                                                    g_ErrLevel[iLevel], iErrno, strerror(iErrno));
    m_iLogStrLen += m_iLogStrPoint;
    
    va_list stArgs;
    va_start(stArgs, fmt);
    /* Write the log */
    WriteLog(fmt, stArgs);
    va_end(stArgs);
    
    return;
}

void CLog::Log(int iLevel, const char *pFileName, int iLine, const char *fmt, ...)
{
    /* m_LogMutex lock */
    CLock lock(&m_LogMutex);

    /* first print log basic string */
    if (!LogBasicStr(iLevel))
    {
        return;
    }

    m_iLogStrPoint = snprintf(m_pLogBuff + m_iLogStrLen, BASIC_LOG_STRLEN - m_iLogStrLen, "[%s][%s: %d] ",
                                                                        g_ErrLevel[iLevel], pFileName, iLine);
    m_iLogStrLen += m_iLogStrPoint;
    
    va_list stArgs;
    va_start(stArgs, fmt);
    /* Write the log */
    WriteLog(fmt, stArgs);
    va_end(stArgs);
    
    return;
}

void CLog::Log(int iLevel, int iErrno, const char *pFileName, int iLine, const char *fmt, ...)
{
    /* m_LogMutex lock */
    CLock lock(&m_LogMutex);

    /* first print log basic string */
    if (!LogBasicStr(iLevel))
    {
        return;
    }

    m_iLogStrPoint = snprintf(m_pLogBuff + m_iLogStrLen, BASIC_LOG_STRLEN - m_iLogStrLen, "[%s][%s: %d][errno:%d, %s] ",
                                                            g_ErrLevel[iLevel], pFileName, iLine, iErrno, strerror(iErrno));
    m_iLogStrLen += m_iLogStrPoint;

    va_list stArgs;
    va_start(stArgs, fmt);
    /* Write the log */
    WriteLog(fmt, stArgs);
    va_end(stArgs);
    
    return;
}

void CLog::WriteLog(const char* &fmt, va_list &stArgs, int iStdFlag)
{
    /* Log to file */
	m_iLogStrPoint = vsnprintf(m_pLogBuff + m_iLogStrLen, MAX_LOG_STRLEN - m_iLogStrLen, fmt, stArgs);
    m_iLogStrLen += m_iLogStrPoint;
    *(m_pLogBuff + strlen(m_pLogBuff)) = '\n';
    
    switch (iStdFlag)
    {
    case STDERR_FILENO:
        write(STDERR_FILENO, m_pLogBuff, m_iLogStrLen + 1);
        break;
    default:
        write(m_stLog.iLogFd, m_pLogBuff, m_iLogStrLen + 1);
        break;
    }

    return;
}
