
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "test_common.h"
#include "test_func.h"
#include "test_global.h"

int SocketInit()
{
	g_iSocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (g_iSocketFd == -1)
	{
		printf("[%s: %d]socket() failed!\n", __FILE__, __LINE__);
		return -1;
	}

	struct sockaddr_in server_in;
	memset(&server_in, 0, sizeof(sockaddr_in));
	server_in.sin_family = AF_INET;
	server_in.sin_port = htons(80);
	server_in.sin_addr.s_addr = inet_addr("192.168.13.132");

	if (connect(g_iSocketFd, (struct sockaddr *)&server_in, sizeof(sockaddr_in)) == -1)
	{
		printf("[%s: %d]connect() failed! errno = %d\n", __FILE__, __LINE__, errno);
		close(g_iSocketFd);
		return -1;
	}

	printf("\n[%s: %d]connect() successful! g_iSocketFd = %d\n", __FILE__, __LINE__, g_iSocketFd);
	return 0;
}