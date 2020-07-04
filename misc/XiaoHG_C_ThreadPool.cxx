
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdarg.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include "XiaoHG_Global.h"
#include "XiaoHG_C_ThreadPool.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_C_Log.h"
#include "XiaoHG_error.h"
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_C_MessageCtl.h"

#define __THIS_FILE__ "XiaoHG_C_ThreadPool.cxx"

pthread_mutex_t CThreadPool::m_pThreadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CThreadPool::m_pThreadCond = PTHREAD_COND_INITIALIZER;
CConfig* CThreadPool::m_pConfig = CConfig::GetInstance();

CThreadPool::CThreadPool()
{
    m_RunningThreadCount = 0;
    m_LastAlartNotEnoughThreadTime = 0;
    m_iRecvMsgQueueCount = 0;
}

CThreadPool::~CThreadPool()
{
    ClearMsgRecvQueue();
}


/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: Create
 * discription: create thread pool
 * parameter:
 * =================================================================*/
void CThreadPool::Init()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CThreadPool::Init track");

    ThreadItem *pNewThreadItem = NULL;
    /* get the recv msg thread numbers from config file */
    /* create thread numbers */
    for(int i = 0; i < m_pConfig->GetIntDefault("ProcMsgRecvWorkThreadCount", 5); i++)
    {
        pNewThreadItem = new ThreadItem(this);
        m_ThreadPoolVector.push_back(pNewThreadItem);
        if(pthread_create(&pNewThreadItem->_Handle, NULL, ThreadFunc, pNewThreadItem) != 0)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Create thread failed");
            exit(0);
        }
    } /* end for */

    /* We must ensure that each thread is started and run to pthread_cond_wait () 
     * before this function returns. Only in this way can these few threads perform 
     * subsequent normal work */
    for(std::vector<ThreadItem*>::iterator iter = m_ThreadPoolVector.begin(); iter != m_ThreadPoolVector.end(); iter++)
    {
        if( (*iter)->bIsRunning == false)
        {
            /* wait 10ms */
            usleep(100 * 1000);
        }
    }
    CLog::Log("Create thread success");
}


/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ClearMsgRecvQueue
 * discription: cClean up the receive message queue
 * parameter:
 * =================================================================*/
void CThreadPool::ClearMsgRecvQueue()
{
	char *pTmpMempoint = NULL;
	while(!m_MsgRecvQueue.empty())
	{
		pTmpMempoint = m_MsgRecvQueue.front();		
		m_MsgRecvQueue.pop_front(); 
		CMemory::GetInstance()->FreeMemory(pTmpMempoint);
	}
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: Create
 * discription: thread pool enter process
 * parameter:
 * =================================================================*/
void* CThreadPool::ThreadFunc(void *pThreadData)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CThreadPool::ThreadFunc track");

    /* This is a static member function, there is no this pointer */
    ThreadItem *pThread = static_cast<ThreadItem *>(pThreadData);
    CThreadPool *pThreadPoolObj = pThread->_pThis;
    /* thread working */
    while(true)
    {
        /* m_pThreadMutex lock */
        if(pthread_mutex_lock(&m_pThreadMutex) != 0)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_lock(&m_pThreadMutex) failed");
            exit(0);
        }
        /* The following line of program writing skills are very important. 
         * You must use while while writing because: pthread_cond_wait() is a notable function.
         * Calling pthread_cond_signal() once may wake up multiple [stunning group] threads 
         * [at least one pthread_cond_signal on multiple processors Multiple threads may be awakened 
         * at the same time] pthread_cond_wait() function, if there is only one message to wake up 
         * two threads to work, then one of the threads cannot get the message, then if you do not 
         * write while, there will be problems, so after being awaken You must use the while again 
         * to get the message and get it before you come down */
        while ((g_MessageCtl.GetRecvMsgList().size() == 0))
        {
            /* If this pthread_cond_wait is awakened [the premise of the program 
             * execution flow going down after being awakened is to obtain a lock, 
             * and when pthread_cond_wait() returns, the mutex is locked again], 
             * then immediately execute pThreadPoolObj-> m_MsgRecvQueue.size() , 
             * If you get an empty, continue to wait () here */
            if(pThread->bIsRunning == false)
            {
                /* It is marked as true before Clear () is allowed to be called: 
                 * The test found that if Create () and Clear () are called pNext 
                 * to each other, it will cause thread confusion, so each thread 
                 * must execute here before it is considered to be successful */
                pThread->bIsRunning = true; 
            }

            /* When pthread_cond_wait () is executed, it will be stuck here, and m_pThreadMutex will be released, 
             * and the awakened thread will get this lock. When the entire server program is initialized, 
             * all threads must be stuck waiting here */
            pthread_cond_wait(&m_pThreadCond, &m_pThreadMutex);
        }

        /* At this point, you can get the message for processing 
         * [there must be a message in the message queue], 
         * note that it is still mutually exclusive, so you can operate the receive data queue */
        char *pJobBuf = g_MessageCtl.GetRecvMsgList().front();
        LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)(pJobBuf + g_LenMsgHeader);  /* packet header */
        g_MessageCtl.GetRecvMsgList().pop_front();
        --CMessageCtl::m_RecvMsgListSize;

        /* m_pThreadMutex unlock */
        if(pthread_mutex_unlock(&m_pThreadMutex) != 0)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_unlock(&m_pThreadMutex) failed");
            exit(0);
        }

        /* handle massage */
        ++pThreadPoolObj->m_RunningThreadCount;
        g_MessageCtl.RecvMsgHandleProc(pJobBuf);
        CMemory::GetInstance()->FreeMemory(pJobBuf);   /* handle over, free memory */ 
        --pThreadPoolObj->m_RunningThreadCount;

    } /* end while(true) */

    return (void*)XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: PushRecvMsgToRecvListAndCond
 * discription: After receiving a complete message, enter the message 
 *              queue and trigger the thread in the thread pool to process the message.
 * parameter:
 * =================================================================*/
