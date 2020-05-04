
#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__


//结构定义------------------------------------
#pragma pack (1) //对齐方式,1字节对齐 

//一些和网络通讯相关的结构放在这里
typedef struct _COMM_PKG_HEADER
{
	unsigned short pkgLen;    //报文总长度【包头+包体】--2字节，2字节可以表示的最大数字为6万多，我们定义_PKG_MAX_LENGTH 30000，所以用pkgLen足够保存下
	unsigned short msgCode;   //消息类型代码--2字节，用于区别每个不同的命令【不同的消息】
	int crc32;     //CRC32效验--4字节，为了防止收发数据中出现收到内容和发送内容不一致的情况，引入这个字段做一个基本的校验用	
}COMM_PKG_HEADER, *LPCOMM_PKG_HEADER;

typedef struct _STRUCT_REGISTER
{
	int iType;          //类型
	char username[56];   //用户名 
	char password[40];   //密码
}STRUCT_REGISTER, *LPSTRUCT_REGISTER;

typedef struct _STRUCT_LOGIN
{
	char username[56];   //用户名 
	char password[40];   //密码

}STRUCT_LOGIN, *LPSTRUCT_LOGIN;

//int g_iLenPkgHeader = sizeof(COMM_PKG_HEADER);

#pragma pack() //取消指定对齐，恢复缺省对齐

#endif //__TEST_COMMON_H__