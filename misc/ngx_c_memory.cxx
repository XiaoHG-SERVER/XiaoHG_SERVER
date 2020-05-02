
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ngx_c_memory.h"
#include "XiaoHG_Log.h"
#include "ngx_func.h"
#include "ngx_macro.h"
#include "ngx_global.h"

CMemory *CMemory::m_Instance = NULL;

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: AllocMemory
 * discription: alloc
 * parameter:
 * =================================================================*/
void *CMemory::AllocMemory(int iMemCount, bool bIsMemSet)
{
	void *pTmpData = (void *)new char[iMemCount];
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
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "FreeMemory, p = %p", p);
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "FreeMemory enter, p = %s", p);
    delete [] ((char *)p);
    p = NULL;
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "FreeMemory over, p = %s", p);
}

