
/*
 * Copyright (C/C++) XiaoHG
 * Copyright (C/C++) XiaoHG_SERVER
 */

#include <stdio.h>
#include <unistd.h>
#include "test_global.h"
#include "test_func.h"

/* global socket fd */
int g_iSocketFd;

int main(int argc, char *argv[])
{
	for (int i = 0; i < 10; i++)
	{
		//fork();
	}

    if(SocketInit() == -1)
	{
		printf("[%s: %d]main()/SocketInit() failed!\n", __FILE__, __LINE__);
		return 0;
	}

	if(ThreadInit() == -1)
	{
		printf("[%s: %d]main()/ThreadInit() failed!\n", __FILE__, __LINE__);
		return 0;
	}

	while (true)
	{
		if(ChooseSendMsg() == -1)
		{
			printf("[%s: %d]main()/ChooseSendMsg() failed!\n", __FILE__, __LINE__);
			close(g_iSocketFd);
			return -1;
		}
	}

	close(g_iSocketFd);
	return 0;
}

