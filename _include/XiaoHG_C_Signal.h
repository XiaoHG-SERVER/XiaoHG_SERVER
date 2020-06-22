
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_C_SIGNAL_H__
#define __XiaoHG_C_SIGNAL_H__

#include <signal.h>

/* signals struct */
typedef struct 
{
    int iSigNo;              /* signal code */
    const char *pSigName;    /* signal string, such: SIGHUP */
    /* signal LogicHandlerCallBack */
    void (*LogicHandlerCallBack)(int iSigNo, siginfo_t *pSigInfo, void *pContext);
}SIGNAL_T;

class CSignalCtl
{
public:
    CSignalCtl(){}
    ~CSignalCtl(){}
    CSignalCtl(uint32_t* pulSigs);

public:
    friend void SignalHandler(int iSigNo, siginfo_t *pSigInfo, void *pContext);
private:
    static void ProcessGetStatus();

public:
    void Init();
    uint32_t RegisterSignals();
    uint32_t SetSigEmpty();
    uint32_t SetSigSuspend();
    uint32_t AddSig(uint32_t ulSignalNo);
    uint32_t DelSig(uint32_t ulSignalNo);
    uint32_t IsSigMember(uint32_t ulSignalNo);
    uint32_t AddSigPending();

private:
    sigset_t m_stSet;
    uint32_t* m_ulSigs;
};




#endif //!__XiaoHG_C_SIGNAL_H__