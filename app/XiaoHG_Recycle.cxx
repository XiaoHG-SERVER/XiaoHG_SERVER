
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <unistd.h>
#include "XiaoHG_Global.h"
#include "XiaoHG_C_Log.h"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: FreeResource
 * discription: source free when process exit
 * =================================================================*/
void ProcessExitFreeResource()
{
    /* Function track */
    CLog::Log(LOG_LEVEL_TRACK, "ProcessExitFreeResource track");
}