
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_C_CRC32_H__
#define __XiaoHG_C_CRC32_H__

#include <stddef.h>

class CCRC32
{
private:
	CCRC32();
public:
	~CCRC32();
private:
	static CCRC32 *m_Instance;

/* Singleton implementation */
public:	
	static CCRC32* GetInstance() 
	{
		if(m_Instance == NULL)
		{
			/* lock */
			if(m_Instance == NULL)
			{				
				m_Instance = new CCRC32();
				static CDeleteInstance cl; 
			}
			/* unlock */
		}
		return m_Instance;
	}	
	class CDeleteInstance 
	{
	public:				
		~CDeleteInstance()
		{
			if (CCRC32::m_Instance)
			{						
				delete CCRC32::m_Instance;
				CCRC32::m_Instance = NULL;				
			}
		}
	};
	
public:
	void InitCRC32Table();
    unsigned int Reflect(unsigned int uiRef, char ch);
    int Get_CRC(unsigned char* ucBuffer, unsigned int uiDWSize);
    
public:
    unsigned int CRC32Table[256];
};

#endif //!__XiaoHG_C_CRC32_H__


