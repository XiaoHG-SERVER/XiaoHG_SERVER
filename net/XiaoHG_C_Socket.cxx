
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
#include "XiaoHG_Func.h"
#include <XiaoHG_C_Socket.h>
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_LockMutex.h"

#define __THIS_FILE__ "XiaoHG_C_Socket.cxx"

CSocket::CSocket()
{
    m_EpollCreateConnectCount = 1;                    /* Epoll max connect */
    m_ListenPortCount = 1;                      /* Default listening port count */
    m_RecyConnectionWaitTime = 60;              /* defult waiting for recycling time */
    m_EpollHandle = -1;                         /* Epoll handle */
    m_iLenPkgHeader = sizeof(COMM_PKG_HEADER);  /* packet header size */
    m_iLenMsgHeader = sizeof(MSG_HEADER_T);     /* Message header size */
    m_iSendMsgQueueCount = 0;                   /* Send queue lenght */
    m_TotalRecyConnectionCount = 0;                  /* Connection queue size to be released */
    m_CurrentHeartBeatListSize = 0;                  /* Current timing queue size */
    m_HBListEarliestTime = 0;                           /* The earliest time value of the current time */
    m_iDiscardSendPkgCount = 0;                 /* Number of send packets dropped */
    m_OnLineUserCount = 0;                      /* Number of online users, default 0 */
    m_LastPrintTime = 0;                        /* Last time the statistics were printed, default 0 */
    return;	
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: Initalize
 * discription: Init socket
 * parameter:
 * =================================================================*/
int CSocket::Initalize()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::Initalize track");

    LoadConfig(); /* load config */
    /* open the listen iPort */
    return OpenListeningSockets();
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: InitializeSubProc
 * discription: Init worker process
 * parameter:
 * =================================================================*/
int CSocket::InitializeSubProc()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::InitializeSubProc track");

    /* Message queue mutex initialization */
    if(pthread_mutex_init(&m_SendMessageQueueMutex, NULL) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_init(m_SendMessageQueueMutex) failed");
        return XiaoHG_ERROR;
    }

    /* Connection queue related mutex initialization */
    if(pthread_mutex_init(&m_ConnectionMutex, NULL) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_init(m_ConnectionMutex) failed");
        return XiaoHG_ERROR;
    }

    /* Initialization of the mutex related to the pConnection recovery queue */
    if(pthread_mutex_init(&m_RecyConnQueueMutex, NULL) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_init(m_RecyConnQueueMutex) failed");
        return XiaoHG_ERROR;   
    }

    /* Initialization of the mutex related to the time processing queue */
    if(pthread_mutex_init(&m_TimeQueueMutex, NULL) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_init(m_TimeQueueMutex) failed");
        return XiaoHG_ERROR; 
    }
   
    /* Initialize the semaphore related to the message. The semaphore is used 
     * for synchronization between processes / threads */
    if(sem_init(&m_SemEventSendQueue, 0, 0) == -1)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "sem_init(m_SemEventSendQueue) failed");
        return XiaoHG_ERROR; 
    }

    /* send msg thread */
    ThreadItem *pSendQueueThreadItem = new ThreadItem(this);  
    m_ThreadPoolVector.push_back(pSendQueueThreadItem);
    if(pthread_create(&pSendQueueThreadItem->_Handle, NULL, SendMsgQueueThreadProc, pSendQueueThreadItem) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_create(SendMsgQueueThreadProc) failed");
        return XiaoHG_ERROR;
    }

    /* recy connect thread */
    ThreadItem *pRecyConnThreadItem = new ThreadItem(this);
    m_ThreadPoolVector.push_back(pRecyConnThreadItem);
    if(pthread_create(&pRecyConnThreadItem->_Handle, NULL, ServerRecyConnectionThreadProc, pRecyConnThreadItem) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_create(ServerRecyConnectionThreadProc) failed");
        return XiaoHG_ERROR; 
    }

    /* Whether to turn on the kick clock, 1：open; 0：close*/
    if(m_IsHBTimeOutCheckEnable == 1)
    {
        /* test head beat thread */
        ThreadItem *pTimemonitor = new ThreadItem(this);
        m_ThreadPoolVector.push_back(pTimemonitor);
        if(pthread_create(&pTimemonitor->_Handle, NULL, HeartBeatMonitorThreadProc, pTimemonitor) != 0)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_create(HeartBeatMonitorThreadProc) failed");
            return XiaoHG_ERROR; 
        }
    }
    
    CLog::Log("Subprocess init successful");
    return XiaoHG_SUCCESS;
}

