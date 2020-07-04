
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdint.h>
#include <stddef.h>

#ifndef __XiaoHG_C_MAINARGCTL_H__
#define __XiaoHG_C_MAINARGCTL_H__

#define WORKER_PROCESS_TITLE "XiaoHG_SERVER worker process"
#define MASTER_PROCESS_TITLE "XiaoHG_SERVER master process"

class CMainArgCtl
{
private:
    static CMainArgCtl* m_Instance;
private:
    CMainArgCtl();
public:
    virtual ~CMainArgCtl();
    static CMainArgCtl* GetInstance()
    {
        if (m_Instance == nullptr)
        {
            /* lock */
            if (m_Instance == nullptr)
            {
                m_Instance = new CMainArgCtl();
                static CDeleteInstance cl;
            }
        }
        return m_Instance;
    }

    class CDeleteInstance
    {
    public:
        ~CDeleteInstance()
        {
            if(CMainArgCtl::m_Instance != nullptr)
            {
                delete CMainArgCtl::m_Instance;
                CMainArgCtl::m_Instance = nullptr;
            }
        }
    };
    

public:
    void Init(uint32_t argc, char *argv[]);
    uint32_t SetProcTitle(const char *pTitle);
    size_t GetArgvNeedMemSize();
    size_t GetEnvNeedMemSize();

public:
    static char** m_pOsArgv;                /* Original command line parameter array */

private:
    char* m_pEnvMem;                 /* Point to the memory of the environment variable allocated by yourself */
    size_t m_ArgvNeedMemSize;          /* argv need memory size */
    size_t m_EnvNeedMemSize;           /* Memory occupied by environment variables */
    uint32_t m_OsArgc;                   /* Number of command line parameters */
};


#endif //!__XiaoHG_C_MAINARGCTL_H__