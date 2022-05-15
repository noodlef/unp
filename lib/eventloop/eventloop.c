/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：eventloop.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月19日
 * 描    述：
 * 
 *===============================================================
 */
#include <assert.h>

#include "../log.h"
#include "../dict.h"
#include "../min_heap.h"
#include "../utils.h"

#include "poll_api.h"
#include "eventloop.h"

#define FIRED_EVENT_LEN 32

/* no warnings for casting to void * from int */
#define INT2VOIDP(i) ((void *)(size_t)(i))

struct time_event { 
    void * user_data;
    free_user_data_t free_user_data;
    time_event_id id;
    time_event_callback_t on_expired;
    char deleted; /* BOOL, deleted when true */
    char perodic; /* BOOL, perodic timer when true */
    struct timeval interval; /* used when it's perodic */
    struct timeval when_expired; /* since the system was booted */
};

struct file_event {
    void * user_data;
    free_user_data_t free_user_data;
    int events;
    file_event_callback_t on_read;
    file_event_callback_t on_write;
};

struct eventloop {
    char running;
    void * poll; /* used for polling api specific data */
    struct poll_event fired_events[FIRED_EVENT_LEN];
    struct dict * file_event_map;
    time_event_id next_time_event_id;
    struct dict * time_event_map;
    struct heap_struct * time_events; /* min heap */
};

static void _event_dict_val_destroy(void * val)
{
    struct file_event * ev;
    /**
     * NOTE
     */
    ev = ((struct file_event *)val);

    if (ev->user_data) {
        if (ev->free_user_data == NO_FREE_USE_DATA_HANDLE)
            return;
        else if (ev->free_user_data)
            ev->free_user_data(ev->user_data);
        else 
            /* invoke free by default */
            free(ev->user_data);
    }

    free(val);
}

struct dict_type file_event_dict_type = {
    .key_destroy = NULL,
    .val_destroy = _event_dict_val_destroy,
    .key_dup = NULL,
    .val_dup = NULL,
    .key_compare = NULL 
};

struct dict_type time_event_dict_type = {
    .key_destroy = NULL,
    .val_destroy = _event_dict_val_destroy,
    .key_dup = NULL,
    .val_dup = NULL,
    .key_compare = NULL 
};

static int _time_event_heap_compare(void * val1, void * val2)
{
    struct timeval ev1, ev2;

    ev1 = ((struct time_event *)val1)->when_expired;
    ev2 = ((struct time_event *)val2)->when_expired;

    if (ev1.tv_sec > ev2.tv_sec) {
        return 0;
    } else if (ev1.tv_sec == ev2.tv_sec) {
        if (ev1.tv_usec > ev2.tv_usec)
            return 0;
    }

    return 1;
}

struct heap_type time_event_heap_type = {
    .val_compare = _time_event_heap_compare,
    .val_destroy = NULL /* don' free here */
};

/* ======================================================================= */
static void _destroy(struct eventloop * loop)
{
    if (!loop)
        return;

    if (loop->file_event_map)
        dict_destroy(loop->file_event_map);

    if (loop->time_events)
        heap_destory(loop->time_events);

    if (loop->time_event_map)
        dict_destroy(loop->time_event_map);

    if (loop->poll)
        poll_destory(loop->poll);

    free(loop);
}

struct eventloop * eventloop_create(void)
{
    struct eventloop * loop;

    if (!(loop = malloc(sizeof(struct eventloop)))) {
        log_error("OOM when create eventloop!");
        goto failed;
    }

    loop->next_time_event_id = 0;
    loop->running = 0;
    loop->poll = NULL;

    if (!(loop->file_event_map = dict_create(&file_event_dict_type))) {
        log_error("Create file event map error!");
        goto failed;
    }

    if (!(loop->time_event_map = dict_create(&time_event_dict_type))) {
        log_error("Create time event map error!");
        goto failed;
    }

    if (!(loop->time_events = heap_create(&time_event_heap_type))) {
        log_error("Create time events heap error!");
        goto failed;
    }

    if (!(loop->poll = poll_create())) {
        log_error("Poll Create error!");
        goto failed;
    }