CSocket::~CSocket()
{
    /* Listen for the release of memory related to the port */
    std::vector<LPLISTENING_T>::iterator pos;	
	for(pos = m_ListenSocketList.begin(); pos != m_ListenSocketList.end(); pos++)
	{
		delete (*pos); 
	}/* end for */
	m_ListenSocketList.clear();
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ShutdownSubProc
 * discription: shut down sub process
 * parameter:
 * =================================================================*/
void CSocket::ShutdownSubProc()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::ShutdownSubProc track");

    /* make ServerSendQueueThread go on */
    if(sem_post(&m_SemEventSendQueue)==-1)  
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "release send message thread failed");
    }

    std::vector<ThreadItem*>::iterator iter;
	for(iter = m_ThreadPoolVector.begin(); iter != m_ThreadPoolVector.end(); iter++)
    {
        /* wait a thread return */
        pthread_join((*iter)->_Handle, NULL); 
    }

    /* free all thread item */    
	for(iter = m_ThreadPoolVector.begin(); iter != m_ThreadPoolVector.end(); iter++)
	{
		if(*iter)
			delete *iter;
	}
	m_ThreadPoolVector.clear();

    /* free */
    ClearMsgSendQueue();
    ClearConnections();
    ClearAllFromTimerQueue();
    
    /* destory */
    pthread_mutex_destroy(&m_ConnectionMutex);          /* Connection related mutex release */
    pthread_mutex_destroy(&m_SendMessageQueueMutex);    /* Message mutex release */
    pthread_mutex_destroy(&m_RecyConnQueueMutex);       /* Connection release queue related mutex release */
    pthread_mutex_destroy(&m_TimeQueueMutex);           /* Time processing queue related mutex release */
    sem_destroy(&m_SemEventSendQueue);                  /* Semaphore related thread release */
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ClearMsgSendQueue
 * discription: clear TCP send message list
 * parameter:
 * =================================================================*/
void CSocket::ClearMsgSendQueue()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::ClearMsgSendQueue track");

	char *pMemPoint = NULL;
	CMemory *pMemory = CMemory::GetInstance();
	while(!m_MsgSendQueue.empty())
	{
		pMemPoint = m_MsgSendQueue.front();
		m_MsgSendQueue.pop_front(); 
		pMemory->FreeMemory(pMemPoint);
	}	
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: LoadConfig
 * discription: load configs
 * parameter:
 * =================================================================*/
void CSocket::LoadConfig()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::LoadConfig track");

    CConfig *pConfig = CConfig::GetInstance();
    /* The largest item connected of epoll*/
    m_EpollCreateConnectCount = pConfig->GetIntDefault("EpollCreateConnectCount", m_EpollCreateConnectCount);
    /* Get listen numbers */
    m_ListenPortCount = pConfig->GetIntDefault("ListenPortCount", m_ListenPortCount);
    /* Get delayed recovery time */
    m_RecyConnectionWaitTime = pConfig->GetIntDefault("Sock_RecyConnectionWaitTime", m_RecyConnectionWaitTime);
    /* delete connect tick */
    m_IsHBTimeOutCheckEnable = pConfig->GetIntDefault("Sock_WaitTimeEnable", 0);
    /* how long chick heat beat */
	m_iWaitTime = pConfig->GetIntDefault("Sock_MaxWaitTime", m_iWaitTime);
    /* It is not recommended to be less than 5 seconds because it does not need to be too frequent */
	m_iWaitTime = m_iWaitTime > 5 ? m_iWaitTime : 5;
    /* Time out, delete connect */
    m_iIsTimeOutKick = pConfig->GetIntDefault("Sock_TimeOutKick", 0);
    /* Flood chick */
    m_FloodAttackEnable = pConfig->GetIntDefault("Sock_FloodAttackKickEnable", 0);
    /* Flood chick time is 100ms for a time */
	m_FloodTimeInterval = pConfig->GetIntDefault("Sock_FloodTimeInterval", 100);
    /* How many times have you kicked this person */
	m_FloodKickCount = pConfig->GetIntDefault("Sock_FloodKickCounter", 10);
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: OpenListeningSockets
 * discription: open listening
 * parameter:
 * =================================================================*/
