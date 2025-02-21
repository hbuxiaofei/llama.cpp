/**
 * ASOCK操作类型
 */
#ifndef LAXCUS_ATYPE_H
#define LAXCUS_ATYPE_H 1

#include "acode.h"

/** 主机类型（直接连接或者NAT）**/
const unsigned char DIRECT_HOST = 0x10;
const unsigned char NAT_HOST = 0x20;

#pragma pack(1)

/** 分布式主机(distributed host) **/
typedef struct __nub
{
	unsigned char type;	// DIRECT_HOST / NAT_HOST;
	char family[16];		// TOP HOME WORK WATCH CLIENT
	char rank[16];			// MANAGER MONITOR GATEWAY JOB
	char ip[64];			// IP地址的字符串，包括IP4/IP6
	unsigned short port;
} nub;

#pragma pack()

/** 计算机集群(computer cluster) **/
typedef struct __cluster
{
	int count;
	nub *nubs;

	__cluster()
	{
		nubs = NULL;
		count = 0;
	}

	~__cluster()
	{
		if(nubs != NULL) { delete[] nubs; nubs = NULL;}
		count = 0;
	}
} cluster;


/** 文件操作模式 (asynchornized file) **/
const int AF_READ = 0x1;
const int AF_WRITE = 0x2;
const int AF_APPEND = 0x4;

/** 任务类型(task types) **/
const int AT_COMMAND = 1;
const int AT_LIBRARY = 2;
const int AT_FILE = 3;
const int AT_VTERM = 4; // 伪终端


/** 任务执行操作符(task execute operator) **/
const int RT_COMMAND = 1;

const int RT_LIBRARY_OPEN = 2;
const int RT_LIBRARY_CLOSE = 3;
const int RT_AFCALL = 4;

const int RT_FILE_OPEN = 5; // open file
const int RT_FILE_LENGTH = 6; // file length


/** field keyword, 用在Field类 **/
const char FKW_COMMAND[] = "COMMAND"; 	// command operate key
const char FKW_FILE_PATH[] = "FILE_PATH";	// open file path
const char FKW_FILE_MODE[] = "FILE_MODE";	// open file mode
const char FKW_FILE_BREAKPOINT[] = "FILE_BREAKPOINT";	// file begin point

const char FKW_LIBNAME[] = "LIBRARY-NAME";
const char FKW_FUNCNAME[] = "FUNCTION-NAME";
const char FKW_FUNCINPUT[] = "FUNCTION-INPUT";

#endif
