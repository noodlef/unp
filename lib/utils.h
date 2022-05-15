/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：utils.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月14日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __UTILS_H
#define __UTILS_H

#include <time.h>
#include <sys/types.h>
#include <netinet/in.h>

#define max(a, b) ((a) > (b) ? (a) : (b)) 
#define min(a, b) ((a) < (b) ? (a) : (b)) 

#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define DIR_MODE    (FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH)

#define EXIT_SUCCESS    0
#define EXIT_FAILURE    1

#define SIG_ERROR   (sig_func_t)0

struct msg_info {
    int flags;
    void * recvbuf;
    size_t bufsize;
    int ifindx;
    int recvflags;
    struct sockaddr_in srcaddr;
    struct sockaddr_in dstaddr;
};

typedef void (*sig_func_t)(int);

/**
 * read 'n' bytes of data from fd 
 * @return:  n on success 
 *           0 on EOF 
 *           less than n on errors
 */
size_t readn(int fd, void * buf, size_t n);

/**
 * write 'n' bytes of data to fd 
 * @return:  n on success 
 *           less than n on errors
 */
size_t writen(int fd, void * buf, size_t n);

sig_func_t signal_act(int signo, sig_func_t func);

int daemon_init(void);

int set_proctitile(char ** argv, char * new_title);

int connect_timeo(int sockfd, struct sockaddr_in * server_addr, int nsec);

int recvmsginfo(int sockfd, struct msg_info * msg_info);

int tcp_connect(struct sockaddr_in * local_addr, 
        struct sockaddr_in * remote_add);

int tcp_listen(struct sockaddr_in * server_addr, int backlog);

int write_pid_file(const char * pid_file);

/* get the real size of file in bytes */
int get_real_file_size(int fd);

struct timeval timeval_add(struct timeval * tv1, struct timeval * tv2);
struct timeval timeval_sub(struct timeval * tv1, struct timeval * tv2);
int timeval_compare(struct timeval * tv1, struct timeval * tv2);
#endif //UTILS_H