void CThreadPool::PushRecvMsgToRecvListAndCond(char *pRecvMsgBuff)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CThreadPool::PushRecvMsgToRecvListAndCond track");

    /* m_pThreadMutex lock */
    if(pthread_mutex_lock(&m_pThreadMutex) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_lock(&m_pThreadMutex) failed");
        return;
    }
    /* Join the message queue */
    m_MsgRecvQueue.push_back(pRecvMsgBuff);
    ++m_iRecvMsgQueueCount;
    /* m_pThreadMutex unlock */
    if(pthread_mutex_unlock(&m_pThreadMutex) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_unlock(&m_pThreadMutex) failed");
        return;
    }
    /* Can excite the thread to work */
    CallRecvMsgHandleThread();
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: Call
 * discription: Here comes the task, transfer the threads in a thread pool to work.
 * parameter:
 * =================================================================*/
void CThreadPool::CallRecvMsgHandleThread()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CThreadPool::CallRecvMsgHandleThread track");

    /* Wake up a thread waiting for the condition, that is, 
     * a thread that can be stuck in pthread_cond_wait() */
    if(pthread_cond_signal(&m_pThreadCond) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_cond_signal(&m_pThreadCond) failed");
        return;
    }
    
    /* If the current worker threads are all busy, the total number of threads in the thread pool 
     * must be reported, which is the same as the current number of working threads,
     * indicating that all threads are busy and the threads are not enough */
    if(m_iThreadNum == m_RunningThreadCount) 
    {
        /* thread numbers is not enough */
        time_t CurrTime = time(NULL);
        /* Report once every 10s */
        if(CurrTime - m_LastAlartNotEnoughThreadTime > 10)
        {
            m_LastAlartNotEnoughThreadTime = CurrTime;  /* flash current time */
            CLog::Log(LOG_LEVEL_ALERT, __THIS_FILE__, __LINE__, "Not enough threads");
        }
    } /* end if */

    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: Clear
 * discription: stop all threads
 * parameter:
 * =================================================================*/
int CThreadPool::Clear()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CThreadPool::Clear track");

    /* weakup all threads in pthread_cond_wait function */
    if(pthread_cond_broadcast(&m_pThreadCond) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__ , "pthread_cond_broadcast(&m_pThreadCond) failed");
        return XiaoHG_ERROR;
    }

    /* wait thread return */
    std::vector<ThreadItem*>::iterator iter;
	for(iter = m_ThreadPoolVector.begin(); iter != m_ThreadPoolVector.end(); iter++)
    {
        /* wait a thread return */
        pthread_join((*iter)->_Handle, NULL); 
    }

    pthread_mutex_destroy(&m_pThreadMutex);
    pthread_cond_destroy(&m_pThreadCond);    

    /* realese all thread item */
	for(iter = m_ThreadPoolVector.begin(); iter != m_ThreadPoolVector.end(); iter++)
	{
		if(*iter)
        {
			delete *iter;
        }
	}
	m_ThreadPoolVector.clear();

    /* this is record some message to help us know more current status */
    CLog::Log(LOG_LEVEL_NOTICE, "All thread pool return successful");
    return XiaoHG_SUCCESS;
}

uint32_t CThreadPool::GetThreadCount()
{
    return m_iThreadNum;
}

uint32_t CThreadPool::GetRunningThreadCount()
{
    return m_RunningThreadCount;
}

uint32_t CThreadPool::GetLastAlartNotEnoughThreadTime()
{
    return m_LastAlartNotEnoughThreadTime;
}

void CThreadPool::SetLastAlartNotEnoughThreadTime(uint32_t time)
{
    m_LastAlartNotEnoughThreadTime = time;
}