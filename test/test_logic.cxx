
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include "XiaoHG_C_Crc32.h"
#include "test_common.h"
#include "test_global.h"
#include "test_func.h"

int g_iLenPkgHeader = sizeof(COMM_PKG_HEADER);

/* 心跳测试数据发送 */
int SendHeartBeatMsg()
{
	CCRC32 *pCrc32 = CCRC32::GetInstance();
	char *pSendBuf = (char *)malloc(g_iLenPkgHeader);

	LPCOMM_PKG_HEADER pInfoHead;
	pInfoHead = (LPCOMM_PKG_HEADER)pSendBuf;
	pInfoHead->iCrc32 = 0;
	pInfoHead->iCrc32 = htons(pInfoHead->iCrc32);
	pInfoHead->usMsgCode = HB_CODE;
	pInfoHead->usMsgCode = htons(pInfoHead->usMsgCode);
	pInfoHead->uPkgLen = htons(g_iLenPkgHeader);

	if (SendData(pSendBuf, g_iLenPkgHeader) == -1)
	{
		printf("[%s: %d]send_pingmsg()/send_data() failed, errno: %d!\n", __FILE__, __LINE__, errno);
		close(g_iSocketFd);
		free(pSendBuf);
		return -1;
	}

	printf("[%s: %d]send_pingmsg() 心跳消息发送完毕!\n", __FILE__, __LINE__);
	free(pSendBuf);
	return 0;
}

/* 普通测试数据发送 */
int SendRegisterMsg()
{
	CCRC32 *pCrc32 = CCRC32::GetInstance();
	char *pSendBuf = (char *)malloc(g_iLenPkgHeader + sizeof(STRUCT_REGISTER));

	LPCOMM_PKG_HEADER pInfoHead;
	pInfoHead = (LPCOMM_PKG_HEADER)pSendBuf;
	pInfoHead->usMsgCode = REGISTER_CODE;
	pInfoHead->usMsgCode = htons(pInfoHead->usMsgCode);
	pInfoHead->uPkgLen = htons(g_iLenPkgHeader + sizeof(STRUCT_REGISTER));

	LPSTRUCT_REGISTER pstSendRegister = (LPSTRUCT_REGISTER)(pSendBuf + g_iLenPkgHeader);
	pstSendRegister->iType = htonl(100);
	strcpy(pstSendRegister->UserName, "XiaoHG_Register");
	strcpy(pstSendRegister->Password, "xiaohaige");
	pInfoHead->iCrc32 = pCrc32->Get_CRC((unsigned char*)pstSendRegister, sizeof(STRUCT_REGISTER));
	pInfoHead->iCrc32 = htonl(pInfoHead->iCrc32);
	
    printf("[%s: %d]thread_send_func()发送注册数据：g_iSocketFd = %d, pid = %d\n", __FILE__, __LINE__, g_iSocketFd, getpid());
	if (SendData(pSendBuf, g_iLenPkgHeader + sizeof(STRUCT_REGISTER)) == -1)
	{
		printf("[%s: %d]send_pingmsg()/SendData() failed!\n", __FILE__, __LINE__);
		close(g_iSocketFd);
		free(pSendBuf);
		return -1;
	}

	printf("[%s: %d] 注册消息发送完毕!\n", __FILE__, __LINE__);
	free(pSendBuf);
	return 0;
}

/* 普通测试数据发送 */
int SendLoginMsg()
{
	CCRC32 *pCrc32 = CCRC32::GetInstance();
	char *pSendBuf = (char *)malloc(g_iLenPkgHeader + sizeof(STRUCT_LOGIN));

	LPCOMM_PKG_HEADER pInfoHead;
	pInfoHead = (LPCOMM_PKG_HEADER)pSendBuf;
	pInfoHead->usMsgCode = LINGIN_CODE;  
	pInfoHead->usMsgCode = htons(pInfoHead->usMsgCode);
	pInfoHead->uPkgLen = htons(g_iLenPkgHeader + sizeof(STRUCT_LOGIN));

	LPSTRUCT_LOGIN pstSendLogin = (LPSTRUCT_LOGIN)(pSendBuf + g_iLenPkgHeader);
	strcpy(pstSendLogin->UserName, "XiaoHG_Login");
	strcpy(pstSendLogin->Password, "xiaohaige");
	pInfoHead->iCrc32 = pCrc32->Get_CRC((unsigned char*)pstSendLogin, sizeof(STRUCT_LOGIN));
	pInfoHead->iCrc32 = htonl(pInfoHead->iCrc32);
	
    printf("[%s: %d]thread_send_func() 发送登录数据：g_iSocketFd = %d, pid = %d\n", __FILE__, __LINE__, g_iSocketFd, getpid());
	if (SendData(pSendBuf, g_iLenPkgHeader + sizeof(STRUCT_LOGIN)) == -1)
	{
		printf("[%s: %d]send_pingmsg()/send_data() failed!\n", __FILE__, __LINE__);
		close(g_iSocketFd);
		free(pSendBuf);
		return -1;
	}
	printf("[%s: %d] 登录消息发送完毕\n", __FILE__, __LINE__);
	free(pSendBuf);
	return 0;
}
