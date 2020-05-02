
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>  //usleep
#include <arpa/inet.h>
#include <string.h>

#include "ngx_c_crc32.h"
#include "test_common.h"
#include "test_global.h"
#include "test_include.h"

int g_iSocketFd = 0;

void *thread_recvmsg_func(void *pThreadData)
{
    printf("[%s: %d]thread_recv_func()接收数据线程启动！\n", __FILE__, __LINE__);

    int err = 0;
    int iSocketFd = *((int *)pThreadData);

    int iRecvLen = 1024;
    char *pRecvBuf = (char *)malloc(iRecvLen * sizeof(char));
    memset(pRecvBuf, 0, iRecvLen);

    while (true)
    {
        err = recv_data(g_iSocketFd, pRecvBuf, iRecvLen);
        if(err == -1)
        {
            printf("[%s: %d]thread_recvmsg_func()/recv_data()数据接收失败！errno = %d\n", __FILE__, __LINE__, errno);
            break;
        }

        if (err == 0)
        {
            printf("[%s: %d]thread_recvmsg_func()/recv_data() 服务器断开！\n", __FILE__, __LINE__);
            break;
        }

        LPCOMM_PKG_HEADER pPkgHeader = (LPCOMM_PKG_HEADER)pRecvBuf;
        if (pPkgHeader->msgCode == 0)   
        {
            printf("[%s: %d]thread_recvmsg_func()收到一个心跳回复！\n", __FILE__, __LINE__);
        }
        else
        {
            printf("[%s: %d]thread_recvmsg_func()收到数据回复！\n", __FILE__, __LINE__);
        }
    }
    
    if(pRecvBuf != NULL)
        free(pRecvBuf);

    return (void *)0;
}

void *thread_ping_func(void *pThreadData)
{

    int iSocketFd = *((int *)pThreadData);
    CCRC32 *pCrc32 = CCRC32::GetInstance();
	char *pSendBuf = (char *)malloc(g_iLenPkgHeader);

	LPCOMM_PKG_HEADER pInfoHead;
	pInfoHead = (LPCOMM_PKG_HEADER)pSendBuf;
    pInfoHead->crc32 = 0;
	pInfoHead->crc32 = htons(pInfoHead->crc32);
	pInfoHead->msgCode = 0;
	pInfoHead->msgCode = htons(pInfoHead->msgCode);
	pInfoHead->pkgLen = htons(g_iLenPkgHeader);

    while(true)
    {
        usleep(20000 * 1000);
        printf("[%s: %d]thread_send_func()发送心跳数据 20s发一个心跳！ iSocketFd = %d, pid = %d\n", __FILE__, __LINE__, g_iSocketFd, getpid());
        if(send_pingmsg(g_iSocketFd) == -1)
        {
            printf("[%s: %d]thread_send_func()/send_pingmsg()失败 errno: %d!\n", __FILE__, __LINE__, errno);
            break;
        }
        printf("[%s: %d]thread_ping_func()心跳消息发送完毕!\n", __FILE__, __LINE__);
    }

    return (void*)0;
}

void *thread_send_func(void *pThreadData)
{
    printf("[%s: %d]thread_send_func()发送数据线程启动！\n", __FILE__, __LINE__);
    int i = 1;
    while(true)
    {
        
        usleep(random() % 2000 * 1000);
        printf("[%s: %d]thread_send_func()发送send_login数据！ iSocketFd = %d, pid = %d\n", __FILE__, __LINE__, g_iSocketFd, getpid());
        if(send_login(g_iSocketFd) == -1)
        {
            printf("[%s: %d]thread_send_func()/send_login()失败 errno: %d!\n", __FILE__, __LINE__, errno);
            break;
        }
       
        usleep(random() % 2000 * 1000);
        printf("[%s: %d]thread_send_func()发送send_register数据！ iSocketFd = %d, pid = %d\n", __FILE__, __LINE__, g_iSocketFd, getpid());
        if(send_register(g_iSocketFd) == -1)
        {
            printf("[%s: %d]thread_send_func()/send_login()失败 errno: %d!\n", __FILE__, __LINE__, errno);
            break;
        }
        
    }

    return (void*)0;
}

int thread_init(int iSocketFd)
{
    int err = 0;
    pthread_t ThreadFd = 0;
    g_iSocketFd = iSocketFd;

    err = pthread_create(&ThreadFd, NULL, thread_recvmsg_func, (void *)&iSocketFd);
    if(err == -1)
    {
        printf("[%s: %d]create_thread()/pthread_create()创建线程失败！\n", __FILE__, __LINE__);
        return -1;
    }
    
    err = pthread_create(&ThreadFd, NULL, thread_ping_func, (void *)&iSocketFd);
    if(err == -1)
    {
        printf("[%s: %d]create_thread()/pthread_create()创建线程失败！\n", __FILE__, __LINE__);
        return -1;
    }
    

    err = pthread_create(&ThreadFd, NULL, thread_send_func, (void *)&iSocketFd);
    if(err == -1)
    {
        printf("[%s: %d]create_thread()/pthread_create()创建线程失败！\n", __FILE__, __LINE__);
        return -1;
    }


    return 0;
}