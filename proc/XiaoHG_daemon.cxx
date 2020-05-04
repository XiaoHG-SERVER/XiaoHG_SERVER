
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h> 
#include <sys/stat.h>
#include <fcntl.h>
#include "XiaoHG_Func.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_C_Conf.h"

#define __THIS_FILE__ "XiaoHG_Deamon.cxx"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: DaemonInit
 * discription: init daemon
 * =================================================================*/
int DaemonInit()
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "DaemonInit track");

    switch (fork())
    {
    case -1:/* fork failed */
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "create subsprocess failed");
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
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "subsprocess set sid failed");
        return XiaoHG_SUCCESS;
    }

    /* 0777 */
    umask(0); 

    /* /dev/null opration */
    int iSockFd = open("/dev/null", O_RDWR);
    if (iSockFd == -1) 
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "open \"/dev/null\" failed");
        return XiaoHG_ERROR;
    }

    /* First close STDIN_FILENO */
    if (dup2(iSockFd, STDIN_FILENO) == -1)
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "dup2(STDIN) failed");
        return XiaoHG_ERROR;
    }

    /* and than close STDIN_FILENO */
    if (dup2(iSockFd, STDOUT_FILENO) == -1) 
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "dup2(STDOUT_FILENO) failed");
        return XiaoHG_ERROR;
    }

    /* now iSockFd = 3 */
    if (iSockFd > STDERR_FILENO)  
    {
        if (close(iSockFd) == -1)  
        {
            XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "close file handle: %d failed", iSockFd);
            return XiaoHG_ERROR;
        }
    }
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "Init deamon process successful");
    /* subprocess return */
    return XiaoHG_SUCCESS; 
}

