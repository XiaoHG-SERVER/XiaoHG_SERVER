
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <string.h>
#include "XiaoHG_Global.h"
#include "XiaoHG_Func.h"
#include "XiaoHG_Macro.h"

#define __THIS_FILE__ "XiaoHG_SetProcTitile.cxx"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: InitSetProcTitle
 * discription: Set executable program pTitle related functions, 
 *              allocate memory, and copy environment variables 
 *              to new memory.
 * =================================================================*/
void InitSetProcTitle()
{ 
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "InitSetProcTitle track");

    g_pEnvMem = new char[g_uiEnvNeedMem]; 
    memset(g_pEnvMem, 0, g_uiEnvNeedMem);
    char *ptmp = g_pEnvMem;
    /* move memory */
    for (int i = 0; environ[i]; i++) 
    {
        size_t size = strlen(environ[i]) + 1;
        strcpy(ptmp, environ[i]);   /* move */
        environ[i] = ptmp;          /* point new memory */
        ptmp += size;
    }

    CLog::Log("Init set Process Title successful");
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: SetProcTitle
 * discription: set process pTitle
 * =================================================================*/
int SetProcTitle(const char *pTitle)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "SetProcTitle track");

    size_t iTitleLen = strlen(pTitle);                       /* get pTitle length */
    size_t iTitolLen = g_uiArgvNeedMem + g_uiEnvNeedMem;    /* argv and environ memory */
 
    /* iTitolLen is max pTitle can be */
    if(iTitolLen <= iTitleLen)
    {
        /* new pTitle length to bit */
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Set the process pTitle to bit, iTitleLen = %d", iTitleLen);
        return XiaoHG_ERROR;
    }

    /* set the end */
    g_pOsArgv[1] = NULL;
    char *ptmp = g_pOsArgv[0];
    strcpy(ptmp, pTitle);
    ptmp += iTitleLen;
    size_t more = iTitolLen - iTitleLen;   /* clean more memory */
    memset(ptmp, 0, more);
    
    CLog::Log("Set the process pTitle \"%s\" successful", pTitle);
    return XiaoHG_SUCCESS;
}