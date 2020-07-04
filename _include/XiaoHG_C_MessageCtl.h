
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <list>
#include <string>
#include <pthread.h>
#include <semaphore.h>
#include "XiaoHG_C_Socket.h"

#ifndef __XiaoHG_C_MESSAGECTL_H__
#define __XiaoHG_C_MESSAGECTL_H__

class CThreadPool;
class CMemory;
class CCRC32;

class CMessageCtl
{
public:
    CMessageCtl(/* args */);
    ~CMessageCtl();

public:
    void Init(); 
    void RecvMsgHandleProc(char *pRecvMsgBuff);
    void PushSendMsgToSendListAndSem(char *pSendMsgBuff);
    void PushRecvMsgToRecvListAndCond(char *pRecvMsgBuff);

    void RecvMsgHeaderParseProc(LPCONNECTION_T pConn, bool &bIsFlood);
    void RecvMsgPacketParseProc(LPCONNECTION_T pConn, bool &bIsFlood);

    std::list<char*>& GetRecvMsgList();

public:
    void ClearMsgSendList();

public://About message ctl thread
    static void* SendMsgThread(void *pThreadData);
    static uint32_t m_RecvMsgListSize;
    static uint32_t m_SendMsgListSize;

public:
    static pthread_mutex_t m_RecvMsgListMutex;
    static pthread_mutex_t m_SendMsgListMutex;

    static CMemory* m_pMemory;
    static CCRC32* m_pCrc32;
    
private:

    struct ThreadItem
    {
        pthread_t _Handle;		/* thread handle */
        CMessageCtl *_pThis;		/* thread pointer */	

        /* Constructor */
        ThreadItem(CMessageCtl *pthis):_pThis(pthis){}                             
        /* Destructor */
        ~ThreadItem(){}
    };
    

private:
    uint32_t m_DiscardSendPkgCount; /* Number of send packets dropped */

private:
    std::list<char*> m_RecvMsgList;
    std::list<char*> m_SendMsgList;
};

#endif//!__XiaoHG_C_MESSAGECTL_H__