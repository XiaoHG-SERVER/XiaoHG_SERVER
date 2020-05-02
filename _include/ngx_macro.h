
#ifndef __NGX_MACRO_H__
#define __NGX_MACRO_H__

#define XiaoHG_SUCCESS 0
#define XiaoHG_ERROR   -1

/* The maximum array length of the error message displayed */
#define NGX_MAX_ERROR_STR   2048

#define NGX_CPYMEM(dst, src, n) (((u_char *) memcpy(dst, src, n)) + (n)) 

/* We divided the log into eight levels [level from high to low, the level 
 * with the smallest number is the highest, 
 * the level with the largest number is the lowest] */
#define LOG_LEVEL_STDERR      0    /* Console error [stderr]: highest level log */
#define LOG_LEVEL_EMERG       1    /* Urgent [emerg] */
#define LOG_LEVEL_ALERT       2    /* Alert [alert] */
#define NGX_LOG_CRIT        3    /* Serious [crit] */
#define LOG_LEVEL_ERR         4    /* Error [error]: belongs to the common level */
#define NGX_LOG_WARN        5    /* Warning [warn]: belongs to the common level */
#define LOG_LEVEL_NOTICE      6    /* Note [notice] */
#define LOG_LEVEL_INFO        7    /* Information [info] */
#define NGX_LOG_DEBUG       8    /* Debug [debug]: the lowest level */

/* Define the default log storage path and file name */
#define DEFAULT_LOG_PATH  "logs/NGX_error.log"

/* Mark current process type */
#define MASTER_PROCESS  0  /* master process */
#define WORKER_PROCESS  1  /* worker process */

/* about message */
#define MAX_SENDQUEUE_COUNT 50000
#define MAX_EACHCONN_SENDCOUNT 400

/* check connect time (ms) */
#define CHECK_HBLIST_TIME 500
#define CHECK_RECYCONN_TIME 1000

/* Specify the log printing method */
enum{
    LOG_STD = 0,    /* write log to stderr*/
    LOG_FILE,       /* write log to Logfile*/
    LOG_ALL         /* borth stderr and Logfile*/
};

/* about epoll event */
enum
{
    EPOLL_ADD = 0,  /* add event */
    EPOLL_DEL,      /* delete event*/
    EPOLL_OVER      /* Coverage event */
};

#endif //!__NGX_MACRO_H__
