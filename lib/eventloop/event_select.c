/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：event_select.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月27日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "../log.h"
#include "poll_api.h"
#include "eventloop.h"
#ifdef __USE_SELECT__

struct poll_struct {
    int maxfd;
    fd_set read_set;
    fd_set write_set;
};

void * poll_create(void)
{
    struct poll_struct * poll;

    log_info("========== Use select ===========");
    if (!(poll = malloc(sizeof(struct poll_struct)))) {
        log_error("OOM when poll_create!");
        return NULL;
    }

    poll->maxfd = -1;
    FD_ZERO(&poll->read_set);
    FD_ZERO(&poll->write_set);
    return poll;
}

void poll_destory(void * poll)
{
    free(poll);
}

int poll_event_add(void * ptr, int fd, int events)
{
    struct poll_struct * poll = ptr;

    if (events & EVENT_READABLE)
        FD_SET(fd, &poll->read_set);

    if (events & EVENT_WRITABLE)
        FD_SET(fd, &poll->write_set);

    if (fd > poll->maxfd)
        poll->maxfd = fd;

    return 0;
}

int poll_event_mod(void * ptr, int fd, int new_events)
{
    struct poll_struct * poll = ptr;

    FD_CLR(fd, &poll->read_set);
    FD_CLR(fd, &poll->write_set);

    return poll_event_add(poll, fd, new_events);
}

int poll_event_del(void * ptr, int fd)
{
    struct poll_struct * poll = ptr;

    FD_CLR(fd, &poll->read_set);
    FD_CLR(fd, &poll->write_set);

    /* update the maxfd */
    if (fd == poll->maxfd) {
        while (--fd >= 0) {
            if (FD_ISSET(fd, &poll->read_set) 
                    || FD_ISSET(fd, &poll->write_set)) {
                poll->maxfd = fd;
                break;
            }
        }

        /* Not found */
        if (fd < 0)
            poll->maxfd = -1;
    }

    return 0;
}

int poll_wait(void * ptr, 
        struct poll_event * events, int maxevents, struct timeval * timeout)
{
    int i, j, k, mask;
    fd_set rset, wset; 
    struct poll_struct * poll;

    poll = ptr;
    memcpy(&rset, &poll->read_set, sizeof(fd_set));
    memcpy(&wset, &poll->write_set, sizeof(fd_set));

    i = select(poll->maxfd + 1, &rset, &wset, NULL, timeout); 
    if (i < 0) {
        log_warning_sys("Poll_wait: select error!");
        return -1;
    }

    for(j = 0, k = 0; k < maxevents && j <= poll->maxfd; j++) {
        mask = 0;
        if (FD_ISSET(j, &rset))
            mask |= EVENT_READABLE;
        if (FD_ISSET(j, &wset))
            mask |= EVENT_WRITABLE;

        if (mask) {
            events[k].fd = j;
            events[k++].events = mask;
        }
    }
    
    return 0;
}
#endif
