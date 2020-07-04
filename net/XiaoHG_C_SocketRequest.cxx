
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
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_LockMutex.h"
#include "XiaoHG_C_Log.h"
#include "XiaoHG_error.h"

#define __THIS_FILE__ "XiaoHG_C_SocketRequest.cxx"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: EpollEventReadRequestHandler
 * discription: Processing when data comes in. When there is data on 
 *              the pstConn, this function will be called by 
 *              EpolWaitlProcessEvents()
 * parameter:
 * =================================================================*/
void CSocket::EpollEventReadRequestHandler(LPCONNECTION_T pConn)
{
    /* Flood flags */
    bool bIsFlood = false;
    /* Receive the package, pay attention to the second and third parameters we use, 
     * we always use these two parameters, so we must ensure that pConn-> pRecvMsgBuff points 
     * to the correct location of the package, and that pConn-> recvMsgLen points to the 
     * correct collection Package width */
    ssize_t recvMsgLen = RecvMsgProc(pConn, pConn->pRecvMsgBuff, pConn->recvMsgLen); 
    if(recvMsgLen <= 0)
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Client disconnected, socket id: %d", pConn->sockFd);
        return;
    }

    /* PKG_HD_INIT is defult */
    if(pConn->recvMsgCurStatus == PKG_HD_INIT) 
    {
        /* received message header, parse header */
        if(recvMsgLen == g_LenPkgHeader)
        {
            /* handle hander */
            g_MessageCtl.RecvMsgHeaderParseProc(pConn, bIsFlood); 
        }
        else
		{
			/* The packet header is not complete */
            pConn->recvMsgCurStatus = PKG_HD_RECVING;   /* status = PKG_HD_RECVING */
            pConn->pRecvMsgBuff += recvMsgLen;
            pConn->recvMsgLen -= recvMsgLen;
        } /* end  if(recvMsgLen == g_LenPkgHeader) */
    } 

    /* Continue to accept the packet header */
    else if(pConn->recvMsgCurStatus == PKG_HD_RECVING) 
    {
        /* packet header is complete */
        if(pConn->recvMsgLen == recvMsgLen) 
        {
            /* handle packet header*/
            g_MessageCtl.RecvMsgHeaderParseProc(pConn, bIsFlood); 
        }
        else
		{
			/* packet header still not complete */
            pConn->pRecvMsgBuff += recvMsgLen;
            pConn->recvMsgLen -= recvMsgLen;
        }
    }

    /* packet header is complete, receive packet body */
    else if(pConn->recvMsgCurStatus == PKG_BD_INIT) 
    {
        /* finish recv msg body */
        if(recvMsgLen == pConn->recvMsgLen)
        {
            /* packet body is complete */
            if(m_FloodAttackEnable == 1) 
            {
                /* Flood check */
                bIsFlood = TestFlood(pConn);
            }
            g_MessageCtl.RecvMsgPacketParseProc(pConn, bIsFlood);
        }
        else
		{
			/* packet body is not complete */
			pConn->recvMsgCurStatus = PKG_BD_RECVING;					
			pConn->pRecvMsgBuff += recvMsgLen;
			pConn->recvMsgLen -= recvMsgLen;
		}
    }

    /* packet body is not complete */
    else if(pConn->recvMsgCurStatus == PKG_BD_RECVING) 
    {
        /* finish recv msg body */
        if(pConn->recvMsgLen == recvMsgLen)
        {
            /* packet body complete */
            if(m_FloodAttackEnable == 1) 
            {
                /* Flood check */
                bIsFlood = TestFlood(pConn);
            }
            g_MessageCtl.RecvMsgPacketParseProc(pConn, bIsFlood);
        }
        else
        {
            /* packet body is not complete */
            pConn->pRecvMsgBuff += recvMsgLen;
			pConn->recvMsgLen -= recvMsgLen;
        }
    }  /* end if(pConn->recvMsgCurStatus == PKG_HD_INIT) */

    /* check flood attack*/
    if(bIsFlood == true)
    {
        /* is flood, disconnection */
        CLog::Log(LOG_LEVEL_NOTICE, "check out socket: %d is flood attack, disconnection", pConn->sockFd);
        CloseConnectionToRecy(pConn);
    }
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: RecvMsgProc
 * discription: receive massage, and handle client disconnection,..
 * parameter:
 * =================================================================*/
ssize_t CSocket::RecvMsgProc(LPCONNECTION_T pConn, char *pSendMsgBuff, ssize_t uiBuffLen)   
{
    ssize_t n = recv(pConn->sockFd, pSendMsgBuff, uiBuffLen, 0);      
    if(n == 0)
    {
        /* The client closes [should be completed 4 waved normally], 
         * directly recycle the pstConn, close the socket and add 
         * the pstConn to the delayed recycle queue */
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "recv failed, pConn->sockFd = %d, uiBuffLen = %d, return: %d", pConn->sockFd, uiBuffLen, n);
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

/* ======================================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: SendMsgProc
 * discription: send message
 * ======================================================================================*/
ssize_t CSocket::SendMsgProc(LPCONNECTION_T pConn, char *pSendMsgBuff, ssize_t uiSendLen)  
{
    ssize_t n = 0;
    for ( ;; )
    {
        n = send(pConn->sockFd, pSendMsgBuff, uiSendLen, 0); 
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
             * send = 0 means timeout, the other party actively closed the pstConn */
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
    /* send message */
    ssize_t sendMsgLen = SendMsgProc(pConn, pConn->pSendMsgBuff, pConn->sendMsgLen);
    if(sendMsgLen > 0 && sendMsgLen != pConn->sendMsgLen)
    {
        /* Not all the data has been sent, only part of the data has been sent, 
         * then how much has been sent and how much is left, continue to record, 
         * so that it can be used pNext time SendMsgProc()*/
        pConn->pSendMsgBuff = pConn->pSendMsgBuff + sendMsgLen;
		pConn->sendMsgLen = pConn->sendMsgLen - sendMsgLen;
        return; /* Driven processing through epoll epollEvents */
    }
    else if(sendMsgLen == -1)
    {
        /* Send buffer full */
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Send buffer full");
        return;
    }

    /* send complete */
    if(sendMsgLen > 0 && sendMsgLen == pConn->sendMsgLen) 
    {
        /* If the data is successfully sent, the write event notification is removed from epoll. 
         * In other cases, it is disconnected. Wait for the system kernel to remove the pstConn 
         * from the red and black tree. */
        if(EpollRegisterEvent(pConn->sockFd,     /* socket id */
                                EPOLL_CTL_MOD,      /* event type */
                                EPOLLOUT,           /* flag, EPOLLOUT：Writable [Notify me when I can write] */
                                EPOLL_DEL,          /* EPOLL_CTL_MOD neet the parement, 0：add，1：del 2：over */
                                pConn               /* pstConn */
                                ) == -1)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "register epoll event failed");
            exit(0);
        }
        else
        {
            m_pMemory->FreeMemory(pConn->pSendMsgMemPointer);
            --pConn->throwSendMsgCount;
        }
    }

    /* Each message send successful, sem_post make send message thread check message to send */
    if(sem_post(&g_SendMsgPushEventSem) == -1)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "sem_post(&g_SendMsgPushEventSem) failed");
        exit(0);
    }
}


