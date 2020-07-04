
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
#include "XiaoHG_C_Log.h"
#include "XiaoHG_error.h"

#define __THIS_FILE__ "XiaoHG_C_SocketInet.cxx"


/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: InitEpoll
 * discription: Init epoll
 * parameter:
 * =================================================================*/
uint32_t CSocket::InitEpoll()
{
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::InitEpoll track");

    m_EpollHandle = epoll_create(m_EpollCreateConnectCount);
    if (m_EpollHandle == -1)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "create epoll failed");
        exit(0);
    }

    /* Set stListenSocket socket join epoll event */	
	for(std::vector<LPLISTENING_T>::iterator pos = m_ListenSocketList.begin(); pos != m_ListenSocketList.end(); pos++)
    {
        /* get free pstConn from free list for new socket */
        LPCONNECTION_T pConn = GetConnection((*pos)->sockFd); 
        if (pConn == NULL)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "get free connection failed");
            exit(0);
        }
        /* The pstConn object is associated with the monitoring object, 
         * which is convenient for finding the monitoring object through 
         * the pstConn object. */
        pConn->stListenSocket = (*pos);  
        /* The monitoring object is associated with the pstConn object, 
         * which is convenient for finding the pstConn object through 
         * the monitoring object. */
        (*pos)->pstConn = pConn; 
        /* Set the processing method for the read event of the stListenSocket port, 
         * because the stListenSocket port is used to wait for the other party to 
         * send a three-way handshake, so the stListenSocket port is concerned with 
         * the read event. */
        pConn->pcbReadHandler = &CSocket::EpollEventAcceptHandler;
        if(EpollRegisterEvent((*pos)->sockFd,          /* socekt id */
                                EPOLL_CTL_ADD,          /* event mode */
                                EPOLLIN|EPOLLRDHUP,     /* uiFlag: EPOLLIN：read, EPOLLRDHUP：TCP connection is closed or half closed */
                                0,                      /* For the event type is increased, this parameter is not required */
                                pConn                   /* pstConn pool obj */
                                ) == -1) 
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "stListenSocket socket add epoll event failed");
            exit(0);
        }
    } /* end for  */
    
    CLog::Log("Epoll Init successful");
    return XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ConnectInfo
 * discription: Convert the socket-bound address to text format 
 *              [According to the information given in parameter 1, 
 *              get the address port string and return the length of this string].
 * parameter:
 * =================================================================*/
size_t CSocket::ConnectionInfo(struct sockaddr *pSockAddr, int port, u_char *pText, size_t uiLen)
{

#ifdef __XiaoHG_LOG_TRACK__
    /* function track */
    CLog::Log(LOG_LEVEL_TRACK, "CSocket::ConnectionInfo track");
#endif//!__XiaoHG_LOG_TRACK__

    u_char *p = NULL;
    struct sockaddr_in *sin = NULL;

    switch (pSockAddr->sa_family)
    {
    case AF_INET:
        sin = (struct sockaddr_in *) pSockAddr;
        p = (u_char *) &sin->sin_addr;
        if(port)  /* Port information is also combined into a string */
        {
            /* The new writable address is returned */
            CLog::Log(LOG_LEVEL_INFO, __THIS_FILE__, __LINE__, "%ud.%ud.%ud.%ud:%d", p[0], p[1], p[2], p[3], ntohs(sin->sin_port));
        }
        else /* No need to combine port information into a string */
        {
            CLog::Log(LOG_LEVEL_INFO, __THIS_FILE__, __LINE__, "%ud.%ud.%ud.%ud", p[0], p[1], p[2], p[3]);  
        }
        return (p - pText);
        break;
    default:
        return 0;
        break;
    }
    return 0;
}

