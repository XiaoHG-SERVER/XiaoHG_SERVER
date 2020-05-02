﻿
#ifndef __NGX_CONF_H__
#define __NGX_CONF_H__

#include <vector>
#include "ngx_global.h"

class CConfig
{
private:
	CConfig();
public:
	~CConfig();
	
private:
	static CConfig *m_Instance;

public:	
	static CConfig* GetInstance() 
	{	
		if(m_Instance == NULL)
		{
			/* lock */
			if(m_Instance == NULL)
			{					
				m_Instance = new CConfig();
				static CGarhuishou cTmp; 
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
			if (CConfig::m_Instance)
			{						
				delete CConfig::m_Instance;
				CConfig::m_Instance = NULL;
			}
		}
	};
	
public:
    int Load(const char *pConfName);
	const char *GetString(const char *pItemName);
	int GetIntDefault(const char *pItemName, const int iDef);

public:
	/* config list */
	std::vector<LPCONF_ITEM> m_ConfigItemList;
};

#endif //!__NGX_CONF_H__
