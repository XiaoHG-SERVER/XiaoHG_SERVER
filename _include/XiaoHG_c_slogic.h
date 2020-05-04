
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
 */

#ifndef __XiaoHG_C_SLOGIC_H__
#define __XiaoHG_C_SLOGIC_H__

#include <sys/socket.h>
#include "XiaoHG_C_Socket.h"

/* Subclasses that handle logic and 
 * communication inherit from the parent class CScoekt */
class CLogicSocket : public CSocket		
{
public:
	CLogicSocket();
	virtual ~CLogicSocket();
	virtual int Initalize(); /* Init */

public:

	/* send heartbeat */
	void  SendNoBodyPkgToClient(LPMSG_HEADER_T pMsgHeader, unsigned short iMsgCode);

	/* Business logic */
	bool HandleRegister(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, unsigned short iBodyLength);
	bool HandleLogIn(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, unsigned short iBodyLength);
	bool HandleHeartbeat(LPCONNECTION_T pConn, LPMSG_HEADER_T pMsgHeader, char *pPkgBody, unsigned short iBodyLength);

	/* When the heartbeat packet detection time is up, detect whether the heartbeat packet times out */
	virtual void HeartBeatTimeOutCheckProc(LPMSG_HEADER_T tmpmsg, time_t cur_time);

public:
	/* To process the received data packet, this function is called by the thread pool, 
	 * this function is a separate thread */
	virtual void ThreadRecvMsgHandleProc(char *pMsgBuf);
};

#endif //!__XiaoHG_C_SLOGIC_H__
