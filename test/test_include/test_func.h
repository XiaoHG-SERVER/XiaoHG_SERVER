
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
 */

#ifndef __TEST_INCLUDE_H__
#define __TEST_INCLUDE_H__

/* Init process */
int SocketInit();

/* More test functions */
void MsgCodeMenu();
int ChooseSendMsg();

/* Send ande receive message process */
int SendData(char *pSendBuf, int iBufLen);
int RecvData(char *pRecvBuf, int iRecvLen);

/* Thread process and handle message process */
int ThreadInit();
int SendHeartBeatMsg();
int SendLoginMsg();
int SendRegisterMsg();

#endif //__TEST_INCLUDE_H__