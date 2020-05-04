
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
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
#include "XiaoHG_Func.h"
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_LockMutex.h"

#define __THIS_FILE__ "XiaoHG_C_SocketConn.cxx"

connection_s::connection_s()
{		
    uiCurrentSequence = 0;    
    pthread_mutex_init(&LogicPorcMutex, NULL);
}
connection_s::~connection_s()
{
    pthread_mutex_destroy(&LogicPorcMutex);
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: GetOneToUse
 * discription: Init pConnection
 * parameter:
 * =================================================================*/
void connection_s::GetOneToUse()
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "connection_s::GetOneToUse track");

    ++uiCurrentSequence;                        /* The serial number is increased by 1 during initialization */
    iSockFd = -1;                             /* in the beging = -1 */
    iRecvCurrentStatus = PKG_HD_INIT;           /* The packet receiving state is in the initial state, ready to receive the data packet header [state machine] */
    pRecvMsgBuff = dataHeadInfo;                /* first receive packet header */
    iRecvMsgLen = sizeof(COMM_PKG_HEADER);      /* receive packet lenght, first receive header = sizeof(COMM_PKG_HEADER) */
    pRecvMsgMemPointer = NULL;                  /* receive packet point */
    iThrowSendCount = 0;                        /* Rely on epoll to send data */
    pSendMsgMemPointer = NULL;                  /* send data point */
    iEpollEvents = 0;                           /* epoll event, defult = 0 */
    iLastHeartBeatTime = time(NULL);            /* last time Heartbeat time */
    uiFloodKickLastTime = 0;                    /* last time receive Flood massage*/
	iFloodAttackCount = 0;	                    /* Flood times */
    iSendQueueCount = 0;                        /* send massage list count */
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: FreeConnection
 * discription: Init back pConnection
 * parameter:
 * =================================================================*/
void connection_s::PutOneToFree()
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "connection_s::PutOneToFree track");

    ++uiCurrentSequence;    /* Increase the serial number by 1 */
    /* free memory */
    /* pRecvMsgMemPointer munber for free */
    if(pRecvMsgMemPointer != NULL)
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "PutOneToFree pRecvMsgMemPointer, pRecvMsgMemPointer = %p", pRecvMsgMemPointer);
        //CMemory::GetInstance()->FreeMemory(pRecvMsgMemPointer);
    }
    /* If there is content in the buffer for sending data, 
     * you need to release the memory */
    if(pSendMsgMemPointer != NULL) 
    {
        XiaoHG_Log(LOG_ALL, LOG_LEVEL_ERR, errno, "PutOneToFree pSendMsgMemPointer, pSendMsgMemPointer = %p", pSendMsgMemPointer);
        //CMemory::GetInstance()->FreeMemory(pSendMsgMemPointer);
    }
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: InitConnectionPool
 * discription: Init pConnection pool
 * parameter:
 * =================================================================*/
void CSocket::InitConnectionPool()
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CSocket::InitConnectionPool track");

    LPCONNECTION_T pConn = NULL;
    CMemory *pMemory = CMemory::GetInstance();   
    /* Create */
    for(int i = 0; i < m_EpollCreateConnectCount; i++) 
    {
        /* Apply for memory, because the new char is allocated here, the constructor cannot be executed
         * so we need to call the constructor manually */
        pConn = (LPCONNECTION_T)pMemory->AllocMemory(sizeof(CONNECTION_T), true); 
        pConn = new(pConn) CONNECTION_T();
        pConn->GetOneToUse();
        m_ConnectionList.push_back(pConn);
        m_FreeConnectionList.push_back(pConn);
    } /* end for */
    /* In the begining is some length */
    m_FreeConnectionCount = m_TotalConnectionCount = m_ConnectionList.size(); 

    XiaoHG_Log(LOG_ALL, LOG_LEVEL_INFO, 0, "Init connection pool successful");
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: GetConnection
 * discription: get a new pConnection from free pConnection list
 * parameter:
 * =================================================================*/
