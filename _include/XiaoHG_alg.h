
/*
 * Copyright(c) XiaoHG
 * Copyright(c) XiaoHG_SERVER
 */

#ifndef __XiaoHG_ALG_H__
#define __XiaoHG_ALG_H__

#include <string>

/* =================================================================
 * auth: XiaoHG
 * discription: String control
 * =================================================================*/
void Trim(std::string &s);/* discription: Clean up the space before and after the string */
void Rtrim(char *string);/* discription: Truncate spaces at the end of the string */
void Ltrim(char *string);/* discription: Truncate the first space of the string */

#endif //!__XiaoHG_ALG_H__