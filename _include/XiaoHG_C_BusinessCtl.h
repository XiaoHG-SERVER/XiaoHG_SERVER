
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_C_BUSINESSCTL_H__
#define __XiaoHG_C_BUSINESSCTL_H__

#include <string.h>
#include "XiaoHG_C_Socket.h"

class CMemory;
class CCRC32;

class CBusinessCtl
{
public:
    CBusinessCtl(/* args */);
    virtual ~CBusinessCtl();

public:
    void Init();

public:
    /* Business logic */
	bool HandleRegister(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, uint16_t iBodyLength);
	bool HandleLogin(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, uint16_t iBodyLength);
	bool HandleHeartbeat(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, uint16_t iBodyLength);

private:
    static CCRC32* m_pCrc32;
    static CMemory* m_pMemory;
};

#endif//!__XiaoHG_C_BUSINESSCTL_H__