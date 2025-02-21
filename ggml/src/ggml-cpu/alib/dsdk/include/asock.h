/**
 * ASOCK接口
 *
 * asock interface
 */
#ifndef LAXCUS_ASOCK_H
#define LAXCUS_ASOCK_H 1

#include "atype.h"

/**
 * 启动分布式异步服务
 * port: 本地监听端口，默认是0，从内存中读取
 * 成功返回0，失败返回-1 
 **/
extern int Startup(unsigned short port=0);

/**
 * 结束分布式异步服务
 * 成功返回0，失败返回-1 
 */
extern int Cleanup();

/**
 * 获得当前节点的异步服务通信端口
 * 成功返回大于0，小于0xFFFF的端口号，失败返回0
 **/
extern unsigned short GetServicePort();

/** 
 * 构建一个异步服务，asock是通信ID，通过asock实现客户端/服务端的连接，以及基于连接的远程服务
 * asock: 异步服务编号
 * type : 异步服务类型，包括：AT_COMMAND / AT_LIBRARY / AT_FILE 等类型
 * hub: 服务器IP地址，如果是NULL，默认本地节点
 * port: 服务器端口号
 * timeout: 异步服务超时时间（以毫秒计），即当空闲X毫秒后，服务器主动关闭服务
 *
 * 成功返回0，失败返回负数
 **/
extern int Attach(int type, const char *hub, unsigned short port, unsigned int timeout);

/** 
 * 结束异步服务
 * asock: 异步服务编号
 *
 * 成功返回0，失败返回负数
 **/
extern int Detach(int asock);

/**
 * 当前节点的主机地址
 * asock - attach编号，如果读取当前节点关联节点，asock是0
 *
 * 成功返回0，失败返回负数
 */
extern int GetLocal(int asock, struct __nub *);

/**
 * 当前集群的主机地址集合（不包括本地地址）
 *
 * asock - attach编号，如果读取当前节点关联节点，asock是0
 *
 * 成功返回0，失败返回负数
 */
extern int GetPeers(int asock, struct __cluster *);

#endif