int CSocket::OpenListeningSockets()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::OpenListeningSockets track");

    int iPort = 0;
    int iSockFd = 0;
    char strListeningPortInfo[100] = { 0 };
    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    CConfig *pConfig = CConfig::GetInstance();

    for(int i = 0; i < m_ListenPortCount; i++)
    {
        /* create socket */
        iSockFd = socket(AF_INET, SOCK_STREAM, 0);
        if(iSockFd == -1)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "create listening socket failed: %d", i);
            return XiaoHG_ERROR;
        }

        /* set socket noblock */
        if(SetNonBlocking(iSockFd) == XiaoHG_ERROR)
        {                
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "set socket nonblock failed: %d", i);
            close(iSockFd);
            return XiaoHG_ERROR;
        }

        /* set listening iPort */
        sprintf(strListeningPortInfo, "ListenPort%d", i);
        iPort = pConfig->GetIntDefault(strListeningPortInfo, 10000);
        serv_addr.sin_port = htons((in_port_t)iPort); /* in_port_t->uint16_t */

        /* bind */
        if(bind(iSockFd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "bind socket failed: %d, socket id: %d", i, iSockFd);
            close(iSockFd);
            return XiaoHG_ERROR;
        }
        
        /* listening */
        if(listen(iSockFd, XHG_LISTEN_SOCKETS) == -1)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "listening socket failed: %d, socket id: %d", i, iSockFd);
            close(iSockFd);
            return XiaoHG_ERROR;
        }

        LPLISTENING_T pListenSocketItem = new LISTENING_T;
        memset(pListenSocketItem, 0, sizeof(LISTENING_T));
        pListenSocketItem->iPort = iPort;
        pListenSocketItem->iSockFd = iSockFd;
        /* push the listening list */
        m_ListenSocketList.push_back(pListenSocketItem); 
        /* listening successful write the log file */
    } /* end for(int i = 0; i < m_ListenPortCount; i++)  */
    
    /* no listening iPort */
    if(m_ListenSocketList.size() <= 0)
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "No listening iPort");
        return XiaoHG_ERROR;
    }

    CLog::Log("Open listening sockets successful");
    return XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: SetNonBlocking
 * discription: set socket nonblock
 * parameter:
 * =================================================================*/
int CSocket::SetNonBlocking(int iSockFd) 
{    
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::SetNonBlocking track");

    int nb = 1; /* 0：clean, 1：set */
    if(ioctl(iSockFd, FIONBIO, &nb) == -1) 
    {
         CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Set nonblock socket id: %d failed", iSockFd);
        return XiaoHG_ERROR;
    }
    return XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: SendMsg
 * discription: Put a message to be sent into the message queue.
 * parameter:
 * =================================================================*/
void CSocket::SendMsg(char *pSendBuff) 
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::SendMsg track");

    CMemory *pMemory = CMemory::GetInstance();
    /* m_SendMessageQueueMutex lock */
    CLock lock(&m_SendMessageQueueMutex);
    /* Send message queue is too large may also bring risks to the server */
    if(m_iSendMsgQueueCount > MAX_SENDQUEUE_COUNT)
    {
        /* If the sending queue is too large, for example, if the client maliciously does not accept the data, 
         * it will cause this queue to grow larger and larger. Then you can consider killing some data 
         * transmission for server security. Although it may cause problems for the client, 
         * it is better than the server. Stability is much better */
        ++m_iDiscardSendPkgCount;
        pMemory->FreeMemory(pSendBuff);
		return;
    }
    /* Process packets normally */
    LPMSG_HEADER_T pMsgHeader = (LPMSG_HEADER_T)pSendBuff;
	LPCONNECTION_T pConn = pMsgHeader->pConn;
    /* The user receives the message too slowly [or simply does not receive the message], 
     * the accumulated number of data entries in the user's sending queue is too large, 
     * and it is considered to be a malicious user and is directly cut*/
    if(pConn->iSendQueueCount > MAX_EACHCONN_SENDCOUNT)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Found that a user %d has a large backlog of packets to be sent", pConn->iSockFd); 
        ++m_iDiscardSendPkgCount;
        pMemory->FreeMemory(pSendBuff);
        CloseConnectionToRecy(pConn);
		return;
    }

    m_MsgSendQueue.push_back(pSendBuff);
    ++pConn->iSendQueueCount;
    ++m_iSendMsgQueueCount;

    /* +1 the value of the semaphore so that others stuck in sem_wait can go on 
     * Let the ServerSendQueueThread() process come down and work */
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
 * function name: CloseConnectionInRecy
 * discription: When actively closing a pConnection, do some aftercare function
 * parameter:
 * =================================================================*/
