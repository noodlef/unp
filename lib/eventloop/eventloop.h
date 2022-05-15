/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：eventloop.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月18日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __EVENTLOOP_H
#define __EVENTLOOP_H
#include <string.h>
#include <sys/time.h>

//#define __USE_SELECT__
#define __USE_EPOLL__

#define EVENT_READABLE 0x01 
#define EVENT_WRITABLE 0x02 

struct eventloop;
typedef unsigned long time_event_id;
typedef void (* free_user_data_t)(void * user_data);
typedef int (* time_event_callback_t)(struct eventloop * loop, 
        time_event_id id, void * user_data); 
typedef int (* file_event_callback_t)(struct eventloop * loop, 
        int fd, void * user_data);

#define NO_FREE_USE_DATA_HANDLE ((free_user_data_t)(-1))

struct poll_file_event {
    int events;
    void * user_data;
    free_user_data_t free_user_data;
    file_event_callback_t on_read;
    file_event_callback_t on_write;
};

struct poll_time_event {
    char perodic; /* perodic if True */
    void * user_data;
    free_user_data_t free_user_data;
    time_event_callback_t on_expired;
    struct timeval interval;
};

#define INIT_POLL_FILE_EVENT(ev) (memset((ev), 0, sizeof(struct poll_file_event)))
#define INIT_POLL_TIME_EVENT(ev) (memset((ev), 0, sizeof(struct poll_time_event)))

/* ======================================================================= */
struct eventloop * eventloop_create(void);

void eventloop_destroy(struct eventloop * loop);

void eventloop_stop(struct eventloop * loop);

int eventloop_file_event_create(
        struct eventloop * loop,
        int fd,
        struct poll_file_event * event
);

int eventloop_file_event_add(
        struct eventloop * loop, 
        int fd, 
        int event, 
        file_event_callback_t callback 
);

int eventloop_file_event_del(
        struct eventloop * loop, 
        int fd, 
        int event
);

time_event_id eventloop_time_event_create(
        struct eventloop * loop,
        struct poll_time_event * event
);

void eventloop_time_event_del(struct eventloop * loop, time_event_id event_id);

void eventloop_events_dispatch(struct eventloop * loop);
#endif //EVENTLOOP_H
