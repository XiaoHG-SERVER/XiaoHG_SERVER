
#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <errno.h>
#include <list>
#include "XiaoHG_C_MessageCtl.h"
#include "XiaoHG_C_Log.h"
#include "XiaoHG_error.h"
#include "XiaoHG_C_ThreadPool.h"
#include "XiaoHG_C_LockMutex.h"
#include "XiaoHG_C_BusinessCtl.h"
#include "XiaoHG_C_Memory.h"
#include "XiaoHG_C_Crc32.h"
#include "XiaoHG_C_Socket.h"

size_t g_LenPkgHeader = sizeof(COMM_PKG_HEADER);  /* packet header size */
size_t g_LenMsgHeader = sizeof(MSG_HEADER_T);     /* Message header size */

#define __THIS_FILE__ "XiaoHG_C_MessageCtl.cxx"

/* Logic handler process call back*/
typedef bool (CBusinessCtl::*LogicHandlerCallBack)(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, uint16_t iBodyLength);

/* Such an array used to store member function pointers */
static const LogicHandlerCallBack StatusHandler[] = 
{
    /* The first 5 elements of the array are reserved for future addition of some basic server functions */
    &CBusinessCtl::HandleHeartbeat,                         /* [0]: realization of heartbeat package */
    NULL,                                                   /* [1]: The subscript starts from 0 */
    NULL,                                                   /* [2]: The subscript starts from 0 */
    NULL,                                                   /* [3]: The subscript starts from 0 */
    NULL,                                                   /* [4]: The subscript starts from 0 */
 
    /* Start processing specific business logic */
    &CBusinessCtl::HandleRegister,                         /* [5]: Realize the specific registration function */
    &CBusinessCtl::HandleLogin,                            /* [6]: Realize the specific login function */
    /* ... */
};

/* How many commands are there, you can know when compiling */
#define TOTAL_COMMANDS sizeof(StatusHandler)/sizeof(LogicHandlerCallBack)

uint32_t CMessageCtl::m_RecvMsgListSize = 0;
uint32_t CMessageCtl::m_SendMsgListSize = 0;
pthread_mutex_t CMessageCtl::m_RecvMsgListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t CMessageCtl::m_SendMsgListMutex = PTHREAD_MUTEX_INITIALIZER;
CMemory* CMessageCtl::m_pMemory = CMemory::GetInstance();
CCRC32* CMessageCtl::m_pCrc32 = CCRC32::GetInstance();
CMessageCtl::CMessageCtl(/* args */)
{
    //Init();
}

CMessageCtl::~CMessageCtl()
{
    ClearMsgSendList();
}

void CMessageCtl::Init()
{
    m_DiscardSendPkgCount = 0;
    /* Create send message threads */
    ThreadItem* pThreadItem = new ThreadItem(this);
    if(pthread_create(&pThreadItem->_Handle, nullptr, SendMsgThread, pThreadItem) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_create(SendMsgThread) failed");
        exit(0);
    }
}

