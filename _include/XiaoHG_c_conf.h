
#ifndef __XiaoHG_CONF_H__
#define __XiaoHG_CONF_H__

#include <vector>
#include "XiaoHG_Global.h"
#include "XiaoHG_Macro.h"
#include "XiaoHG_Func.h"

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
		/* function track */
    	XiaoHG_Log(LOG_ALL, LOG_LEVEL_TRACK, 0, "GetInstance(CConfig) track");

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

#endif //!__XiaoHG_CONF_H__
