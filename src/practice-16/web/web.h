/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：web.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月07日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __WEB_H
#define __WEB_H
#include<sys/select.h>

#define MAXFILES    20
#define SERVER_PORT 80

struct file {
    char * f_name;
    char * f_host;
    int sockfd;
    int flags;
};
extern struct file file[MAXFILES];

#define F_CONNECTING    1
#define F_READING       2
#define F_DONE          4

extern int G_nconn;
extern int G_nfiles;
extern int G_nleftconn;
extern int G_nlefttoread;
extern int G_maxfd;

extern fd_set G_rset;
extern fd_set G_wset;

void home_page(char *, char *);
void start_connect(struct file *);
void write_get_cmd(struct file *);

#define GET_CMD "GET %s HTTP/1.0\r\n\r\n"

#endif //WEB_H