uint32_t CSocket::EpollRegisterEvent(uint32_t fd, uint32_t uiEventType, uint32_t uiFlag, uint32_t action, LPCONNECTION_T pConn)
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
        /* record to the pstConn */  
        pConn->epollEvents = uiFlag;
    }
    
    /* the node already, change the mark*/
    else if(uiEventType == EPOLL_CTL_MOD)
    {
        ev.events = pConn->epollEvents;  /* Restore the mark first */
        switch (action)
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
        pConn->epollEvents = ev.events; 
    }
    else
    {
        /* Keep */
        return XiaoHG_SUCCESS;
    } 
    /* set the event memory point*/
    ev.data.ptr = (void *)pConn;
    
    /* This function is used to control epollEvents on an epoll file descriptor. 
     * You can register epollEvents, modify epollEvents, and delete epollEvents */
    if(epoll_ctl(m_EpollHandle, uiEventType, fd, &ev) == -1)
    {
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "epoll_ctl(%d, %ud, %ud, %d) failed", fd, uiEventType, uiFlag, action);
        return XiaoHG_ERROR;
    }
    return XiaoHG_SUCCESS;
}

uint32_t CSocket::EpolWaitlProcessEvents(uint32_t iTimer) 
{
    /* Wait for the event, the event will return to m_Events, at most return EPOLL_MAX_EVENTS epollEvents [because I only provide these memories], 
     * if the interval between two calls to epoll_wait () is relatively long, it may accumulate Events, 
     * so calling epoll_wait may fetch multiple epollEvents, blocking iTimer for such a long time unless: 
     * a) the blocking time arrives, 
     * b) the event received during the blocking [such as a new user to connect] will return immediately, 
     * c) when calling The event will return immediately, 
     * d) If there is a signal, for example, you use kill -1 pid, 
     * if iTimer is -1, it will always block, if iTimer is 0, it will return immediately, even if there is no event.
     * Return value: If an error occurs, -1 is returned. The error is in errno. For example, 
     * if you send a signal, it returns -1. The error message is (4: Interrupted system call)
     * If you are waiting for a period of time, and it times out, it returns 0. If it returns> 0, 
     * it means that so many epollEvents have been successfully captured [in the return value] */
    int epollEvents = epoll_wait(m_EpollHandle, m_Events, EPOLL_MAX_EVENTS, iTimer);
    if(epollEvents == -1)
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
            exit(0);  /* error return */
        }
    }

    /* Timed out, but no event came */
    if(epollEvents == 0)
    {
        if(iTimer != -1)
        {
            /* Require epoll_wait to block for a certain period of time instead of blocking all the time. 
             * This is blocking time and returns normally */
            return XiaoHG_SUCCESS;
        }
        /* Infinite wait [so there is no timeout], but no event is returned, this should be abnormal */
        CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "epoll wait failed");
        exit(0); /* error return */
    }

    uint32_t uiRevents = 0;
    LPCONNECTION_T pConn = NULL;
    /* Traverse all epollEvents returned by epoll_wait this time, 
     * note that epollEvents is the actual number of epollEvents returned */
    for(int i = 0; i < epollEvents; i++)
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
        
        /* write event [the other party closes the pstConn also triggers this] */
        if(uiRevents & EPOLLOUT) 
        {
            /* The client is closed, if a write notification event is hung on the server, 
             * this condition is possible */
            if(uiRevents & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) 
            {
                /* EPOLLERR：The corresponding pstConn has an error.
                 * EPOLLHUP：The corresponding pstConn is suspended.
                 * EPOLLRDHUP：Indicates that the remote end of the TCP pstConn is closed or half closed. 
                 * We only posted the write event, but when the peer is disconnected, the program flow came here. Posting the write event means that the 
                 * throwSendMsgCount flag must be +1, and here we subtract. */
                --pConn->throwSendMsgCount;
            }
            else
            {
                /* Call data sending */
                (this->* (pConn->pcbReadHandler))(pConn);
            }
        }
    } /* end for(int i = 0; i < epollEvents; ++i) */
    return XiaoHG_SUCCESS;
}
