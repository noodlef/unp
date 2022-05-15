/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：if_info.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月08日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __IF_INFO_H
#define __IF_INFO_H

#include <net/if.h>
#include <sys/socket.h>

#define IF_NAME     16
#define IF_HADDR    8
#define IF_ADDR     16

struct if_info {
    char name[IF_NAME];
    short index;
    short mtu;
    unsigned char hwaddr[IF_HADDR];
    short flags;
    char addr[IF_ADDR];
    char broadaddr[IF_ADDR];
    char dstaddr[IF_ADDR];
    char netmask[IF_ADDR];
    struct if_info * next; 
};

#define IF_ALIAS    1
#define IF_BUF_SIZE 100

struct if_info * get_if_info(void);
void free_if_info(struct if_info *);

#endif //IF_INFO_H