    return loop;

failed:
    log_error("Create eventloop error!");
    _destroy(loop);
    return NULL;
}

void eventloop_destroy(struct eventloop * loop)
{
    /* TODO: */
    _destroy(loop);
}

void eventloop_stop(struct eventloop * loop)
{
    assert(loop != NULL);

    loop->running = 0;
    /* TODO: notify the main loop by signal maybe */
}

int eventloop_file_event_create(
        struct eventloop * loop,
        int fd,
        struct poll_file_event * event)
{
    struct file_event * ev;

    assert(fd >= 0);
    assert(loop != NULL);
    assert(event != NULL);

    if (!(ev = calloc(1, sizeof(struct file_event)))) {
        log_error("OOM when create file event!, fd: %d", fd);
        return -1;
    }

    ev->events = event->events;
    ev->free_user_data = event->free_user_data;
    ev->user_data = event->user_data;
    if (event->events & EVENT_READABLE)
        ev->on_read = event->on_read;
    if (event->events & EVENT_WRITABLE)
        ev->on_write = event->on_write;

    if (dict_add(loop->file_event_map, INT2VOIDP(fd), ev) < 0) {
        log_error("Dict add file event error!, fd: %d", fd);
        free(ev);
        return -1;
    }

    if (poll_event_add(loop->poll, fd, event->events) < 0) {
        log_error("Poll add event error!, fd: %d", fd);
        dict_delete(loop->file_event_map, INT2VOIDP(fd));
        return -1;
    }

    return 0;
}

int eventloop_file_event_add(
        struct eventloop * loop, 
        int fd, 
        int event,
        file_event_callback_t callback)
{
    int new_events;
    struct file_event * ev;

    assert(loop != NULL);
    assert(callback != NULL);

    if (dict_find(loop->file_event_map, 
                INT2VOIDP(fd), (void **)&ev) < 0) {
        log_error("Add file event error, event not found!, fd: %d", fd);
        return -1;
    }

    if (ev->events & event)
        return 0;

    new_events = ev->events | event;
    if (poll_event_mod(loop->poll, fd, new_events) < 0) {
        log_error("Poll add file event error!, fd: %d", fd);
        return -1;
    }

    ev->events = new_events;
    if (event == EVENT_READABLE)
        ev->on_read = callback;
    else if (event == EVENT_WRITABLE)
        ev->on_write = callback;

    return 0;
}

int eventloop_file_event_del(
        struct eventloop * loop, 
        int fd, 
        int event)
{
    int new_events;
    struct file_event * ev;

    assert(loop != NULL);

    if (dict_find(loop->file_event_map, 
                INT2VOIDP(fd), (void **)&ev) < 0)
        return 0; /* NOT found */

    if (!(ev->events & event))
        return 0;

    new_events = ev->events & (~event);
    if (!new_events) {
        /**
         * remove the fd from the intrest list
         */
        if (poll_event_del(loop->poll, fd) < 0) {
            log_error("Poll del event error!, fd: %d", fd);
            return -1;
        }
        dict_delete(loop->file_event_map, INT2VOIDP(fd));
    } else {
        if (poll_event_mod(loop->poll, fd, new_events) < 0) {
            log_error("Poll mod event error!, fd: %d", fd);
            return -1;
        }
        ev->events = new_events;
    }

    return 0;
}

static void _set_expired_time(struct timeval * interval)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    interval->tv_sec += tv.tv_sec;
    interval->tv_usec += tv.tv_usec;

    if (interval->tv_usec >= 1000000) {
        interval->tv_usec -= 1000000;
        ++interval->tv_sec;
    }
}

time_event_id eventloop_time_event_create(
        struct eventloop * loop,
        struct poll_time_event * event)
{
    struct time_event * ev;

    assert(loop != NULL);
    assert(event != NULL);

    if (!(ev = calloc(1, sizeof(struct time_event)))) {
        log_error("OOM when create time event!");
        return -1;
    }

