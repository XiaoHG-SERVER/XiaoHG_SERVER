﻿
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_THREADPOOL_H__
#define __XiaoHG_THREADPOOL_H__

#include <vector>
#include <pthread.h>
#include <atomic>

class CMemory;
class CConfig;

/* Thread pool related classes */
class CThreadPool
{
public:
    CThreadPool();
    ~CThreadPool();        

public:/* Thread pool creation, business processing, business triggering */
    void Init();        /* Create all threads in this thread pool */
    /* After receiving a complete message, enter the message queue and trigger the thread in the thread pool to process the message */
    void PushRecvMsgToRecvListAndCond(char *pMsgBuff);
    void CallRecvMsgHandleThread();    /* Here comes the task, transfer the threads in a thread pool to work */

public:/* Reserved for measurement */
    /* Get the size of the received message queue */
    int GetRecvMsgQueueCount(){return m_iRecvMsgQueueCount;} 

    uint32_t GetThreadCount();
    uint32_t GetRunningThreadCount();
    uint32_t GetLastAlartNotEnoughThreadTime();
    void SetLastAlartNotEnoughThreadTime(uint32_t time);

private:/* Thread proc */
    static void* ThreadFunc(void *threadData);  /* Thread proc */
    
private:/* Recycle */
    void ClearMsgRecvQueue();   /* Clean up the receive message queue */

public:
    int Clear();              /* Make all threads in the thread pool exit */

private:/* Thread structure */
    struct ThreadItem   
    {
        pthread_t _Handle;      /* Thread id */
        CThreadPool *_pThis;    /* Record thread pool pointer */	
        /* Whether the flag is officially started, 
         * and it is only allowed to call Clear() to release after it is started */
        bool bIsRunning;        
        ThreadItem(CThreadPool *pthis):_pThis(pthis), bIsRunning(false){}
        ~ThreadItem(){}        
    };

public:
    static pthread_cond_t m_pThreadCond;        /* Thread synchronization condition variable */
    static pthread_mutex_t m_pThreadMutex;      /* Thread synchronization mutex / also called thread synchronization lock */
    static CConfig* m_pConfig;

private:
    /* Thread pool creation, management, scheduling */
    int m_iThreadNum;                           /* Thread pool numbers */
    std::atomic<int> m_RunningThreadCount;       /* Number of threads, number of running threads, atomic operations */
    std::vector<ThreadItem *> m_ThreadPoolVector;   /* Thread container, each thread is in the container */

    /* Receiving message queue */
	int m_iRecvMsgQueueCount;                   /* Receive message queue size */
    std::list<char *> m_MsgRecvQueue;           /* Receive data message queue */

    /* The last time the thread occurred did not use the 
     * [emergency event] time to prevent the log report from being too frequent */
    time_t m_LastAlartNotEnoughThreadTime;                      

};

#endif //!__XiaoHG_THREADPOOL_H__
