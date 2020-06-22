
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_Global.h"
#include "XiaoHG_Func.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_Crc32.h"
#include "XiaoHG_C_SLogic.h"  
#include "XiaoHG_LogicComm.h"  
#include "XiaoHG_C_LockMutex.h"
#include "XiaoHG_C_Log.h"
#include "XiaoHG_error.h"

#define __THIS_FILE__ "XiaoHG_C_SLogic.cxx"

/* Logic handler process call back*/
typedef bool (CLogicSocket::*LogicHandlerCallBack)(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, unsigned short iBodyLength);

/* Such an array used to store member function pointers */
static const LogicHandlerCallBack StatusHandler[] = 
{
    /* The first 5 elements of the array are reserved for future addition of some basic server functions */
    &CLogicSocket::HandleHeartbeat,                         /* [0]: realization of heartbeat package */
    NULL,                                                   /* [1]: The subscript starts from 0 */
    NULL,                                                   /* [2]: The subscript starts from 0 */
    NULL,                                                   /* [3]: The subscript starts from 0 */
    NULL,                                                   /* [4]: The subscript starts from 0 */
 
    /* Start processing specific business logic */
    &CLogicSocket::HandleRegister,                         /* [5]: Realize the specific registration function */
    &CLogicSocket::HandleLogin,                            /* [6]: Realize the specific login function */
    /* ... */
};

/* How many commands are there, you can know when compiling */
#define TOTAL_COMMANDS sizeof(StatusHandler)/sizeof(LogicHandlerCallBack)

CLogicSocket::CLogicSocket()
{
    Init();
}

CLogicSocket::~CLogicSocket(){}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: Initalize
 * discription: Init socket
 * =================================================================*/
void CLogicSocket::Init()
{
    
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ThreadRecvMsgHandleProc
 * discription: To process the received data packet, this function 
 *              is called by the thread pool, this function is a separate thread
 *              -> pMsgBuf: message header + packet header + packet body
 * parameter:
 * =================================================================*/
void CLogicSocket::RecvMsgHandleThreadProc(char *pMsgBuf)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CLogicSocket::ThreadRecvMsgHandleProc track");

    void *pPkgBody = NULL;  /* packet point */
    LPMSG_HEADER_T pMsgHeader = (LPMSG_HEADER_T)pMsgBuf;    /* massage header */
    LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)(pMsgBuf + m_iLenMsgHeader);  /* packet header */
    unsigned short sPkgLen = ntohs(pPkgHeader->sPkgLength); /* packet length */
    /* if only packet header*/
    if(m_iLenPkgHeader == sPkgLen)
    {
        /* only packet header */
        /* iCrc32 value if equeal 0, heart beat iCrc32 = 0, if not 0 is error packet header*/
		if(pPkgHeader->iCrc32 != 0)
		{
            CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Only packet header, and not a heartbeat packet");
			return; /* throw away */
		}
    }
    else 
	{
        /* the message have packet body */
		pPkgHeader->iCrc32 = ntohl(pPkgHeader->iCrc32);
        /* get packet body */
		pPkgBody = (void *)(pMsgBuf + m_iLenMsgHeader + m_iLenPkgHeader);
		/* Calculate the crc value to judge the integrity of the package */
		int iCalcCrc32 = CCRC32::GetInstance()->Get_CRC((unsigned char *)pPkgBody, sPkgLen - m_iLenPkgHeader);
        /* crc validity check */
        if(iCalcCrc32 != pPkgHeader->iCrc32)
		{
            CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Crc validity check failed");
			return; /* throw away */
		}   
	}
    /* crc check is OK */
    unsigned short sMsgCode = ntohs(pPkgHeader->sMsgCode);   /* Get message code */
    LPCONNECTION_T pConn = pMsgHeader->pConn;   /* Get pConnection info */
    /* (a)If the client disconnects from the process of receiving the packet 
     * sent by the client to the server releasing a thread in the thread pool 
     * to process the packet, then obviously, we do not have to process 
     * this received packet */    
    if(pConn->uiCurrentSequence != pMsgHeader->uiCurrentSequence)   
    {
        /* The pConnection in the pConnection pool is occupied by other tcp connections 
         * [other sockets], which shows that the original client and the server are disconnected, 
         * this packet is directly discarded, [the client is disconnected]*/
        return;
    }

    /* (b)Determine that the message code is correct, prevent the client from maliciously 
     * invading our server, and send a message code that is not within the processing range of our server */
	if(sMsgCode >= TOTAL_COMMANDS)
    {
        /* Discard this package [malicious package or error package] */
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Message code is error, message code = %d", sMsgCode);
        return; 
    }

    /* (c)Is there a corresponding message processing function */
    /* This way of using sMsgCode can make finding member functions to be executed particularly efficient */
    if(StatusHandler[sMsgCode] == NULL) 
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "This is not a handle message process, message code = %d", sMsgCode);
        return;  /* throw away */
    }

    /* (d)Call the member function corresponding to the message code to process */
    (this->*StatusHandler[sMsgCode])(pConn, pMsgHeader, (char *)pPkgBody, sPkgLen - m_iLenPkgHeader);
    
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: HeartBeatTimeOutCheck
 * discription: TWhen the heartbeat packet detection time is up, 
 *              it is necessary to detect whether the heartbeat packet has timed out. 
 *              This function is a subclass function to implement specific judgment actions.
 * parameter:
 * =================================================================*/
