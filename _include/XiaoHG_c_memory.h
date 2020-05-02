
#ifndef __XiaoHG_MEMORY_H__
#define __XiaoHG_MEMORY_H__

#include <stddef.h>

/* Memory application release management */
class CMemory 
{
private:
	CMemory() {}	

public:
	~CMemory(){};

private:
	static CMemory *m_Instance;

public:	
	static CMemory* GetInstance()
	{
		if(m_Instance == NULL)
		{
			/* lock */
			if(m_Instance == NULL)
			{
				m_Instance = new CMemory(); 
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
			if (CMemory::m_Instance)
			{
				delete CMemory::m_Instance; 
				CMemory::m_Instance = NULL;
			}
		}
	};

public:
	void *AllocMemory(int memCount,bool ifmemset);
	void FreeMemory(void *point);
};

#endif //!__XiaoHG_MEMORY_H__