    ev->deleted = 0;
    ev->perodic = event->perodic;
    ev->id = ++loop->next_time_event_id;
    ev->user_data = event->user_data;
    ev->free_user_data = event->free_user_data;
    ev->on_expired = event->on_expired;
    ev->interval = ev->when_expired = event->interval;
    _set_expired_time(&ev->when_expired);

    if (dict_add(loop->time_event_map, (void *)(ev->id), ev) < 0) {
        log_error("Dict add time event error!");
        free(ev);
        return -1;
    }

    if (heap_push(loop->time_events, (void *)ev) < 0) {
        log_error("Add timer to min heap error!");
        dict_delete(loop->time_event_map, (void*)(ev->id));
        return -1;
    }

    return ev->id;
}

void eventloop_time_event_del(struct eventloop * loop, time_event_id time_id)
{
    struct time_event * ev;

    assert(loop != NULL);

    if (dict_find(loop->time_event_map, 
                (void *)time_id, (void **)&ev) < 0)
        return; /* NOT found */

    /* TODO */
    ev->deleted = 1;
}

static void _calcuate_time_from_now(
        struct timeval * when, struct timeval * res)
{
    struct timeval now;

    gettimeofday(&now, NULL);
    res->tv_sec = when->tv_sec - now.tv_sec;
    if (when->tv_usec >= now.tv_usec) {
        res->tv_usec = when->tv_usec - now.tv_usec;
    } else {
        res->tv_usec = when->tv_usec + 1000000 - now.tv_usec;
        --res->tv_sec;
    }

    if (res->tv_sec < 0) {
        res->tv_sec = 0;
        res->tv_usec = 0;
    }
}

void eventloop_events_dispatch(struct eventloop * loop)
{
    int i, fd, events;
    struct time_event * time_ev;
    struct timeval tmp, * timeout, now;
    struct file_event * file_ev;

    assert(loop != NULL);

    log_info("==================== Eventloop running =====================");
    loop->running = 1;
    while (loop->running) {
        /**
         * calcuate the timeout before we can wakeup 
         * from 'poll_wait'
         */
        if ((time_ev = heap_top(loop->time_events))) {
            _calcuate_time_from_now(&(time_ev->when_expired), &tmp);
            timeout = &tmp;
        } else {
            timeout = NULL; /* No time event, we can block */ 
        }

        i = poll_wait(loop->poll, loop->fired_events, 
                      sizeof(loop->fired_events), timeout);
        while(loop->running && i-- > 0) {
            fd = loop->fired_events[i].fd;
            events = loop->fired_events[i].events;

            if (dict_find(loop->file_event_map, 
                        INT2VOIDP(fd), (void **)&file_ev) < 0) {
                log_warning("Dispatch events, event not found!, fd: %d", fd);
                continue; /* NOT found */
            }

            if (events & EVENT_READABLE) {
                if (file_ev->on_read)
                    file_ev->on_read(loop, fd, file_ev->user_data);
            }

            if (events & EVENT_WRITABLE) {
                if (file_ev->on_write)
                    file_ev->on_write(loop, fd, file_ev->user_data);
            }
        }

        /* process time events */
        while (loop->running && heap_size(loop->time_events)) {
            time_ev = heap_top(loop->time_events);

            /* the time event has been canceled */
            if (time_ev->deleted) {
                heap_pop(loop->time_events);
                dict_delete(loop->time_event_map, (void *)time_ev->id);
                continue;
            }

            gettimeofday(&now, NULL);
            if (time_ev->when_expired.tv_sec > now.tv_sec 
                    || (time_ev->when_expired.tv_sec == now.tv_sec &&
                        time_ev->when_expired.tv_usec > now.tv_usec))
                break;

            heap_pop(loop->time_events);
            time_ev->on_expired(loop, time_ev->id, time_ev->user_data);
            if (time_ev->perodic) {
                time_ev->when_expired = timeval_add(
                        &time_ev->when_expired, &time_ev->interval);
                //time_ev->when_expired = time_ev->interval;
                //_set_expired_time(&time_ev->when_expired);
                heap_push(loop->time_events, (void *)time_ev);
            }
        }
    }

    log_info("==================== Eventloop exit =====================");
}

