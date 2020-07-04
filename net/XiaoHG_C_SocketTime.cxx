
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

#define __THIS_FILE__ "XiaoHG_C_SocketTime.cxx"

pthread_mutex_t CSocket::m_ConnectionMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t CSocket::m_TimeQueueMutex = PTHREAD_MUTEX_INITIALIZER;

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: AddToTimerList
 * discription: Set the kick-out clock (add content to the multimap table), 
 * 				the user successfully connects with the three handshake, 
 * 				and then we turn on the kicker switch [Sock_WaitTimeEnable = 1], 
 * 				then this function is called.
 * parameter:
 * =================================================================*/
void CSocket::AddToTimerList(LPCONNECTION_T pConn)
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::AddToTimerList track");

    time_t CurrTime = time(NULL);		/* Get Current time */
    CurrTime += m_WaitTime;			/* 20s after */

    CLock lock(&m_TimeQueueMutex); /* lock */
    LPMSG_HEADER_T pMsgHeader = (LPMSG_HEADER_T)m_pMemory->AllocMemory(g_LenMsgHeader, false);
    pMsgHeader->pConn = pConn;
    pMsgHeader->currSequence = pConn->currSequence;
    m_TimerQueueMultiMap.insert(std::make_pair(CurrTime, pMsgHeader)); /* Buttons are automatically sorted small-> large */
    ++m_CurrentHeartBeatListSize; /* Queue size +1 */
    m_HBListEarliestTime = GetTimerListEarliestTime(); /* m_HBListEarliestTime saves the time value of the head of the timing queue */
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: GetTimerListEarliestTime
 * discription: Get the earliest time from multimap and go back
 * parameter:
 * =================================================================*/
