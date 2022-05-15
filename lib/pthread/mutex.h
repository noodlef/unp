/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：mutex.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月18日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __MUTEX_H
#define __MUTEX_H

#include <pthread.h>
#include <string.h>

#include "../log.h"

static inline int mutex_init(pthread_mutex_t * mutex, pthread_mutexattr_t * attr)
{
    int ret;

    if ((ret = pthread_mutex_init(mutex, attr)) != 0)
        log_error_sys("Init mutex failed!, errno: %s", strerror(ret));

    return ret;
}

static inline int mutex_destroy(pthread_mutex_t * mutex)
{
    int ret;

    if ((ret = pthread_mutex_destroy(mutex)) != 0)
        log_error_sys("Destroy mutex failed!, errno: %s", strerror(ret));

    return ret;
}

static inline int mutex_lock(pthread_mutex_t * mutex)
{
    int ret;

    if ((ret = pthread_mutex_lock(mutex)) != 0)
        log_error_sys("Lock mutex failed!, errno: %s", strerror(ret));

    return ret;
}

static inline int mutex_unlock(pthread_mutex_t * mutex)
{
    int ret;

    if ((ret = pthread_mutex_unlock(mutex)) != 0)
        log_error_sys("Unlock mutex failed!, errno: %s", strerror(ret));

    return ret;
}

static inline int mutex_trylock(pthread_mutex_t * mutex)
{
    int ret;

    if ((ret = pthread_mutex_trylock(mutex)) != 0)
        log_error_sys("Trylock mutex failed!, errno: %s", strerror(ret));

    return ret;
}
#endif //MUTEX_H
