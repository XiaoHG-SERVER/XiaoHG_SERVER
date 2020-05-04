
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_Log.h"
#include "XiaoHG_Func.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_Global.h"

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
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CMemory::AllocMemory track");

	void *pTmpData = (void *)new char[iMemCount];
    if(pTmpData != NULL)
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_DEBUG, 0, "Alloc memory is: %p", pTmpData);
    }
    else
    {
        XiaoHG_Log(LOG_FILE, LOG_LEVEL_ERR, errno, "Alloc memory failed");
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
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CMemory::FreeMemory track");

    XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, errno, "FreeMemory, p = %p", p);
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, errno, "FreeMemory enter, p = %s", p);
    delete [] ((char *)p);
    p = NULL;
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, errno, "FreeMemory over, p = %s", p);
}

