
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

public:	
	static CCRC32* GetInstance() 
	{
		if(m_Instance == NULL)
		{
			/* lock */
			if(m_Instance == NULL)
			{				
				m_Instance = new CCRC32();
				static CGarhuishou cl; 
			}
			/* unlock */
		}
		return m_Instance;
	}	
	class CGarhuishou 
	{
	public:				
		~CGarhuishou()
		{
			if (CCRC32::m_Instance)
			{						
				delete CCRC32::m_Instance;
				CCRC32::m_Instance = NULL;				
			}
		}
	};
	
public:

	void Init_CRC32_Table();
	/* unsigned long Reflect(unsigned long ref, char ch) */	/*  Reflects CRC bits in the lookup table */
    unsigned int Reflect(unsigned int ref, char ch);		/*  Reflects CRC bits in the lookup table */
    
	/* int Get_CRC(unsigned char* buffer, unsigned long dwSize) */
    int Get_CRC(unsigned char* buffer, unsigned int dwSize);
    
public:
	/* unsigned long crc32_table[256] */	/*  Lookup table arrays */
    unsigned int crc32_table[256];			/*  Lookup table arrays */
};

#endif //!__XiaoHG_C_CRC32_H__


