
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_C_SLogic.h"
#include "XiaoHG_C_ThreadPool.h"
#include "XiaoHG_Global.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_Func.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_Crc32.h"

#define __THIS_FILE__ "XiaoHG_Init.cxx"

/* set process title */
size_t g_uiArgvNeedMem;          /* argv need memory size */
size_t g_uiEnvNeedMem;           /* Memory occupied by environment variables */
int g_iOsArgc;                   /* Number of command line parameters */
char **g_pOsArgv;                /* Original command line parameter array */
char *g_pEnvMem;                 /* Point to the memory of the environment variable allocated by yourself */
int g_iDaemonized;               /* daemon flag, 0:no, 1:yes */

/* socket/thread pool */
CLogicSocket g_LogicSocket;          /* socket obj */
CThreadPool g_ThreadPool;       /* thread pool objs */

/* log arg */
LOG_T g_stLog;

/* about process ID */
pid_t g_iCurPid;        /* current pid */
pid_t g_iParentPid;     /* parent pid */
int g_iProcessID;       /* process id, made myself */
bool g_bIsStopEvent;    /* process exit(1) or 0 */

/* Child process state changes flag */
sig_atomic_t g_stReap;

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.24
 * test time: 2020.04.24 pass
 * function name: XiaoHG_Init
 * discription: Init global values
 * =================================================================*/
void XiaoHG_Init(int argc, char *argv[])
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "XiaoHG_Init track");

    int iExitCode = 0;                  /* exit code */
    g_stLog.iLogFd = -1;                /* -1：defualt*/
    g_stLog.iLogLevel = LOG_LEVEL_ERR;  /* log level */
    g_iProcessID = MASTER_PROCESS;      /* this is master process */
    g_iCurPid = getpid();               /* Current process pid */
    g_iParentPid = getppid();           /* parent process pid */
    g_iOsArgc = argc;                   /* param numbers */
    g_pOsArgv = (char **)argv;          /* param point */
    g_bIsStopEvent = false;             /* 1: exit, 0: nothing*/

    /* get commend line size */
    for(int i = 0; i < argc; i++)
    {
        /* +1 for '\0' */
        g_uiArgvNeedMem += strlen(argv[i]) + 1;
    }/* end for */

    /* Statistics memory occupied by environment variables */
    for(int i = 0; environ[i]; i++) 
    {
        /* +1 for '\0' */
        g_uiEnvNeedMem += strlen(environ[i]) + 1; 
    }/* end for */

    /* init object */
    CMemory::GetInstance();
    CCRC32::GetInstance();
}