void CSocket::CloseConnectionToRecy(LPCONNECTION_T pConn)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::CloseConnectionToRecy track");

    if(m_IsHBTimeOutCheckEnable == 1)
    {
        /* clrean up the connect if in the time list */
        DeleteFromTimerQueue(pConn);
    }
    if(pConn->iSockFd != -1)
    {
        /* This socket is closed, and epoll will be deleted 
         * from the red and black tree after closing, 
         * so no epoll iEpollEvents can be received after this */
        close(pConn->iSockFd); 
        pConn->iSockFd = -1;
    }
    if(pConn->iThrowSendCount > 0)
    {
        pConn->iThrowSendCount = 0;   /* 0 */
    }
    /* Add the pConnection to the delayed recycling 
     * queue and let the delayed recycling mechanism manage */
    PutConnectToRecyQueue(pConn);
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: TestFlood
 * discription: 
 * parameter:
 * =================================================================*/
bool CSocket::TestFlood(LPCONNECTION_T pConn)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::TestFlood track");

    struct timeval sCurrTime;   /* Current time structure */
	uint64_t iCurrTime;         /* Current time (ms) */
    /* Get current time */
	gettimeofday(&sCurrTime, NULL); 
    iCurrTime = (sCurrTime.tv_sec * 1000 + sCurrTime.tv_usec / 1000);
    /* When the package was received twice <100 ms */
	if((iCurrTime - pConn->uiFloodKickLastTime) < m_FloodTimeInterval)    
	{
        /* Recording too frequently */
		++pConn->iFloodAttackCount;
		pConn->uiFloodKickLastTime = iCurrTime;
	}
	else
	{
         /* Back to normal */
		pConn->iFloodAttackCount = 0;
		pConn->uiFloodKickLastTime = iCurrTime;
	}

    /* Whether it is a flood attack */
	if(pConn->iFloodAttackCount >= m_FloodKickCount)
	{
		/* disconnection */
        CLog::Log(LOG_LEVEL_ALERT, "Check out socket: %d is flood attack, disconnection", pConn->iSockFd);
		return true;
	}
	return false;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: EpollInit
 * discription: Init epoll
 * parameter:
 * =================================================================*/
int CSocket::EpollInit()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::EpollInit track");

    m_EpollHandle = epoll_create(m_EpollCreateConnectCount);
    if (m_EpollHandle == -1) 
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "create epoll failed");
        return XiaoHG_ERROR; 
    }
    /* Init connect */
    InitConnectionPool();
    std::vector<LPLISTENING_T>::iterator pos;
    /* Set listening socket join epoll event */	
	for(pos = m_ListenSocketList.begin(); pos != m_ListenSocketList.end(); pos++)
    {
        /* get free pConnection from free list for new socket */
        LPCONNECTION_T pConn = GetConnection((*pos)->iSockFd); 
        if (pConn == NULL)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "get free connection failed");
            return XiaoHG_ERROR;
        }
        /* The pConnection object is associated with the monitoring object, 
         * which is convenient for finding the monitoring object through 
         * the pConnection object. */
        pConn->listening = (*pos);  
        /* The monitoring object is associated with the pConnection object, 
         * which is convenient for finding the pConnection object through 
         * the monitoring object. */
        (*pos)->pConnection = pConn;  
        /* Set the processing method for the read event of the listening iPort, 
         * because the listening iPort is used to wait for the other party to 
         * send a three-way handshake, so the listening iPort is concerned with 
         * the read event. */
        pConn->pcbReadHandler = &CSocket::EpollEventAcceptHandler;
        if(EpollRegisterEvent((*pos)->iSockFd,          /* socekt id */
                                EPOLL_CTL_ADD,          /* event mode */
                                EPOLLIN|EPOLLRDHUP,     /* uiFlag: EPOLLIN：read, EPOLLRDHUP：TCP connection is closed or half closed */
                                0,                      /* For the event type is increased, this parameter is not required */
                                pConn                   /* pConnection pool obj */
                                ) == -1) 
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "listening socket add epoll event failed");
            return XiaoHG_ERROR;
        }
    } /* end for  */
    
    CLog::Log("Epoll Init successful");
    return XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: EpollRegisterEvent
 * discription: register epoll event
 * parameter:
 * =================================================================*/
