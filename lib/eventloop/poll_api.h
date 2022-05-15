/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：poll_api.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月23日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __POLL_API_H
#define __POLL_API_H
#include <sys/time.h>

struct poll_event {
    int fd;
    int events;
};

void * poll_create(void);

void poll_destory(void * poll);

int poll_event_add(void * poll, int fd, int events);

int poll_event_mod(void * poll, int fd, int new_events);

int poll_event_del(void * poll, int fd);

int poll_wait(void * poll, 
        struct poll_event * events, int maxevents, struct timeval * timeout);

#endif //POLL_API_H