void CMessageCtl::RecvMsgHandleProc(char *pRecvMsgBuff)
{
    uint16_t MsgCode;
    void *pPkgBody = NULL;  /* packet point */
    LPMSG_HEADER_T pMsgHeader = (LPMSG_HEADER_T)pRecvMsgBuff;    /* massage header */
    LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)(pRecvMsgBuff + g_LenMsgHeader);  /* packet header */
    uint16_t PkgLen = ntohs(pPkgHeader->PkgLength); /* packet length */
    /* if only packet header*/
    if(g_LenPkgHeader == PkgLen)
    {
        /* only packet header */
        /* iCrc32 value if equeal 0, heart beat iCrc32 = 0, if not 0 is error packet header*/
		if(pPkgHeader->iCrc32 != 0)
		{
            CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Only packet header, and not a heartbeat packet");
			exit(0); /* throw away */
		}
        MsgCode = ntohs(pPkgHeader->MsgCode);   /* Get message code */
    }
    else
    {
        /* the message have packet body */
		pPkgHeader->iCrc32 = ntohl(pPkgHeader->iCrc32);
        /* get packet body */
		pPkgBody = (void *)(pRecvMsgBuff + g_LenMsgHeader + g_LenPkgHeader);
		/* Calculate the crc value to judge the integrity of the package */
		int iCalcCrc32 = m_pCrc32->Get_CRC((unsigned char *)pPkgBody, PkgLen - g_LenPkgHeader);
        MsgCode = ntohs(pPkgHeader->MsgCode);   /* Get message code */
        /* crc validity check */
        if(iCalcCrc32 != pPkgHeader->iCrc32)
		{
            CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "Crc validity check failed, iCalcCrc32 = %d, pPkgHeader->iCrc32 = %d", iCalcCrc32, pPkgHeader->iCrc32);
			exit(0); /* throw away */
		}
	}
    /* crc check is OK */
    //uint16_t MsgCode = ntohs(pPkgHeader->MsgCode);   /* Get message code */
    LPCONNECTION_T pConn = pMsgHeader->pConn;   /* Get pstConn info */
    /* (a)If the client disconnects from the process of receiving the packet 
     * sent by the client to the server releasing a thread in the thread pool 
     * to process the packet, then obviously, we do not have to process 
     * this received packet */    
    if(pConn->currSequence != pMsgHeader->currSequence)   
    {
        /* The pstConn in the pstConn pool is occupied by other tcp connections 
         * [other sockets], which shows that the original client and the server are disconnected, 
         * this packet is directly discarded, [the client is disconnected]*/
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "pConn->currSequence = %d, pMsgHeader->currSequence = %d", 
                                                                pConn->currSequence, pMsgHeader->currSequence);
        exit(0);
    }

    /* (b)Determine that the message code is correct, prevent the client from maliciously 
     * invading our server, and send a message code that is not within the processing range of our server */
	if(MsgCode >= TOTAL_COMMANDS)
    {
        /* Discard this package [malicious package or error package] */
        CLog::Log(LOG_LEVEL_INFO, __THIS_FILE__, __LINE__, "Message code is error, message code = %d", MsgCode);
        //CMemory::GetInstance()->FreeMemory(pRecvMsgBuff);
        exit(0);
    }

    /* (c)Is there a corresponding message processing function */
    /* This way of using MsgCode can make finding member functions to be executed particularly efficient */
    if(StatusHandler[MsgCode] == NULL) 
    {
        CLog::Log(LOG_LEVEL_ERR, __THIS_FILE__, __LINE__, "This is not a handle message process, message code = %d", MsgCode);
        //CMemory::GetInstance()->FreeMemory(pRecvMsgBuff);  /* throw away */
        exit(0);
    }

    /* (d)Call the member function corresponding to the message code to process */
    (g_BusinessCtl.*(StatusHandler[MsgCode]))(pConn, pMsgHeader, (char *)pPkgBody, PkgLen - g_LenPkgHeader);
    
}


/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: PushSendMsgToSendListAndSem
 * discription: Put a message to be sent into the message queue.
 * parameter:
 * =================================================================*/
