
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdint.h>

#ifndef __XiaoHG_C_MAINARGCTL_H__
#define __XiaoHG_C_MAINARGCTL_H__

#define WORKER_PROCESS_TITLE    "XiaoHG_SERVER worker process"
#define MASTER_PROCESS_TITLE    "XiaoHG_SERVER master process"

class CMainArgCtl
{
private:
    char **m_pOsArgv;                /* Original command line parameter array */
    char *m_pEnvMem;                 /* Point to the memory of the environment variable allocated by yourself */
    size_t m_uiArgvNeedMem;          /* argv need memory size */
    size_t m_uiEnvNeedMem;           /* Memory occupied by environment variables */
    uint32_t m_iOsArgc;                   /* Number of command line parameters */

public:
    CMainArgCtl();
    virtual ~CMainArgCtl();

public:
    void Init(uint32_t argc, char *argv[]);
    uint32_t SetProcTitle(const char *pTitle);
};


#endif //!__XiaoHG_C_MAINARGCTL_H__