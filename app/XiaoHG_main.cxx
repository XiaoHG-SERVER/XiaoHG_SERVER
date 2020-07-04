
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include "XiaoHG_C_MainArgCtl.h"
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_C_ThreadPool.h"
#include "XiaoHG_C_BusinessCtl.h"
#include "XiaoHG_C_MessageCtl.h"
#include "XiaoHG_C_MainArgCtl.h"
#include "XiaoHG_C_ProcessCtl.h"
#include "XiaoHG_C_RunInit.h"

#define __THIS_FILE__ "XiaoHG_main.cxx"

class CRun;

CSocket g_Socket;
CThreadPool g_ThreadPool;
CBusinessCtl g_BusinessCtl;
CMessageCtl g_MessageCtl;
CProcessCtl g_ProcessCtl;

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.24
 * test time: 2020.04.24 pass
 * function name: main
 * =================================================================*/
int main(int argc, char *argv[])
{
    CRun* run = CRun::GetInstance();
    run->Runing(argc, argv);
}


