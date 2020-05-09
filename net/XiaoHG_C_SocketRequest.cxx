
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
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_LockMutex.h"

#define __THIS_FILE__ "XiaoHG_C_SocketRequest.cxx"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: EpollEventAcceptHandler
 * discription: Processing when data comes in. When there is data on 
 *              the pConnection, this function will be called by 
 *              EpolWaitlProcessEvents()
 * parameter:
 * =================================================================*/
void CSocket::EpollEventReadRequestHandler(LPCONNECTION_T pConn)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::EpollEventReadRequestHandler track");

    /* Flood flags */
    bool bIsFlood = false;
    /* Receive the package, pay attention to the second and third parameters we use, 
     * we always use these two parameters, so we must ensure that pConn-> pRecvMsgBuff points 
     * to the correct location of the package, and that pConn-> iRecvMsgLen points to the 
     * correct collection Package width */
    ssize_t uiReceiveLen = RecvProc(pConn, pConn->pRecvMsgBuff, pConn->iRecvMsgLen); 
    if(uiReceiveLen <= 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Client disconnected, socket id: %d", pConn->iSockFd);
        return;
    }

    /* PKG_HD_INIT is defult */
    if(pConn->iRecvCurrentStatus == PKG_HD_INIT) 
    {
        /* received message header, parse header */
        if(uiReceiveLen == m_iLenPkgHeader)
        {
            /* handle hander */
            RecvMsgHeaderParseProc(pConn, bIsFlood); 
        }
        else
		{
			/* The packet header is not complete */
            pConn->iRecvCurrentStatus = PKG_HD_RECVING;   /* status = PKG_HD_RECVING */
            pConn->pRecvMsgBuff = pConn->pRecvMsgBuff + uiReceiveLen;
            pConn->iRecvMsgLen = pConn->iRecvMsgLen - uiReceiveLen;
        } /* end  if(uiReceiveLen == m_iLenPkgHeader) */
    } 
    /* Continue to accept the packet header */
    else if(pConn->iRecvCurrentStatus == PKG_HD_RECVING) 
    {
        /* packet header is complete */
        if(pConn->iRecvMsgLen == uiReceiveLen) 
        {
            /* handle packet header*/
            RecvMsgHeaderParseProc(pConn, bIsFlood); 
        }
        else
		{
			/* packet header still not complete */
            pConn->pRecvMsgBuff = pConn->pRecvMsgBuff + uiReceiveLen;
            pConn->iRecvMsgLen = pConn->iRecvMsgLen - uiReceiveLen;
        }
    }
    /* packet header is complete, receive packet body */
    else if(pConn->iRecvCurrentStatus == PKG_BD_INIT) 
    {
        if(uiReceiveLen == pConn->iRecvMsgLen)
        {
            /* packet body is complete */
            if(m_FloodAttackEnable == 1) 
            {
                /* Flood check */
                bIsFlood = TestFlood(pConn);
            }
            RecvMsgPacketParseProc(pConn, bIsFlood);
        }
        else
		{
			/* packet body is not complete */
			pConn->iRecvCurrentStatus = PKG_BD_RECVING;					
			pConn->pRecvMsgBuff = pConn->pRecvMsgBuff + uiReceiveLen;
			pConn->iRecvMsgLen = pConn->iRecvMsgLen - uiReceiveLen;
		}
    }
    /* packet body is not complete */
    else if(pConn->iRecvCurrentStatus == PKG_BD_RECVING) 
    {
        if(pConn->iRecvMsgLen == uiReceiveLen)
        {
            /* packet body complete */
            if(m_FloodAttackEnable == 1) 
            {
                /* Flood check */
                bIsFlood = TestFlood(pConn);
            }
            RecvMsgPacketParseProc(pConn, bIsFlood);
        }
        else
        {
            /* packet body is not complete */
            pConn->pRecvMsgBuff = pConn->pRecvMsgBuff + uiReceiveLen;
			pConn->iRecvMsgLen = pConn->iRecvMsgLen - uiReceiveLen;
        }
    }  /* end if(pConn->iRecvCurrentStatus == PKG_HD_INIT) */

    /* check flood attack*/
    if(bIsFlood == true)
    {
        /* is flood, disconnection */
        CLog::Log(LOG_LEVEL_NOTICE, "check out socket: %d is flood attack, disconnection", pConn->iSockFd);
        CloseConnectionToRecy(pConn);
    }
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: RecvProc
 * discription: receive massage, and handle client disconnection,..
 * parameter:
 * =================================================================*/
