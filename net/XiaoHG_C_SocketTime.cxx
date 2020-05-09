
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
#include "XiaoHG_C_Socket.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_LockMutex.h"

#define __THIS_FILE__ "XiaoHG_C_SocketTime.cxx"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: AddToTimerQueue
 * discription: Set the kick-out clock (add content to the multimap table), 
 * 				the user successfully connects with the three handshake, 
 * 				and then we turn on the kicker switch [Sock_WaitTimeEnable = 1], 
 * 				then this function is called.
 * parameter:
 * =================================================================*/
void CSocket::AddToTimerQueue(LPCONNECTION_T pConn)
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::AddToTimerQueue track");

    CMemory *pMemory = CMemory::GetInstance();
    time_t CurrTime = time(NULL);		/* Get Current time */
    CurrTime += m_iWaitTime;			/* 20s after */

    CLock lock(&m_TimeQueueMutex); /* lock */
    LPMSG_HEADER_T pMsgHeader = (LPMSG_HEADER_T)pMemory->AllocMemory(m_iLenMsgHeader, false);
    pMsgHeader->pConn = pConn;
    pMsgHeader->uiCurrentSequence = pConn->uiCurrentSequence;
    m_TimerQueueMultiMap.insert(std::make_pair(CurrTime, pMsgHeader)); /* Buttons are automatically sorted small-> large */
    ++m_CurrentHeartBeatListSize; /* Queue size +1 */
    m_HBListEarliestTime = GetTimerQueueEarliestTime(); /* m_HBListEarliestTime saves the time value of the head of the timing queue */
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: GetTimerQueueEarliestTime
 * discription: Get the earliest time from multimap and go back
 * parameter:
 * =================================================================*/
time_t CSocket::GetTimerQueueEarliestTime()
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::GetTimerQueueEarliestTime track");

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

	CMemory *pMemory = CMemory::GetInstance();
	LPMSG_HEADER_T pOverTimeMsg = NULL;

	if (m_CurrentHeartBeatListSize == 0 || m_TimerQueueMultiMap.empty())
	{
		return NULL;
	}
	/* Go to multimap to query */
	time_t EerliestTime = GetTimerQueueEarliestTime(); 
	if (EerliestTime <= CurrentTime)
	{
		/* Get timeout node */
		pOverTimeMsg = RemoveTimerQueueFirstTime();

		/* Configuration file flag bit, this flag bit indicates 
		 * whether it is necessary to directly kick people overtime, 
		 * rather than waiting for heartbeat */
		if(m_iIsTimeOutKick != 1)
		{
			/* Because we still have to judge the time of the pNext timeout, 
			 * so we need to add this node back */        
			time_t NewTime = CurrentTime + m_iWaitTime;
			LPMSG_HEADER_T pMsgHeader = (LPMSG_HEADER_T)pMemory->AllocMemory(sizeof(MSG_HEADER_T), false);
			pMsgHeader->pConn = pOverTimeMsg->pConn;
			pMsgHeader->uiCurrentSequence = pOverTimeMsg->uiCurrentSequence;			
			/* Automatic sorting Small-> Large, add new time */	
			m_TimerQueueMultiMap.insert(std::make_pair(NewTime, pMsgHeader)); 		
			++m_CurrentHeartBeatListSize;
		}

		if(m_CurrentHeartBeatListSize > 0) 
		{
			m_HBListEarliestTime = GetTimerQueueEarliestTime();
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
 * discription: Remove the specified user tcp pConnection from the timer table
 * parameter:
 * =================================================================*/
void CSocket::DeleteFromTimerQueue(LPCONNECTION_T pConn)
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::DeleteFromTimerQueue track");

	CMemory *pMemory = CMemory::GetInstance();
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
			pMemory->FreeMemory(pos->second);
			m_TimerQueueMultiMap.erase(pos);
			--m_CurrentHeartBeatListSize;							
			goto lblMTQM;
		}
	}

	if(m_CurrentHeartBeatListSize > 0)
	{
		m_HBListEarliestTime = GetTimerQueueEarliestTime();
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

	CMemory *pMemory = CMemory::GetInstance();	
	std::multimap<time_t, LPMSG_HEADER_T>::iterator pos = m_TimerQueueMultiMap.begin();
	std::multimap<time_t, LPMSG_HEADER_T>::iterator posEnd = m_TimerQueueMultiMap.end();
	for(; pos != posEnd; pos++)	
	{
		pMemory->FreeMemory(pos->second);		
		pos->second = NULL;
		--m_CurrentHeartBeatListSize;
	}
	m_TimerQueueMultiMap.clear();
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: HeartBeatMonitorThreadProc
 * discription: Time queue monitoring and processing threads, 
 * 				processing threads kicked by users who do not 
 * 				send heartbeat packets when they expire.
 * parameter:
 * =================================================================*/
void* CSocket::HeartBeatMonitorThreadProc(void* pThreadData)
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::HeartBeatMonitorThreadProc track");

	time_t CurrentTime = 0;
    ThreadItem *pThread = static_cast<ThreadItem*>(pThreadData);
    CSocket *pSocketObj = pThread->_pThis;

    while(!g_bIsStopEvent)
    {
		/* Check every 500ms */
        usleep(CHECK_HBLIST_TIME * 1000);
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
 * discription: When the heartbeat packet detection time is up, 
 * 				it is time to detect whether the heartbeat packet has timed out. 
 * 				This function just releases the memory. Subclasses should restart 
 * 				this function in advance to achieve specific judgment actions。
 * parameter:
 * =================================================================*/
void CSocket::HeartBeatTimeOutCheck(LPMSG_HEADER_T pstMsgHeader, time_t CurrentTime)
{
	/* function track */
	CLog::Log(LOG_LEVEL_TRACK, "CSocket::HeartBeatTimeOutCheck track");

	CMemory *pMemory = CMemory::GetInstance();
	pMemory->FreeMemory(pstMsgHeader);
}


