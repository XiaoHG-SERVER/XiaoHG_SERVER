
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_SOCKET_H__
#define __XiaoHG_SOCKET_H__

#include <vector>
#include <list>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>
#include <atomic>
#include <map>
#include "XiaoHG_Comm.h"

class CMemory;
class CConfig;

/* Connection queue completed */
#define XHG_LISTEN_SOCKETS 511

/* epoll_wait receives at most so many iEpollEvents at a time
 * This value cannot be greater than the iBuffLen when creating epoll_create() */
#define EPOLL_MAX_EVENTS 512

typedef struct listening_s		LISTENING_T, *LPLISTENING_T;
typedef struct connection_s		CONNECTION_T, *LPCONNECTION_T;
typedef class CSocket 			CSocket;

/* Epoll event processing callback */
typedef void (CSocket::*EpollEventHandlerCallBack)(LPCONNECTION_T pConn); 

/* listening socket */
struct listening_s
{
	int iPort;        				/* port */
	int iSockFd;          			/* socket id */
	LPCONNECTION_T pConnection;  	/* Corresponding to a connection in the connection pool */ 
};

/* This structure represents a TCP connection 
 * [TCP connection initiated by the client and passively accepted by the server] */
struct connection_s
{
	connection_s();							/* Constructor */
	virtual ~connection_s();				/* Destructor */
	void GetOneToUse();						/* Initialize some content when assigned */
	void PutOneToFree();					/* Initialize variables when reclaiming */

	/* about connect cofing */
	int iSockFd;							/* socket id */
	struct sockaddr stPeerSockAddr;				/* save peer address info */
	LPLISTENING_T listening;				/* Corresponding listening socket */		

	/* epoll event */
	int iEpollEvents;									/* epoll events */  
	EpollEventHandlerCallBack pcbReadHandler;			/* read event LogicHandlerCallBack */
	EpollEventHandlerCallBack pcbWriteHandler;			/* write event LogicHandlerCallBack */
	
	/* receive message */
	int iRecvCurrentStatus;					/* current receive message status */
	char dataHeadInfo[MSG_HEADER_LEN];		/* save receive message header */			
	char *pRecvMsgBuff;						/* The header pointer of the buffer receiving the data, very useful for receiving incomplete packets */
	int iRecvMsgLen;						/* How much data to receive is specified by this variable and used in conjunction with pRecvMsgBuff */
	char *pRecvMsgMemPointer;				/* The first memory address for receiving packets from new will be used when releasing */

	/* send message */
	/* iThrowSendCount: Send a message, if the send buffer is full, you need to drive the continued sending of the message through the epoll event, 
	 * so if the send buffer is full, use this variable tag*/
	std::atomic<int> iThrowSendCount;
	/* The header pointer of the entire data used for release after sending is actually: 
	 * message header + packet header + packet body */
	char *pSendMsgMemPointer;				
	char *pSendMsgBuff;						/* The header pointer of the buffer to send data is actually the header + body */
	int iSendMsgLen;						/* Send data iBuffLen */

	/* Related to recycling */
	time_t PutRecyConnQueueTime;						/* Time to enter the resource recycling bin */

	/* Heartbeat */
	time_t iLastHeartBeatTime;				/* The time when the heartbeat was last received */

	/* cyber security */
	uint64_t uiFloodKickLastTime;			/* The time when the Flood attack last received the packet */
	int iFloodAttackCount;					/* Statistics on the number of times flood attacks received packets during this time */
	/* The number of data entries in the sending queue. If the client only sends and does not receive, 
	 * it may cause this number to be too large. */ 
	std::atomic<int> iSendQueueCount;		
	
	/* others */
	/* A serial number introduced, +1 every time it is allocated, 
	 * this method is also possible to detect wrong packets and waste packets to a certain extent */
	uint64_t uiCurrentSequence;				
	pthread_mutex_t LogicPorcMutex;			/* Logic processing related mutex */

	/* This is a pointer to the next object of this type, 
	 * used to string free connection pool objects to form a singly linked list for easy access. */
	LPCONNECTION_T pNext;
};

/* message header */
typedef struct msg_header_s
{
	LPCONNECTION_T pConn;			/* connect info */
	/* Record the serial number of the corresponding connection when the data packet is received, 
	 * which can be used to compare whether the connection has been invalidated */
	uint64_t uiCurrentSequence;
}MSG_HEADER_T, *LPMSG_HEADER_T;

