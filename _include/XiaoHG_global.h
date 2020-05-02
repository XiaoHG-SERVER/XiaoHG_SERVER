
#ifndef __XiaoHG_GBLDEF_H__
#define __XiaoHG_GBLDEF_H__

#include <signal.h> 
#include "XiaoHG_c_slogic.h"
#include "XiaoHG_c_threadpool.h"

/* Read config file, name length, content length*/
#define CONF_ITEMNAME_LEN 50
#define CONF_ITEMCONTENT_LEN 500

/* config struct */
typedef struct config_item_s
{
	char ItemName[CONF_ITEMNAME_LEN];		/* name */
	char ItemContent[CONF_ITEMCONTENT_LEN];	/* content */
}CONF_ITEM, *LPCONF_ITEM;

/* Log struct */
typedef struct log_s
{
	int iLogLevel;
	int iLogFd;
}LOG_T;

/* global vrg */
extern size_t g_uiArgvNeedMem;
extern size_t g_uiEnvNeedMem; 
extern int g_iOsArgc; 
extern char **g_pOsArgv;
extern char *g_pEnvMem; 
extern int g_iDaemonized;

extern CLogicSocket g_LogicSocket;  
extern CThreadPool g_ThreadPool;

extern pid_t g_iCurPid;
extern pid_t g_iParentPid;
extern LOG_T g_stLog;
extern int g_iProcessID;
extern sig_atomic_t g_stReap;
extern bool g_bIsStopEvent;

extern int g_iErrCode;

#endif //!__XiaoHG_GBLDEF_H__
