
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__

enum{
	HB_CODE = 0,
	REGISTER_CODE = 5,
	LINGIN_CODE = 6
	/* ... */
};

#pragma pack (1)

typedef struct _COMM_PKG_HEADER
{
	unsigned short usPkgLen;
	unsigned short usMsgCode;
	int iCrc32;
}COMM_PKG_HEADER, *LPCOMM_PKG_HEADER;

typedef struct _STRUCT_REGISTER
{
	int iType;
	char UserName[56];
	char Password[40];
}STRUCT_REGISTER, *LPSTRUCT_REGISTER;

typedef struct _STRUCT_LOGIN
{
	char UserName[56]; 
	char Password[40];
}STRUCT_LOGIN, *LPSTRUCT_LOGIN;

#pragma pack()

#endif //__TEST_COMMON_H__