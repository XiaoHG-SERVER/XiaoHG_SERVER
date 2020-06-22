
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdint.h>
#include <stdint.h>
#include <signal.h>
#include <map>

class CMainArgCtl;
class CSignalCtl;

#ifndef __XiaoHG_C_RUNINIT_H__
#define __XiaoHG_C_RUNINIT_H__

class CRun
{
public:
    CRun();
    ~CRun();

private:
    void Init();

public:
    void Runing(int argc, char *argv[]);

private:
    uint32_t DaemonInit();

private:
    CMainArgCtl* m_pMainArgCtl;
    CSignalCtl* m_pSignalCtl;
};

#endif //!__XiaoHG_C_RUNINIT_H__