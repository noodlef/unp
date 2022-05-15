/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：cond.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月19日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __COND_H
#define __COND_H

#include <time.h>
#include <pthread.h>
#include <string.h>

#include "../log.h"

static inline int cond_init(pthread_cond_t * cond, pthread_condattr_t * attr)
{
    int ret;

    if ((ret = pthread_cond_init(cond, attr)) != 0)
        log_error_sys("Init cond failed!, errno: %s", strerror(ret));

    return ret;
}

static inline int cond_destroy(pthread_cond_t * cond)
{
    int ret;

    if ((ret = pthread_cond_destroy(cond)) != 0)
        log_error_sys("Destroy cond failed!, errno: %s", strerror(ret));

    return ret;
}

static inline int cond_wait(pthread_cond_t * cond, pthread_mutex_t * mutex)
{
    int ret;

    if ((ret = pthread_cond_wait(cond, mutex)) != 0)
        log_error_sys("Cond wait failed!, errno: %s", strerror(ret));

    return ret;
}

static inline int cond_timewait(pthread_cond_t * cond, 
        pthread_mutex_t * mutex, struct timespec * spec)
{
    int ret;

    if ((ret = pthread_cond_timedwait(cond, mutex, spec)) != 0)
        log_error_sys("Cond timewait failed!, errno: %s", strerror(ret));

    return ret;
}

static inline int cond_signal(pthread_cond_t * cond)
{
    int ret;

    if ((ret = pthread_cond_signal(cond)) != 0)
        log_error_sys("Cond signal failed!, errno: %s", strerror(ret));

    return ret;
}

static inline int cond_broadcast(pthread_cond_t * cond)
{
    int ret;

    if ((ret = pthread_cond_broadcast(cond)) != 0)
        log_error_sys("Cond broadcast failed!, errno: %s", strerror(ret));

    return ret;
}
#endif //COND_H
