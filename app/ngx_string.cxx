﻿
#include <stdio.h>
#include <string.h>

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23 pass
 * function name: Ltrim
 * discription: Truncate spaces at the end of the string
 * =================================================================*/
void Rtrim(char *string)   
{   
	size_t len = 0;   
	if(string == NULL)   
		return;

	len = strlen(string);   
	while(len > 0 && string[len-1] == ' ')
		string[--len] = 0;   
	return;   
}

/* =================================================================
 * auth: XiaoHG
 * date: 2020.04.23
 * test time: 2020.04.23 pass
 * function name: Ltrim
 * discription: Truncate the first space of the string
 * =================================================================*/
void Ltrim(char *string)
{
	size_t len = 0;
	len = strlen(string);   
	char *p_tmp = string;
	if( (*p_tmp) != ' ')
		return;
	//找第一个不为空格的
	while((*p_tmp) != '\0')
	{
		if( (*p_tmp) == ' ')
			p_tmp++;
		else
			break;
	}
	if((*p_tmp) == '\0')
	{
		*string = '\0';
		return;
	}
	char *p_tmp2 = string; 
	while((*p_tmp) != '\0')
	{
		(*p_tmp2) = (*p_tmp);
		p_tmp++;
		p_tmp2++;
	}
	(*p_tmp2) = '\0';
    return;
}