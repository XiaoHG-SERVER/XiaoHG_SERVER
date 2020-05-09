
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_THREADPOOL_H__
#define __XiaoHG_THREADPOOL_H__

#include <vector>
#include <pthread.h>
#include <atomic>

/* Thread pool related classes */
class CThreadPool
{
public:
    CThreadPool();
    ~CThreadPool();        

public:/* Thread pool creation, business processing, business triggering */
    int Create(int iThreadNum); /* Create all threads in this thread pool */
    /* After receiving a complete message, enter the message queue and trigger the thread in the thread pool to process the message */
    void PutMsgRecvQueueAndSignal(char *pMsgBuff);      
    void CallRecvMsgHandleThread();    /* Here comes the task, transfer the threads in a thread pool to work */

public:/* Reserved for measurement */
    /* Get the size of the received message queue */
    int  getRecvMsgQueueCount(){return m_iRecvMsgQueueCount;} 

private:/* Thread proc */
    static void* ThreadFunc(void *threadData);  /* Thread proc */
    
private:/* Recycle */
    void ClearMsgRecvQueue();   /* Clean up the receive message queue */

public:
    int StopAll();              /* Make all threads in the thread pool exit */

private:/* Thread structure */
    struct ThreadItem   
    {
        pthread_t _Handle;      /* Thread id */
        CThreadPool *_pThis;    /* Record thread pool pointer */	
        /* Whether the flag is officially started, 
         * and it is only allowed to call StopAll() to release after it is started */
        bool bIsRunning;        
        ThreadItem(CThreadPool *pthis):_pThis(pthis), bIsRunning(false){}
        ~ThreadItem(){}        
    };

private:
    /* Thread pool creation, management, scheduling */
    int m_iThreadNum;                           /* Thread pool numbers */
    std::atomic<int> m_iRunningThreadCount;       /* Number of threads, number of running threads, atomic operations */
    std::vector<ThreadItem *> m_ThreadPoolVector;   /* Thread container, each thread is in the container */
    static pthread_cond_t m_pThreadCond;        /* Thread synchronization condition variable */
    static pthread_mutex_t m_pThreadMutex;      /* Thread synchronization mutex / also called thread synchronization lock */

    /* Receiving message queue */
	int m_iRecvMsgQueueCount;                   /* Receive message queue size */
    std::list<char *> m_MsgRecvQueue;           /* Receive data message queue */

    /* Miscellaneous */
    static bool m_Shutdown; /* Thread exit flag, false does not exit, true exit */
    /* The last time the thread occurred did not use the 
     * [emergency event] time to prevent the log report from being too frequent */
    time_t m_LastAlartNotEnoughThreadTime;                      

};

#endif //!__XiaoHG_THREADPOOL_H__
