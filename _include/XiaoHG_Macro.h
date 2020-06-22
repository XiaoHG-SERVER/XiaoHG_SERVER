
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_MACRO_H__
#define __XiaoHG_MACRO_H__

#define __AUTHER__  "XiaoHG"
#define __XiaoHG_VERSION__ "V1.1.1.20200509"

/* The maximum array length of the error message displayed */
#define MAX_ERRORSTR_LENGTH   2048

#define XiaoHG_CPYMEM(dst, src, n) (((u_char *) memcpy(dst, src, n)) + (n)) 

/* Mark current process type */
#define MASTER_PROCESS  0  /* master process */
#define WORKER_PROCESS  1  /* worker process */

/* about message */
#define MAX_SENDQUEUE_COUNT 50000
#define MAX_EACHCONN_SENDCOUNT 400

/* check connect time (ms) */
#define CHECK_HBLIST_TIMER 500
#define CHECK_RECYCONN_TIMER 1000

/* Specify the log printing method */
enum{
    LOG_STD = 0,    /* write log to stderr*/
    LOG_FILE,       /* write log to Logfile*/
    LOG_ALL         /* borth stderr and Logfile*/
};

/* about epoll event */
enum{
    EPOLL_ADD = 0,  /* add event */
    EPOLL_DEL,      /* delete event*/
    EPOLL_OVER      /* Coverage event */
};

#endif //!__XiaoHG_MACRO_H__
