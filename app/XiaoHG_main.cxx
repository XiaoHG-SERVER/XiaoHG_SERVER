
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include "XiaoHG_Func.h"
#include "XiaoHG_C_MainArgCtl.h"
#include "XiaoHG_C_RunInit.h"
#include "XiaoHG_C_SLogic.h"
#include "XiaoHG_C_ThreadPool.h"

#define __THIS_FILE__ "XiaoHG_main.cxx"

CMainArgCtl g_MainArgCtl;

/* socket/thread pool */
CLogicSocket g_LogicSocket;          /* socket obj */
CThreadPool g_ThreadPool;       /* thread pool objs */

/* about process ID */
pid_t g_iCurPid;        /* current pid */
pid_t g_iParentPid;     /* parent pid */
int g_iProcessID;       /* process id, made myself */
bool g_bIsStopEvent;    /* process exit(1) or 0 */

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.24
 * test time: 2020.04.24 pass
 * function name: main
 * =================================================================*/
int main(int argc, char *argv[])
{
    CRun* run = new CRun();
    run->Runing(argc, argv);
}


