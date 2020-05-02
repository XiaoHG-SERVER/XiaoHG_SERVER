
#ifndef __NGX_COMM_H__
#define __NGX_COMM_H__

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
#define INIT_MSGHEADER_DATA_BUFSIZE 20

/* Alignment, 1 byte alignment [Members of the structure
 * do not do any byte alignment: arranged closely together] */
#pragma pack (1)
/* Packet header */
typedef struct comm_msg_header_s
{
	/* The total length of the packet [packet header + body]-2 bytes, 
	 * the maximum number that 2 bytes can represent is more than 60,000, 
	 * we define PKG_MAX_LENGTH 30000, so use sPkgLength to save it enough */
	/*The length of the entire packet [header + body] is recorded in the header */
	unsigned short sPkgLength;
	unsigned short sMsgCode;		/* Message type code-2 bytes, used to distinguish each different command [different message] */
	int iCrc32;						/* CRC32 check */	
}COMM_PKG_HEADER, *LPCOMM_PKG_HEADER;

/* Cancel the specified alignment and restore the default alignment */
#pragma pack() 

#endif //!__NGX_COMM_H__
