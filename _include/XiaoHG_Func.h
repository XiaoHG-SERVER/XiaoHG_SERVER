﻿
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_FUNC_H__
#define __XiaoHG_FUNC_H__

/* Remove spaces before and after the string */
void Rtrim(char *string);
void Ltrim(char *string);

/* Process header LogicHandlerCallBack */
void XiaoHG_Init(int argc, char *argv[]);
void InitSetProcTitle();
int SetProcTitle(const char *title);

/* Signal */
int InitSignals();
void MasterProcessCycle();
int DaemonInit();
void ProcessEventsAndTimers();

/* Recyclc */
void ProcessExitFreeResource();

#endif //!__XiaoHG_FUNC_H__