void CMessageCtl::PushSendMsgToSendListAndSem(char *pSendMsgBuff) 
{
    /* function track */
    //CLog::Log(LOG_LEVEL_TRACK, "CSocket::PushSendMsgToSendListAndSem track 1, pSendMsgBuff = %p", pSendMsgBuff);

    /* m_SendMessageQueueMutex lock */
    CLock lock(&m_SendMsgListMutex);
    /* Send message queue is too large may also bring risks to the server */
    if(m_SendMsgListSize > MAX_SENDQUEUE_COUNT)
    {
        /* If the sending queue is too large, for example, if the client maliciously does not accept the data, 
         * it will cause this queue to grow larger and larger. Then you can consider killing some data 
         * transmission for server security. Although it may cause problems for the client, 
         * it is better than the server. Stability is much better */
        ++m_DiscardSendPkgCount;
        CMemory::GetInstance()->FreeMemory(pSendMsgBuff);
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "m_SendMsgListSize > MAX_SENDQUEUE_COUNT, m_SendMsgListSize = %d", m_SendMsgListSize);
    }
    /* Process packets normally */
    LPMSG_HEADER_T pMsgHeader = (LPMSG_HEADER_T)pSendMsgBuff;
	LPCONNECTION_T pConn = pMsgHeader->pConn;
    //int i = pConn->sendMsgListCount; //signal 11 pConn->sendMsgListCount
    /* The user receives the message too slowly [or simply does not receive the message], 
     * the accumulated number of data entries in the user's sending queue is too large, 
     * and it is considered to be a malicious user and is directly cut*/
    if(pConn->sendMsgListCount > MAX_EACHCONN_SENDCOUNT)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Found that a user %d has a large backlog of packets to be sent", pConn->sockFd); 
        ++m_DiscardSendPkgCount;
        CMemory::GetInstance()->FreeMemory(pSendMsgBuff);
        g_Socket.CloseConnectionToRecy(pConn);
    }
    m_SendMsgList.push_back(pSendMsgBuff);
    ++pConn->sendMsgListCount;
    ++m_SendMsgListSize;

    /* +1 the value of the semaphore so that others stuck in sem_wait can go on 
     * Let the ServerSendQueueThread() process come down and work */
    if(sem_post(&g_SendMsgPushEventSem) == -1) 
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "sem_post(&g_SendMsgPushEventSem) failed");
        exit(0);
    }
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: SendMsgThread
 * discription: send message.
 * parameter:
 * =================================================================*/
