
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_FUNC_H__
#define __XiaoHG_FUNC_H__

#include <signal.h>

/* Remove spaces before and after the string */
void Rtrim(char *string);
void Ltrim(char *string);

void MasterProcessCycle();
void ProcessEventsAndTimers();
void SignalHandler(int iSigNo, siginfo_t *pSigInfo, void *pContext);

/* Recyclc */
void ProcessExitFreeResource();

#endif //!__XiaoHG_FUNC_H__