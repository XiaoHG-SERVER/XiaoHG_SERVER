
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdint.h>
#include <stdint.h>
#include <signal.h>
#include <map>

class CSignalCtl;
class CConfig;

#ifndef __XiaoHG_C_RUNINIT_H__
#define __XiaoHG_C_RUNINIT_H__

class CRun
{
private:
    CRun();

public:
    ~CRun();

private:
    static CRun *m_Instance;
/* Singleton implementation */
public:
    static CRun* GetInstance()
    {
        if (m_Instance == NULL)
        {
            if (m_Instance == NULL)
            {
                m_Instance = new CRun();
                static CDeleteInstance ci;
            }
        }
        return m_Instance;
    }
    class CDeleteInstance
    {
    public:
        ~CDeleteInstance()
        {
            if (CRun::m_Instance != NULL)
            {
                delete CRun::m_Instance;
                CRun::m_Instance = NULL;
            }
        }
    };

private:
    void Init();

public:
    void Runing(int argc, char *argv[]);

private:
    static CConfig* m_pConfig;
    CSignalCtl* m_pSignalCtl;
};

#endif //!__XiaoHG_C_RUNINIT_H__