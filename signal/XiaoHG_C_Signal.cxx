
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h> 
#include <errno.h> 
#include <sys/wait.h> 
#include "XiaoHG_C_Signal.h"
#include "XiaoHG_Global.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_Func.h" 
#include "XiaoHG_C_Log.h"
#include "XiaoHG_error.h"

#define __THIS_FILE__ "XiaoHG_Signal.cxx"

/* signals */
SIGNAL_T signals[] = {
    { SIGHUP, "SIGHUP", SignalHandler },
    { SIGINT, "SIGINT", SignalHandler },
	{ SIGTERM, "SIGTERM", SignalHandler },
    { SIGCHLD, "SIGCHLD", SignalHandler },
    { SIGQUIT, "SIGQUIT", SignalHandler },
    { SIGIO, "SIGIO", SignalHandler },
    { SIGSYS, "SIGSYS, SIG_IGN", NULL },
    
    /* more */
    { 0, NULL, NULL }
};

CSignalCtl::CSignalCtl(uint32_t* pulSigs)
    :m_ulSigs(pulSigs)
{
    //m_ulSigs = {SIGCHLD, SIGALRM, SIGIO, SIGINT, SIGHUP, 
                //SIGUSR1, SIGUSR2, SIGWINCH, SIGTERM, SIGQUIT};
}

void CSignalCtl::Init()
{
    SIGNAL_T *pstSig = NULL;
    struct sigaction stSa;  /* sigaction：A system-defined structure related to signals */
    /* The number of the signal is not 0 */
    for (pstSig = signals; pstSig->iSigNo != 0; pstSig++)
    {
        memset(&stSa, 0, sizeof(struct sigaction));
        /* if the signal is not null, set the LogicHandlerCallBack */
        if (pstSig->LogicHandlerCallBack)  
        {
            stSa.sa_sigaction = SignalHandler;//pstSig->LogicHandlerCallBack;    /* set the LogicHandlerCallBack*/
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
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "sigaction(%s) failed", pstSig->pSigName);
        }
        else
        {
            /* successful wirte some log */
            CLog::Log(LOG_LEVEL_TRACK, "sigaction(%s) successful -- CLog::Log", pstSig->pSigName);
        }
    } /* end for */
}

void CSignalCtl::ProcessGetStatus()
{
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
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "waitpid failed");
                return;
            }
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "waitpid failed");
            return;
        }  /* end if(pid == -1) */

        iOne = 1;    /* 标记waitpid()返回了正常的返回值 */
        
        /* Get child process exit code */
        if(WTERMSIG(iStatus))
        {
            /* Get child process exit signal code */
            CLog::Log(LOG_LEVEL_NOTICE, "exited on signal %d", WTERMSIG(iStatus));
        }
        else
        {
            /* WEXITSTATUS() */
            CLog::Log(LOG_LEVEL_NOTICE, "exited on signal %d", WEXITSTATUS(iStatus));
        }
    } /* end for */

    return;
}

uint32_t CSignalCtl::RegisterSignals()
{
    sigemptyset(&m_stSet);   /* clrean signals */

    /* The following signals are not expected to be received during the execution of this function 
        (protect the critical section of code that is not expected to be interrupted by the signal) */
    /* fork()This method is used when the child process prevents signal interference */
    /* Here is to prevent the phenomenon that the creation process fails due to receiving some 
        signals when creating the child process, to prevent signal interference */
    for (int i = 0; i < sizeof(m_ulSigs)/sizeof(m_ulSigs[0]); i++)
    {
        sigaddset(&m_stSet, m_ulSigs[i]);
    }
    
    /* Shield signal */
    if(sigprocmask(SIG_BLOCK, &m_stSet, NULL) == -1)
    {
        CLog::Log(LOG_LEVEL_NOTICE, "Shield signal failed");
    }
}

uint32_t CSignalCtl::SetSigEmpty()
{
    return sigemptyset(&m_stSet);   /* clrean signals */
}

uint32_t CSignalCtl::SetSigSuspend()
{
    return sigsuspend(&m_stSet);
}

uint32_t CSignalCtl::AddSig(uint32_t ulSignalNo)
{
    return sigaddset(&m_stSet, ulSignalNo);
}

uint32_t CSignalCtl::DelSig(uint32_t ulSignalNo)
{
    return sigdelset(&m_stSet, ulSignalNo);
}

uint32_t CSignalCtl::IsSigMember(uint32_t ulSignalNo)
{
    return sigismember(&m_stSet, ulSignalNo);
}

uint32_t CSignalCtl::AddSigPending()
{
    return sigpending(&m_stSet);
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: SignalHandler
 * discription: signal LogicHandlerCallBack
 * =================================================================*/
void SignalHandler(int iSigNo, siginfo_t *pSigInfo, void *pContext)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "SignalHandler track");

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
        CLog::Log(LOG_LEVEL_INFO, "master process receive signal: %d", iSigNo);
        /* master */
        switch (iSigNo)
        {
        case SIGCHLD: /* receive the signal when subprocess exit */
            break;
        /* this is handle more signals */
        default:
            break;
        } /* end switch */
    }
    /* worker */
    else if(g_iProcessID == WORKER_PROCESS)
    {
        CLog::Log(LOG_LEVEL_INFO, "worker process receive signal: %d", iSigNo);
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
        CLog::Log(LOG_LEVEL_INFO, "Not a mater process or a worker process receive signal: %d", iSigNo);
    } /* end if(g_iProcessID == MASTER_PROCESS) */

    /* this is record some informetion */
    if(pSigInfo && pSigInfo->si_pid)
    {
        CLog::Log(LOG_LEVEL_INFO, "receive signal: %d", iSigNo);
    }
    else
    {
        CLog::Log(LOG_LEVEL_INFO, "receive signal: %d", iSigNo);
    }

    if (iSigNo == SIGCHLD) 
    {
        /* Get the end iStatus of a child process */
        CSignalCtl::ProcessGetStatus();
    } /* end if */

    return;
}