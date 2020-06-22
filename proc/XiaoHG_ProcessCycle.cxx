
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
#include "XiaoHG_C_Log.h"
#include "XiaoHG_error.h"
#include "XiaoHG_C_Signal.h"

#define __THIS_FILE__ "XiaoHG_ProcessCycle.cxx"

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
    uint32_t ulSigs[] = {SIGCHLD, SIGALRM, SIGIO, SIGINT, SIGHUP, 
                            SIGUSR1, SIGUSR2, SIGWINCH, SIGTERM, SIGQUIT};
    CSignalCtl sigCtl(ulSigs);
    sigCtl.RegisterSignals();
    /* stSet title */
    g_MainArgCtl.SetProcTitle(CConfig::GetInstance()->GetString("MasterProcessTitle"));

    /* get the worker process numbers from config file */
    /* if not stSet WorkerProcessesCount in the config file, the function 
     * return second parameter */
    /* create worker process */
    StartWorkerProcesses(CConfig::GetInstance()->GetIntDefault("WorkerProcessesCount", 1));

    /* this is master process cycle */
    sigCtl.SetSigEmpty();
    
    for ( ;; ) 
    {
        /* Blocked here, waiting for a signal, at this time the process is suspended, 
         * does not occupy CPU time, only to receive the signal will be awakened (return)
         * At this time, the master process is completely driven by signals. */
        CLog::Log("Master process wait a signal");
        sigCtl.SetSigSuspend();
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
    g_MainArgCtl.SetProcTitle(CConfig::GetInstance()->GetString("WorkerProcessTitle"));
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

    /* create thread pool*/
    if(g_ThreadPool.Init() != XiaoHG_SUCCESS)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Worker process (%d) create thread pool failed", iWorkProcNo);
        exit(-2);
    }

    /* Sub process init */
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
