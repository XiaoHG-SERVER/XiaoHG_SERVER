
#ifndef __XiaoHG_LOGICCOMM_H__
#define __XiaoHG_LOGICCOMM_H__

/* Macro definition of send and receive commands */
#define CMD_START				0  
#define CMD_HEARTBEAT			CMD_START + 0   /* Heartbeat packet */
#define CMD_REGISTER			CMD_START + 5   /* Register packet */
#define CMD_LOGIN				CMD_START + 6   /* Login packet */

/* Alignment, 1 byte alignment [Members of the structure do not 
 * do any byte alignment: arranged closely together] */
#pragma pack (1) 

typedef struct register_s
{
	int iType;				/* type */
	char UserName[56];		/* user name */ 
	char Password[40];		/* password */

}REGISTER_T, *LPREGISTER_T;

typedef struct login_s
{
	char UserName[56];		/* user name */
	char Password[40];		/* password */

}LOGIN_T, *LPLOGIN_T;

/* Cancel the specified alignment and restore the default alignment */
#pragma pack()

#endif //!__XiaoHG_LOGICCOMM_H__
