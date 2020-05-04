
#include <stddef.h>
#include <unistd.h>
#include "XiaoHG_Global.h"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: FreeResource
 * discription: source free when process exit
 * =================================================================*/
void ProcessExitFreeResource()
{
    /* free Save global environment variables*/
    if(g_pEnvMem)
    {
        delete []g_pEnvMem;
        g_pEnvMem = NULL;
    }

    /* close log file */
    if(g_stLog.iLogFd != STDERR_FILENO && g_stLog.iLogFd != -1)  
    {
        close(g_stLog.iLogFd);
        g_stLog.iLogFd = -1;
    }
}