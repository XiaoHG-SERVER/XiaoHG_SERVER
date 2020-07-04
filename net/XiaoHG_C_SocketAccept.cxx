
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

#define __THIS_FILE__ "XiaoHG_C_SocketAccpet.cxx"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: EpollEventAcceptHandler
 * discription: accept a new pstConn
 * parameter:
 * =================================================================*/
void CSocket::EpollEventAcceptHandler(LPCONNECTION_T pOldConn)
{
    int errNo = 0;   /* errno */
    int sockFd = 0;    /* socket id */
    static bool iIsAccept4 = true;  /* defult use accept4 */
    struct sockaddr stPeerSocketAddr;   /* the peer socket address */
    socklen_t iSockAddrLen = sizeof(struct sockaddr);
    memset(&stPeerSocketAddr, 0, iSockAddrLen);
    
    do
    {     
        if(iIsAccept4)
        {
            /* accept4 can set SOCK_NONBLOCK */
            sockFd = accept4(pOldConn->sockFd, &stPeerSocketAddr, &iSockAddrLen, SOCK_NONBLOCK); 
        }
        else
        {
            sockFd = accept(pOldConn->sockFd, &stPeerSocketAddr, &iSockAddrLen);
        }

        if(sockFd == -1)
        {
            errNo = errno;
            /* For accept, send, and recv, errno is usually set to EAGAIN (meaning "again")
             * or EWOULDBLOCK (meaning "expect to block") when the event has not occurred. */
            if(errNo == EAGAIN)
            {
                CLog::Log(LOG_LEVEL_ERR, errNo, __THIS_FILE__, __LINE__, "accept new connection failed");
            }

            /* ECONNRESET error occurs after the other party accidentally closes the socket 
             * [The software in your host gave up an established connection-the connection 
             * was aborted due to timeout or other failures (this error may occur when the user 
             * plugs and unplugs the network cable)] */
            else if (errNo == ECONNABORTED)
            {
                /* this errno discription: “software caused connection abort” */
                CLog::Log(LOG_LEVEL_ERR, errNo, __THIS_FILE__, __LINE__, "accept new connection failed");
            }

            /* EMFILE: The sockFd of the process has been exhausted [the total number of files / sockets 
             * that can be opened by a single process allowed by the system has been reached] .
             * ulimit -n，Take a look at the file descriptor limit, if it is 1024, it needs to be changed, 
             * the number of open file handles is too large, and the system's sockFd soft limit and hard limit are raised.
             * The existence of the errno ENFILE indicates that there must be system-wide resource limits, 
             * not just process-specific resource limits. According to common sense, process-specific 
             * resource limits must be limited to system-wide resource limits.*/
            else if (errNo == EMFILE || errNo == ENFILE) 
            {
                CLog::Log(LOG_LEVEL_ERR, errNo, __THIS_FILE__, __LINE__, "accept new connection failed");
            }

            /* accept4() if no surpose */
            else if(iIsAccept4 && errNo == ENOSYS) 
            {
                iIsAccept4 = false;     /* use accept() function*/
                continue;               /* back */
            }
            else
            {
                CLog::Log(LOG_LEVEL_ERR, errNo, __THIS_FILE__, __LINE__, "accept new connection failed, We didn't check this error");
            }
            return;
        }/* end if(sockFd == -1) */

        /* too meny connection, reject new connect */
        if(m_OnLineUserCount >= m_EpollCreateConnectCount)
        {
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "the maximum number of incoming users allowed by the system, close the incoming request");
            close(sockFd);
            return;
        }

        /* If some malicious users connect and send 1 data, they will disconnect and continue to connect, 
         * which will cause frequent calls to GetConnection() to use us to generate a large number of
         * connections in a short time, endangering the security of this server. */
        if(m_ConnectionList.size() > (m_EpollCreateConnectCount * 5))
        {
            if(m_FreeConnectionList.size() < m_EpollCreateConnectCount)
            {                                     
                /* The entire connection pool is so large, but there are so few idle connections, 
                 * so I think that a large number of connections are generated in a short period of time, 
                 * and it is disconnected after sending a packet. We cannot let this happen continuously,
                 *  so we must disconnect new users. Connection until m_FreeConnectionList becomes large enough 
                 * [more than enough connections in the connection pool are recycled] */
                CLog::Log(LOG_LEVEL_ALERT, __THIS_FILE__, __LINE__, "Accept new connection is unusual");
                close(sockFd);
                return;
            }
        }

        /* accept access, get a new connect pool obj */
        LPCONNECTION_T pNewConn = GetConnection(sockFd); 
        if(pNewConn == NULL)
        {
            /* no more free connection */
            if(close(sockFd) == -1)
            {
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "close socket: %d failed", sockFd);
            }
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Socket: %d get a new connection failed", sockFd);
            exit(0);
        }

        /* Copy the client address to the connection object */
        memcpy(&pNewConn->stPeerSockAddr, &stPeerSocketAddr, sizeof(stPeerSocketAddr));

        if(!iIsAccept4)
        {
            /* If it is not a socket obtained with accept4(), 
             * then it must be set to non-blocking [because accept4() 
             * has been set to non-blocking by accept4()]*/
            if(SetNonBlocking(sockFd) == XiaoHG_ERROR)
            {
                /* set nonblock failed */
                CloseConnectionImmediately(pNewConn);
                CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "set socket: %d nonblock failed", sockFd);
                exit(0);
            }
        }
        /* The connection object is associated with the monitoring object, 
         * which is convenient for finding the monitoring object through 
         * the connection object [associated to the monitoring port] */
        pNewConn->stListenSocket = pOldConn->stListenSocket;
        /* Set the read processing function when the data comes */
        pNewConn->pcbReadHandler = &CSocket::EpollEventReadRequestHandler; 
        /* Set the write processing function when data is sent */
        pNewConn->pcbWriteHandler = &CSocket::EpollEventWriteRequestHandler; 
        /* The client should proactively send the first data. Here, 
         * the read event is added to epoll monitoring, so that when 
         * the client sends data, it will trigger XiaoHG_wait_request_handler() 
         * to be called by EpolWaitlProcessEvents() */
        if(EpollRegisterEvent(sockFd,                  /* socekt id */
                                EPOLL_CTL_ADD,          /* event type */
                                EPOLLIN|EPOLLRDHUP,     /* flag, EPOLLIN：read，EPOLLRDHUP：TCP connection is closed or half closed */
                                0,
                                pNewConn                /* Corresponding to the connection in the connection pool */
                                ) == -1)
        {
            CloseConnectionImmediately(pNewConn);
            CLog::Log(LOG_LEVEL_ERR, errno, __THIS_FILE__, __LINE__, "Add event epoll failed, socket id: %d", sockFd);
            exit(0);
        }
        /* Heartbeat kick */
        if(m_IsHBTimeOutCheckEnable == 1)
        {
            AddToTimerList(pNewConn);
        }
        ++m_OnLineUserCount;
        break;
    } while(1);
}

