
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h> 
#include <errno.h> 
#include <sys/wait.h> 
#include "XiaoHG_Global.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_Func.h" 

#define __THIS_FILE__ "XiaoHG_Signal.cxx"

/* signals struct */
typedef struct 
{
    int iSigNo;              /* signal code */
    const char *pSigName;    /* signal string, such: SIGHUP */
    /* signal LogicHandlerCallBack */
    void (*LogicHandlerCallBack)(int iSigNo, siginfo_t *pSigInfo, void *pContext);
}SIGNAL_T;

/* signal LogicHandlerCallBack function */
static void SignalHandler(int iSigNo, siginfo_t *pSigInfo, void *pContext);

/* Get the end iStatus of the child process to prevent 
 * the child process from becoming a zombie process when 
 * killing the child process alone */
static void ProcessGetStatus(void);

/* signals */
SIGNAL_T signals[] = {
    { SIGHUP,    "SIGHUP",           SignalHandler },
    { SIGINT,    "SIGINT",           SignalHandler },
	{ SIGTERM,   "SIGTERM",          SignalHandler },
    { SIGCHLD,   "SIGCHLD",          SignalHandler },
    { SIGQUIT,   "SIGQUIT",          SignalHandler },
    { SIGIO,     "SIGIO",            SignalHandler },
    { SIGSYS,    "SIGSYS, SIG_IGN",  NULL          },
    
    /* more */
    { 0,         NULL,               NULL          }
};

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: InitSignals
 * discription: Init signal, Registration LogicHandlerCallBack
 * =================================================================*/
int InitSignals()
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "InitSignals track");

    SIGNAL_T *pstSig = NULL;
    struct sigaction stSa;  /* sigaction：A system-defined structure related to signals */
    /* The number of the signal is not 0 */
    for (pstSig = signals; pstSig->iSigNo != 0; pstSig++)
    {
        memset(&stSa, 0, sizeof(struct sigaction));
        /* if the signal is not null, set the LogicHandlerCallBack */
        if (pstSig->LogicHandlerCallBack)  
        {
            stSa.sa_sigaction = pstSig->LogicHandlerCallBack;    /* set the LogicHandlerCallBack*/
            stSa.sa_flags = SA_SIGINFO;             /* SA_SIGINFO: make your LogicHandlerCallBack Effective */
        }
        else
        {
            /* SIG_IGN: Ignore signal LogicHandlerCallBack, the signal coming, do nothing*/
            stSa.sa_handler = SIG_IGN;
        } /* end if */

        /* epmpty signals, accept any signal */
        sigemptyset(&stSa.sa_mask);

        /* set signal heandler*/
        if (sigaction(pstSig->iSigNo, &stSa, NULL) == -1)    
        {
            XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "sigaction(%s) failed", pstSig->pSigName);
            return XiaoHG_ERROR;
        }
        else
        {
            /* successful wirte some log */
            XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "sigaction(%s) successful", pstSig->pSigName);
        }
    } /* end for */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "Init signals is successful");
    return XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: SignalHandler
 * discription: signal LogicHandlerCallBack
 * =================================================================*/
static void SignalHandler(int iSigNo, siginfo_t *pSigInfo, void *pContext)
{    
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "SignalHandler track");

    SIGNAL_T *pstSig = NULL;
    /* find the register signal */
    for (pstSig = signals; pstSig->iSigNo != 0; pstSig++)     
    {         
        /* Found the corresponding registration signal */
        if (pstSig->iSigNo = iSigNo)
        {
            break;
        }
    } /* end for */

    /* master process or worker process */
    if(g_iProcessID == MASTER_PROCESS)
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "master process receive signal: %d", iSigNo);
        /* master */
        switch (iSigNo)
        {
        case SIGCHLD:       /* receive the signal when subprocess exit */
            g_stReap = 1;   /* Mark changes in subprocesses and process them (such as creating a subprocess)  */
            break;
        /* this is handle more signals */
        default:
            break;
        } /* end switch */
    }
    /* worker */
    else if(g_iProcessID == WORKER_PROCESS)
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "worker process receive signal: %d", iSigNo);
        /* worker */
        /* for the future */
        switch (iSigNo)
        {
        case 0:
            break;
        /* this is handle more signals */
        default:
            break;
        } /* end switch */
    }
    else
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "Not a mater process or a worker process receive signal: %d", iSigNo);
    } /* end if(g_iProcessID == MASTER_PROCESS) */

    /* this is record some informetion */
    if(pSigInfo && pSigInfo->si_pid)
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "receive signal: %d", iSigNo);
    }
    else
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "receive signal: %d", iSigNo);
    }

    if (iSigNo == SIGCHLD) 
    {
        /* Get the end iStatus of a child process */
        ProcessGetStatus(); 
    } /* end if */

    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ProcessGetStatus
 * discription: Get the end iStatus of a child process, Prevent child 
 *              processes from becoming zombie processes when killing 
 *              child processes alone.
 * =================================================================*/
static void ProcessGetStatus(void)
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "ProcessGetStatus track");

    int iOne = 0;        /* only iOne time to signal handle */
    int iStatus = 0;
    pid_t pid = 0;
    int iErr = 0;
    /* Parent process receive SIGCHLD signal when kill Child process */
    for ( ;; ) 
    {
        /* wait a Child process exit iStatus */
        pid = waitpid(-1, &iStatus, WNOHANG);
        if(pid == 0)
        {
            return;
        } /* end if(pid == 0) */
        
        /* this is waitpid return error */
        if(pid == -1)
        {
            iErr = errno;
            /* The call was interrupted by a signal */
            if(iErr == EINTR)           
            {
                continue;
            }
            /* no Child process */
            if(iErr == ECHILD  && iOne)  
            {
                return;
            }
            /* no Child process */
            if (iErr == ECHILD)
            {
                XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, 0, "waitpid failed");
                return;
            }
            XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, 0, "waitpid failed");
            return;
        }  /* end if(pid == -1) */
        iOne = 1;    /* 标记waitpid()返回了正常的返回值 */
        
        /* Get child process exit code */
        if(WTERMSIG(iStatus))
        {
            /* Get child process exit signal code */
            XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, 0, "exited on signal %d", WTERMSIG(iStatus));
        }
        else
        {
            /* WEXITSTATUS() */
            XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, 0, "exited on signal %d", WEXITSTATUS(iStatus));
        }
    } /* end for */

    return;
}
