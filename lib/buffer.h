/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：buffer.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月23日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __BUFFER_H
#define __BUFFER_H
#include <sys/types.h>
#include <sys/uio.h>

#define SEEK_HEAD   0
#define SEEK_TAIL   1

#define SBUF_RECV_OOM -1 
#define SBUF_RECV_ERR -2 
#define SBUF_RECV_EOF -3 
struct socket_buffer;

/**
 * 本文件实现简单的缓冲区管理功能, 缓冲区用户视图如下示：
 * 
 *  ~~~ ======================================== ~~~  
 *      |                                      |      
 *   +  |                                      |  +   
 *      |                                      |      
 *  ~~~ ======================================== ~~~   
 *      |                                      |
 *      ----> 缓冲区头                         ----> 缓冲区尾
 */

/**
 * 创建一个缓冲区， 返回值如下：
 *     返回 NULL：失败
 */
struct socket_buffer * socket_buffer_new(void);

/**
 * 删除一个缓冲区 
 */
void socket_buffer_free(struct socket_buffer * socket_buffer);

/**
 * 返回缓冲区当前的大小，单位：字节 
 */
size_t socket_buffer_size(struct socket_buffer * socket_buffer);

/**
 * 移动缓冲区头或尾指针的位置
 * seek_type: SEEK_HEAD 缓冲区头向前或向后移动 sz 字节, sz 为正时向前
 *            移动，最大移动到缓冲区尾，sz 为负向后移动
 *            SEEK_TAIL 缓冲区尾向前或向后移动 sz 字节，sz 为正时向前
 *            移动，sz 为负向后移动, 最多移动到缓冲区头
 * return: 成功返回移动字节数, 可能比 sz 小
 */
int socket_buffer_seek(struct socket_buffer * socket_buffer, 
        int sz, int seek_type);

/**
 * 从缓冲区头开始读出 len 字节数，最多读出缓冲当前的总字节数
 * 返回值如下：
 *     返回 = len：成功读取 len 字节数据
 *     返回 < len：当前缓冲区的总字节数小于 len
 */
int socket_buffer_read(struct socket_buffer * socket_buffer, 
        char * buf, int len);

/**
 * 从缓冲区头起 offset 位置开始读取 len 字节数，读取完后不更新
 * 缓冲区头, 返回值如下：
 *     返回 = len：成功读取 len 字节数据
 *     返回 < len：len 大于从缓冲区头 offset 位置到缓冲区尾的长度
 */
int socket_buffer_peek(struct socket_buffer * socket_buffer, 
        int offset, char * buf, int len);

/**
 * 从缓冲区尾开始写入 len 字节数，返回值如下：
 *     返回 = len：成功写入 len 字节数据
 *     返回 < len：发生 OOM 错误
 */
int socket_buffer_write(struct socket_buffer * socket_buffer, 
        char * buf, int len);

/**
 * 从缓冲区头 offset 位置开始替换缓冲区中 len 字节的数据, 返回实际
 * 替换的字节, 返回值如下：
 *     返回 = len：成功替换 len 字节数据
 *     返回 < len：len 大于从缓冲区头 offset 位置到缓冲区尾的长度
 */
int socket_buffer_replace(struct socket_buffer * socket_buffer, 
        int offset, char * buf, int len);

/**
 * 从缓冲区头开始读出 len 字节数据写入 fd 中, 最多写入字节数等于
 * 缓冲区的大小，不管是否发生错误 len 中都返回已经成功写入的数据, 
 * 返回值如下：
 *     返回  0：成功
 *     返回 -1：写 fd 失败
 */
int socket_buffer_send(struct socket_buffer * buffer, int fd, int * len);

/**
 * 从缓冲尾开始写入从fd 中读出的 len 字节数据, 不管是否发生错误 len 中都
 * 返回已经成功接收的数据, 返回值如下：
 *     返回  0：成功, 至少写入了 len 字节
 *     返回 -1：发生 OOM
 *     返回 -2：读 fd 失败
 *     返回 -3：读到 EOF
 */
int socket_buffer_recv(struct socket_buffer * socket_buffer, int fd, int * len);

/**
 * 复制已有的缓冲区， 返回值如下：
 *     返回 NULL：失败
 */
struct socket_buffer * socket_buffer_copy(struct socket_buffer * socket_buffer);
#endif //BUFFER_H
