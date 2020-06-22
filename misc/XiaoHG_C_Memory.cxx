
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_Func.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_Global.h"
#include "XiaoHG_C_Log.h"

#define __THIS_FILE__ "XiaoHG_C_Memory.cxx"

CMemory *CMemory::m_Instance = NULL;

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: AllocMemory
 * discription: alloc
 * parameter:
 * =================================================================*/
void* CMemory::AllocMemory(int iMemCount, bool bIsMemSet)
{

	void *pTmpData = (void *)new char[iMemCount];
    if(pTmpData == NULL)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, "Alloc memory failed");
        return NULL;
    }
    
    /* Requires memory to be cleared */
    if(bIsMemSet)
    {
	    memset(pTmpData, 0, iMemCount);
    }
	return pTmpData;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: FreeMemory
 * discription: free
 * parameter:
 * =================================================================*/
void CMemory::FreeMemory(void *p)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CMemory::FreeMemory track");

    CLog::Log("FreeMemory enter, p = %s", p);
    delete [] ((char *)p);
    p = NULL;
}

