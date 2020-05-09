
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

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

/* Singleton implementation */
public:	
	static CMemory* GetInstance()
	{
		if(m_Instance == NULL)
		{
			/* lock */
			if(m_Instance == NULL)
			{
				m_Instance = new CMemory(); 
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
			if (CMemory::m_Instance)
			{
				delete CMemory::m_Instance; 
				CMemory::m_Instance = NULL;
			}
		}
	};

public:
	void* AllocMemory(int iMemCount, bool bIsMemSet);
	void FreeMemory(void *p);
};

#endif //!__XiaoHG_MEMORY_H__
