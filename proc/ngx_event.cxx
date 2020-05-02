
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h> 
#include <errno.h> 
#include <unistd.h>
#include "ngx_func.h"
#include "ngx_macro.h"
#include "ngx_c_conf.h"

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
    /* -1: always waiting */
    g_LogicSocket.EpolWaitlProcessEvents(-1);
    /* printf dbg information */
    g_LogicSocket.PrintTDInfo();
}

