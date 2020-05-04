
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>    //STDERR_FILENO等

#include "test_global.h"
#include "test_include.h"
#include "test_common.h"

//-----------调式代码----------------

void witch_msg(char *pSendBuf, int iBufLen)
{
	LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)pSendBuf;
	printf("[%s: %d]send_data()即将要发送测试数据给服务端：pPkgHeader->msgCode = %d, pPkgHeader->pkgLen = %d\n",
												__FILE__, __LINE__, ntohs(pPkgHeader->msgCode), ntohs(pPkgHeader->pkgLen));
}
	
//-----------调式代码----------------

//发送数据，值得讲解
int send_data(int iSocketFd, char *pSendBuf, int iBufLen)
{
	int iUsend = iBufLen; //要发送的数目
	int iUwrote = 0;      //已发送的数目
	int iTmpSret;

	if(pSendBuf == NULL)
	{
		printf("[%s: %d]send_data() pSendBuf == NULL\n", __FILE__, __LINE__);
		return -1;
	}

	//-----------调式代码----------------
	witch_msg(pSendBuf, iBufLen);
	//-----------调式代码----------------

	while (iUwrote < iUsend)
	{
		iTmpSret = send(iSocketFd, pSendBuf + iUwrote, iUsend - iUwrote, 0);
		if ((iTmpSret == -1) || (iTmpSret == 0))
		{
			printf("[%s: %d]send_data()/send()失败! iTmpSret = %d, errno = %d, iSocketFd = %d\n", __FILE__, __LINE__, iTmpSret, errno, iSocketFd);
			return -1;
		}
		iUwrote += iTmpSret;
	}//end while

	return iUwrote;
}

int recv_data(int iSocketFd, char *pRecvBuf, int iRecvLen)
{
	int n = 0;
	n = recv(iSocketFd, pRecvBuf, iRecvLen, 0);
	if(n == -1)
	{
		printf("[%s: %d]recv_data()/recv()失败! errno = %d\n", __FILE__, __LINE__, errno);
		close(iSocketFd);
		return -1;
	}
	return n;
}

int choose_sendmsg(int iSocketFd, int iCode)
{
	menu();
	scanf("%d", &iCode);
	switch (iCode)
	{
	case 0:
		if(send_pingmsg(iSocketFd) == -1)
		{
			printf("[%s: %d]main()/send_pingmsg()失败!\n", __FILE__, __LINE__);
			return -1;
		}
		break;
	case 5:
		if(send_register(iSocketFd) == -1)
		{
			printf("[%s: %d]main()/send_register()失败!\n", __FILE__, __LINE__);
			return -1;
		}
		break;
	case 6:
		if(send_login(iSocketFd) == -1)
		{
			printf("[%s: %d]main()/send_login()失败!\n", __FILE__, __LINE__);
			return -1;
		}
		break;
	case 110:
		return -1;
	default:
		printf("[%s: %d]还没有这个测试用例，啥也不做!\n", __FILE__, __LINE__);
		break;
	}
}

void menu()
{
	printf("\n--------------------------menu----------------------------");
	printf("\n发送心跳<0>；普通测试包<5/6>:1.注册包、2.登录包；退出代码<110>\n");
	printf("--------------------------menu----------------------------\n");
	printf(">>");
}