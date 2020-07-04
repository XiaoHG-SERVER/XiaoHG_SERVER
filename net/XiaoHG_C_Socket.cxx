
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

#define __THIS_FILE__ "XiaoHG_C_Socket.cxx"

#define THREAD_ITEM_COUNT 2

/* Thread process */
typedef void* (*ThreadProc)(void *pThreadData);

CMemory* CSocket::m_pMemory = CMemory::GetInstance();
CConfig* CSocket::m_pConfig = CConfig::GetInstance();
CSocket::CSocket()
{
    CConfig::GetInstance();
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
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: Initalize
 * discription: Init socket
 * parameter:
 * =================================================================*/
void CSocket::Init()
{
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::Init track");

    m_EpollHandle = -1;                         /* Epoll handle */
    m_ListenPortCount = m_pConfig->GetIntDefault("ListenPortCount");
    m_DelayRecoveryConnectionWaitTime = m_pConfig->GetIntDefault("Sock_RecyConnectionWaitTime");
    m_IsHBTimeOutCheckEnable = m_pConfig->GetIntDefault("Sock_WaitTimeEnable");
	m_WaitTime = m_pConfig->GetIntDefault("Sock_MaxWaitTime");
	m_WaitTime = m_WaitTime > 5 ? m_WaitTime : 5;
    m_IsTimeOutKick = m_pConfig->GetIntDefault("Sock_TimeOutKick");
    m_FloodAttackEnable = m_pConfig->GetIntDefault("Sock_FloodAttackKickEnable");
	m_FloodTimeInterval = m_pConfig->GetIntDefault("Sock_FloodTimeInterval", 100);
	m_FloodKickCount = m_pConfig->GetIntDefault("Sock_FloodKickCounter", 10);
    m_EpollCreateConnectCount = m_pConfig->GetIntDefault("EpollCreateConnectCount");

    if (pthread_mutex_init(&m_RecyConnQueueMutex, NULL) == -1)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_init(m_RecyConnQueueMutex) failed");
        exit(0);
    }

    /* Create threads */
    ThreadProc pfTmpThreadPro[] = {DelayRecoveryConnectionThread, HeartBeatMonitorThread};
    for (int i = 0; i < sizeof(pfTmpThreadPro) / sizeof(pfTmpThreadPro[0]); i++)
    {
        ThreadItem* pThreadItems = new ThreadItem(this);
        m_ThreadPoolVector.push_back(pThreadItems);

        if(pthread_create(&pThreadItems->_Handle, NULL, pfTmpThreadPro[i], pThreadItems) != 0)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_create(%d) failed", i);
            exit(0);
        }
        CLog::Log("pthread_create(%d) successful", i);
    }

    /* Init connection pool */
    LPCONNECTION_T pConn = NULL;
    /* Create */
    for(int i = 0; i < m_EpollCreateConnectCount; i++) 
    {
        /* Apply for memory, because the new char is allocated here, the constructor cannot be executed
         * so we need to call the constructor manually */
        pConn = (LPCONNECTION_T)(m_pMemory->AllocMemory(sizeof(CONNECTION_T), true));
        pConn = new(pConn) CONNECTION_T();
        pConn->GetOneToUse();
        m_ConnectionList.push_back(pConn);
        m_FreeConnectionList.push_back(pConn);
    } /* end for */
    /* In the begining is some length */
    m_FreeConnectionCount = m_TotalConnectionCount = m_ConnectionList.size(); 
    /* open the listen port */
    OpenListeningSockets();
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: Clear
 * discription: shut down sub process
 * parameter:
 * =================================================================*/
void CSocket::Clear()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::Clear track");

    /* make ServerSendQueueThread go on */
    if(sem_post(&g_SendMsgPushEventSem) == -1)  
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
        {
			delete *iter;
        }
	}
	m_ThreadPoolVector.clear();

    ClearConnections();
    ClearAllFromTimerQueue();
    
    /* destory */
    pthread_mutex_destroy(&m_ConnectionMutex);          /* Connection related mutex release */
    pthread_mutex_destroy(&m_TimeQueueMutex);           /* Time processing queue related mutex release */
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: OpenListeningSockets
 * discription: open stListenSocket
 * parameter:
 * =================================================================*/