LPCONNECTION_T CSocket::GetConnection(int iSockFd)
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CSocket::GetConnection track");

    /* lock the pConnection list */
    CLock lock(&m_ConnectionMutex);
    /* have free pConnection or not*/
    if(!m_FreeConnectionList.empty())
    {
        LPCONNECTION_T pConn = m_FreeConnectionList.front();
        m_FreeConnectionList.pop_front();
        pConn->GetOneToUse();
        --m_FreeConnectionCount; 
        pConn->iSockFd = iSockFd;
        return pConn;
    }

    /* this is no more free pConnection for, need to alloc more for use */
    CMemory *pMemory = CMemory::GetInstance();
    LPCONNECTION_T pConn = (LPCONNECTION_T)pMemory->AllocMemory(sizeof(CONNECTION_T), true);
    pConn = new(pConn) CONNECTION_T();
    pConn->GetOneToUse();
    m_ConnectionList.push_back(pConn);
    ++m_TotalConnectionCount;
    pConn->iSockFd = iSockFd;
    return pConn;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: FreeConnection
 * discription: Return the pConnection represented by the parameter 
 *              pConn to the pConnection pool.
 * parameter:
 * =================================================================*/
void CSocket::FreeConnection(LPCONNECTION_T pConn) 
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CSocket::FreeConnection track");

    /* lock free pConnection list */
    CLock lock(&m_ConnectionMutex);
    /* free pConnection */
    pConn->PutOneToFree();
    /* push free pConnection list */
    m_FreeConnectionList.push_back(pConn);
    ++m_FreeConnectionCount;
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: PutConnectToRecyQueue
 * discription: Put the pConnection to be recycled into a queue,    
 *              and a subsequent special thread will handle the 
 *              recovery of the pConnection in this queue. Some 
 *              connections, we do not want to release immediately, 
 *              we must release it after a period of time to ensure 
 *              the stability of the server, We put this pConnection 
 *              that was released after a certain period of time into 
 *              a queue first.
 * parameter:
 * =================================================================*/
void CSocket::PutConnectToRecyQueue(LPCONNECTION_T pConn)
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CSocket::PutConnectToRecyQueue track");

    bool bIsFind = false;
    std::list<LPCONNECTION_T>::iterator pos;
    /* lock recy connect queue */
    CLock lock(&m_RecyConnQueueMutex); 
    /* put one time */
    for(pos = m_RecyConnectionList.begin(); pos != m_RecyConnectionList.end(); pos++)
	{
		if((*pos) == pConn)
		{	
			bIsFind = true;
			break;
		}
	}
    /* exist */
    if(bIsFind == true) 
	{
        return;
    }
    pConn->PutRecyConnQueueTime = time(NULL);              /* record put time */
    ++pConn->uiCurrentSequence;                 /* CurrSequence change */
    m_RecyConnectionList.push_back(pConn);      /* push */
    ++m_TotalRecyConnectionCount;
    --m_OnLineUserCount;
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ServerRecyConnectionThread
 * discription: Thread to handle delayed pConnection recovery
 * parameter:
 * =================================================================*/