int CSocket::EpollRegisterEvent(int iSockFd, uint32_t uiEventType, uint32_t uiFlag, int iAction, LPCONNECTION_T pConn)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::EpollRegisterEvent track");

    struct epoll_event ev;    
    memset(&ev, 0, sizeof(ev));
    /* add node */
    if(uiEventType == EPOLL_CTL_ADD)
    {
        /* EPOLL_CTL_ADD flag: Direct coverage */
        ev.events = uiFlag;
        /* record to the pConnection */  
        pConn->iEpollEvents = uiFlag;
    }
    
    /* the node already, change the mark*/
    else if(uiEventType == EPOLL_CTL_MOD)
    {
        ev.events = pConn->iEpollEvents;  /* Restore the mark first */
        switch (iAction)
        {
        case EPOLL_ADD:/* add mark */  
            ev.events |= uiFlag;
            break;
        case EPOLL_DEL:/* delete mark */
            ev.events &= ~uiFlag;
            break;
        case EPOLL_OVER:/* Overwrite a mark */
            ev.events = uiFlag; 
            break;
        default: 
            break;
        }
        /* record */
        pConn->iEpollEvents = ev.events; 
    }
    else
    {
        /* Keep */
        return XiaoHG_SUCCESS;
    } 
    /* set the event memory point*/
    ev.data.ptr = (void *)pConn;
    
    /* This function is used to control iEpollEvents on an epoll file descriptor. 
     * You can register iEpollEvents, modify iEpollEvents, and delete iEpollEvents */
    if(epoll_ctl(m_EpollHandle, uiEventType, iSockFd, &ev) == -1)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "epoll_ctl(%d, %ud, %ud, %d) failed", iSockFd, uiEventType, uiFlag, iAction);
        return XiaoHG_ERROR;
    }
    return XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: EpolWaitlProcessEvents
 * discription: Start to get the event message that occurred, 
 *              parameter unsigned int iTimer: epoll_wait() blocking duration, 
 *              unit is milliseconds, return value, XiaoHG_SUCCESS: normal return, 
 *              XiaoHG_ERROR: return if there is a problem, generally whether 
 *              the process is normal or return, you should keep the process continuing Run, 
 *              this function is called by ProcessEventsAndTimers(), and ProcessEventsAndTimers() 
 *              is called repeatedly in the endless loop of the child process
 * parameter:
 * =================================================================*/
