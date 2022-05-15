/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
  
 * 文件名称：thread_pool.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月24日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H

struct thread_pool;
typedef void (*task_func_t)(void *);

struct task_struct {
    task_func_t func;
    void * args;    
};

/**
 * return NULL if failed, otherwise at most 'size' number of 
 * threads will be created. note that no tasks will be handled 
 * utill 'thread_pool_run' is invoked.
 */
struct thread_pool * thread_pool_create(int size);

void thread_pool_run(struct thread_pool * pool);

/* append 'task' to the tail of the task_list */
int thread_pool_push(struct thread_pool * pool, struct task_struct * task);

/**
 * all remain tasks in pool will no longer be excuted, and active 
 * threads will exit safely when they finish it's task, but the 
 * resources allocated will not be released untill the last active 
 * thread exits. 
 */
void thread_pool_destroy(struct thread_pool * pool);

#endif //THREAD_POOL_H