void* CSocket::ServerRecyConnectionThread(void* pThreadData)
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CSocket::ServerRecyConnectionThread track");

    time_t CurrentTime = 0;
    LPCONNECTION_T pConn = NULL;
    std::list<LPCONNECTION_T>::iterator pos;
    std::list<LPCONNECTION_T>::iterator posEnd;

    ThreadItem *pThread = static_cast<ThreadItem*>(pThreadData);
    CSocket *pSocketObj = pThread->_pThis;
    
    while(true)
    {   
        /* Check once every 1000ms */
        usleep(CHECK_RECYCONN_TIME * 1000);
        /* Regardless of the situation, first of all the actions that should be done when this condition is established, 
         * the first thing to do is to determine whether there is a pConnection in the delayed collection queue 
         * that requires delayed collection. 200 milliseconds coming */
        if(pSocketObj->m_TotalRecyConnectionCount > 0)
        {
            CurrentTime = time(NULL);
            if(pthread_mutex_lock(&pSocketObj->m_RecyConnQueueMutex) != 0)
            {
                XiaoHG_Log(LOG_FILE, LOG_LEVEL_ERR, errno, "pthread_mutex_lock(&pSocketObj->m_RecyConnQueueMutex) failed");
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
                /* If you do n’t want the whole system to exit, you can continue, otherwise you have to force release */
                if(((pConn->PutRecyConnQueueTime + pSocketObj->m_RecyConnectionWaitTime) > CurrentTime) && (!g_bIsStopEvent))
                {
                    /* Before the release time, the pConnection continues to check */
                    continue;
                }
                XiaoHG_Log(LOG_FILE, LOG_LEVEL_INFO, 0, "ServerRecyConnectionThread, Delay the recovery time to recover the connection: socket id = %d", 
                                                                                                                                    pConn->iSockFd);
                /* Anything up to the release time, iThrowSendCount should be 0, here we add some logs */
                if(pConn->iThrowSendCount > 0)
                {
                    pConn->iThrowSendCount = 0;
                    XiaoHG_Log(LOG_FILE, LOG_LEVEL_EMERG, errno, "pConn->iThrowSendCount = 0, Should not enter this branch");
                }
                /* Return the pConnection represented by the parameter pConn to the pConnection pool */
                --pSocketObj->m_TotalRecyConnectionCount;
                pSocketObj->m_RecyConnectionList.erase(pos);
                pSocketObj->FreeConnection(pConn);
                goto lblRRTD;
            } /* end for */

            if(pthread_mutex_unlock(&pSocketObj->m_RecyConnQueueMutex) != 0)
            {
                XiaoHG_Log(LOG_FILE, LOG_LEVEL_ERR, errno, "pthread_mutex_unlock(&pSocketObj->m_RecyConnQueueMutex) failed");
                return (void *)XiaoHG_ERROR;
            }
        } /* end if */

        /* Exit the entire program */
        if(g_bIsStopEvent)
        {
            if(pSocketObj->m_TotalRecyConnectionCount > 0)
            {
                /* Because you want to quit, you have to hard release [no matter when the time is up, 
                 * no matter whether there are other requirements that do not allow release, you have to hard release] */
                if(pthread_mutex_lock(&pSocketObj->m_RecyConnQueueMutex) != 0)
                {
                    XiaoHG_Log(LOG_FILE, LOG_LEVEL_ERR, errno, "pthread_mutex_lock(&pSocketObj->m_RecyConnQueueMutex) failed");
                    return (void *)XiaoHG_ERROR;
                }

        lblRRTD2:
                pos = pSocketObj->m_RecyConnectionList.begin();
			    posEnd = pSocketObj->m_RecyConnectionList.end();
                for(; pos != posEnd; ++pos)
                {
                    /* Return the pConnection represented by the parameter pConn to the pConnection pool */
                    pConn = *pos;
                    --pSocketObj->m_TotalRecyConnectionCount;
                    pSocketObj->m_RecyConnectionList.erase(pos);
                    pSocketObj->FreeConnection(pConn);
                    goto lblRRTD2;
                } /* end for */
                
                if(pthread_mutex_unlock(&pSocketObj->m_RecyConnQueueMutex) != 0)
                {
                    XiaoHG_Log(LOG_FILE, LOG_LEVEL_ERR, errno, "m_RecyConnQueueMutex unlock failed");
                    return (void *)XiaoHG_ERROR;
                }
            } /* end if */
            XiaoHG_Log(LOG_FILE, LOG_LEVEL_NOTICE, 0, "g_bIsStopEvent is true, process exit");
            break; /* The whole program is about to exit, so break */
        }  /* end if */
    } /* end while */
    
    return (void*)XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: CloseConnectionImmediately
 * discription: Close a pConnection 
 * parameter:
 * =================================================================*/
void CSocket::CloseConnectionImmediately(LPCONNECTION_T pConn)
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CSocket::CloseConnectionImmediately track");

    FreeConnection(pConn); 
    if(pConn->iSockFd != -1)
    {
        close(pConn->iSockFd);
        pConn->iSockFd = -1;
    }
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ClearConnection
 * discription: clear pConnection list 
 * parameter:
 * =================================================================*/
void CSocket::ClearConnections()
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CSocket::ClearConnections track");
    
    LPCONNECTION_T pConn = NULL;
	CMemory *pMemory = CMemory::GetInstance();
	while(!m_ConnectionList.empty())
	{
        /* this is nedd us Call the destructor manually 
         * Because when it is released, it is released 
         * according to void *, and the destructor 
         * will not be called*/ 
		pConn = m_ConnectionList.front();
		m_ConnectionList.pop_front(); 
        pConn->~CONNECTION_T();
		pMemory->FreeMemory(pConn);
	}
}
