/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：timer_manager.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月26日
 * 描    述：
 * 
 *===============================================================
 */

/**
 * An implemention of timing wheels algorithm
 */
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>

#include "../dict.h"
#include "../log.h"
#include "../utils.h"
#include "../../3rd/list.h"

#include "timer_manager.h"

#define TVR_BITS 8
#define TVN_BITS 6
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_size (1 << TVN_BITS)
#define TVR_MASK (TVR_SIZE - 1)
#define TVN_MASK (TVN_size - 1)

struct _root_wheel {
    struct list_head vec[TVR_SIZE];
};

struct _wheel {
    struct list_head vec[TVN_size];
};

struct timer_manager {
    timer_id_t next_timer_id;
    struct dict * timer_dict;
    uint32_t ticks; /* milliseconds */
    uint32_t delta; /* nsec */
    struct timespec last_update_time;
    struct list_head expired_timers;
    struct _root_wheel root_wheel;
    struct _wheel wheel[4];
};

static inline int timer_id_compare(void * key1, void * key2) {
    return (timer_id_t)key1 > (timer_id_t) key2;
}

struct dict_type timer_dict_type = {
    .key_destroy = NULL, 
    .val_destroy = NULL,
    .key_compare = timer_id_compare,
    .key_dup = NULL,
    .val_dup = NULL,
};

struct timer_manager * timer_manager_create(void)
{
    int i, j;
    struct list_head * list;
    struct timer_manager * manager;

    if (!(manager = malloc(sizeof(struct timer_manager)))) {
        log_error("Malloc error!, timer_manager.");
        return NULL;
    }

    if (!(manager->timer_dict = dict_create(&timer_dict_type))) {
        log_error("Create dict error!");
        free(manager);
        return NULL;
    }

    manager->next_timer_id = 0;
    manager->ticks = 0;
    INIT_LIST_HEAD(&(manager->expired_timers));
    clock_gettime(CLOCK_MONOTONIC, &(manager->last_update_time));

    for (i = 0; i < TVR_SIZE; i++) {
        list = &(manager->root_wheel.vec[i]);
        INIT_LIST_HEAD(list);
    }

    for (i = 0; i < 4; i++) {
        for (j = 0; j < TVN_size; j++) {
            list = &(manager->wheel[i].vec[j]);
            INIT_LIST_HEAD(list);
        }
    }

    return manager;
}

static void _destory_timers(struct list_head * list) 
{
    struct timer_struct * timer;
    struct list_head * pos, * n;

    list_for_each_safe(pos, n, list) {
        list_del(pos);
        timer = list_entry(pos, struct timer_struct, node);
        timer_destory(timer);
    }
}

void timer_manager_destory(struct timer_manager * manager)
{
    int i, j;

    assert(manager != NULL);  

    _destory_timers(&(manager->expired_timers));
    for (i = 0; i < TVR_SIZE; i++)
        _destory_timers(&(manager->root_wheel.vec[i]));

    for (i = 0; i < 4; i++) {
        for (j = 0; j < TVN_size; j++)
            _destory_timers(&(manager->wheel[i].vec[j]));
    }

    dict_destroy(manager->timer_dict);
    free(manager);
}

static void _cascade_add_timer(
        struct timer_manager * manager, struct list_head * entry)
{
    int i;
    uint32_t mask = 0xfc000000, pos;
    struct timer_struct * timer;

    timer = list_entry(entry, struct timer_struct, node); 
    for (i = 3; i >= 0; i--) {
        pos = timer->expiration & mask;
        if (pos)
            break;
        mask >>= TVN_BITS;
    } 

    if (i < 0) {
        pos = timer->expiration & TVR_MASK;
        timer->expiration = 0;
        list_add(&(timer->node), &(manager->root_wheel.vec[pos]));
    } else {
        timer->expiration &= (~mask); 
        list_add(&(timer->node), &(manager->wheel[i].vec[pos]));
    }
}

static void _add_timer(struct timer_manager * manager, 
        struct timer_struct * timer)
{
    int i;
    uint32_t xor, mask = 0xfc000000, pos;

    assert(timer != NULL);
    assert(manager != NULL);

    _timing_wheel_update(manager);

