/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：event_epoll.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月27日
 * 描    述：
 * 
 *===============================================================
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

#include "../log.h"
#include "poll_api.h"
#include "eventloop.h"
#ifdef __USE_EPOLL__

struct poll_struct {
    int epoll_fd;
    struct epoll_event events[32];
};

void * poll_create(void)
{
    struct poll_struct * poll;

    if (!(poll = malloc(sizeof(struct poll_struct)))) {
        log_error("OOM when invoke poll_create!");
        return NULL;
    }

    log_info("========== Use epoll ===========");
    if ((poll->epoll_fd = epoll_create(1024)) < 0) {
        log_error_sys("Epoll_create error!");
        free(poll);
        return NULL;
    }

    return poll;
}

void poll_destory(void * poll)
{
    if (poll) {
        close(((struct poll_struct *)poll)->epoll_fd);
        free(poll);
    }
}

static int _poll_ctl(struct poll_struct * poll, int op, int fd, int events)
{
    struct epoll_event epoll_data;

    if (op != EPOLL_CTL_DEL) {
        memset(&epoll_data, 0, sizeof(epoll_data));
        if (events & EVENT_READABLE)
            epoll_data.events |= EPOLLIN;
        if (events & EVENT_WRITABLE)
            epoll_data.events |= EPOLLOUT;
        epoll_data.data.fd = fd;
    }

    if (epoll_ctl(poll->epoll_fd, op, fd, &epoll_data) < 0) {
        log_error_sys("Epoll_ctl error!");
        return -1;
    }

    return 0;
}

int poll_event_add(void * poll, int fd, int events)
{
    return _poll_ctl((struct poll_struct *)poll, EPOLL_CTL_ADD, fd, events);
}

int poll_event_mod(void * poll, int fd, int events)
{
    return _poll_ctl((struct poll_struct *)poll, EPOLL_CTL_MOD, fd, events);
}

int poll_event_del(void * poll, int fd)
{
    return _poll_ctl((struct poll_struct *)poll, EPOLL_CTL_DEL, fd, 0);
}

int poll_wait(void * ptr, 
        struct poll_event * events, int maxevents, struct timeval * timeout)
{
    int i, j, mask, ms_timeout; /* millseconds */ 
    struct poll_struct * poll = ptr; 

    ms_timeout = timeout ? timeout->tv_sec * 1000 + timeout->tv_usec / 1000 : -1;
    if ((size_t)maxevents > sizeof(poll->events))
        maxevents = sizeof(poll->events);

    i = epoll_wait(poll->epoll_fd, poll->events, maxevents, ms_timeout);
    if (i < 0) {
        log_warning_sys("Epoll_wait error!");
        return 0;
    }

    for (j = 0; j < i; j++) {
        mask = (poll->events)[j].events;
        events[j].events = 0;
        if (mask & EPOLLIN)
            events[j].events |= EVENT_READABLE;
        if (mask & EPOLLOUT)
            events[j].events |= EVENT_WRITABLE;
        events[j].fd = (poll->events[j]).data.fd;
    }

    return i;
}
#endif