void* CMessageCtl::SendMsgThread(void *pThreadData)
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::SendMsgThread track");

    ssize_t uiSendLen = 0;                  /* Record the size of the data that has been sent */
    char *pMsgBuff = NULL;                  /* point send message */
    LPMSG_HEADER_T pMsgHeader = NULL;
	LPCOMM_PKG_HEADER pPkgHeader = NULL;
    LPCONNECTION_T pConn = NULL;
    std::list<char*>::iterator pos;
    std::list<char*>::iterator posTmp;
    std::list<char*>::iterator posEnd;

    ThreadItem *pThread = static_cast<ThreadItem *>(pThreadData);
    CMessageCtl *pMessageCtlObj = pThread->_pThis; 
    
    while(true)
    {
        /* If the semaphore value is> 0, then decrement by 1 and go on, otherwise the card is stuck here. 
         * [In order to make the semaphore value +1, you can call sem_post in other threads to achieve it. 
         * In fact, CSocket :: PushSendMsgToSendListAndSem() calls sem_post. The purpose of letting sem_wait go down here] 
         * If it is interrupted by a signal, sem_wait may also return prematurely. The error is EINTR. 
         * Before the entire program exits, sem_post() is also necessary to ensure that if this thread is stuck in sem_wait (), 
         * Can go on so that this thread returns successfully */
        if(sem_wait(&g_SendMsgPushEventSem) == -1)
        {
            /* This is not an error. [When a process blocked by a slow system call catches a 
             * signal and the corresponding signal processing function returns, the system call may return an EINTR error] */
            if(errno != EINTR) 
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "sem_wait(pSocketObj->g_SendMsgPushEventSem) failed");
                exit(0);
            }
        }

        /* Atomic, to determine whether there is information in the sending message queue, 
         * this should be true, because sem_wait is in, when the message is added to the 
         * information queue, sem_post is called to trigger processing */
        if(pMessageCtlObj->m_SendMsgListSize > 0)
        {
            /* m_SendMessageQueueMutex lock */   
            if(pthread_mutex_lock(&CMessageCtl::m_SendMsgListMutex) != 0)
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "CMessageCtl::m_SendMsgListMutex lock failed");
                exit(0);
            }
            pos = pMessageCtlObj->m_SendMsgList.begin();
			posEnd = pMessageCtlObj->m_SendMsgList.end();
            while(pos != posEnd)
            {
                /* Get message form list to pMsgBuff*/
                pMsgBuff = (*pos);
                pMsgHeader = (LPMSG_HEADER_T)pMsgBuff;
                pPkgHeader = (LPCOMM_PKG_HEADER)(pMsgBuff + g_LenMsgHeader);
                pConn = pMsgHeader->pConn;

                /* The data packet expires, because if this pstConn is recycled, 
                 * for example in CloseConnectionImmediately(), PutRecyConnectQueue(), 
                 * currSequence will be incremented and there is no need to use 
                 * m_ConnectionMutex critical for this pstConn. As long as the following 
                 * conditions are true, it must be that the client pstConn has been disconnected, 
                 * The data to be sent definitely need not be sent */
                if(pConn->currSequence != pMsgHeader->currSequence)
                {
                    /* The serial number saved in this package is different from the actual 
                     * serial number in pConn [pstConn in pstConn pool], so discard this message */
                    posTmp = pos;
                    ++pos;
                    pMessageCtlObj->m_SendMsgList.erase(posTmp);
                    --pMessageCtlObj->m_SendMsgListSize;
                    m_pMemory->FreeMemory(pMsgBuff);
                    continue; /* pNext message */
                } /* end if */

                if(pConn->throwSendMsgCount > 0) 
                {
                    /* Rely on epoll event sending */
                    ++pos;
                    continue; /* pNext message */ 
                }

                /* When you come here, you can send a message, 
                 * some necessary information records, 
                 * and the things you want to send must be removed from the send queue */
                --pConn->sendMsgListCount;
                pConn->pSendMsgMemPointer = pMsgBuff; /* for free memory pSendMsgMemPointer */
                posTmp = pos;
				++pos;
                pMessageCtlObj->m_SendMsgList.erase(posTmp);
                --pMessageCtlObj->m_SendMsgListSize;                     /* send message queue -1 */
                pConn->pSendMsgBuff = (char *)pPkgHeader;               /* save send buffer */
                pConn->sendMsgLen = ntohs(pPkgHeader->PkgLength);     /* host -> net */

                /* Here is the key point. We use the epoll level-triggered strategy. Anyone who can get here 
                 * should have not yet posted a write event to epoll.
                 * The improved scheme of sending data by epoll level trigger:
                 * I don't add the socket write event notification to epoll at the beginning. When I need to write data, 
                 * I directly call write / send to send the data.
                 * If you return EAGIN [indicating that the send buffer is full, you need to wait for a writeable event to continue writing data to the buffer], 
                 * at this time, I will add the write event notification to epoll, and it will become the epoll driver to write data , 
                 * After all the data is sent, then remove the write event notification from epoll
                 * Advantages: When there is not much data, the increase / deletion of epoll's write epollEvents can be avoided, 
                 * and the execution efficiency of the program is improved.*/
                /* (a)Directly call write or send to send data */
                uiSendLen = g_Socket.SendMsgProc(pConn, pConn->pSendMsgBuff, pConn->sendMsgLen);
                if(uiSendLen > 0)
                {
                    /* The data was successfully sent, and all the data was sent out at once */
                    if(uiSendLen == pConn->sendMsgLen)
                    {
                        /* The data successfully sent is equal to the data required to be sent, 
                         * indicating that all the transmissions are successful, 
                         * and the sending buffer is gone [All the data is sent] */
                        m_pMemory->FreeMemory(pConn->pSendMsgMemPointer);
                    }
                    /* Not all the transmission is completed (EAGAIN), 
                     * the data is only sent out partly, 
                     * but it must be because the transmission buffer is full */
                    else
                    {                        
                        /* How much is sent and how much is left, record it, 
                         * so that it can be used pNext time SendMsgProc() */
                        pConn->pSendMsgBuff += uiSendLen;
				        pConn->sendMsgLen -= uiSendLen;	

                        /* Because the send buffer is full, so now I have to rely on the system 
                         * notification to send data to mark the send buffer is full, 
                         * I need to drive the continued sending of messages through epoll epollEvents*/
                        ++pConn->throwSendMsgCount;

                        /* After posting this event, we will rely on the epoll driver to call the EpollEventWriteRequestHandler() function to send data */
                        if(g_Socket.EpollRegisterEvent(pConn->sockFd,       /* socket id */
                                                            EPOLL_CTL_MOD,      /* event type */
                                                            EPOLLOUT,           /* flags, EPOLLOUT：Writable [Notify me when I can write] */
                                                            EPOLL_ADD,          /* EPOLL_ADD，EPOLL_CTL_MOD need the paremeter, 0：add，1：del，2：over */
                                                            pConn               /* pstConn info */
                                                            ) == -1)
                        {
                            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "register epoll event failed");
                            exit(0);
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
                    CLog::Log(LOG_LEVEL_INFO, "CMessageCtl::SendMsgThread track 2");
                    m_pMemory->FreeMemory(pConn->pSendMsgMemPointer);
                    continue; /* pNext message */
                }
                else if(uiSendLen == -1)
                {
                    /* The send buffer is full [one byte has not been sent out, 
                     * indicating that the send buffer is currently full] mark the send buffer is full, 
                     * you need to drive the message to continue sending through epoll */
                    ++pConn->throwSendMsgCount; 
                    /* After posting this event, we will rely on the epoll driver to call the EpollEventWriteRequestHandler() function to send data */
                    if(g_Socket.EpollRegisterEvent(pConn->sockFd,           /* socket id */
                                                        EPOLL_CTL_MOD,      /* event type */
                                                        EPOLLOUT,           /* flags, EPOLLOUT：Writable [Notify me when I can write] */
                                                        EPOLL_ADD,          /* EPOLL_ADD，EPOLL_CTL_MOD need the paremeter, 0：add，1：del，2：over */
                                                        pConn               /* pstConn info */
                                                        ) == -1)
                    {
                        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "register epoll event failed");
                        exit(0);
                    }
                    continue; /* pNext message */ 
                }
                else /* -2 */
                {
                    /* If you can get here, it should be the return value of -2. 
                     * It is generally considered that the peer is disconnected, 
                     * waiting for recv() to disconnect the socket and recycle resources. */
                    CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Send failed sockFd: %d", pConn->sockFd);
                    m_pMemory->FreeMemory(pConn->pSendMsgMemPointer);
                    continue; /* pNext message */ 
                }
            } /* end while(pos != posEnd) */

            if(pthread_mutex_unlock(&CMessageCtl::m_SendMsgListMutex) != 0)
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_unlock(pSocketObj->m_SendMessageQueueMutex) failed");
                exit(0);
            }
        } /* if(pSocketObj->m_iSendMsgQueueCount > 0) */
    } /* end while */

    return (void*)XiaoHG_SUCCESS;
}