time_t CSocket::GetTimerListEarliestTime()
{
    std::multimap<time_t, LPMSG_HEADER_T>::iterator pos;
	pos = m_TimerQueueMultiMap.begin();		
	return pos->first;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: GetOverTimeTimer
 * discription: Remove the earliest time from m_timeQueuemap, 
 * 				and return the pointer corresponding to the 
 * 				value of the earliest item at this time.
 * parameter:
 * =================================================================*/
LPMSG_HEADER_T CSocket::RemoveTimerQueueFirstTime()
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::RemoveTimerQueueFirstTime track");

	LPMSG_HEADER_T pMsgHeader = NULL;
	std::multimap<time_t, LPMSG_HEADER_T>::iterator pos;
	
	/* m_CurrentHeartBeatListSize is not null*/
	if(m_CurrentHeartBeatListSize <= 0)
	{
		return NULL;
	}
	/* The caller is responsible for mutual exclusion */
	pos = m_TimerQueueMultiMap.begin();
	pMsgHeader = pos->second;
	m_TimerQueueMultiMap.erase(pos);
	--m_CurrentHeartBeatListSize;
	return pMsgHeader;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: GetOverTimeTimer
 * discription: According to the current time given, find a node [1] 
 * 				older than this time from m_timeQueuemap. These nodes are all over time. 
 * 				The caller of the node to be processed is responsible for mutual exclusion. 
 * 				Rebuke.
 * parameter:
 * =================================================================*/
LPMSG_HEADER_T CSocket::GetOverTimeTimer(time_t CurrentTime)
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::GetOverTimeTimer track");

	LPMSG_HEADER_T pOverTimeMsg = NULL;

	if (m_CurrentHeartBeatListSize == 0 || m_TimerQueueMultiMap.empty())
	{
		return NULL;
	}
	/* Go to multimap to query */
	time_t EerliestTime = GetTimerListEarliestTime(); 
	if (EerliestTime <= CurrentTime)
	{
		/* Get timeout node */
		pOverTimeMsg = RemoveTimerQueueFirstTime();

		/* Configuration file flag bit, this flag bit indicates 
		 * whether it is necessary to directly kick people overtime, 
		 * rather than waiting for heartbeat */
		if(m_IsTimeOutKick != 1)
		{
			/* Because we still have to judge the time of the pNext timeout, 
			 * so we need to add this node back */        
			time_t NewTime = CurrentTime + m_WaitTime;
			LPMSG_HEADER_T pMsgHeader = (LPMSG_HEADER_T)CMemory::GetInstance()->AllocMemory(sizeof(MSG_HEADER_T), false);
			pMsgHeader->pConn = pOverTimeMsg->pConn;
			pMsgHeader->currSequence = pOverTimeMsg->currSequence;			
			/* Automatic sorting Small-> Large, add new time */	
			m_TimerQueueMultiMap.insert(std::make_pair(NewTime, pMsgHeader)); 		
			++m_CurrentHeartBeatListSize;
		}

		if(m_CurrentHeartBeatListSize > 0) 
		{
			m_HBListEarliestTime = GetTimerListEarliestTime();
		}
		return pOverTimeMsg;
	}
	return NULL;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: DeleteFromTimerQueue
 * discription: Remove the specified user tcp pstConn from the timer table
 * parameter:
 * =================================================================*/
void CSocket::DeleteFromTimerQueue(LPCONNECTION_T pConn)
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::DeleteFromTimerQueue track");

    CLock lock(&m_TimeQueueMutex);
	std::multimap<time_t, LPMSG_HEADER_T>::iterator pos;
	std::multimap<time_t, LPMSG_HEADER_T>::iterator posEnd;	
lblMTQM:
	pos = m_TimerQueueMultiMap.begin();
	posEnd = m_TimerQueueMultiMap.end();
	for(; pos != posEnd; pos++)	
	{
		if(pos->second->pConn == pConn)
		{
			CMemory::GetInstance()->FreeMemory(pos->second);
			m_TimerQueueMultiMap.erase(pos);
			--m_CurrentHeartBeatListSize;							
			goto lblMTQM;
		}
	}

	if(m_CurrentHeartBeatListSize > 0)
	{
		m_HBListEarliestTime = GetTimerListEarliestTime();
	}
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ClearAllFromTimerQueue
 * discription: clear about Time list
 * parameter:
 * =================================================================*/
void CSocket::ClearAllFromTimerQueue()
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::ClearAllFromTimerQueue track");

	std::multimap<time_t, LPMSG_HEADER_T>::iterator pos = m_TimerQueueMultiMap.begin();
	std::multimap<time_t, LPMSG_HEADER_T>::iterator posEnd = m_TimerQueueMultiMap.end();
	for(; pos != posEnd; pos++)	
	{
		CMemory::GetInstance()->FreeMemory(pos->second);		
		pos->second = NULL;
		--m_CurrentHeartBeatListSize;
	}
	m_TimerQueueMultiMap.clear();
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: HeartBeatMonitorThread
 * discription: Time queue monitoring and processing threads, 
 * 				processing threads kicked by users who do not 
 * 				send heartbeat packets when they expire.
 * parameter:
 * =================================================================*/
void* CSocket::HeartBeatMonitorThread(void* pThreadData)
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::HeartBeatMonitorThread track");

	time_t CurrentTime = 0;
    ThreadItem *pThread = static_cast<ThreadItem*>(pThreadData);
    CSocket *pSocketObj = pThread->_pThis;

    while(true)
    {
		/* Check every 500ms */
        usleep(CHECK_HBLIST_TIMER * 1000);
		/* m_CurrentHeartBeatListSize is not null */
		if(pSocketObj->m_CurrentHeartBeatListSize > 0)	
        {
			/* Put the time of the most recent event in the time queue into AbsoluteTime */
            /* pSocketObj->m_HBListEarliestTime is Earliest time in the time list */
            CurrentTime = time(NULL);
            if(pSocketObj->m_HBListEarliestTime < CurrentTime)
            {
				/* m_TimeQueueMutex lock */
				if(pthread_mutex_lock(&pSocketObj->m_TimeQueueMutex) != 0)
				{
					CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_lock(&pSocketObj->m_TimeQueueMutex) failed");
					return (void *)XiaoHG_ERROR;
				}
                
				/* Take all the timeout nodes and checkout */
				LPMSG_HEADER_T pOverTimeMsg = NULL;
				while ((pOverTimeMsg = pSocketObj->GetOverTimeTimer(CurrentTime)) != NULL) 
				{
					/* Each time out message to checkout */
					pSocketObj->HeartBeatTimeOutCheck(pOverTimeMsg, CurrentTime);
				}/* end while */

                /* m_TimeQueueMutex unlock */ 
				if(pthread_mutex_unlock(&pSocketObj->m_TimeQueueMutex) != 0)
				{
					CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_unlock(&pSocketObj->m_TimeQueueMutex) failed");
					return (void *)XiaoHG_ERROR;
				}
            }
        } /* end if(pSocketObj->m_CurrentHeartBeatListSize > 0) */
    } /* end while */
    return (void*)XiaoHG_SUCCESS;
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
void CSocket::HeartBeatTimeOutCheck(LPMSG_HEADER_T pstMsgHeader, time_t CurrentTime)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::HeartBeatTimeOutCheck track");

    /* check the connect is OK */
    if(pstMsgHeader->currSequence == pstMsgHeader->pConn->currSequence)
    {
        LPCONNECTION_T pConn = pstMsgHeader->pConn;
        /* Determine if the direct kick function is enabled if the timeout is enabled. 
         * If it is, the user will be kicked directly after the first heartbeat timeout, 
         * otherwise the heartbeat detection will be entered again, 
         * and the user will be kicked out when the maximum allowable time for heartbeat detection. */
        if(m_IsTimeOutKick == 1)
        {
            /* The need to kick out directly at the time */
            CloseConnectionToRecy(pConn);
        }
        /* The judgment criterion for timeout kicking is the interval of each check * 3. 
         * If no heartbeat packet is sent after this time, 
         * the lastHeartBeatTime will be refreshed every time the client heartbeat packet is received. */
        else if((CurrentTime - pConn->lastHeartBeatTime) > (m_WaitTime * 3 + 10))
        {
            CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Heart beat check time out kick socket: %d", pConn->sockFd);
            CloseConnectionToRecy(pConn);
        }
        CMemory::GetInstance()->FreeMemory(pstMsgHeader);
    }
    /* This connect is broken */
    else
    {
        CMemory::GetInstance()->FreeMemory(pstMsgHeader);
    }
    return;
}

int CSocket::GetFloodAttackEnable()
{
	return m_FloodAttackEnable;
}


