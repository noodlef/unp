/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：timer_manager.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月27日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __TIMER_MANAGER_H
#define __TIMER_MANAGER_H

#include <inttypes.h>

#include "timer.h"

/**
 * Return an new timer-manager instance on success, otherwise 'NULL'
 * is retured. 
 */
struct timer_manager * timer_manager_create(void);

/**
 * Destory the timer-manager instance. basically, it will first destory 
 * all timers it has(note that expried timers will not be handled),
 * and then destory itself. 
 */
void timer_manager_destory(struct timer_manager * manager);

/**
 * 'timer_id' on success, 0 on failure. 
 */
timer_id_t timer_manager_add_timer(
        struct timer_manager * manager, 
        timer_callback_t callback, 
        void * args, 
        uint32_t expiration, /* millisecond */ 
        int is_periodic);

/**
 * can not fail, note that running timers can not be canceled 
 */
void timer_manager_cancel_timer(struct timer_manager * manager, 
        timer_id_t timer_id);

/**
 * invoke all expired timers 
 *
 * Called from the timer interrupt handler to update the current 
 * ticks, better to be invoked every millisecond(can achieve a 
 * timing accuracy of 1 ms)
 */
void timer_manager_run_expired_timers(struct timer_manager * manager);

#endif //TIMER_MANAGER_H
