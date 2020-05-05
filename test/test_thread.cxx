
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "XiaoHG_C_Crc32.h"
#include "test_common.h"
#include "test_global.h"
#include "test_func.h"

/* defauled receive message length is 1024 */
#define MAX_RECVMSG_LENGTH 1024

void *RecvMsgThreadProc(void *pThreadData)
{
    printf("[%s: %d]RecvMsgThreadProc() 接收数据线程启动\n", __FILE__, __LINE__);

    int iErr = 0;
    int iRecvLen = MAX_RECVMSG_LENGTH;
    char *pRecvBuf = (char *)malloc(iRecvLen * sizeof(char));
    memset(pRecvBuf, 0, iRecvLen);

    while (true)
    {
        iErr = RecvData(pRecvBuf, iRecvLen);
        if(iErr == -1)
        {
            printf("[%s: %d]RecvMsgThreadProc()/recv_data() 数据接收失败, errno = %d\n", __FILE__, __LINE__, errno);
            break;
        }

        if (iErr == 0)
        {
            printf("[%s: %d]RecvMsgThreadProc()/recv_data() 服务器断开\n", __FILE__, __LINE__);
            break;
        }

        LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)pRecvBuf;
        unsigned short sMsgCode = htons(pPkgHeader->msgCode);

        switch (sMsgCode)
        {
        case HB_CODE:
            printf("[%s: %d]RecvMsgThreadProc() 收到一个心跳回复\n", __FILE__, __LINE__);
            break;
        case REGISTER_CODE:
            printf("[%s: %d]RecvMsgThreadProc() 收到一个注册回复\n", __FILE__, __LINE__);
            break;
        case LINGIN_CODE:
            printf("[%s: %d]RecvMsgThreadProc() 收到一个登录回复\n", __FILE__, __LINE__);
            break;
        default:
            printf("[%s: %d]RecvMsgThreadProc() 不应该收到这个消息\n", __FILE__, __LINE__);
            break;
        }
    }
    
    if(pRecvBuf != NULL)
        free(pRecvBuf);

    return (void *)0;
}

void *HeartBeatThreadProc(void *pThreadData)
{
    printf("[%s: %d]SendMsgThreadProc() 心跳检测线程启动\n", __FILE__, __LINE__);
    while(true)
    {
        usleep(20000 * 1000);
        if(SendHeartBeatMsg() == -1)
        {
            printf("[%s: %d]HeartbeatThreadProc()/SendHeartBeatMsg() failed, errno: %d!\n", __FILE__, __LINE__, errno);
            break;
        }
        printf("[%s: %d]HeartbeatThreadProc() 心跳消息发送完毕\n", __FILE__, __LINE__);
    }

    return (void*)0;
}

void *SendMsgThreadProc(void *pThreadData)
{
    printf("[%s: %d]SendMsgThreadProc() 发送数据线程启动\n", __FILE__, __LINE__);

    while(true)
    {
        usleep(random() % 2000 * 1000);
        if(SendLoginMsg() == -1)
        {
            printf("[%s: %d]SendMsgThreadProc()/SendLoginMsg() failed, errno: %d!\n", __FILE__, __LINE__, errno);
            break;
        }
       
        usleep(random() % 2000 * 1000);
        if(SendRegisterMsg() == -1)
        {
            printf("[%s: %d]SendMsgThreadProc()/SendRegisterMsg() failed, errno: %d!\n", __FILE__, __LINE__, errno);
            break;
        }
    }

    return (void*)0;
}

int ThreadInit()
{
    pthread_t ThreadFd[3] = { 0 };

    if(pthread_create(&ThreadFd[0], NULL, RecvMsgThreadProc, (void *)0) == -1)
    {
        printf("[%s: %d]ThreadInit()/pthread_create() 创建线程失败, errno = %d\n", __FILE__, __LINE__, errno);
        return -1;
    }
    
    if(pthread_create(&ThreadFd[1], NULL, HeartBeatThreadProc, (void *)0) == -1)
    {
        printf("[%s: %d]ThreadInit()/pthread_create() 创建线程失败, errno = %d\n", __FILE__, __LINE__, errno);
        return -1;
    }
    
    if(pthread_create(&ThreadFd[2], NULL, SendMsgThreadProc, (void *)0) == -1)
    {
        printf("[%s: %d]ThreadInit()/pthread_create() 创建线程失败, errno = %d\n", __FILE__, __LINE__, errno);
        return -1;
    }


    return 0;
}