int CSocket::EpolWaitlProcessEvents(int iTimer) 
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::EpolWaitlProcessEvents track");

    /* Wait for the event, the event will return to m_Events, at most return EPOLL_MAX_EVENTS iEpollEvents [because I only provide these memories], 
     * if the interval between two calls to epoll_wait () is relatively long, it may accumulate Events, 
     * so calling epoll_wait may fetch multiple iEpollEvents, blocking iTimer for such a long time unless: 
     * a) the blocking time arrives, 
     * b) the event received during the blocking [such as a new user to connect] will return immediately, 
     * c) when calling The event will return immediately, 
     * d) If there is a signal, for example, you use kill -1 pid, 
     * if iTimer is -1, it will always block, if iTimer is 0, it will return immediately, even if there is no event.
     * Return value: If an error occurs, -1 is returned. The error is in errno. For example, 
     * if you send a signal, it returns -1. The error message is (4: Interrupted system call)
     * If you are waiting for a period of time, and it times out, it returns 0. If it returns> 0, 
     * it means that so many iEpollEvents have been successfully captured [in the return value] */
    int iEpollEvents = epoll_wait(m_EpollHandle, m_Events, EPOLL_MAX_EVENTS, iTimer);
    if(iEpollEvents == -1)
    {
        /* If an error occurs, sending a signal to the process can cause this condition to be established, 
         * and the error code is #define EINTR 4, according to the observation, 
         * EINTR error occurs: when a process blocked by a slow system call captures a signal and 
         * When the corresponding signal processing function returns, the system call may return an EINTR error, 
         * for example: on the socket server side, a signal capture mechanism is set, there is a child process, 
         * when the parent process is blocked by a slow system call, the parent process captures a valid When signaled, 
         * the kernel will cause accept to return an EINTR error (interrupted system call) */
        if(errno == EINTR) 
        {
            /* Directly returned by the signal. It is generally considered that this is not a problem, 
             * but it is still printed in the log record, because it is generally not artificially sent to the worker process. */
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "epoll wait failed");
            return XiaoHG_SUCCESS;  /* Normal return */
        }
        else
        {
            /* error */
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "epoll wait failed");
            return XiaoHG_ERROR;  /* error return */
        }
    }

    /* Timed out, but no event came */
    if(iEpollEvents == 0)
    {
        if(iTimer != -1)
        {
            /* Require epoll_wait to block for a certain period of time instead of blocking all the time. 
             * This is blocking time and returns normally */
            return XiaoHG_SUCCESS;
        }
        /* Infinite wait [so there is no timeout], but no event is returned, this should be abnormal */
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "epoll wait failed");
        return XiaoHG_ERROR; /* error return */
    }

    uint32_t uiRevents = 0;
    LPCONNECTION_T pConn = NULL;
    /* Traverse all iEpollEvents returned by epoll_wait this time, 
     * note that iEpollEvents is the actual number of iEpollEvents returned */
    for(int i = 0; i < iEpollEvents; i++)
    {
        /* XiaoHG_epoll_add_event(), You can take it in here. */
        pConn = (LPCONNECTION_T)(m_Events[i].data.ptr);           

        /* Get event type */
        uiRevents = m_Events[i].events;
  
        /* read event */
        if(uiRevents & EPOLLIN)
        {
            /* (a)A new client is connected, this will be established.
             * (b)Connected to send data, this is also true. */
            (this->* (pConn->pcbReadHandler))(pConn);
        }
        
        /* write event [the other party closes the pConnection also triggers this] */
        if(uiRevents & EPOLLOUT) 
        {
            /* The client is closed, if a write notification event is hung on the server, 
             * this condition is possible */
            if(uiRevents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) 
            {
                /* EPOLLERR：The corresponding pConnection has an error.
                 * EPOLLHUP：The corresponding pConnection is suspended.
                 * EPOLLRDHUP：Indicates that the remote end of the TCP pConnection is closed or half closed. 
                 * We only posted the write event, but when the peer is disconnected, the program flow came here. Posting the write event means that the 
                 * iThrowSendCount flag must be +1, and here we subtract. */
                --pConn->iThrowSendCount;
            }
            else
            {
                /* Call data sending */
                (this->* (pConn->pcbWriteHandler))(pConn);
            }
        }
    } /* end for(int i = 0; i < iEpollEvents; ++i) */
    return XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: SendMsgQueueThreadProc
 * discription: send message.
 * parameter:
 * =================================================================*/
