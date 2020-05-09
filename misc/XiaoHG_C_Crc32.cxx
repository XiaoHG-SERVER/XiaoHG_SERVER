
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "XiaoHG_C_Crc32.h"
#include "XiaoHG_Func.h"
#include "XiaoHG_Macro.h"

#define __THIS_FILE__ "XiaoHG_C_Crc32.cxx"

CCRC32 *CCRC32::m_Instance = NULL;

CCRC32::CCRC32()
{
	InitCRC32Table();
}
CCRC32::~CCRC32(){}

unsigned int CCRC32::Reflect(unsigned int uiRef, char ch)
{
    unsigned int uiValue(0);
	for(int i = 1; i < (ch + 1); i++)
	{
		if(uiRef & 1)
			uiValue |= 1 << (ch - i);
		uiRef >>= 1;
	}
	return uiValue;
}

void CCRC32::InitCRC32Table()
{
    unsigned int ulPolynomial = 0x04c11db7;
	for(int i = 0; i <= 0xFF; i++)
	{
		CRC32Table[i] = Reflect(i, 8) << 24;
		for (int j = 0; j < 8; j++)
        {
			CRC32Table[i] = (CRC32Table[i] << 1) ^ (CRC32Table[i] & (1 << 31) ? ulPolynomial : 0);
        }
		CRC32Table[i] = Reflect(CRC32Table[i], 32);
	}

}

int CCRC32::Get_CRC(unsigned char* ucBuffer, unsigned int uiDWSize)
{
    unsigned int CRC(0xffffffff);
	int iLen = uiDWSize;
	while(iLen--)
	{
		CRC = (CRC >> 8) ^ CRC32Table[(CRC & 0xFF) ^ *ucBuffer++];
	}
	return CRC^0xffffffff;
}

