/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：timer.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月06日
 * 描    述：
 * 
 *===============================================================
 */
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>

#include "timer.h"

struct timer_struct * timer_create(timer_callback_t cb, 
        void * args, uint32_t expriation, timer_id_t id, int is_periodic) {
    struct timer_struct * timer;
    
    if (!(timer = malloc(sizeof(struct timer_struct))))
        return NULL;

    timer->args = args;
    timer->callback = cb;
    timer->expiration = expriation;
    timer->interval = 0;
    timer->id = id;
    if (is_periodic)
        timer->interval = expriation;

    return timer;
}

void timer_destory( struct timer_struct * timer) {
    assert(timer != NULL);

    if (timer->args)
        free(timer->args);

    free(timer);
}

