
#ifndef __XiaoHG_FUNC_H__
#define __XiaoHG_FUNC_H__

/* Remove spaces before and after the string */
void Rtrim(char *string);
void Ltrim(char *string);

/* Log */
int LogInit();
void SetLogLevel(int iErrorLevel);
void XiaoHG_Log(int iLogType, int iLogLevel, int iErrCode, const char* pFmt, ...);

/* Process header LogicHandlerCallBack */
void InitSetProcTitle();
int SetProcTitle(const char *title);

/* Signal */
int InitSignals();
void MasterProcessCycle();
int DaemonInit();
void ProcessEventsAndTimers();

#endif //!__XiaoHG_FUNC_H__