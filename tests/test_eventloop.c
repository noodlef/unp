/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：test_eventloop.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月30日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "../lib/log.h"
#include "../lib/eventloop/eventloop.h"

int on_perodic_timer(struct eventloop * loop, time_event_id id, void * user_data)
{
    static int call_times = 0;

    printf("On_perodic_timer: call time_event[%ld] %d times!\n", id, ++call_times);
    if (call_times > 8)
        eventloop_stop(loop);
    return 0;
}

int on_del_perodic_timer(struct eventloop * loop, time_event_id id, 
        void * user_data)
{
    static int call_times = 0;

    printf("On_del_perodic_timer: call time_event[%ld] %d times!\n", 
            id, ++call_times);
    if (call_times >= 1)
        eventloop_time_event_del(loop, id);
    return 0;
}

int on_oneshot_timer(struct eventloop * loop, time_event_id id, void * user_data)
{
    static int call_times = 0;

    printf("On_oneshot_timer: call time_event[%ld] %d times!\n", id, ++call_times);
    return 0;
}

int on_read(struct eventloop * loop, int fd, void * user_data)
{
    static int call_times = 0;

    printf("On_read: call [file: %d] %d times!\n", fd, ++call_times);
    sleep(2);
    return 0;
}

int on_write(struct eventloop * loop, int fd, void * user_data)
{
    static int call_times = 0;

    printf("On_write: call [file: %d] %d times!\n", fd, ++call_times);
    write(fd, &call_times, sizeof(int));
    eventloop_file_event_del(loop, fd, EVENT_WRITABLE);
    sleep(2);
    return 0;
}

int main(int argc, char * argv[])
{
    int pfds[2];
    struct eventloop * loop;
    struct timeval interval;
    struct poll_time_event time_event;
    struct poll_file_event file_event;

    if (pipe(pfds) < 0) {
        log_error_sys("Pipe create error!");
        return -1;
    } 

    loop = eventloop_create();
    INIT_POLL_TIME_EVENT(&time_event);
    
    /* perodic */
    interval.tv_sec = 1;
    interval.tv_usec = 0;
    time_event.interval = interval;
    time_event.perodic = 1;
    time_event.on_expired = on_perodic_timer;
    time_event.user_data = malloc(8); /* test */
    eventloop_time_event_create(loop, &time_event);

    /* del timer */
    interval.tv_sec = 2;
    time_event.interval = interval;
    time_event.user_data = NULL; /* test */
    time_event.free_user_data = NULL;
    time_event.on_expired = on_del_perodic_timer;
    eventloop_time_event_create(loop, &time_event);

    /* oneshot */
    interval.tv_sec = 3;
    time_event.perodic = 0;
    time_event.interval = interval;
    time_event.on_expired = on_oneshot_timer;
    time_event.user_data = malloc(8); /* test */
    time_event.free_user_data = free;
    eventloop_time_event_create(loop, &time_event);

    /* read */
    INIT_POLL_FILE_EVENT(&file_event);
    file_event.events = EVENT_READABLE;
    file_event.on_read = on_read;
    eventloop_file_event_create(loop, pfds[0], &file_event);

    /* write */
    INIT_POLL_FILE_EVENT(&file_event);
    file_event.events = EVENT_WRITABLE;
    file_event.on_write = on_write;
    eventloop_file_event_create(loop, pfds[1], &file_event);

    eventloop_events_dispatch(loop);

    eventloop_destroy(loop);

    return 0;
}
