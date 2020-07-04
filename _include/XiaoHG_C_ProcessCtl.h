
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_C_PROCESSCTL_H__
#define __XiaoHG_C_PROCESSCTL_H__

#include <stdint.h>

class CConfig;
class CMainArgCtl;
class CSocket;

class CProcessCtl
{
private:
    /* data */
public:
    CProcessCtl(/* args */);
    ~CProcessCtl();

public:
    void Init();
    void MasterProcessCycle();
    uint32_t SetProcTitle(const char *pTitle);
    uint32_t InitDaemon();

private:
    void WorkerProcessCycle();
    void WorkerProcessInit();

public:
    static CConfig* m_pConfig;
    static uint32_t m_ProcessID;   /* process id, made myself */

private:
    char* m_pMaterTitle;
    char* m_pWorkerTitle;
    uint32_t m_WorkProcessCount;
    char** m_pOsArgv;
};


#endif //!__XiaoHG_C_PROCESSCTL_H__