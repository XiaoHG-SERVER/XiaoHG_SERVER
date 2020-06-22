
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

#define __THIS_FILE__ "XiaoHG_Event.cxx"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ProcessEventsAndTimers
 * discription: Handle network iEpollEvents and timer iEpollEvents
 * parameter:
 * =================================================================*/
void ProcessEventsAndTimers()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "ProcessEventsAndTimers track");

    /* -1: always waiting */
    g_LogicSocket.EpolWaitlProcessEvents(-1);
    /* printf dbg information */
    g_LogicSocket.PrintTDInfo();
}

