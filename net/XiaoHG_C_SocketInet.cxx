
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> 
#include <stdarg.h> 
#include <unistd.h> 
#include <sys/time.h> 
#include <time.h> 
#include <fcntl.h> 
#include <errno.h> 
#include <sys/ioctl.h> 
#include <arpa/inet.h>
#include "XiaoHG_C_Conf.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_Global.h"
#include "XiaoHG_Func.h"
#include "XiaoHG_C_Socket.h"

#define __THIS_FILE__ "XiaoHG_C_SocketInet.cxx"

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23
 * function name: ConnectInfo
 * discription: Convert the socket-bound address to text format 
 *              [According to the information given in parameter 1, 
 *              get the address port string and return the length of this string].
 * parameter:
 * =================================================================*/
size_t CSocket::ConnectionInfo(struct sockaddr *pSockAddr, int iPort, u_char *pText, size_t uiLen)
{
    /* function track */
    XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "CSocket::ConnectionInfo track");

    u_char *p = NULL;
    struct sockaddr_in *sin = NULL;

    switch (pSockAddr->sa_family)
    {
    case AF_INET:
        sin = (struct sockaddr_in *) pSockAddr;
        p = (u_char *) &sin->sin_addr;
        if(iPort)  /* Port information is also combined into a string */
        {
            /* The new writable address is returned */
            XiaoHG_Log(LOG_FILE, LOG_LEVEL_INFO, 0, "%ud.%ud.%ud.%ud:%d", p[0], p[1], p[2], p[3], ntohs(sin->sin_port));
        }
        else /* No need to combine port information into a string */
        {
            XiaoHG_Log(LOG_FILE, LOG_LEVEL_INFO, 0, "%ud.%ud.%ud.%ud", p[0], p[1], p[2], p[3]);  
        }
        return (p - pText);
        break;
    default:
        return 0;
        break;
    }
    return 0;
}