/* socket class */
class CSocket
{
public:
	CSocket();
	virtual ~CSocket();
	void Init();						/* Initialization function [executed in parent process] */
	virtual int InitializeSubProc();		/* Initialization function [executed in subprocess] */
	virtual void ShutdownSubProc();			/* Close exit function [execute in child process] */
	void PrintTDInfo();						/* Print statistical information for use in dimension testing */

public:
	/* Receive message process, implemented in subclasses */
	virtual void RecvMsgHandleThreadProc(char *pMsgBuf);

	/* When the heartbeat packet detection time is up, it is time to detect whether 
	 * the heartbeat packet has timed out. This function just releases the memory. 
	 * Subclasses should reimplement this function to implement specific judgment actions */
	virtual void HeartBeatTimeOutCheck(LPMSG_HEADER_T tmpmsg, time_t CurrentTime);  

public:/* Epoll */
	int EpollInit();						/* Epoll init */	
	int EpolWaitlProcessEvents(int iTimer);	/* epoll waits to receive and process events */
	/* Register epoll event */
	int EpollRegisterEvent(int iSockFd, uint32_t uiEventType, uint32_t uiFlag, int iAction, LPCONNECTION_T pConn); 
	void EpollEventAcceptHandler(LPCONNECTION_T pOldConn);		/* Create a new connection event LogicHandlerCallBack */
	void EpollEventReadRequestHandler(LPCONNECTION_T pConn);	/* Read processing event program when data comes */
	void EpollEventWriteRequestHandler(LPCONNECTION_T pConn);	/* Write processing event program when data is sent */                                               
	
public:/* Recycle connection */
	/* When actively closing a connection, do some aftercare function */
	void CloseConnectionToRecy(LPCONNECTION_T pConn);

public:
	/* Throw the data into the queue to be sent */	
	void SendMsg(char *pSendMsgBuff);					
	
private:	
	void LoadConfig();						/* Specially used to read various configuration items */
	int OpenListeningSockets();				/* The necessary port for monitoring [supports multiple ports] */
	int SetNonBlocking(int sockfd);			/* Set up non-blocking sockets */

	/* Business processing function */
	ssize_t RecvProc(LPCONNECTION_T pConn, char *pMsgBuff, ssize_t iBuffLen); 	/* Receive data from the client */
	ssize_t SendProc(LPCONNECTION_T pConn, char *pMsgBuff, ssize_t iBuffLen);	/* Send data to the client */

	void RecvMsgHeaderParseProc(LPCONNECTION_T pConn, bool &isflood);		/* Parse message header */      
	void RecvMsgPacketParseProc(LPCONNECTION_T pConn, bool &isflood);		/* Parse packat */	

	/* Obtaining peer information */
	/* According to the information given in parameter 1, get the address port string and return the length of this string */
	size_t ConnectionInfo(struct sockaddr *pstSockAddr, int iPort, u_char *pText, size_t uiLen);  	

	/* Connection */
	void InitConnectionPool();							/* Connect init */
	LPCONNECTION_T GetConnection(int iSockFd);	/* Get a new connection */
	
	/* Timer */
	void AddToTimerQueue(LPCONNECTION_T pConn);		/*Set the kick-out clock (add content to the map table) */
	time_t GetTimerQueueEarliestTime();						/* Get the earliest time from multimap and go back */
	/* Remove the earliest time from m_timeQueuemap, 
	 * and return the pointer corresponding to the value of the earliest item at this time */
	LPMSG_HEADER_T RemoveTimerQueueFirstTime();
	/* According to the given current time, find the nodes older than this time (earlier) from m_timeQueuemap [1], 
	 * these nodes are all nodes that have exceeded the time, and need to be processed */
	LPMSG_HEADER_T GetOverTimeTimer(time_t CurrentTime);

	/* cyber security */
	/* Test whether the flood attack is established, return true if it is established, otherwise return false */
	bool TestFlood(LPCONNECTION_T pConn);	
	
	/* Thread */
	static void* SendMsgQueueThreadProc(void *threadData);				/* A thread dedicated to sending data */
	static void* ServerRecyConnectionThreadProc(void *threadData);			/* Specially used to recycle connected threads */
	/* Time queue monitoring thread, processing threads kicked by users who do not send heartbeat packets when they expire */
	static void* HeartBeatMonitorThreadProc(void *threadData);		
	
private:/* Recycle */
	void CloseListeningSocket();						/* Close Listening socket */
	void CloseConnectionImmediately(LPCONNECTION_T pConn);			/* General connection close function, resources are released with this function */
	void ClearMsgSendQueue();							/* Processing the send message queue */

	void PutConnectToRecyQueue(LPCONNECTION_T pConn);		/* Put the connection to be recycled into the delayed recycling queue */
	void ClearConnections();								/* Recycle connection pool */
	void FreeConnection(LPCONNECTION_T pConn);			/* Return the connection represented by the parameter pConn to the connection pool */

