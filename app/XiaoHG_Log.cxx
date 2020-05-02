
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
#include "ngx_c_conf.h"
#include "ngx_macro.h"
#include "ngx_global.h"

/* level print */
static u_char g_stErriLevels[][20]  = 
{
    {"stderr"},    /* 0 */
    {"emerg"},     /* 1 */
    {"alert"},     /* 2 */
    {"crit"},      /* 3 */
    {"error"},     /* 4 */
    {"warn"},      /* 5 */
    {"notice"},    /* 6 */
    {"info"},      /* 7 */
    {"debug"}      /* 8 */
};
#define MAX_LOG_STRLEN   2048

#define TEST_LOG 0

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23 pass
 * function name: SetLogLevel
 * =================================================================*/
void SetLogLevel(int iErrorLevel)
{
    g_stLog.iLogLevel = iErrorLevel;
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23 pass
 * function name: XiaoHG_Log
 * =================================================================*/
//(such)2020/04/09 20:32:40 [alert] [pid: 30112] [errno: 98, Address already in use] [nginx]: ProcessGetStatus() pid = 30113 exited on signal 11!
void XiaoHG_Log(int iLogType, int iLogLevel, int iErrCode, const char* pFmt, ...)
{
	char pLogStr[MAX_LOG_STRLEN] = { 0 };
    char pStrLogInfo[256] = { 0 };
	int iLogStrLen = 0;
	va_list stArgs = { 0 };
    struct tm stTM = { 0 };
    time_t time_seconds = time(0);
    int iFileHandle = STDERR_FILENO;

    /* log level control */
    /* The larger the number, the lower the level, the higher the level is set */
    if (iLogLevel > g_stLog.iLogLevel)
    {
        return;
    }

    /* add date: (such: 2020/04/09 20:32:40) and Loglevel, and pid ...*/
    /* Convert the time_t of parameter 1 to local time, 
     * save it to parameter 2, the one with _r is the 
     * thread-safe version, try to use 
     * */
    localtime_r(&time_seconds, &stTM);

    if(iErrCode != 0)
    {
        iLogStrLen = snprintf(pStrLogInfo, sizeof(pStrLogInfo), 
                                "%4d/%02d/%02d %02d:%02d:%02d [XiaoHG][pid:%d][%s][errno:%d, %s] ", 
                                stTM.tm_year + 1900, stTM.tm_mon + 1,
                                stTM.tm_mday, stTM.tm_hour,
                                stTM.tm_min, stTM.tm_sec, 
                                getpid(), g_stErriLevels[iLogLevel], 
                                iErrCode, strerror(iErrCode));
    }
    else
    {
        iLogStrLen = snprintf(pStrLogInfo, sizeof(pStrLogInfo), 
                                "%4d/%02d/%02d %02d:%02d:%02d [XiaoHG][pid:%d][%s] ", 
                                stTM.tm_year + 1900, stTM.tm_mon + 1,
                                stTM.tm_mday, stTM.tm_hour,
                                stTM.tm_min, stTM.tm_sec, 
                                getpid(), g_stErriLevels[iLogLevel]);
    }
    memcpy(pLogStr, pStrLogInfo, iLogStrLen);

	va_start(stArgs, pFmt);
	/* Log to file */
	/* int _vsnprintf(char* str, size_t size, const char* format, va_list ap); */
	iLogStrLen = vsnprintf(pLogStr + iLogStrLen, NGX_MAX_ERROR_STR - iLogStrLen, pFmt, stArgs);
	va_end(stArgs);
    *(pLogStr + strlen(pLogStr)) = '\n';

    switch (iLogType)
    {
    case LOG_ALL:
        write(iFileHandle, pLogStr, strlen(pLogStr));
        //break;
    case LOG_FILE:
        //break;
        if (g_stLog.iLogFd != -1)
        {
            iFileHandle = g_stLog.iLogFd;
            //break;
        }
        return;
    case LOG_STD:
        //write(iFileHandle, pLogStr, strlen(pLogStr));
        break;
    default:
        break;
    }
    /* write log buff to LogFile*/
    //write(iFileHandle, pLogStr, strlen(pLogStr));
    return;
}


/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23 pass
 * function name: LogInit
 * discription: init log file, open log file.
 * =================================================================*/

int LogInit()
{
    u_char *pLogFile = NULL;
    CConfig *pConfig = CConfig::GetInstance();
    pLogFile = (u_char *)pConfig->GetString("Log");
    if(pLogFile == NULL)
    {
        //default log file
        pLogFile = (u_char *)DEFAULT_LOG_PATH; 
    }
    g_stLog.iLogLevel = pConfig->GetIntDefault("LogLevel", LOG_LEVEL_NOTICE);
    g_stLog.iLogFd = open((const char *)pLogFile, O_WRONLY|O_APPEND|O_CREAT, 0644);  
    if (g_stLog.iLogFd == -1)
    {
        XiaoHG_Log(LOG_STD, LOG_LEVEL_ALERT, errno, " could not open error log file: open \"%s\" failed", pLogFile);
        return XiaoHG_ERROR;
    }
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "Init log file is successful");
    return XiaoHG_SUCCESS;
}
