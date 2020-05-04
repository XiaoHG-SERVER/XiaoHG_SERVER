
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>    //uintptr_t
#include <stdarg.h>    //va_start....
#include <unistd.h>    //STDERR_FILENO等
#include <sys/time.h>  //gettimeofday
#include <time.h>      //localtime_r
#include <fcntl.h>     //open
#include <errno.h>     //errno
#include <sys/socket.h>
#include <sys/ioctl.h> //ioctl
#include <arpa/inet.h>
#include "XiaoHG_C_Crc32.h"
#include "test_common.h"
#include "test_global.h"
#include "test_include.h"

int g_iLenPkgHeader = sizeof(COMM_PKG_HEADER);

int send_pingmsg(int iSocketFd)
{
	CCRC32 *pCrc32 = CCRC32::GetInstance();
	char *pSendBuf = (char *)malloc(g_iLenPkgHeader);

	LPCOMM_PKG_HEADER pInfoHead;
	pInfoHead = (LPCOMM_PKG_HEADER)pSendBuf;
	pInfoHead->crc32 = 0;
	pInfoHead->crc32 = htons(pInfoHead->crc32);
	pInfoHead->msgCode = 0;
	pInfoHead->msgCode = htons(pInfoHead->msgCode);
	pInfoHead->pkgLen = htons(g_iLenPkgHeader);

	if (send_data(iSocketFd, pSendBuf, g_iLenPkgHeader) == -1)
	{
		printf("[%s: %d]send_pingmsg()/send_data()失败 errno: %d!\n", __FILE__, __LINE__, errno);
		close(iSocketFd);
		free(pSendBuf);
		return -1;
	}

	printf("[%s: %d]send_pingmsg()心跳消息发送完毕!\n", __FILE__, __LINE__);
	free(pSendBuf);
	return 0;
}

int send_register(int iSocketFd)
{
	CCRC32 *pCrc32 = CCRC32::GetInstance();
	char *pSendBuf = (char *)malloc(g_iLenPkgHeader + sizeof(STRUCT_REGISTER));

	LPCOMM_PKG_HEADER pInfoHead;
	pInfoHead = (LPCOMM_PKG_HEADER)pSendBuf;
	pInfoHead->msgCode = 5;
	pInfoHead->msgCode = htons(pInfoHead->msgCode);
	pInfoHead->pkgLen = htons(g_iLenPkgHeader + sizeof(STRUCT_REGISTER));

	LPSTRUCT_REGISTER pstSendRegister = (LPSTRUCT_REGISTER)(pSendBuf + g_iLenPkgHeader);
	pstSendRegister->iType = htonl(100);
	strcpy(pstSendRegister->username, "XiaoHG");
	strcpy(pstSendRegister->password, "xiaohaige");

	pInfoHead->crc32 = pCrc32->Get_CRC((unsigned char*)pstSendRegister, sizeof(STRUCT_REGISTER));
	pInfoHead->crc32 = htonl(pInfoHead->crc32);	//计算一个CRC32值给服务器做校验

	if (send_data(iSocketFd, pSendBuf, g_iLenPkgHeader + sizeof(STRUCT_REGISTER)) == -1)
	{
		printf("[%s: %d]send_pingmsg()/send_data()失败\n", __FILE__, __LINE__);
		close(iSocketFd);
		free(pSendBuf);
		return -1;
	}

	printf("[%s: %d]注册消息发送完毕!\n", __FILE__, __LINE__);
	free(pSendBuf);
	return 0;
}

int send_login(int iSocketFd)
{
	CCRC32 *pCrc32 = CCRC32::GetInstance();
	char *pSendBuf = (char *)malloc(g_iLenPkgHeader + sizeof(STRUCT_LOGIN));

	LPCOMM_PKG_HEADER pInfoHead;
	pInfoHead = (LPCOMM_PKG_HEADER)pSendBuf;
	pInfoHead->msgCode = 6;  
	pInfoHead->msgCode = htons(pInfoHead->msgCode);
	pInfoHead->pkgLen = htons(g_iLenPkgHeader + sizeof(STRUCT_LOGIN));

	LPSTRUCT_LOGIN pstSendLogin = (LPSTRUCT_LOGIN)(pSendBuf + g_iLenPkgHeader);
	strcpy(pstSendLogin->username, "XiaoHG");
	strcpy(pstSendLogin->password, "xiaohaige");
	pInfoHead->crc32 = pCrc32->Get_CRC((unsigned char*)pstSendLogin, sizeof(STRUCT_LOGIN));
	pInfoHead->crc32 = htonl(pInfoHead->crc32);	//计算一个CRC32值给服务器做校验
	
	if (send_data(iSocketFd, pSendBuf, g_iLenPkgHeader + sizeof(STRUCT_LOGIN)) == -1)
	{
		printf("[%s: %d]send_pingmsg()/send_data()失败\n", __FILE__, __LINE__);
		close(iSocketFd);
		free(pSendBuf);
		return -1;
	}
	printf("[%s: %d]登录消息发送完毕\n", __FILE__, __LINE__);
	free(pSendBuf);
	return 0;
}
