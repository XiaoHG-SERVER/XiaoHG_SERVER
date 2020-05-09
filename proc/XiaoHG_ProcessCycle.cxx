
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include "XiaoHG_Func.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_C_Conf.h"

#define __THIS_FILE__ "XiaoHG_ProcessCycle.cxx"

#define MAX_PROCESS_TITILE_LEN  1000
#define WORKER_PROCESS_TITLE    "XiaoHG_SERVER worker process"
#define MASTER_PROCESS_TITLE    "XiaoHG_SERVER master process"

static void StartWorkerProcesses(int iThreadNums);
static int SpawnProcess(int iThreadNums);
static void WorkerProcessCycle(int iWorkProcNo);
static void WorkerProcessInit(int iWorkProcNo);

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: MasterProcessCycle
 * discription: create worker process
 * =================================================================*/
void MasterProcessCycle()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "MasterProcessCycle track");

    sigset_t stSet;        /* signal stSet */
    sigemptyset(&stSet);   /* clrean signals */

    /* The following signals are not expected to be received during the execution of this function 
        (protect the critical section of code that is not expected to be interrupted by the signal) */
    /* fork()This method is used when the child process prevents signal interference */
    /* Here is to prevent the phenomenon that the creation process fails due to receiving some 
        signals when creating the child process, to prevent signal interference */
    /* You can add other signals to be shielded according to actual needs */
    sigaddset(&stSet, SIGCHLD);     /* Child process state changes */
    sigaddset(&stSet, SIGALRM);     /* Timer timeout */
    sigaddset(&stSet, SIGIO);       /* Asynchronous I / O */
    sigaddset(&stSet, SIGINT);      /* Terminal break */
    sigaddset(&stSet, SIGHUP);      /* Disconnect */
    sigaddset(&stSet, SIGUSR1);     /* User-defined signals */
    sigaddset(&stSet, SIGUSR2);     /* User-defined signals */
    sigaddset(&stSet, SIGWINCH);    /* Terminal window size changes */
    sigaddset(&stSet, SIGTERM);     /* termination */
    sigaddset(&stSet, SIGQUIT);     /* Terminal exit character */
    
    /* Shield signal */
    if(sigprocmask(SIG_BLOCK, &stSet, NULL) == -1)
    {
        CLog::Log(LOG_LEVEL_NOTICE, "Shield signal failed");
    }

    /* stSet title */
    SetProcTitle(MASTER_PROCESS_TITLE);
    /* make some master msg to log file */

    /* get the worker process numbers from config file */
    CConfig *pConfig = CConfig::GetInstance();
    /* if not stSet WorkerProcessesCount in the config file, the function 
     * return second parameter */
    int iWorkProcessCount = pConfig->GetIntDefault("WorkerProcessesCount", 1);
    
    /* create worker process */
    StartWorkerProcesses(iWorkProcessCount);

    /* this is master process cycle */
    sigemptyset(&stSet);
    
    for ( ;; ) 
    {
        /* Blocked here, waiting for a signal, at this time the process is suspended, 
         * does not occupy CPU time, only to receive the signal will be awakened (return)
         * At this time, the master process is completely driven by signals. */
        CLog::Log("Master process wait a signal");
        sigsuspend(&stSet);
        /* more for future */
    }/*  end for(;;) */
    
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: StartWorkerProcesses
 * discription: start create worker process
 * parameter:
 *  int iThreadNums: need to create worker process numbers
 * =================================================================*/
static void StartWorkerProcesses(int iWorkProcessCount)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "StartWorkerProcesses track");

    /* master进程在走这个循环，来创建若干个子进程 */
    for(int i = 0; i < iWorkProcessCount; i++)
    {
        SpawnProcess(i);
    } /* end for */
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: SpawnProcess
 * discription: create a worker process
 * parameter:
 * =================================================================*/
static int SpawnProcess(int iWorkProcNo)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "SpawnProcess track");

    pid_t pid = fork();
    /* 0: Child process, or: parent process*/
    switch (pid)
    {
    case -1:/* failde */
        CLog::Log(LOG_LEVEL_ERR, errno, "Fork subprocess failed, iWorkProcNo: %d", iWorkProcNo);
        return XiaoHG_ERROR;
    case 0:/* Child process */
        /* change process id*/
        g_iParentPid = g_iCurPid;
        g_iCurPid = getpid();
        /* worker process cycle*/
        WorkerProcessCycle(iWorkProcNo); 
        break;
    default:/* parent process */           
        break;
    }/* end switch */
    /* turn back to parent process */
    return pid;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: WorkerProcessCycle
 * discription: worker process work
 * parameter:
 * =================================================================*/
static void WorkerProcessCycle(int iWorkProcNo) 
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "WorkerProcessCycle track");

    /* stSet process id, this is worker process id = WORKER_PROCESS */
    g_iProcessID = WORKER_PROCESS;
    /* stSet worker process name */
    SetProcTitle(WORKER_PROCESS_TITLE);
    /* Init worker process*/
    WorkerProcessInit(iWorkProcNo);
    /* recod some info to log file */
    for(;;)
    {
        /* Handle network iEpollEvents and timer iEpollEvents */
        /* Event-driven processing */
        ProcessEventsAndTimers();
    } /* end for(;;) */

    /* free sourcd */
    g_ThreadPool.StopAll(); 
    g_LogicSocket.ShutdownSubProc(); 
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: WorkerProcessInit
 * discription: Init worker process
 * parameter:
 * =================================================================*/
static void WorkerProcessInit(int iWorkProcNo)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "WorkerProcessInit track");

    sigset_t stSet; 
    sigemptyset(&stSet);  /* clear signals */
    /* Inherited signals, recv all signals */
    if (sigprocmask(SIG_SETMASK, &stSet, NULL) == -1)
    {
        /* now not exit */
        CLog::Log(LOG_LEVEL_NOTICE, "Worker process (%d) Set signals failed", iWorkProcNo);
    }

    CConfig *pConfig = CConfig::GetInstance();
    /* get the recv msg thread numbers from config file */
    int iThreadCount = pConfig->GetIntDefault("ProcMsgRecvWorkThreadCount", 5);   
    /* create thread pool*/
    if(g_ThreadPool.Create(iThreadCount) != XiaoHG_SUCCESS)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Worker process (%d) create thread pool failed", iWorkProcNo);
        exit(-2);
    }
    /* more 1 sec insure all thread is runings */
    sleep(1);

    /* Init */
    if(g_LogicSocket.InitializeSubProc() != XiaoHG_SUCCESS) 
    {
        CLog::Log("Worker process (%d) InitializeSubProc failed", iWorkProcNo);
        exit(-2);
    }
    
    /* Init Epoll */
    if(g_LogicSocket.EpollInit() != XiaoHG_SUCCESS)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Worker process (%d) EpollInit failed", iWorkProcNo);
        exit(-2);
    }

    CLog::Log("Worker process (%d) Init successful", iWorkProcNo);
    return;
}