uint32_t CSocket::OpenListeningSockets()
{
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::OpenListeningSockets track");

    uint16_t port = 0;
    uint16_t sockFd = 0;
    char strListeningPortInfo[100] = { 0 };
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    for(int i = 0; i < m_ListenPortCount; i++)
    {
        /* create socket */
        sockFd = socket(AF_INET, SOCK_STREAM, 0);
        if(sockFd == -1)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "create stListenSocket socket failed: %d", i);
            exit(0);
        }

        /* set socket noblock */
        if(SetNonBlocking(sockFd) == XiaoHG_ERROR)
        {                
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "set socket nonblock failed: %d", i);
            exit(0);
        }

        /* set stListenSocket port */
        sprintf(strListeningPortInfo, "ListenPort%d", i);
        port = m_pConfig->GetIntDefault(strListeningPortInfo, 30723);
        serv_addr.sin_port = htons((in_port_t)port); /* in_port_t->uint16_t */

        /* bind */
        if(bind(sockFd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "bind(%s) failed", strListeningPortInfo);
            exit(0);
        }
        
        /* stListenSocket */
        if(listen(sockFd, XiaoHG_LISTEN_SOCKETS) == -1)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "stListenSocket socket failed: %d, socket id: %d", i, sockFd);
            exit(0);
        }

        LPLISTENING_T pListenSocketItem = new LISTENING_T();
        memset(pListenSocketItem, 0, sizeof(LISTENING_T));
        pListenSocketItem->port = port;
        pListenSocketItem->sockFd = sockFd;
        /* push the stListenSocket list */
        m_ListenSocketList.push_back(pListenSocketItem); 
        /* stListenSocket successful write the log file */
    } /* end for(int i = 0; i < m_ListenPortCount; i++)  */
    
    /* no stListenSocket port */
    if(m_ListenSocketList.size() <= 0)
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "No stListenSocket port");
        exit(0);
    }

    if(InitEpoll() != XiaoHG_SUCCESS)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "InitEpoll failed");
        exit(0);
    }

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
int CSocket::SetNonBlocking(int sockFd) 
{
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::SetNonBlocking track");
    int nb = 1; /* 0：clean, 1：set */
    if(ioctl(sockFd, FIONBIO, &nb) == -1) 
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Set nonblock socket id: %d failed", sockFd);
        exit(0);
    }
    return XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: CloseConnectionInRecy
 * discription: When actively closing a pstConn, do some aftercare function
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
    if(pConn->sockFd != -1)
    {
        /* This socket is closed, and epoll will be deleted 
         * from the red and black tree after closing, 
         * so no epoll epollEvents can be received after this */
        close(pConn->sockFd); 
        pConn->sockFd = -1;
    }
    if(pConn->throwSendMsgCount > 0)
    {
        pConn->throwSendMsgCount = 0;   /* 0 */
    }
    /* Add the pstConn to the delayed recycling 
     * queue and let the delayed recycling mechanism manage */
    PushConnectToRecyQueue(pConn);
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
	uint64_t currTime;         /* Current time (ms) */
    /* Get current time */
	gettimeofday(&sCurrTime, NULL); 
    currTime = (sCurrTime.tv_sec * 1000 + sCurrTime.tv_usec / 1000);
    /* When the package was received twice <100 ms */
	if((currTime - pConn->floodAttackLastTime) < m_FloodTimeInterval)    
	{
        /* Recording too frequently */
		++pConn->floodAttackCount;
		pConn->floodAttackLastTime = currTime;
	}
	else
	{
         /* Back to normal */
		pConn->floodAttackCount = 0;
		pConn->floodAttackLastTime = currTime;
	}

    /* Whether it is a flood attack */
	if(pConn->floodAttackCount >= m_FloodKickCount)
	{
		/* disconnection */
        CLog::Log(LOG_LEVEL_ALERT, "Check out socket: %d is flood attack, disconnection", pConn->sockFd);
		return true;
	}
	return false;
}