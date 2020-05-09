
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
#include <arpa/inet.h>
#include <sys/time.h>
#include <XiaoHG_Macro.h>
#include <XiaoHG_Func.h>
#include <XiaoHG_C_Conf.h>
#include <XiaoHG_C_Socket.h>
#include <XiaoHG_C_Memory.h>
#include <XiaoHG_C_ThreadPool.h>
#include <XiaoHG_C_Crc32.h>
#include <XiaoHG_C_SLogic.h>

#define __THIS_FILE__ "XiaoHG.cxx"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.24
 * test time: 2020.04.24 pass
 * function name: main
 * =================================================================*/
int main(int argc, char *argv[])
{
    /* Iint a object config */
    CConfig *pConfig = CConfig::GetInstance();
    /* Init global values */
    XiaoHG_Init(argc, argv);
    
    /* signal init */
    if(InitSignals() != XiaoHG_SUCCESS) 
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "InitSignals() failed");
        iExitCode = 1;
        goto lblexit;
    }

    /* Init socket */
    if(g_LogicSocket.Initalize() == XiaoHG_ERROR)
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "g_LogicSocket.Initalize() failed");
        iExitCode = 2;
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
            CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "DaemonInit() failed");
            iExitCode = 3;
            goto lblexit;
        }
        /* parent return 1 */
        if(iCdaemonResult == 1)
        {
            /* exit parent process */
            ProcessExitFreeResource();
            return 0;
        }
        /* the master process change, is daemon master process */
        g_iDaemonized = 1;
    }
    
    /* syclc process */
    MasterProcessCycle();

lblexit:
    /* free source */
    ProcessExitFreeResource();
    CLog::Log("XiaoHG_SERVER process exit, bye!");
    return iExitCode;
}


