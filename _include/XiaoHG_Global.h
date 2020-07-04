
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_GLOBAL_H__
#define __XiaoHG_GLOBAL_H__

#include<semaphore.h>
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_C_ThreadPool.h"
#include "XiaoHG_C_BusinessCtl.h"
#include "XiaoHG_C_MessageCtl.h"
#include "XiaoHG_C_ProcessCtl.h"

extern CSocket g_Socket;
extern CThreadPool g_ThreadPool;
extern CBusinessCtl g_BusinessCtl;
extern CMessageCtl g_MessageCtl;
extern CProcessCtl g_ProcessCtl;

extern sem_t g_SendMsgPushEventSem; /* Send Message sem*/
extern uint32_t g_LenPkgHeader;     /* packet header size */
extern uint32_t g_LenMsgHeader;     /* Message header size */

#endif //!__XiaoHG_GLOBAL_H__
