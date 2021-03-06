﻿
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_COMM_H__
#define __XiaoHG_COMM_H__

/* The maximum length of each packet [packet header + body] does not exceed this number. 
 * In order to leave some space, the actual encoding is that the length of the packet 
 * header + body must not be greater than this value -1000 [29000] */
#define PKG_MAX_LENGTH 30000

/* Communication receiving status definition, 
 * receiving status machine */
#define PKG_HD_INIT		0	/* Initial state, ready to receive the packet header, which is the state when the initial connection is initialized */
#define PKG_HD_RECVING 	1	/* Received packet header, packet header is incomplete, continue to receive */
#define PKG_BD_INIT		2	/* Baotou just finished receiving, ready to receive the body */
#define PKG_BD_RECVING 	3	/* In the receiving package, the package is incomplete, continue receiving, \
								and directly return to the PKG_HD_INIT state after processing */

/* Because I want to receive the packet header first, I want to define a fixed-size array specifically for receiving the packet header. 
 * The size of this number must be> sizeof (COMM_PKG_HEADER), so I define it as 20 here is more than enough. 
 * If the size of COMM_PKG_HEADER changes in the future, this number must also be Adjust to meet the requirements of> sizeof (COMM_PKG_HEADER) */
#define MSG_HEADER_LEN 20

/* Macro definition of send and receive commands */
#define CMD_START				0  
#define CMD_HEARTBEAT			CMD_START + 0   /* Heartbeat packet */
#define CMD_REGISTER			CMD_START + 5   /* Register packet */
#define CMD_LOGIN				CMD_START + 6   /* Login packet */

/* Alignment, 1 byte alignment [Members of the structure
 * do not do any byte alignment: arranged closely together] */
#pragma pack (1)
/* Packet header */
typedef struct comm_msg_header_s
{
	/* The total length of the packet [packet header + body]-2 bytes, 
	 * the maximum number that 2 bytes can represent is more than 60,000, 
	 * we define PKG_MAX_LENGTH 30000, so use PkgLength to save it enough */
	/*The length of the entire packet [header + body] is recorded in the header */
	uint16_t PkgLength;
	uint16_t MsgCode;		/* Message type code-2 bytes, used to distinguish each different command [different message] */
	int iCrc32;						/* CRC32 check */	
}COMM_PKG_HEADER, *LPCOMM_PKG_HEADER;

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

/* Cancel the specified alignment and restore the default alignment */
#pragma pack()

#endif //!__XiaoHG_COMM_H__
