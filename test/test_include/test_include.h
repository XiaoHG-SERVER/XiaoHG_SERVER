
#ifndef __TEST_INCLUDE_H__
#define __TEST_INCLUDE_H__

int socket_init(int &iSocketFd);
int choose_sendmsg(int iSocketFd, int iCode);
void menu();

int send_data(int iSocketFd, char *pSendBuf, int iBufLen);
int recv_data(int iSocketFd, char *pRecvBuf, int iRecvLen);

int send_pingmsg(int iSocketFd);
int send_login(int iSocketFd);
int send_register(int iSocketFd);

int thread_init(int iSocketFd);

#endif //__TEST_INCLUDE_H__