/**
 * ALIB接口
 *
 * asock interface
 */
#ifndef LAXCUS_ALIB_H
#define LAXCUS_ALIB_H 1

#include "atype.h"

/**
 * 加载共享库
 * 成功返回0，失败-1
 */
extern int LoadLibrary(int asock, const char *name);

/**
 * 释放共享库资源
 * 成功返回0，失败-1
 */
extern int ReleaseLibrary(int asock);

/**
 * 调用共享库函数
 *
 * agent function call
 */
extern int AFCall(int asock, const char *name, const void *in, int inlen, void **out, int *outlen);

/**
 * 释放AFCall函数的out参数内存
 * 成功返回0，失败-1
 */
extern int AFree(void **out);

#endif