void CMessageCtl::RecvMsgHeaderParseProc(LPCONNECTION_T pConn, bool &bIsFlood)
{
    LPCOMM_PKG_HEADER pPkgHeader = NULL;
    /* packet head in dataHaeadInfo munber */
    pPkgHeader = (LPCOMM_PKG_HEADER)pConn->dataHeadInfo;
    /* Get packet length */
    uint16_t PkgLen = ntohs(pPkgHeader->PkgLength);

    /* Malformed packet filtering: judgment of malicious packets or wrong packets */
    if(PkgLen < g_LenPkgHeader || PkgLen > PKG_MAX_LENGTH) 
    {
        pConn->recvMsgCurStatus = PKG_HD_INIT;
        pConn->pRecvMsgBuff = pConn->dataHeadInfo;
        pConn->recvMsgLen = g_LenPkgHeader;
    }
    /* Legal packet header, continue processing */
    else
    {
        char *pMsgBuffer = (char *)m_pMemory->AllocMemory(g_LenMsgHeader + PkgLen, false); 
        pConn->pRecvMsgMemPointer = pMsgBuffer;

        /* a)Fill in the message header first */
        LPMSG_HEADER_T ptmpMsgHeader = (LPMSG_HEADER_T)pMsgBuffer;
        ptmpMsgHeader->pConn = pConn;
        ptmpMsgHeader->currSequence = pConn->currSequence;
        
        /* b)Then fill in the data packet header */
        pMsgBuffer += g_LenMsgHeader;
        memcpy(pMsgBuffer, pPkgHeader, g_LenPkgHeader);
        if(PkgLen == g_LenPkgHeader)
        {
            /* only packet header */
            if(g_Socket.GetFloodAttackEnable() == 1) 
            {
                /* Flood check*/
                bIsFlood = g_Socket.TestFlood(pConn);
            }
            RecvMsgPacketParseProc(pConn, bIsFlood);
        }
        else
        {
            /* receive packet body */
            pConn->recvMsgCurStatus = PKG_BD_INIT;                /* change statuc to PKG_BD_INIT */
            pConn->pRecvMsgBuff = pMsgBuffer + g_LenPkgHeader;     /* pMsgBuffer + g_LenPkgHeader point packet body */
            pConn->recvMsgLen = PkgLen - g_LenPkgHeader;         /* PkgLen - g_LenPkgHeader is packet body length */
        }
    }/* end if(PkgLen < g_LenPkgHeader)  */
}


