
#include <stdio.h>
#include <arpa/inet.h>

void main()
{
	int a = 0;
	for(int i = 0; i < 100; i++)
	{
		printf("ntohl(0) = %d\n", ntohl(a));
		printf("htonl(0) = %d\n", htonl(a));
	}
}
