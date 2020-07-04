
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/wait.h> 
#include <sys/types.h>
#include <sys/stat.h>
#include "XiaoHG_C_RunInit.h"
#include "XiaoHG_error.h"
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_C_Log.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_Signal.h"
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_C_Crc32.h"
#include "XiaoHG_C_ProcessCtl.h"
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_C_MainArgCtl.h"

#define __THIS_FILE__ "XiaoHG_C_RunInit.cxx"

sem_t g_SendMsgPushEventSem; /* Handle the semaphore related to the message thread */

CConfig* CRun::m_pConfig = CConfig::GetInstance();
CRun *CRun::m_Instance = nullptr;
CRun::CRun()
{
    Init();
}

CRun::~CRun()
{
    sem_destroy(&g_SendMsgPushEventSem); /* Semaphore related thread release */
}

void CRun::Init()
{
    CLog::GetInstance();
    m_pSignalCtl = new CSignalCtl();
    /* This sem init place can be thread pool class */
    if(sem_init(&g_SendMsgPushEventSem, 0, 0) == -1)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "sem_init(g_SendMsgPushEventSem) failed");
        exit(0);
    }
}

void CRun::Runing(int argc, char *argv[])
{
    CLog::Log(LOG_LEVEL_TRACK, "CRun::Runing track");
    CMainArgCtl::GetInstance()->Init(argc, argv);
    /* Determine whether to run as a daemon */
    if(m_pConfig->GetIntDefault("Daemon", 0))
    {
        switch (g_ProcessCtl.InitDaemon())
        {
        case XiaoHG_ERROR:
            CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "InitDaemon() failed");
            exit(0);
            break;
        case 1:/* exit parent process */
            CLog::Log("InitDaemon() exit parent process");
            exit(0);
            break;
        default:
            CLog::Log("Go to worker process");
            break;
        }
    }

    /* syclc process */
    g_ProcessCtl.MasterProcessCycle();
}