ssize_t CSocket::RecvProc(LPCONNECTION_T pConn, char *pSendBuff, ssize_t uiBuffLen)   
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::RecvProc track");

    ssize_t n = recv(pConn->iSockFd, pSendBuff, uiBuffLen, 0);      
    if(n == 0)
    {
        /* The client closes [should be completed 4 waved normally], 
         * directly recycle the pConnection, close the socket and add 
         * the pConnection to the delayed recycle queue */
        CloseConnectionToRecy(pConn);
        return XiaoHG_ERROR;
    }

    if(n < 0) 
    {
        /* no data */
        if(errno == EAGAIN || errno == EWOULDBLOCK)
        {
            /* Not treated as an error, simply return */
            CLog::Log(LOG_LEVEL_NOTICE, "errno == EAGAIN || errno == EWOULDBLOCK failed");
            return XiaoHG_ERROR;
        }

        /* EINTR error generation: When a process blocked by a slow system call captures 
         * a signal and the corresponding signal processing function returns, the system 
         * call may return an EINTR error, for example: on the socket server side, 
         * a signal capture mechanism is set, There is a child process. When a valid signal is 
         * captured by the parent process when the parent process is blocked by a slow system call, 
         * the kernel will cause accept to return an EINTR error (interrupted system call) */
        /* This is not an error */
        if(errno == EINTR)
        {
            /* Not treated as an error, simply return */
            CLog::Log(LOG_LEVEL_NOTICE, "errno == EINTR failed");
            return XiaoHG_ERROR;
        }

        /* #define ECONNRESET 104 /* Connection reset by peer */
        if(errno == ECONNRESET)  
        {
            /* If the client does not normally close the socket connection, but closes the 
             * entire running program [should be sent directly to the server instead of 4 waved
             * packets to complete the connection disconnection], then this error will occur */
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "receive message failed");
        }
        else
        {
            /* error */
            /*  #define EBADF   9 /* Bad file descriptor */
            if(errno == EBADF)  
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "receive message failed");
            }
            else
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "receive message failed");
            }
        }

        /* Close the socket directly, release the connection in the connection pool */
        CloseConnectionToRecy(pConn);
        return XiaoHG_ERROR;
    }
    return n;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: RecvMsgHeaderParseProc
 * discription: handle message packet phase 1
 * parameter:
 * =================================================================*/
void CSocket::RecvMsgHeaderParseProc(LPCONNECTION_T pConn, bool &bIsFlood)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::RecvMsgHeaderParseProc track");

    LPCOMM_PKG_HEADER pPkgHeader = NULL;
    /* packet head in dataHaeadInfo munber */
    pPkgHeader = (LPCOMM_PKG_HEADER)pConn->dataHeadInfo;
    /* Get packet length */
    unsigned short sPkgLen = ntohs(pPkgHeader->sPkgLength);
    CMemory *pMemory = CMemory::GetInstance();  

    /* Malformed packet filtering: judgment of malicious packets or wrong packets */
    if(sPkgLen < m_iLenPkgHeader || sPkgLen > PKG_MAX_LENGTH) 
    {
        pConn->iRecvCurrentStatus = PKG_HD_INIT;      
        pConn->pRecvMsgBuff = pConn->dataHeadInfo;
        pConn->iRecvMsgLen = m_iLenPkgHeader;
    }
    /* Legal packet header, continue processing */
    else
    {
        char *pMsgBuffer = (char *)pMemory->AllocMemory(m_iLenMsgHeader + sPkgLen, false); 
        pConn->pRecvMsgMemPointer = pMsgBuffer;

        /* a)Fill in the message header first */
        LPMSG_HEADER_T ptmpMsgHeader = (LPMSG_HEADER_T)pMsgBuffer;
        ptmpMsgHeader->pConn = pConn;
        ptmpMsgHeader->uiCurrentSequence = pConn->uiCurrentSequence;
        
        /* b)Then fill in the data packet header */
        pMsgBuffer += m_iLenMsgHeader;
        memcpy(pMsgBuffer, pPkgHeader, m_iLenPkgHeader);
        if(sPkgLen == m_iLenPkgHeader)
        {
            /* only packet header */
            if(m_FloodAttackEnable == 1) 
            {
                /* Flood check*/
                bIsFlood = TestFlood(pConn);
            }
            RecvMsgPacketParseProc(pConn, bIsFlood);
        }
        else
        {
            /* receive packet body */
            pConn->iRecvCurrentStatus = PKG_BD_INIT;                /* change statuc to PKG_BD_INIT */
            pConn->pRecvMsgBuff = pMsgBuffer + m_iLenPkgHeader;     /* pMsgBuffer + m_iLenPkgHeader point packet body */
            pConn->iRecvMsgLen = sPkgLen - m_iLenPkgHeader;         /* sPkgLen - m_iLenPkgHeader is packet body length */
        }                       
    }/* end if(sPkgLen < m_iLenPkgHeader)  */

    return;
}


/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: RecvMsgPacketParseProc
 * discription: receive compelet message packet and save to recvice 
 *              message list.
 * parameter:
 * =================================================================*/
