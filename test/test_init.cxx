
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

#include "test_common.h"
#include "test_include.h"

int socket_init(int &iSocketFd)
{
	//连接
	//(1)创建socket
	iSocketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (iSocketFd == -1)
	{
		printf("[%s: %d]socket()失败!\n", __FILE__, __LINE__);
		return -1;
	}
	//(2)连接服务器
	struct sockaddr_in server_in;
	memset(&server_in, 0, sizeof(sockaddr_in));
	server_in.sin_family = AF_INET;
	server_in.sin_port = htons(80);
	server_in.sin_addr.s_addr = inet_addr("192.168.13.132");

	if (connect(iSocketFd, (struct sockaddr *)&server_in, sizeof(sockaddr_in)) == -1)
	{
		printf("[%s: %d]connect()失败! errno = %d\n", __FILE__, __LINE__, errno);
		close(iSocketFd);
		return -1;
	}

	printf("\n[%s: %d]connect()成功! iSocketFd = %d\n", __FILE__, __LINE__, iSocketFd);
	return 0;
}