/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: RecvMsgPacketParseProc
 * discription: receive compelet message packet and save to recvice 
 *              message list.
 * parameter:
 * =================================================================*/
void CMessageCtl::RecvMsgPacketParseProc(LPCONNECTION_T pConn, bool &bIsFlood)
{
    if(bIsFlood == false)
    {
        /* Enter the message queue and trigger the thread to process the message */
        PushRecvMsgToRecvListAndCond(pConn->pRecvMsgMemPointer);
    }
    else
    {
        /* Is flood*/
        /* Directly release the memory without entering the message queue at all */
        m_pMemory->FreeMemory(pConn->pRecvMsgMemPointer); 
    }
    pConn->recvMsgCurStatus = PKG_HD_INIT;   /* Init */                  
    pConn->pRecvMsgBuff = pConn->dataHeadInfo; /* Init */
    pConn->recvMsgLen = g_LenPkgHeader;      /* Init */
}

void CMessageCtl::PushRecvMsgToRecvListAndCond(char *pRecvMsgBuff)
{
    /* m_RecvMsgListMutex lock */
    if(pthread_mutex_lock(&m_RecvMsgListMutex) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_lock(&m_RecvMsgListMutex) failed");
        exit(0);
    }
    /* Join the message queue */
    LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)(pRecvMsgBuff + g_LenMsgHeader);  /* packet header */
    m_RecvMsgList.push_back(pRecvMsgBuff);
    ++m_RecvMsgListSize;

    /* m_RecvMsgListMutex unlock */
    if(pthread_mutex_unlock(&m_RecvMsgListMutex) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_mutex_unlock(&m_RecvMsgListMutex) failed");
        exit(0);
    }
    /* Can excite the thread to work */
    /* Wake up a thread waiting for the condition, that is, 
     * a thread that can be stuck in pthread_cond_wait() */
    if(pthread_cond_signal(&CThreadPool::m_pThreadCond) != 0)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "pthread_cond_signal(&m_pThreadCond) failed");
        exit(0);
    }
}

/* */
std::list<char*>& CMessageCtl::GetRecvMsgList()
{
    return m_RecvMsgList;
}

void CMessageCtl::ClearMsgSendList()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CMessageCtl::ClearMsgSendList track");

	char *pMemPoint = NULL;
	while(!m_SendMsgList.empty())
	{
		pMemPoint = m_SendMsgList.front();
		m_SendMsgList.pop_front(); 
		CMemory::GetInstance()->FreeMemory(pMemPoint);
	}
}
