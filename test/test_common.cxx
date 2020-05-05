
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
 */

#include <stdio.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "test_global.h"
#include "test_func.h"
#include "test_common.h"

void PrintMsgInfo(char *pSendBuf, int iBufLen)
{
	LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)pSendBuf;
	printf("[%s: %d]SendData()即将要发送测试数据给服务端：pPkgHeader->msgCode = %d, pPkgHeader->pkgLen = %d\n",
												__FILE__, __LINE__, ntohs(pPkgHeader->msgCode), ntohs(pPkgHeader->pkgLen));
}
	
int SendData(char *pSendBuf, int iBufLen)
{
	int iUwrote = 0;
	int iTmpSret = 0;
	int iUsend = iBufLen;
	
	/* dbg code */
	PrintMsgInfo(pSendBuf, iBufLen);

	if(pSendBuf == NULL)
	{
		printf("[%s: %d]SendData() pSendBuf == NULL\n", __FILE__, __LINE__);
		return -1;
	}

	while (iUwrote < iUsend)
	{
		iTmpSret = send(g_iSocketFd, pSendBuf + iUwrote, iUsend - iUwrote, 0);
		if ((iTmpSret == -1) || (iTmpSret == 0))
		{
			printf("[%s: %d]SendData()/send() failed! iTmpSret = %d, errno = %d, g_iSocketFd = %d\n", __FILE__, __LINE__, iTmpSret, errno, g_iSocketFd);
			return -1;
		}
		iUwrote += iTmpSret;
	}//end while

	return iUwrote;
}

int RecvData(char *pRecvBuf, int iRecvLen)
{
	int n = 0;
	n = recv(g_iSocketFd, pRecvBuf, iRecvLen, 0);
	if(n == -1)
	{
		printf("[%s: %d]RecvData()/recv() failed! errno = %d\n", __FILE__, __LINE__, errno);
		close(g_iSocketFd);
		return -1;
	}
	return n;
}

int ChooseSendMsg()
{
	MsgCodeMenu();
	int iCode = 0;
	scanf("%d", &iCode);
	switch (iCode)
	{
	case HB_CODE:
		if(SendHeartBeatMsg() == -1)
		{
			printf("[%s: %d]main()/SendHeartbeatProc() failed!\n", __FILE__, __LINE__);
			return -1;
		}
		break;
	case REGISTER_CODE:
		if(SendRegisterMsg() == -1)
		{
			printf("[%s: %d]main()/SendRegisterProc() failed!\n", __FILE__, __LINE__);
			return -1;
		}
		break;
	case LINGIN_CODE:
		if(SendLoginMsg() == -1)
		{
			printf("[%s: %d]main()/SendLoginProc() failed!\n", __FILE__, __LINE__);
			return -1;
		}
		break;
	default:
		printf("[%s: %d] 还没有这个测试用例，啥也不做!\n", __FILE__, __LINE__);
		break;
	}
}

void MsgCodeMenu()
{
	printf("\n--------------------------menu----------------------------");
	printf("\n发送心跳<0>；普通测试包<5/6>:1.注册包、2.登录包；退出代码<110>\n");
	printf("--------------------------menu----------------------------\n");
	printf(">>");
}