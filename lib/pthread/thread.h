/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：thread.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月20日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __THREAD_H
#define __THREAD_H

#include <pthread.h>

typedef void *(*th_func_t) (void *);

struct thread_struct;

struct thread_struct * thread_create(
        th_func_t func, void * args, pthread_attr_t * attr);

void thread_run(struct thread_struct * th);

void thread_exit(void * exit_code);

int thread_join(struct thread_struct * th, void ** retval);

int thread_cancel(struct thread_struct * th);

void thread_destroy(struct thread_struct * th);

int thread_detach(struct thread_struct * th);

inline struct thread_struct * thread_current(void);

#endif //THREAD_H
