/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：timer.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月26日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __TIMER_H
#define __TIMER_H

#include <inttypes.h>

#include "../../3rd/list.h"

struct timer_struct;
typedef uint32_t timer_id_t;
typedef void (*timer_callback_t)(timer_id_t timer_id, void * args);

struct timer_struct {
    timer_id_t id;
    void * args;
    uint32_t interval; /* only used for periodic timers */
    timer_callback_t callback; 
    uint32_t expiration; /* milliseconds */ 
    struct list_head node; 
};

struct timer_struct * timer_create(timer_callback_t cb, 
        void * args, uint32_t expriation, timer_id_t id, int is_periodic);

void timer_destory(struct timer_struct * timer);

#endif //TIMER_H