void CLogicSocket::HeartBeatTimeOutCheck(LPMSG_HEADER_T pstMsgHeader, time_t CurrentTime)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CLogicSocket::HeartBeatTimeOutCheck track");

    /* check the connect is OK */
    if(pstMsgHeader->uiCurrentSequence == pstMsgHeader->pConn->uiCurrentSequence)
    {
        LPCONNECTION_T pConn = pstMsgHeader->pConn;
        /* Determine if the direct kick function is enabled if the timeout is enabled. 
         * If it is, the user will be kicked directly after the first heartbeat timeout, 
         * otherwise the heartbeat detection will be entered again, 
         * and the user will be kicked out when the maximum allowable time for heartbeat detection. */
        if(m_iIsTimeOutKick == 1)
        {
            /* The need to kick out directly at the time */
            CloseConnectionToRecy(pConn);
        }
        /* The judgment criterion for timeout kicking is the interval of each check * 3. 
         * If no heartbeat packet is sent after this time, 
         * the iLastHeartBeatTime will be refreshed every time the client heartbeat packet is received. */
        else if((CurrentTime - pConn->iLastHeartBeatTime) > (m_iWaitTime * 3 + 10))
        {
            CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Heart beat check time out kick socket: %d", pConn->iSockFd);
            CloseConnectionToRecy(pConn);
        }
        m_pMemory->FreeMemory(pstMsgHeader);
    }
    /* This connect is broken */
    else
    {
        m_pMemory->FreeMemory(pstMsgHeader);
    }
    return;
}

/* 发送没有包体的数据包给客户端，心跳包 */
void CLogicSocket::SendNoBodyPkgToClient(LPMSG_HEADER_T pMsgHeader,unsigned short iMsgCode)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CLogicSocket::SendNoBodyPkgToClient track");

    char *pSendBuff = (char *)m_pMemory->AllocMemory(m_iLenMsgHeader + m_iLenPkgHeader,false);

    char *pTmpBuff = pSendBuff;
	memcpy(pTmpBuff,pMsgHeader, m_iLenMsgHeader);
	pTmpBuff += m_iLenMsgHeader;

    LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)pTmpBuff;
    pPkgHeader->sMsgCode = htons(iMsgCode);	
    pPkgHeader->sPkgLength = htons(m_iLenPkgHeader); 
	pPkgHeader->iCrc32 = 0;		
    SendMsg(pSendBuff);
    return;
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
bool CLogicSocket::HandleRegister(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, unsigned short iBodyLength)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CLogicSocket::HandleRegister track");

    /* Check packet is OK */
    if(pPkgBody == NULL || sizeof(REGISTER_T) != iBodyLength)
    {
        return false;
    }
    /* For the same user, multiple requests may be sent at the same time, 
     * causing multiple threads to serve the user at the same time */
    CLock lock(&pConn->LogicPorcMutex); 
    
    /* Get data */
    LPREGISTER_T pRecvMsgInfo = (LPREGISTER_T)pPkgBody; 
    pRecvMsgInfo->iType = ntohl(pRecvMsgInfo->iType);
    /* This is very critical to prevent the client from sending malformed packets and cause the server to use this data directly. */
    pRecvMsgInfo->UserName[sizeof(pRecvMsgInfo->UserName) - 1] = 0; 
    pRecvMsgInfo->Password[sizeof(pRecvMsgInfo->Password) - 1] = 0;

    CLog::Log("HandleRegister receive message: pRecvMsgInfo->iType = %d, pRecvMsgInfo->UserName = %s,pRecvMsgInfo->Password = %s", 
                                                                pRecvMsgInfo->iType, pRecvMsgInfo->UserName, pRecvMsgInfo->Password);
	
    /* Process the packet ... */

    /* Assuming the packet is processed, we need to reply to the client */
	CCRC32 *pCrc32 = CCRC32::GetInstance();
    /* Analog send */
    LPCOMM_PKG_HEADER pPkgHeader = NULL;
    int iMsgBodylen = sizeof(REGISTER_T);
    char *pSendBuff = (char *)m_pMemory->AllocMemory(m_iLenMsgHeader + m_iLenPkgHeader + iMsgBodylen, false);
    
    /* Full message header */
    memcpy(pSendBuff, pMsgHeader, m_iLenMsgHeader);
    
    /* Full packet header */
    pPkgHeader = (LPCOMM_PKG_HEADER)(pSendBuff + m_iLenMsgHeader);
    pPkgHeader->sMsgCode = CMD_REGISTER;
    pPkgHeader->sMsgCode = htons(pPkgHeader->sMsgCode);
    pPkgHeader->sPkgLength = htons(m_iLenPkgHeader + iMsgBodylen);
    
    /* int: htonl
     * short: htons */
    LPREGISTER_T pSendInfo = (LPREGISTER_T)(pSendBuff + m_iLenMsgHeader + m_iLenPkgHeader);	
    
    /* Calc crc32 value */
    pPkgHeader->iCrc32 = pCrc32->Get_CRC((unsigned char *)pSendInfo, iMsgBodylen);
    pPkgHeader->iCrc32 = htonl(pPkgHeader->iCrc32);

    /* send data */
    SendMsg(pSendBuff);

    return true;
}

