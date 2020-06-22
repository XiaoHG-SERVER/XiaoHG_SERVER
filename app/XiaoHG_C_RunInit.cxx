
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
#include "XiaoHG_C_Crc32.h"
#include "XiaoHG_Func.h"
#include "XiaoHG_C_Signal.h"
#include "XiaoHG_C_Conf.h"

#define __THIS_FILE__ "XiaoHG_C_RunInit.cxx"

CRun::CRun()
{
    Init();
}

CRun::~CRun(){}

void CRun::Init()
{
    g_iProcessID = MASTER_PROCESS;      /* this is master process */
    g_iCurPid = getpid();               /* Current process pid */
    g_iParentPid = getppid();           /* parent process pid */
    g_bIsStopEvent = false;             /* 1: exit, 0: nothing*/

    m_pMainArgCtl = new CMainArgCtl();
    m_pSignalCtl = new CSignalCtl();

    /* init object */
    CConfig::GetInstance();
    CLog::GetInstance();
    CMemory::GetInstance();
    CCRC32::GetInstance();
}

void CRun::Runing(int argc, char *argv[])
{
    //m_pMainArgCtl->Init(argc, argv);
    g_MainArgCtl.Init(argc, argv);
    m_pSignalCtl->Init();

    /* Determine whether to run as a daemon */
    if(CConfig::GetInstance()->GetIntDefault("Daemon", 0))
    {
        switch (DaemonInit())
        {
        case XiaoHG_ERROR:
            CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "DaemonInit() failed");
            exit(0);
            break;
        case 1:/* exit parent process */
            exit(0);
            break;
        default:
            break;
        }
    }

    /* syclc process */
    MasterProcessCycle();
}

uint32_t CRun::DaemonInit()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "DaemonInit track");

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

    /* first change pid */
    g_iParentPid = g_iCurPid;
    g_iCurPid = getpid();
    
    /* The terminal is closed and the terminal is closed, 
     * which will have nothing to do with this subprocess */
    if (setsid() == -1)  
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "subsprocess set sid failed");
        return XiaoHG_SUCCESS;
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