void CSocket::RecvMsgPacketParseProc(LPCONNECTION_T pConn, bool &bIsFlood)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::RecvMsgPacketParseProc track");

    if(bIsFlood == false)
    {
        /* Enter the message queue and trigger the thread to process the message */
        g_ThreadPool.PutMsgRecvQueueAndSignal(pConn->pRecvMsgMemPointer);
    }
    else
    {
        /* Is flood*/
        /* Directly release the memory without entering the message queue at all */
        CMemory *pMemory = CMemory::GetInstance();
        pMemory->FreeMemory(pConn->pRecvMsgMemPointer); 
    }
    pConn->iRecvCurrentStatus = PKG_HD_INIT;   /* Init */                  
    pConn->pRecvMsgBuff = pConn->dataHeadInfo; /* Init */
    pConn->iRecvMsgLen = m_iLenPkgHeader;      /* Init */
    return;
}

/* ======================================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: SendProc
 * discription: send message
 * ======================================================================================*/
ssize_t CSocket::SendProc(LPCONNECTION_T pConn, char *pSendBuff, ssize_t uiSendLen)  
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::SendProc track");

    ssize_t n = 0;
    for ( ;; )
    {
        n = send(pConn->iSockFd, pSendBuff, uiSendLen, 0); 
        /* Some data was sent successfully */
        if(n > 0)
        {
            /* Some data was sent successfully, but how much was sent, we do n’t care here, and we do n’t need to send it again
             * There are two cases here： 
             * (1) n == uiSendLen : That is, you want to send as much as you want, which means that it is completely sent. 
             * (2) n < uiSendLen : If the sending is not completed, it must be that the sending buffer is full, 
             * so there is no need to retry sending, just go back. */
            return n;
        }

        if(n == 0)
        {
            /* send() returns 0, generally recv() returns 0 means disconnected, send() returns 0, 
             * I will return 0 directly here [let the caller handle] send() returns 0, 
             * or the byte you send is 0, Either the peer may disconnect online to find information: 
             * send = 0 means timeout, the other party actively closed the pConnection */
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "send failed");
            return 0;
        }

        if(errno == EAGAIN)
        {
            /* The kernel buffer is full, this is not an error */
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "send failed");
            return -1;  /* Indicates that the send buffer is full */
        }

        if(errno == EINTR)
        {
            /* This should not be an error. I received a signal that caused the send to produce this error. 
             * The pNext time the for loop re-send, I tried to print a log to see when this error occurred. */
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "send failed");
        }
        else
        {
            /* error, recv() handle disconnection event */
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "send failed");
            return -2;
        }
    } /* end for */
}

/* ======================================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: EpollEventWriteRequestHandler
 * discription: Set the write processing function when the data is sent. 
 *              When the data can be written, epoll informs us that we can 
 *              call this function in int CSocket :: EpolWaitlProcessEvents(int timer) 
 *              to get here. The data cannot be sent. Continue sending.
 * ======================================================================================*/
void CSocket::EpollEventWriteRequestHandler(LPCONNECTION_T pConn)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::EpollEventWriteRequestHandler track");

    CMemory *pMemory = CMemory::GetInstance();
    /* send message */
    ssize_t uiSendSize = SendProc(pConn, pConn->pSendMsgBuff, pConn->iSendMsgLen);
    if(uiSendSize > 0 && uiSendSize != pConn->iSendMsgLen)
    {
        /* Not all the data has been sent, only part of the data has been sent, 
         * then how much has been sent and how much is left, continue to record, 
         * so that it can be used pNext time SendProc()*/
        pConn->pSendMsgBuff = pConn->pSendMsgBuff + uiSendSize;
		pConn->iSendMsgLen = pConn->iSendMsgLen - uiSendSize;
        return; /* Driven processing through epoll iEpollEvents */
    }
    else if(uiSendSize == -1)
    {
        /* Send buffer full */
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Send buffer full");
        return;
    }

    /* send complete */
    if(uiSendSize > 0 && uiSendSize == pConn->iSendMsgLen) 
    {
        /* If the data is successfully sent, the write event notification is removed from epoll. 
         * In other cases, it is disconnected. Wait for the system kernel to remove the pConnection 
         * from the red and black tree. */
        if(EpollRegisterEvent(pConn->iSockFd,     /* socket id */
                                EPOLL_CTL_MOD,      /* event type */
                                EPOLLOUT,           /* flag, EPOLLOUT：Writable [Notify me when I can write] */
                                EPOLL_DEL,          /* EPOLL_CTL_MOD neet the parement, 0：add，1：del 2：over */
                                pConn               /* pConnection */
                                ) == -1)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "register epoll event failed");
        }
        else
        {
            pMemory->FreeMemory(pConn->pSendMsgMemPointer);
            --pConn->iThrowSendCount;
        }
    }

    /* Each message send successful, sem_post make send message thread check message to send */
    if(sem_post(&m_SemEventSendQueue) == -1)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "sem_post(&m_SemEventSendQueue) failed");
    }

    return;
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
void CSocket::RecvMsgHandleThreadProc(char *pMsgBuf)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::ThreadRecvMsgHandleProc track");
    return;
}