	void DeleteFromTimerQueue(LPCONNECTION_T pConn);	/* Delete the specified user tcp connection from the Timer table */
	void ClearAllFromTimerQueue();						/* Clean up everything in the time queue */
	
protected:/* communication numbers */
	size_t m_iLenMsgHeader;		/* sizeof(MSG_HEADER_T) */
	size_t m_iLenPkgHeader;		/* sizeof(COMM_PKG_HEADER) */

	/* Time */
	/* When the time reaches the time specified by Sock_MaxWaitTime, 
	 * the client will be kicked out directly. This item is valid only when Sock_WaitTimeEnable = 1. */
	int m_iIsTimeOutKick;		
	/* How many seconds to detect whether the heartbeat times out, 
	 * this item is only valid when Sock_WaitTimeEnable = 1 */
	int m_iWaitTime;			
	
private:/* Thread struct */
	struct ThreadItem
    {
        pthread_t _Handle;		/* thread handle */
        CSocket *_pThis;		/* thread pointer */	
        bool bIsRunning;		/* Whether the flag is officially started, and it is only allowed to call StopAll () to release after it is started */

        /* Constructor */
        ThreadItem(CSocket *pthis):_pThis(pthis), bIsRunning(false){}                             
        /* Destructor */
        ~ThreadItem(){}        
    };

protected:
	static CMemory *m_pMemory;
	static CConfig *m_pConfig;

private:
	/* Thread pool config */
	std::atomic<int> m_TotalConnectionCount;                  		/* The total number of connections in the connection pool */
	std::list<LPCONNECTION_T> m_ConnectionList;         			/* Connection list [connection pool] */
	std::list<LPCONNECTION_T> m_FreeConnectionList;     			/* Idle connection list [all idle connections installed here] */
	std::atomic<int> m_FreeConnectionCount;                   		/* Connection pool idle connections */

	/* Mutual correlation of the connection queue, mutually exclusive m_FreeConnectionList, m_ConnectionList two lists */
	pthread_mutex_t m_ConnectionMutex;								

	/* Delayed reclamation queue */
	std::list<LPCONNECTION_T> m_RecyConnectionList;					/* Delayed reclamation queue */				
	std::atomic<int> m_TotalRecyConnectionCount;              			/* Connection queue size to be released */
	int m_RecyConnectionWaitTime;              						/* Waiting for recycling time */
	pthread_mutex_t m_RecyConnQueueMutex;							/* Mutex related to connection delay recovery queue */

	/* epoll */
	int m_EpollHandle;                         						/* epoll_create handle */
	int m_EpollCreateConnectCount;                  						/* epoll max */
	struct epoll_event m_Events[EPOLL_MAX_EVENTS];					/* Used to carry the returned event in epoll_wait() */

	/* Listen socket queue */
	int m_ListenPortCount;                     						/* Number of ports monitored */
	std::vector<LPLISTENING_T> m_ListenSocketList;					/* Listen socket queue */				

	/* message queue */
	std::list<char *> m_MsgSendQueue;								/* Send data message queue */
	std::atomic<int> m_iSendMsgQueueCount;							/* Message queue size */

	/* Thread pool */
	std::vector<ThreadItem *> m_ThreadPoolVector;						/* Thread container, each thread is in the container */	
	pthread_mutex_t m_SendMessageQueueMutex;						/* Message queue mutex */
	sem_t m_SemEventSendQueue;										/* Handle the semaphore related to the message thread */

	/* Heartbeat detection time queue */
	std::multimap<time_t, LPMSG_HEADER_T> m_TimerQueueMultiMap;		/* Heartbeat detection time queue */
	size_t m_CurrentHeartBeatListSize;									/* Time queue size */
	time_t m_HBListEarliestTime;											/* Current head value of the timing queue */
	int m_IsHBTimeOutCheckEnable;											/* Whether to enable kicking clock, 1: on, 0: off */
	pthread_mutex_t m_TimeQueueMutex;								/* Mutex related to time queue */

	/* Network security related */
	int m_FloodAttackEnable;										/* Whether flood attack detection is enabled, 1: enabled, 0: disabled */
	int m_FloodTimeInterval;										/* It means that the interval of receiving each data packet is 100ms */
	int m_FloodKickCount;											/* How many times have you kicked this person */

	/* Maintenance information */
	time_t m_LastPrintTime;											/* Last time the statistics were printed (printed every 10 seconds) */
	std::atomic<int> m_OnLineUserCount;								/* Current online user statistics */
	int m_iDiscardSendPkgCount;										/* Number of send packets dropped */
};

#endif //!__XiaoHG_SOCKET_H__