void* CSocket::SendMsgQueueThreadProc(void *pThreadData)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::SendMsgQueueThreadProc track");

    ssize_t uiSendLen = 0;                  /* Record the size of the data that has been sent */
    char *pMsgBuff = NULL;                  /* point send message */
    LPMSG_HEADER_T pMsgHeader = NULL;
	LPCOMM_PKG_HEADER pPkgHeader = NULL;
    LPCONNECTION_T pConn = NULL;
    std::list <char *>::iterator pos;
    std::list <char *>::iterator posTmp;
    std::list <char *>::iterator posEnd;

    ThreadItem *pThread = static_cast<ThreadItem *>(pThreadData);
    CSocket *pSocketObj = pThread->_pThis; 
    CMemory *pMemory = CMemory::GetInstance();
    
    while(!g_bIsStopEvent)
    {
        /* If the semaphore value is> 0, then decrement by 1 and go on, otherwise the card is stuck here. 
         * [In order to make the semaphore value +1, you can call sem_post in other threads to achieve it. 
         * In fact, CSocket :: SendMsg() calls sem_post. The purpose of letting sem_wait go down here] 
         * If it is interrupted by a signal, sem_wait may also return prematurely. The error is EINTR. 
         * Before the entire program exits, sem_post() is also necessary to ensure that if this thread is stuck in sem_wait (), 
         * Can go on so that this thread returns successfully */
        if(sem_wait(&pSocketObj->m_SemEventSendQueue) == -1)
        {
            /* This is not an error. [When a process blocked by a slow system call catches a 
             * signal and the corresponding signal processing function returns, the system call may return an EINTR error] */
            if(errno != EINTR) 
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "sem_wait(pSocketObj->m_SemEventSendQueue) failed");
            }
        }
        
        /* Require the entire process to exit */
        if(g_bIsStopEvent)
        {
            CLog::Log(LOG_LEVEL_NOTICE, __THIS_FILE__, __LINE__, "g_bIsStopEvent is true exit");
            break;
        }

        /* Atomic, to determine whether there is information in the sending message queue, 
         * this should be true, because sem_wait is in, when the message is added to the 
         * information queue, sem_post is called to trigger processing */
        if(pSocketObj->m_iSendMsgQueueCount > 0)
        {
            /* m_SendMessageQueueMutex lock */   
            if(pthread_mutex_lock(&pSocketObj->m_SendMessageQueueMutex) != 0)
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "m_SendMessageQueueMutex lock failed");
                return (void *)XiaoHG_ERROR;
            }
            pos = pSocketObj->m_MsgSendQueue.begin();
			posEnd = pSocketObj->m_MsgSendQueue.end();
            while(pos != posEnd)
            {
                /* Get message form list to pMsgBuff*/
                pMsgBuff = (*pos);
                pMsgHeader = (LPMSG_HEADER_T)pMsgBuff;
                pPkgHeader = (LPCOMM_PKG_HEADER)(pMsgBuff + pSocketObj->m_iLenMsgHeader);
                pConn = pMsgHeader->pConn;

                /* The data packet expires, because if this pConnection is recycled, 
                 * for example in CloseConnectionImmediately(), PutRecyConnectQueue(), 
                 * uiCurrentSequence will be incremented and there is no need to use 
                 * m_ConnectionMutex critical for this pConnection. As long as the following 
                 * conditions are true, it must be that the client pConnection has been disconnected, 
                 * The data to be sent definitely need not be sent */
                if(pConn->uiCurrentSequence != pMsgHeader->uiCurrentSequence)
                {
                    /* The serial number saved in this package is different from the actual 
                     * serial number in pConn [pConnection in pConnection pool], so discard this message */
                    posTmp = pos;
                    ++pos;
                    pSocketObj->m_MsgSendQueue.erase(posTmp);
                    --pSocketObj->m_iSendMsgQueueCount;
                    pMemory->FreeMemory(pMsgBuff);
                    continue; /* pNext message */
                } /* end if */

                if(pConn->iThrowSendCount > 0) 
                {
                    /* Rely on epoll event sending */
                    ++pos;
                    continue; /* pNext message */ 
                }

                /* When you come here, you can send a message, 
                 * some necessary information records, 
                 * and the things you want to send must be removed from the send queue */
                --pConn->iSendQueueCount;
                pConn->pSendMsgMemPointer = pMsgBuff; /* for free memory pSendMsgMemPointer */
                posTmp = pos;
				++pos;
                pSocketObj->m_MsgSendQueue.erase(posTmp);
                --pSocketObj->m_iSendMsgQueueCount;                     /* send message queue -1 */
                pConn->pSendMsgBuff = (char *)pPkgHeader;               /* save send buffer */
                pConn->iSendMsgLen = ntohs(pPkgHeader->sPkgLength);     /* host -> net */

                /* Here is the key point. We use the epoll level-triggered strategy. Anyone who can get here 
                 * should have not yet posted a write event to epoll.
                 * The improved scheme of sending data by epoll level trigger:
                 * I don't add the socket write event notification to epoll at the beginning. When I need to write data, 
                 * I directly call write / send to send the data.
                 * If you return EAGIN [indicating that the send buffer is full, you need to wait for a writeable event to continue writing data to the buffer], 
                 * at this time, I will add the write event notification to epoll, and it will become the epoll driver to write data , 
                 * After all the data is sent, then remove the write event notification from epoll
                 * Advantages: When there is not much data, the increase / deletion of epoll's write iEpollEvents can be avoided, 
                 * and the execution efficiency of the program is improved.*/
                /* (a)Directly call write or send to send data */
                uiSendLen = pSocketObj->SendProc(pConn, pConn->pSendMsgBuff, pConn->iSendMsgLen);
                if(uiSendLen > 0)
                {
                    /* The data was successfully sent, and all the data was sent out at once */
                    if(uiSendLen == pConn->iSendMsgLen)
                    {
                        /* The data successfully sent is equal to the data required to be sent, 
                         * indicating that all the transmissions are successful, 
                         * and the sending buffer is gone [All the data is sent] */
                        pMemory->FreeMemory(pConn->pSendMsgMemPointer);
                    }
                    /* Not all the transmission is completed (EAGAIN), 
                     * the data is only sent out partly, 
                     * but it must be because the transmission buffer is full */
                    else
                    {                        
                        /* How much is sent and how much is left, record it, 
                         * so that it can be used pNext time SendProc() */
                        pConn->pSendMsgBuff += uiSendLen;
				        pConn->iSendMsgLen -= uiSendLen;	

                        /* Because the send buffer is full, so now I have to rely on the system 
                         * notification to send data to mark the send buffer is full, 
                         * I need to drive the continued sending of messages through epoll iEpollEvents*/
                        ++pConn->iThrowSendCount;

                        /* After posting this event, we will rely on the epoll driver to call the EpollEventWriteRequestHandler() function to send data */
                        if(pSocketObj->EpollRegisterEvent(pConn->iSockFd,       /* socket id */
                                                            EPOLL_CTL_MOD,      /* event type */
                                                            EPOLLOUT,           /* flags, EPOLLOUT：Writable [Notify me when I can write] */
                                                            EPOLL_ADD,          /* EPOLL_ADD，EPOLL_CTL_MOD need the paremeter, 0：add，1：del，2：over */
                                                            pConn               /* pConnection info */
                                                            ) == -1)
                        {
                            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "register epoll event failed");
                            return (void *)XiaoHG_ERROR;
                        }
                    } /* end if(uiSendLen > 0) */
                    continue; /* pNext message */ 
                }  /* end if(uiSendLen > 0) */

                /* error */
                else if(uiSendLen == 0)
                {
                    /* Send 0 bytes, first of all because the content I sent is not 0 bytes, 
                     * then if the send buffer is full, the return should be -1, 
                     * and the error code should be EAGAIN, so comprehensively think that in this case, 
                     * this The sent packet was discarded [the socket processing was closed according to the peer] 
                     * Then the packet was killed and not sent. */
                    pMemory->FreeMemory(pConn->pSendMsgMemPointer);
                    continue; /* pNext message */
                }
                else if(uiSendLen == -1)
                {
                    /* The send buffer is full [one byte has not been sent out, 
                     * indicating that the send buffer is currently full] mark the send buffer is full, 
                     * you need to drive the message to continue sending through epoll */
                    ++pConn->iThrowSendCount; 
                    /* After posting this event, we will rely on the epoll driver to call the EpollEventWriteRequestHandler() function to send data */
                    if(pSocketObj->EpollRegisterEvent(pConn->iSockFd,           /* socket id */
                                                        EPOLL_CTL_MOD,      /* event type */
                                                        EPOLLOUT,           /* flags, EPOLLOUT：Writable [Notify me when I can write] */
                                                        EPOLL_ADD,          /* EPOLL_ADD，EPOLL_CTL_MOD need the paremeter, 0：add，1：del，2：over */
                                                        pConn               /* pConnection info */
                                                        ) == -1)
                    {
                        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "register epoll event failed");
                        return (void *)XiaoHG_ERROR;
                    }
                    continue; /* pNext message */ 
                }
                else /* -2 */
                {
                    /* If you can get here, it should be the return value of -2. 
                     * It is generally considered that the peer is disconnected, 
                     * waiting for recv() to disconnect the socket and recycle resources. */
                    CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Send failed sockfd: %d", pConn->iSockFd);
                    pMemory->FreeMemory(pConn->pSendMsgMemPointer);
                    continue; /* pNext message */ 
                }
            } /* end while(pos != posEnd) */

            if(pthread_mutex_unlock(&pSocketObj->m_SendMessageQueueMutex) != 0)
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_unlock(pSocketObj->m_SendMessageQueueMutex) failed");
                return (void *)XiaoHG_ERROR;
            }
        } /* if(pSocketObj->m_iSendMsgQueueCount > 0) */
    } /* end while */

    return (void*)XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: CloseListeningSocket
 * discription: clean listening socket
 * parameter:
 * =================================================================*/
void CSocket::CloseListeningSocket()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::CloseListeningSocket track");

    /* all listening sockets */
    for(int i = 0; i < m_ListenPortCount; i++) 
    {  
        close(m_ListenSocketList[i]->iSockFd);
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "close %d iPort", m_ListenSocketList[i]->iPort);
    }/* end for(int i = 0; i < m_ListenPortCount; i++) */
    return;
}