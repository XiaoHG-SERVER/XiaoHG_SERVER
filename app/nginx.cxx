
/* main */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h> 
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include "ngx_macro.h"
#include "ngx_func.h"
#include "ngx_c_conf.h"
#include "ngx_c_socket.h"
#include "ngx_c_memory.h"
#include "ngx_c_threadpool.h"
#include "ngx_c_crc32.h"
#include "ngx_c_slogic.h"

/* source free function */
static void FreeResource();

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
 * function name: main
 * =================================================================*/
int main(int argc, char *const *argv)
{
    int iExitCode = 0;                  /* exit code */
    g_stLog.iLogFd = -1;                /* -1：defualt*/
    g_iProcessID = MASTER_PROCESS;      /* this is master process */
    g_iCurPid = getpid();               /* Current process pid */
    g_iParentPid = getppid();           /* parent process pid */
    g_iOsArgc = argc;                   /* param numbers */
    g_pOsArgv = (char **)argv;          /* param point */
    g_bIsStopEvent = false;                 /* 1: exit, 0: nothing*/

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

    /* load config file */
    CConfig *pConfig = CConfig::GetInstance();
    if(pConfig->Load("nginx.conf") == XiaoHG_ERROR)
    {
        if(LogInit() == XiaoHG_ERROR)
        {
            goto lblexit;
        }
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "Load config file failed");
        goto lblexit;
    }

    /* init */
    CMemory::GetInstance();
    CCRC32::GetInstance();

    /* init log system -> open log file */
    if(LogInit() == XiaoHG_ERROR)
    {
        XiaoHG_Log(LOG_STD, LOG_LEVEL_ERR, errno, "Init log file failed");
        goto lblexit;
    }
        
    /* signal init */
    if(InitSignals() != XiaoHG_SUCCESS) 
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "signal init failed");
        goto lblexit;
    }

    /* Init socket */
    if(g_LogicSocket.Initalize() == XiaoHG_ERROR)
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "socket init failed");
        goto lblexit;
    }

    /* Move environment variables */
    InitSetProcTitle();
    
    /* Determine whether to run as a daemon */
    if(pConfig->GetIntDefault("Daemon", 0) == 1)
    {
        /* daemon mode */
        int iCdaemonResult = DaemonInit();
        if(iCdaemonResult == XiaoHG_ERROR)
        {
            XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "Init daemon process failed");
            goto lblexit;
        }
        /* parent return 1 */
        if(iCdaemonResult == 1)
        {
            /* exit parent process */
            FreeResource();
            return 0;
        }
        /* the master process change, is daemon master process */
        g_iDaemonized = 1;
    }
    
    /* syclc process */
    MasterProcessCycle(); 

lblexit:

    /* free source */
    FreeResource();
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_NOTICE, 0, "hold process exit, byb");
    return 0;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: FreeResource
 * discription: source free
 * =================================================================*/
void FreeResource()
{
    /* free Save global environment variables*/
    if(g_pEnvMem)
    {
        delete []g_pEnvMem;
        g_pEnvMem = NULL;
    }

    /* close log file */
    if(g_stLog.iLogFd != STDERR_FILENO && g_stLog.iLogFd != -1)  
    {
        close(g_stLog.iLogFd);
        g_stLog.iLogFd = -1;
    }
}
