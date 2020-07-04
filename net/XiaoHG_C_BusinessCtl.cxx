
#include <arpa/inet.h>
#include "XiaoHG_C_BusinessCtl.h"
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_C_LockMutex.h"
#include "XiaoHG_C_Log.h"
#include "XiaoHG_error.h"
#include "XiaoHG_Comm.h"
#include "XiaoHG_C_Crc32.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_MessageCtl.h"

#define __THIS_FILE__ "XiaoHG_C_BusinessCtl.cxx"

CCRC32* CBusinessCtl::m_pCrc32 = CCRC32::GetInstance();
CMemory* CBusinessCtl::m_pMemory = CMemory::GetInstance();
CBusinessCtl::CBusinessCtl(/* args */)
{
    Init();
}

CBusinessCtl::~CBusinessCtl()
{
}

void CBusinessCtl::Init()
{
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: HandleRegister
 * discription: After receiving a complete message, enter the message 
 *              queue and trigger the thread in the thread pool to process the message.
 * parameter:
 * =================================================================*/
bool CBusinessCtl::HandleRegister(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, uint16_t iBodyLength)
{
    /* Check packet is OK */
    if(pPkgBody == NULL || sizeof(REGISTER_T) != iBodyLength)
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "CBusinessCtl::HandleRegister, (pPkgBody == NULL || sizeof(LOGIN_T) != iBodyLength)");
        return false;
    }
    /* For the same user, multiple requests may be sent at the same time, 
     * causing multiple threads to serve the user at the same time */
    CLock lock(&pConn->logicProcessMutex); 
    
    /* Get data */
    LPREGISTER_T pRecvMsgInfo = (LPREGISTER_T)pPkgBody; 
    pRecvMsgInfo->iType = ntohl(pRecvMsgInfo->iType);
    /* This is very critical to prevent the client from sending malformed packets and cause the server to use this data directly. */
    pRecvMsgInfo->UserName[sizeof(pRecvMsgInfo->UserName) - 1] = 0; 
    pRecvMsgInfo->Password[sizeof(pRecvMsgInfo->Password) - 1] = 0;

    CLog::Log("HandleRegister: pRecvMsgInfo->iType = %d, pRecvMsgInfo->UserName = %s,pRecvMsgInfo->Password = %s", 
                                                                pRecvMsgInfo->iType, pRecvMsgInfo->UserName, pRecvMsgInfo->Password);
	
    /* Process the packet ... */

    /* Analog send */
    LPCOMM_PKG_HEADER pPkgHeader = NULL;
    int iMsgBodylen = sizeof(REGISTER_T);
    char *pSendMsgBuff = (char *)m_pMemory->AllocMemory(g_LenMsgHeader + g_LenPkgHeader + iMsgBodylen, false);
    /* Full message header */
    memcpy(pSendMsgBuff, pMsgHeader, g_LenMsgHeader);
    /* Full packet header */
    pPkgHeader = (LPCOMM_PKG_HEADER)(pSendMsgBuff + g_LenMsgHeader);
    pPkgHeader->MsgCode = CMD_REGISTER;
    pPkgHeader->MsgCode = htons(pPkgHeader->MsgCode);
    pPkgHeader->PkgLength = htons(g_LenPkgHeader + iMsgBodylen);
    LPREGISTER_T pSendInfo = (LPREGISTER_T)(pSendMsgBuff + g_LenMsgHeader + g_LenPkgHeader);	
    pPkgHeader->iCrc32 = m_pCrc32->Get_CRC((unsigned char *)pSendInfo, iMsgBodylen);
    pPkgHeader->iCrc32 = htonl(pPkgHeader->iCrc32);

    /* send data */
    g_MessageCtl.PushSendMsgToSendListAndSem(pSendMsgBuff);
    return true;
}

bool CBusinessCtl::HandleLogin(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, uint16_t iBodyLength)
{
    if(pPkgBody == NULL || sizeof(LOGIN_T) != iBodyLength)
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "CBusinessCtl::HandleLogin, (pPkgBody == NULL || sizeof(LOGIN_T) != iBodyLength)");
        return false;
    }
    CLock lock(&pConn->logicProcessMutex);
    LPLOGIN_T pRecvMsgInfo = (LPLOGIN_T)pPkgBody;
    pRecvMsgInfo->UserName[sizeof(pRecvMsgInfo->UserName) - 1] = 0;
    pRecvMsgInfo->Password[sizeof(pRecvMsgInfo->Password) - 1] = 0;

    CLog::Log("HandleLogin: pRecvMsgInfo->UserName = %s, pRecvMsgInfo->Password = %s",
                                                        pRecvMsgInfo->UserName, pRecvMsgInfo->Password);

    /* Process the packet ... */

	LPCOMM_PKG_HEADER pPkgHeader = nullptr;
    int iMsgBodylen = sizeof(LOGIN_T);  
    char *pSendMsgBuff = (char *)m_pMemory->AllocMemory(g_LenMsgHeader + g_LenPkgHeader + iMsgBodylen, false);   
    memcpy(pSendMsgBuff, pMsgHeader, g_LenMsgHeader);    
    pPkgHeader = (LPCOMM_PKG_HEADER)(pSendMsgBuff + g_LenMsgHeader);
    pPkgHeader->MsgCode = CMD_LOGIN;
    pPkgHeader->MsgCode = htons(pPkgHeader->MsgCode);
    pPkgHeader->PkgLength = htons(g_LenPkgHeader + iMsgBodylen);    
    LPLOGIN_T pSendInfo = (LPLOGIN_T)(pSendMsgBuff + g_LenMsgHeader + g_LenPkgHeader);
    pPkgHeader->iCrc32 = htonl(m_pCrc32->Get_CRC((unsigned char *)pSendInfo, iMsgBodylen));

    g_MessageCtl.PushSendMsgToSendListAndSem(pSendMsgBuff);
    return true;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.24
 * test time: 2020.04.24 pass
 * function name: HandleHeartbeat
 * discription: send heartbeat
 * =================================================================*/
bool CBusinessCtl::HandleHeartbeat(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, uint16_t iBodyLength)
{
    /* Heartbeat packet is not body */
    if(iBodyLength != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "CBusinessCtl::HandleHeartbeat, (pPkgBody == NULL || sizeof(LOGIN_T) != iBodyLength)");
		return false; 
    }
    /* All access related to this user should be considered to be mutually exclusive, 
     * so as to avoid that the user sends two commands at the same time to achieve various cheating purposes */
    CLock lock(&pConn->logicProcessMutex); 
    /* Refresh the time of the last heartbeat packet received */
    pConn->lastHeartBeatTime = time(NULL); 

    CLog::Log("HandleHeartbeat: pRecvMsgInfo->UserName = 0, pRecvMsgInfo->Password = 0");
    
    char *pSendMsgBuff = (char *)m_pMemory->AllocMemory(g_LenMsgHeader + g_LenPkgHeader, false);
    char *pTmpBuff = pSendMsgBuff;
	memcpy(pTmpBuff, pMsgHeader, g_LenMsgHeader);
	pTmpBuff += g_LenMsgHeader;

    /* Group heartbeat package */
    LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)pTmpBuff;
    pPkgHeader->MsgCode = htons(CMD_HEARTBEAT);	
    pPkgHeader->PkgLength = htons(g_LenPkgHeader); 
	pPkgHeader->iCrc32 = 0;	

    g_MessageCtl.PushSendMsgToSendListAndSem(pSendMsgBuff);
    return true;
}