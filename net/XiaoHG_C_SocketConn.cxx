
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
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_Global.h"
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_LockMutex.h"
#include "XiaoHG_C_Log.h"
#include "XiaoHG_error.h"

#define __THIS_FILE__ "XiaoHG_C_SocketConn.cxx"

stConnection::stConnection()
{		
    currSequence = 0;
    pthread_mutex_init(&logicProcessMutex, NULL);
}
stConnection::~stConnection()
{
    pthread_mutex_destroy(&logicProcessMutex);
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: GetOneToUse
 * discription: Init pstConn
 * parameter:
 * =================================================================*/
void stConnection::GetOneToUse()
{
    ++currSequence;                        /* The serial number is increased by 1 during initialization */
    sockFd = -1;                             /* in the beging = -1 */
    recvMsgCurStatus = PKG_HD_INIT;           /* The packet receiving state is in the initial state, ready to receive the data packet header [state machine] */
    pRecvMsgBuff = dataHeadInfo;                /* first receive packet header */
    recvMsgLen = sizeof(COMM_PKG_HEADER);      /* receive packet lenght, first receive header = sizeof(COMM_PKG_HEADER) */
    pRecvMsgMemPointer = NULL;                  /* receive packet point */
    throwSendMsgCount = 0;                        /* Rely on epoll to send data */
    pSendMsgMemPointer = NULL;                  /* send data point */
    epollEvents = 0;                           /* epoll event, defult = 0 */
    lastHeartBeatTime = time(NULL);            /* last time Heartbeat time */
    floodAttackLastTime = 0;                    /* last time receive Flood massage*/
	floodAttackCount = 0;	                    /* Flood times */
    sendMsgListCount = 0;                        /* send massage list count */
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: FreeConnection
 * discription: Init back pstConn
 * parameter:
 * =================================================================*/
void stConnection::PutOneToFree()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "stConnection::PutOneToFree track");

    ++currSequence;    /* Increase the serial number by 1 */
    /* free memory */
    /* pRecvMsgMemPointer munber for free */
    if(pRecvMsgMemPointer != NULL)
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "PutOneToFree pRecvMsgMemPointer, pRecvMsgMemPointer = %p", pRecvMsgMemPointer);
        CMemory::GetInstance()->FreeMemory(pRecvMsgMemPointer);
    }
    /* If there is content in the buffer for sending data, 
     * you need to release the memory */
    if(pSendMsgMemPointer != NULL) 
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "PutOneToFree pSendMsgMemPointer, pSendMsgMemPointer = %p", pSendMsgMemPointer);
        CMemory::GetInstance()->FreeMemory(pSendMsgMemPointer);
    }
}


/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: GetConnection
 * discription: get a new pstConn from free pstConn list
 * parameter:
 * =================================================================*/
LPCONNECTION_T CSocket::GetConnection(int sockFd)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::GetConnection track");

    /* lock the pstConn list */
    CLock lock(&m_ConnectionMutex);
    /* have free pstConn or not*/
    if(!m_FreeConnectionList.empty())
    {
        LPCONNECTION_T pConn = m_FreeConnectionList.front();
        m_FreeConnectionList.pop_front();
        pConn->GetOneToUse();
        --m_FreeConnectionCount; 
        pConn->sockFd = sockFd;
        return pConn;
    }

    /* this is no more free pstConn for, need to alloc more for use */
    LPCONNECTION_T pConn = (LPCONNECTION_T)m_pMemory->AllocMemory(sizeof(CONNECTION_T), true);
    pConn = new(pConn) CONNECTION_T();
    pConn->GetOneToUse();
    m_ConnectionList.push_back(pConn);
    ++m_TotalConnectionCount;
    pConn->sockFd = sockFd;
    return pConn;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: FreeConnection
 * discription: Return the pstConn represented by the parameter 
 *              pConn to the pstConn pool.
 * parameter:
 * =================================================================*/
void CSocket::FreeConnection(LPCONNECTION_T pConn) 
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::FreeConnection track");

    /* lock free pstConn list */
    CLock lock(&m_ConnectionMutex);
    /* free pstConn */
    pConn->PutOneToFree();
    /* push free pstConn list */
    m_FreeConnectionList.push_back(pConn);
    ++m_FreeConnectionCount;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: PushConnectToRecyQueue
 * discription: Put the pstConn to be recycled into a queue,    
 *              and a subsequent special thread will handle the 
 *              recovery of the pstConn in this queue. Some 
 *              connections, we do not want to release immediately, 
 *              we must release it after a period of time to ensure 
 *              the stability of the server, We put this pstConn 
 *              that was released after a certain period of time into 
 *              a queue first.
 * parameter:
 * =================================================================*/
void CSocket::PushConnectToRecyQueue(LPCONNECTION_T pConn)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::PushConnectToRecyQueue track");

    std::list<LPCONNECTION_T>::iterator pos;
    /* lock recy connect queue */
    CLock lock(&m_RecyConnQueueMutex); 
    /* put one time */
    for(pos = m_RecyConnectionList.begin(); pos != m_RecyConnectionList.end(); pos++)
	{
		if((*pos) == pConn)
		{
            return;
		}
	}
    pConn->putRecyConnQueueTime = time(NULL);              /* record put time */
    ++pConn->currSequence;                 /* CurrSequence change */
    m_RecyConnectionList.push_back(pConn);      /* push */
    ++m_DelayRecoveryConnectionCount;
    --m_OnLineUserCount;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: DelayRecoveryConnectionThread
 * discription: Thread to handle delayed pstConn recovery
 * parameter:
 * =================================================================*/
