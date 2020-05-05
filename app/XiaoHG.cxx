
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
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
#include "XiaoHG_Func.h"
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_ThreadPool.h"
#include "XiaoHG_C_Crc32.h"
#include "XiaoHG_C_SLogic.h"

#define __THIS_FILE__ "XiaoHG.cxx"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.24
 * test time: 2020.04.24 pass
 * function name: main
 * =================================================================*/
int main(int argc, char *argv[])
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "main track");

    /* Init global values */
    XiaoHG_Init(argc, argv);

    /* load config file */
    CConfig *pConfig = CConfig::GetInstance();
    if(pConfig->Load("XiaoHG_SERVER.conf") == XiaoHG_ERROR)
    {
        if(LogInit() == XiaoHG_ERROR)
        {
            goto lblexit;
        }
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "Load config file failed");
        goto lblexit;
    }

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
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_NOTICE, 0, "hold process exit, byb");
    return 0;
}


