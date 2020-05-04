
#include <stdio.h>
#include <stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include <sys/socket.h>
#include <unistd.h>  //usleep
#include "test_global.h"
#include "test_include.h"

int main(int argc, char *const *argv)
{
	for (int i = 0; i < 1; i++)
	{
		fork();
	}
	int iSocketFd = 0;
	int iCode = 0;

    if(socket_init(iSocketFd) == -1)
	{
		printf("[%s: %d]main()/socket_init()失败!\n", __FILE__, __LINE__);
		return 0;
	}

	if(thread_init(iSocketFd) == -1)
	{
		printf("[%s: %d]main()/thread_init()失败!\n", __FILE__, __LINE__);
		return 0;
	}

	while (true)
	{
		if(choose_sendmsg(iSocketFd, iCode) == -1)
		{
			printf("[%s: %d]main()/choose_sendmsg()失败!\n", __FILE__, __LINE__);
			close(iSocketFd);
			return -1;
		}
	}

	close(iSocketFd);
	return 0;
}