    timer->expiration += manager->ticks;
    if (timer->expiration < manager->ticks)
        list_add(&(timer->node), &(manager->wheel[3].vec[0]));

    xor = timer->expiration ^ manager->ticks;
    for (i = 3; i >= 0; i--) {
        pos = xor & mask;
        if (pos)
            break;
        mask = 0xfc000000 | (mask >> TVN_BITS);
    } 

    if (i < 0) {
        pos = timer->expiration & TVR_MASK;
        timer->expiration = 0; 
        list_add(&(timer->node), &(manager->root_wheel.vec[pos]));
    } else {
        timer->expiration &= (~mask);
        list_add(&(timer->node), &(manager->wheel[i].vec[pos]));
    }
}

timer_id_t timer_manager_add_timer(
        struct timer_manager * manager, 
        timer_callback_t callback, 
        void * args, 
        uint32_t expiration, /* millisecond */ 
        int is_periodic)
{
    timer_id_t timer_id;
    struct timer_struct * timer;

    timer_id = ++manager->next_timer_id;
    if (!(timer = timer_create(callback, args, expiration, 
                    timer_id, is_periodic))) {
        log_error("Add timer error!");
        --manager->next_timer_id;
        return 0;
    }

    _add_timer(manager, timer);
    
    dict_add(manager->timer_dict, (void *)timer_id, (void *)timer);

    return timer_id;
}

void timer_manager_cancel_timer(struct timer_manager * manager, 
        timer_id_t timer_id) 
{
    struct timer_struct * timer;

    if (dict_find(manager->timer_dict, 
                (void *)timer_id, (void *)&timer) < 0)
        return; /* Not found */

    list_del(&timer->node);
    dict_delete(manager->timer_dict, (void *)timer_id);
    timer_destory(timer);
}

static void _timing_wheel_update(struct timer_manager * manager)
{
    uint32_t pos, i, ticks;
    struct list_head * list, * entry, * n;

    ticks = ++manager->ticks;
    if ((pos = ticks & TVR_MASK)) {
        list = &(manager->root_wheel.vec[pos]);
        list_splice(list, &(manager->expired_timers));
        return;
    }

    pos = ticks >> TVR_BITS;
    for (i = 0; i < 4; i++) {
        if (pos & TVN_MASK)
            break;
        pos >>= TVN_BITS;  
    }

    list = &(manager->wheel[i].vec[pos & TVN_MASK]);
    if (i == 4)
        list = &(manager->wheel[3].vec[0]);

    list_for_each_safe(entry, n, list) {
        list_del(entry);
        _cascade_add_timer(manager, entry);
    }

    list = &(manager->root_wheel.vec[0]);
    list_splice(list, &(manager->expired_timers));
}

void timer_manager_update_ticks(struct timer_manager * manager)
{
    double delta;
    uint32_t ticks;
    struct timespec current;

    assert(manager != NULL);

    clock_gettime(CLOCK_MONOTONIC, &current);
    delta = (manager->delta + current.tv_nsec - manager->last_update_time.tv_nsec) / 1000000;
    delta += (current.tv_sec - manager->last_update_time.tv_sec) * 1000;

    ticks = (uint32_t) delta;
    manager->delta = (delta - (double) ticks) * 1000000;

    while (ticks-- > 0)
        _timing_wheel_update(manager);

    manager->last_update_time = current;
}

void timer_manager_run_expired_timers(struct timer_manager * manager)
{
    struct timer_struct * timer;
    struct list_head * pos, * n, expired_timers;

    timer_manager_update_ticks(manager); 

    INIT_LIST_HEAD(&expired_timers);
    list_cut_position(&expired_timers, &manager->expired_timers,
            manager->expired_timers.prev);

    list_for_each(pos, &expired_timers) {
        timer = list_entry(pos, struct timer_struct, node);
        dict_delete(manager->timer_dict, (void *)timer->id);
    }

    list_for_each_safe(pos, n, &manager->expired_timers) {
        list_del(pos);

        timer = list_entry(pos, struct timer_struct, node);
        if (timer->interval) {
            /* periodic timer */
            timer->expiration = timer->interval;
            _add_timer(manager, timer);
        }

        /**
         * FIXME: 
         */
        timer->callback(timer->timer_id, timer->args);
        
        if (!timer->interval)
            timer_destory(timer);
    }
}
