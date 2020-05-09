﻿
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "XiaoHG_Func.h"
#include "XiaoHG_C_Conf.h" 
#include "XiaoHG_Macro.h"

#define __THIS_FILE__ "XiaoHG_C_Conf.cxx"

#define CONFIG_FILE_PATH "XiaoHG_SERVER.conf"

CConfig *CConfig::m_Instance = NULL;

CConfig::CConfig()
{
    LoadConfigFileToList();
}
CConfig::~CConfig()
{
	std::vector<LPCONF_ITEM>::iterator pos;	
	for(pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
	{
		delete (*pos);
	}/* end for */
	m_ConfigItemList.clear(); 
    return;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.24
 * test time: 
 * function name: Load
 * discription: load log file.
 * =================================================================*/
int CConfig::LoadConfigFileToList()
{
    /* max item 500b */
    char LineBuf[501] = { 0 };
    FILE *fp = fopen(CONFIG_FILE_PATH, "r");
    if(fp == NULL)
    {
        return XiaoHG_ERROR;
    }
    
    /* if end of config file*/
    while(!feof(fp))
    {
        if(fgets(LineBuf, 500, fp) == NULL)
            continue;

        /* Handle blank lines, comment lines ... */
        if(*LineBuf == 0 || *LineBuf == ';' || *LineBuf == ' ' || *LineBuf == '[' || 
                    *LineBuf == '#' || *LineBuf == '\t' || *LineBuf == '\n')
        {
            continue;
        }
        
    lblprocstring:
        /* Handle trailing characters */
		if(strlen(LineBuf) > 0)
		{
			if(LineBuf[strlen(LineBuf) - 1] == 10 || LineBuf[strlen(LineBuf) -1 ] == 13 || LineBuf[strlen(LineBuf) -1 ] == 32) 
			{
				LineBuf[strlen(LineBuf)-1] = 0;
				goto lblprocstring;
			}
		}

        /* Process after filtering out the tail characters */
        if(LineBuf[0] == 0)
			continue;

        /* like “ListenPort = 5678” */
        char *ptmp = strchr(LineBuf, '=');
        if(ptmp != NULL)
        {
            LPCONF_ITEM p_confitem = new CONF_ITEM;
            memset(p_confitem, 0, sizeof(CONF_ITEM));
            strncpy(p_confitem->ItemName, LineBuf, (int)(ptmp - LineBuf));
            strcpy(p_confitem->ItemContent, ptmp + 1);

            /* Remove spaces before and after the string */
            Rtrim(p_confitem->ItemName);
			Ltrim(p_confitem->ItemName);
			Rtrim(p_confitem->ItemContent);
			Ltrim(p_confitem->ItemContent);

            m_ConfigItemList.push_back(p_confitem);

        } /* end if */
    } /* end while(!feof(fp)) */ 
    fclose(fp);
    return XiaoHG_SUCCESS;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.24
 * test time: 
 * function name: GetString
 * discription: get config from item name
 * =================================================================*/
const char* CConfig::GetString(const char *pItemName)
{
	std::vector<LPCONF_ITEM>::iterator pos;	
	for(pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
	{
		if(strcasecmp( (*pos)->ItemName, pItemName) == 0)
			return (*pos)->ItemContent;
	}/* end for */
	return NULL;
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.24
 * test time: 
 * function name: GetIntDefault
 * discription: get config from item name, default def if the config
 *              item is null.
 * =================================================================*/
int CConfig::GetIntDefault(const char *pItemName, const int def)
{
	std::vector<LPCONF_ITEM>::iterator pos;	
	for(pos = m_ConfigItemList.begin(); pos != m_ConfigItemList.end(); ++pos)
	{
		if(strcasecmp( (*pos)->ItemName, pItemName) == 0)
        {
            return atoi((*pos)->ItemContent);
        }
	}/* end for */
	return def;
}



