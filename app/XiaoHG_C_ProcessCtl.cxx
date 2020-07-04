
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "XiaoHG_C_ProcessCtl.h"
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_Global.h"
#include "XiaoHG_error.h"
#include "XiaoHG_C_Log.h"
#include "XiaoHG_C_Signal.h"
#include "XiaoHG_C_MainArgCtl.h"

#define __THIS_FILE__ "XiaoHG_C_ProcessCtl.cxx"

CConfig* CProcessCtl::m_pConfig = nullptr;
uint32_t CProcessCtl::m_ProcessID = MASTER_PROCESS;
CProcessCtl::CProcessCtl(/* args */)
{
    Init();
}

CProcessCtl::~CProcessCtl()
{}

void CProcessCtl::Init()
{
    m_pConfig = CConfig::GetInstance();
    m_pMaterTitle = m_pConfig->GetString("MasterProcessTitle");
    m_pWorkerTitle = m_pConfig->GetString("WorkerProcessTitle");
    m_WorkProcessCount = m_pConfig->GetIntDefault("WorkerProcessesCount", 1);
}

uint32_t CProcessCtl::InitDaemon()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "InitDaemon track");

    switch (fork())
    {
    case -1:/* fork failed */
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "create subsprocess failed");
        return XiaoHG_ERROR;
    case 0:/* subprocess */
        break;
    default:/* parent process */
        /* return 1 for free some source */
        return 1;
    } /* end switch */

    /* this is cycle is suprocess */
    
    /* The terminal is closed and the terminal is closed, 
     * which will have nothing to do with this subprocess */
    if (setsid() == -1)  
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "subsprocess set sid failed");
        exit(0);
    }

    /* 0777 */
    umask(0); 

    /* /dev/null opration */
    int iFd = open("/dev/null", O_RDWR);
    if (iFd == -1) 
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "open \"/dev/null\" failed");
        return XiaoHG_ERROR;
    }

    /* First close STDIN_FILENO */
    if (dup2(iFd, STDIN_FILENO) == -1)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "dup2(STDIN) failed");
        return XiaoHG_ERROR;
    }

    /* and than close STDIN_FILENO */
    if (dup2(iFd, STDOUT_FILENO) == -1) 
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "dup2(STDOUT_FILENO) failed");
        return XiaoHG_ERROR;
    }

    /* now iFd = 3 */
    if (iFd > STDERR_FILENO)  
    {
        if (close(iFd) == -1)  
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "close file handle: %d failed", iFd);
            return XiaoHG_ERROR;
        }
    }
    CLog::Log("Init deamon process successful");
    /* subprocess return */
    return XiaoHG_SUCCESS; 
}

void CProcessCtl::MasterProcessCycle()
{
    CLog::Log(LOG_LEVEL_TRACK, "CProcessCtl::MasterProcessCycle track");
    m_ProcessID = MASTER_PROCESS;
    uint32_t signals[] = {SIGCHLD, SIGALRM, SIGIO, SIGINT, SIGHUP, 
                            SIGUSR1, SIGUSR2, SIGWINCH, SIGTERM, SIGQUIT};
    CSignalCtl signalCtl(signals);
    signalCtl.RegisterSignals();
    /* stSet title */
    /* still bug */
    if(SetProcTitle(m_pMaterTitle) != XiaoHG_SUCCESS)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Set master process title failed\n");
        exit(0);
    }

    /* Create m_WorkProcessCount subprocess count */
    for(int i = 0; i < m_WorkProcessCount; i++)
    {
        /* 0: Child process, or: parent process*/
        switch(fork())
        {
        case -1:/* failde */
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Fork subprocess failed");
            exit(0);
        case 0:/* Child process */
            /* worker process cycle*/
            CLog::Log("Worker process cycle, %d", i);
            WorkerProcessCycle();
            break;
        default:/* parent process */           
            break;
        }/* end switch */
    } /* end for */
    /* this is master process cycle, access all signals */
    signalCtl.SetSigEmpty();
    for ( ;; ) 
    {
        /* Blocked here, waiting for a signal, at this time the process is suspended, 
         * does not occupy CPU time, only to receive the signal will be awakened (return)
         * At this time, the master process is completely driven by signals. */
        CLog::Log("Master process wait a signal");
        signalCtl.SetSigSuspend();
        /* more for future */
    }/*  end for(;;) */
}

void CProcessCtl::WorkerProcessCycle() 
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CProcessCtl::WorkerProcessCycle track");

    m_ProcessID = WORKER_PROCESS;
    /* still bug*/
    if(SetProcTitle(m_pWorkerTitle) != XiaoHG_SUCCESS)
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Set worker process title failed");
        exit(0);
    }
    /* Init worker process*/
    WorkerProcessInit();
    /* recod some info to log file */
    for(;;)
    {
        /* Handle network epollEvents and timer epollEvents */
        /* Event-driven processing */
        //ProcessEventsAndTimers();
        /* -1: always waiting */
        g_Socket.EpolWaitlProcessEvents(-1);
        /* printf dbg information */
        //g_Socket.PrintTDInfo();
    } /* end for(;;) */

    /* Exit worker process */
    g_ThreadPool.Clear(); 
    g_Socket.Clear(); 
}

void CProcessCtl::WorkerProcessInit()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CProcessCtl::WorkerProcessInit track");

    sigset_t stSet; 
    sigemptyset(&stSet);  /* clear signals */
    /* Inherited signals, recv all signals */
    if (sigprocmask(SIG_SETMASK, &stSet, NULL) == -1)
    {
        /* now not exit */
        CLog::Log(LOG_LEVEL_NOTICE, "Worker process Set signals failed");
        exit(0);
    }
    g_ThreadPool.Init();    /* create thread pool*/
    g_MessageCtl.Init();    /* create send message thread, sem_wait send message */
    g_Socket.Init();
}

uint32_t CProcessCtl::SetProcTitle(const char *pTitle)
{
    CLog::Log(LOG_LEVEL_TRACK, "CProcessCtl::SetProcTitle track");

    uint32_t titleLen = strlen(pTitle); /* get pTitle length */
    uint32_t totalLen = CMainArgCtl::GetInstance()->GetArgvNeedMemSize() + CMainArgCtl::GetInstance()->GetEnvNeedMemSize();
    /* totalLen is max pTitle can be */
    if(totalLen <= titleLen)
    {
        /* new pTitle length to bit */
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Set the process pTitle to bit, titleLen = %d", titleLen);
        return XiaoHG_ERROR;
    }
    /* set the end */
    CMainArgCtl::m_pOsArgv[1] = NULL;
    char *ptmp = CMainArgCtl::m_pOsArgv[0];
    strcpy(ptmp, pTitle);
    ptmp += titleLen;
    size_t more = totalLen - titleLen;   /* clean more memory */
    memset(ptmp, 0, more);
    
    return XiaoHG_SUCCESS;
}