﻿
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_CONF_H__
#define __XiaoHG_CONF_H__

#include <vector>

/* config struct */
typedef struct config_item_s
{
	char ItemName[50];		/* name */
	char ItemContent[500];	/* content */
}CONF_ITEM, *LPCONF_ITEM;

class CConfig
{
private:
	CConfig();
public:
	~CConfig();
	
private:
	static CConfig *m_Instance;

/* Singleton implementation */
public:	
	static CConfig* GetInstance()
	{
		if(m_Instance == nullptr)
		{
			/* lock */
			if(m_Instance == nullptr)
			{					
				m_Instance = new CConfig();
				static CDeleteInstance cTmp; 
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
			if (CConfig::m_Instance)
			{						
				delete CConfig::m_Instance;
				CConfig::m_Instance = nullptr;
			}
		}
	};
	
private:
	int Init();

public:
	char *GetString(const char *pItemName);
	int GetIntDefault(const char *pItemName, const int iDef = 0);

public:
	/* config list */
	std::vector<LPCONF_ITEM> m_ConfigItemList;
};

#endif //!__XiaoHG_CONF_H__
