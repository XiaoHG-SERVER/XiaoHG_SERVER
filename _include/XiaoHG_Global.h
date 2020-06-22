
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_GLOBAL_H__
#define __XiaoHG_GLOBAL_H__

#include <signal.h> 
#include "XiaoHG_C_SLogic.h"
#include "XiaoHG_C_ThreadPool.h"
#include "XiaoHG_C_MainArgCtl.h"

/* XiaoHG server process exit error code */
extern int iExitCode;

extern CLogicSocket g_LogicSocket;
extern CThreadPool g_ThreadPool;
extern CMainArgCtl g_MainArgCtl;

extern pid_t g_iCurPid;
extern pid_t g_iParentPid;
extern int g_iProcessID;
extern bool g_bIsStopEvent;

#endif //!__XiaoHG_GLOBAL_H__