void* CSocket::DelayRecoveryConnectionThread(void* pThreadData)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::DelayRecoveryConnectionThread track");

    time_t CurrentTime = 0;
    LPCONNECTION_T pConn = NULL;
    std::list<LPCONNECTION_T>::iterator pos;
    std::list<LPCONNECTION_T>::iterator posEnd;

    ThreadItem *pThread = static_cast<ThreadItem*>(pThreadData);
    CSocket *pSocketObj = pThread->_pThis;
    
    while(true)
    {   
        /* Check once every 1000ms */
        usleep(CHECK_RECYCONN_TIMER * 1000);
        /* Regardless of the situation, first of all the actions that should be done when this condition is established, 
         * the first thing to do is to determine whether there is a pstConn in the delayed collection queue 
         * that requires delayed collection. 200 milliseconds coming */
        if(pSocketObj->m_DelayRecoveryConnectionCount > 0)
        {
            CurrentTime = time(NULL);
            if(pthread_mutex_lock(&pSocketObj->m_RecyConnQueueMutex) != 0)
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_lock(&pSocketObj->m_RecyConnQueueMutex) failed");
                return (void *)XiaoHG_ERROR;
            }

lblRRTD:
            /* Each time a message in a queue is processed, 
             * the iterator will be invalidated and need to be reassigned */
            pos = pSocketObj->m_RecyConnectionList.begin();
			posEnd = pSocketObj->m_RecyConnectionList.end();
            for(; pos != posEnd; ++pos)
            {
                pConn = *pos;
                /* If you don’t want the whole system to exit, you can continue, otherwise you have to force release */
                if((pConn->putRecyConnQueueTime + pSocketObj->m_DelayRecoveryConnectionWaitTime) > CurrentTime)
                {
                    /* Before the release time, the pstConn continues to check */
                    continue;
                }
                
                /* Anything up to the release time, throwSendMsgCount should be 0, here we add some logs */
                if(pConn->throwSendMsgCount > 0)
                {
                    pConn->throwSendMsgCount = 0;
                    CLog::Log(LOG_LEVEL_EMERG, "pConn->throwSendMsgCount = 0, Should not enter this branch");
                }
                /* Return the pstConn represented by the parameter pConn to the pstConn pool */
                --pSocketObj->m_DelayRecoveryConnectionCount;
                pSocketObj->m_RecyConnectionList.erase(pos);
                pSocketObj->FreeConnection(pConn);
                goto lblRRTD;
            } /* end for */

            if(pthread_mutex_unlock(&pSocketObj->m_RecyConnQueueMutex) != 0)
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_unlock(&pSocketObj->m_RecyConnQueueMutex) failed");
                return (void *)XiaoHG_ERROR;
            }
        } /* end if */
    } /* end while */
    
    return (void*)XiaoHG_SUCCESS;
}

void CSocket::ClearDelayRecoveryConnections()
{
    LPCONNECTION_T pConn = NULL;
    std::list<LPCONNECTION_T>::iterator pos;
    std::list<LPCONNECTION_T>::iterator posEnd;

    if(m_DelayRecoveryConnectionCount > 0)
    {
        /* Because you want to quit, you have to hard release [no matter when the time is up, 
         * no matter whether there are other requirements that do not allow release, you have to hard release] */
        if(pthread_mutex_lock(&m_RecyConnQueueMutex) != 0)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_lock(&pSocketObj->m_RecyConnQueueMutex) failed");
            exit(0);
        }

lblRRTD2:
        pos = m_RecyConnectionList.begin();
        posEnd = m_RecyConnectionList.end();
        for(; pos != posEnd; ++pos)
        {
            /* Return the pstConn represented by the parameter pConn to the pstConn pool */
            pConn = *pos;
            --m_DelayRecoveryConnectionCount;
            m_RecyConnectionList.erase(pos);
            FreeConnection(pConn);
            goto lblRRTD2;
        } /* end for */
        
        if(pthread_mutex_unlock(&m_RecyConnQueueMutex) != 0)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "m_RecyConnQueueMutex unlock failed");
            exit(0);
        }
    } /* end if */
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: CloseConnectionImmediately
 * discription: Close a pstConn 
 * parameter:
 * =================================================================*/
void CSocket::CloseConnectionImmediately(LPCONNECTION_T pConn)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::CloseConnectionImmediately track");

    FreeConnection(pConn); 
    if(pConn->sockFd != -1)
    {
        close(pConn->sockFd);
        pConn->sockFd = -1;
    }
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ClearConnection
 * discription: clear pstConn list 
 * parameter:
 * =================================================================*/
void CSocket::ClearConnections()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::ClearConnections track");
    
    LPCONNECTION_T pConn = NULL;
	while(!m_ConnectionList.empty())
	{
        /* this is nedd us Call the destructor manually 
         * Because when it is released, it is released 
         * according to void *, and the destructor 
         * will not be called*/ 
		pConn = m_ConnectionList.front();
		m_ConnectionList.pop_front(); 
        pConn->~CONNECTION_T();
		CMemory::GetInstance()->FreeMemory(pConn);
	}
}