bool CLogicSocket::HandleLogin(LPCONNECTION_T pConn,LPMSG_HEADER_T pMsgHeader,char *pPkgBody,unsigned short iBodyLength)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CLogicSocket::HandleLogin track");

    if(pPkgBody == NULL || sizeof(LOGIN_T) != iBodyLength)
    {        
        return false;
    }
    CLock lock(&pConn->LogicPorcMutex);

    LPLOGIN_T pRecvMsgInfo = (LPLOGIN_T)pPkgBody;
    pRecvMsgInfo->UserName[sizeof(pRecvMsgInfo->UserName) - 1] = 0;
    pRecvMsgInfo->Password[sizeof(pRecvMsgInfo->Password) - 1] = 0;

    CLog::Log("HandleLogin receive message: pRecvMsgInfo->UserName = %s, pRecvMsgInfo->Password = %s",
                                                        pRecvMsgInfo->UserName, pRecvMsgInfo->Password);

	LPCOMM_PKG_HEADER pPkgHeader;
	CCRC32 *pCrc32 = CCRC32::GetInstance();

    int iMsgBodylen = sizeof(LOGIN_T);  
    char *pSendBuff = (char *)m_pMemory->AllocMemory(m_iLenMsgHeader + m_iLenPkgHeader + iMsgBodylen, false);    
    memcpy(pSendBuff, pMsgHeader, m_iLenMsgHeader);    
    pPkgHeader = (LPCOMM_PKG_HEADER)(pSendBuff + m_iLenMsgHeader);
    pPkgHeader->sMsgCode = CMD_LOGIN;
    pPkgHeader->sMsgCode = htons(pPkgHeader->sMsgCode);
    pPkgHeader->sPkgLength = htons(m_iLenPkgHeader + iMsgBodylen);    
    LPLOGIN_T pSendInfo = (LPLOGIN_T)(pSendBuff + m_iLenMsgHeader + m_iLenPkgHeader);
    pPkgHeader->iCrc32 = htonl(pCrc32->Get_CRC((unsigned char *)pSendInfo, iMsgBodylen));
    SendMsg(pSendBuff);
    return true;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.24
 * test time: 2020.04.24 pass
 * function name: HandleHeartbeat
 * discription: send heartbeat
 * =================================================================*/
bool CLogicSocket::HandleHeartbeat(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, unsigned short iBodyLength)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CLogicSocket::HandleHeartbeat track");

    /* Heartbeat packet is not body */
    if(iBodyLength != 0)
    {
		return false; 
    }
    /* All access related to this user should be considered to be mutually exclusive, 
     * so as to avoid that the user sends two commands at the same time to achieve various cheating purposes */
    CLock lock(&pConn->LogicPorcMutex); 
    /* Refresh the time of the last heartbeat packet received */
    pConn->iLastHeartBeatTime = time(NULL); 
    
    char *pSendBuff = (char *)m_pMemory->AllocMemory(m_iLenMsgHeader + m_iLenPkgHeader, false);
    char *pTmpBuff = pSendBuff;
	memcpy(pTmpBuff, pMsgHeader, m_iLenMsgHeader);
	pTmpBuff += m_iLenMsgHeader;

    /* Group heartbeat package */
    LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)pTmpBuff;
    pPkgHeader->sMsgCode = htons(CMD_HEARTBEAT);	
    pPkgHeader->sPkgLength = htons(m_iLenPkgHeader); 
	pPkgHeader->iCrc32 = 0;		
    SendMsg(pSendBuff);

    return true;
}