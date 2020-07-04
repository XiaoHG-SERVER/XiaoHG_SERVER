
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "XiaoHG_C_MainArgCtl.h"
#include "XiaoHG_error.h"
#include "XiaoHG_C_Log.h"

#define __THIS_FILE__ "XiaoHG_C_MainArgCtl.cxx"

CMainArgCtl* CMainArgCtl::m_Instance = nullptr;
char** CMainArgCtl::m_pOsArgv = nullptr;
CMainArgCtl::CMainArgCtl()
{
}

CMainArgCtl::~CMainArgCtl()
{
    if(m_pEnvMem != NULL)
    {
        delete m_pEnvMem;
    }
}

void CMainArgCtl::Init(uint32_t argc, char *argv[])
{
    m_OsArgc = argc;
    m_pOsArgv = argv;
    /* get commend line size */
    for(int i = 0; i < m_OsArgc; i++)
    {
        /* +1 for '\0' */
        m_ArgvNeedMemSize += strlen(m_pOsArgv[i]) + 1;
    }/* end for */

    /* Statistics memory occupied by environment variables */
    for(int i = 0; environ[i]; i++) 
    {
        /* +1 for '\0' */
        m_EnvNeedMemSize += strlen(environ[i]) + 1; 
    }/* end for */

    m_pEnvMem = new char[m_EnvNeedMemSize]; 
    memset(m_pEnvMem, 0, m_EnvNeedMemSize);
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

size_t CMainArgCtl::GetArgvNeedMemSize()
{
    return m_ArgvNeedMemSize;
}

size_t CMainArgCtl::GetEnvNeedMemSize()
{
    return m_EnvNeedMemSize;
}