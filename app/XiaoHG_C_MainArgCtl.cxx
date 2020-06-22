
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "XiaoHG_C_MainArgCtl.h"
#include "XiaoHG_error.h"
#include "XiaoHG_C_Log.h"

#define __THIS_FILE__ "XiaoHG_C_MainArgCtl.cxx"

CMainArgCtl::CMainArgCtl(){}

CMainArgCtl::~CMainArgCtl()
{
    if(m_pEnvMem != NULL)
    {
        delete m_pEnvMem;
    }
}

void CMainArgCtl::Init(uint32_t argc, char *argv[])
{
    m_iOsArgc = argc;
    m_pOsArgv = argv;

    /* get commend line size */
    for(int i = 0; i < m_iOsArgc; i++)
    {
        /* +1 for '\0' */
        m_uiArgvNeedMem += strlen(m_pOsArgv[i]) + 1;
    }/* end for */

    /* Statistics memory occupied by environment variables */
    for(int i = 0; environ[i]; i++) 
    {
        /* +1 for '\0' */
        m_uiEnvNeedMem += strlen(environ[i]) + 1; 
    }/* end for */

    m_pEnvMem = new char[m_uiEnvNeedMem]; 
    memset(m_pEnvMem, 0, m_uiEnvNeedMem);
    char *pTmp = m_pEnvMem;
    /* move memory */
    for(int i = 0; environ[i]; i++) 
    {
        size_t size = strlen(environ[i]) + 1;
        strcpy(pTmp, environ[i]);   /* move */
        environ[i] = pTmp;          /* point new memory */
        pTmp += size;
    }
}

uint32_t CMainArgCtl::SetProcTitle(const char *pTitle)
{
    uint32_t iTitleLen = strlen(pTitle);                       /* get pTitle length */
    uint32_t iTitolLen = m_uiArgvNeedMem + m_uiEnvNeedMem;    /* argv and environ memory */
 
    /* iTitolLen is max pTitle can be */
    if(iTitolLen <= iTitleLen)
    {
        /* new pTitle length to bit */
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Set the process pTitle to bit, iTitleLen = %d", iTitleLen);
        return XiaoHG_ERROR;
    }

    /* set the end */
    m_pOsArgv[1] = NULL;
    char *ptmp = m_pOsArgv[0];
    strcpy(ptmp, pTitle);
    ptmp += iTitleLen;
    size_t more = iTitolLen - iTitleLen;   /* clean more memory */
    printf("ptmp = %p\n", ptmp);
    memset(ptmp, 0, more);
    
    return XiaoHG_SUCCESS;
}