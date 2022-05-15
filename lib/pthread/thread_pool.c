/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：thread_pool.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月22日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdlib.h>

#include "cond.h"
#include "mutex.h"
#include "thread.h"
#include "../log.h"
#include "../../3rd/list.h"
#include "thread_pool.h"

#define POOL_STATUS_READY   0
#define POOL_STATUS_RUNNING 1
#define POOL_STATUS_EXITED  2

struct thread_pool {
    int active_cnts; /* active thread cnts */
    int status;
    pthread_mutex_t lock;
    pthread_cond_t is_empty;
    struct list_head thread_list;
    struct list_head task_list;
};

struct _thread_node {
    struct thread_struct * thread;
    struct list_head node;
};

struct _task_node {
    struct task_struct * task;
    struct list_head node;
};

struct _thread_args {
    struct thread_pool * pool;
    struct _thread_node * thread_node;
};

static void _cleanup(void * arg)
{
    int free_pool = 0;
    struct thread_pool * pool;
    struct _thread_node * thread_node;
    struct _thread_args * thread_args;

    thread_args = (struct _thread_args *) arg;
    thread_node = thread_args->thread_node; 
    pool = thread_args->pool;

    mutex_lock(&(pool->lock));

    list_del(&(thread_node->node));
    if (pool->status == POOL_STATUS_EXITED 
            && list_empty(&pool->thread_list))
        free_pool = 1;

    mutex_unlock(&(pool->lock));

    free(thread_args);
    thread_destroy(thread_node->thread);
    free(thread_node);

    if (free_pool) {
        mutex_destroy(&(pool->lock));
        cond_destroy(&(pool->is_empty));
        free(pool);
    }
}

static void * _main(void * arg)
{
    struct thread_pool * pool;
    struct _task_node * task_node;
    struct _thread_args * thread_args;

    thread_args = (struct _thread_args *) arg;
    pool = thread_args->pool;

    pthread_cleanup_push(_cleanup, thread_args);

    while (pool->status == POOL_STATUS_RUNNING) {
        mutex_lock(&(pool->lock));

        if (pool->status != POOL_STATUS_RUNNING) {
            mutex_unlock(&(pool->lock));
            break;
        }

        while (list_empty(&(pool->task_list))) {
            cond_wait(&(pool->is_empty), &(pool->lock));
            if (pool->status != POOL_STATUS_RUNNING) {
                mutex_unlock(&(pool->lock));
                break;
            }
        }
        task_node = list_first_entry(
                &(pool->task_list), struct _task_node, node);
        list_del(&(task_node->node));

        mutex_unlock(&(pool->lock));

        task_node->task->func(task_node->task->args);

        /**
         * safe to cancel while we are not holding the lock
         */
        pthread_testcancel();
    }
    pthread_cleanup_pop(1);

    return (void *)0;
}

struct thread_pool * thread_pool_create(int size)
{
    int i;
    struct thread_pool * pool;
    struct thread_struct * thread;
    struct _thread_node * thread_node;
    struct _thread_args * thread_args;

    if (!(pool = (struct thread_pool *) malloc(
                    sizeof(struct thread_pool)))) {
        log_error("Malloc error!, thread_pool.");
        return NULL;
    }

    pool->active_cnts = 0;
    pool->status = POOL_STATUS_READY;
    INIT_LIST_HEAD(&(pool->thread_list));
    INIT_LIST_HEAD(&(pool->task_list));

    if (mutex_init(&(pool->lock), NULL)) {
        free(pool);
        return NULL;
    }

    if (cond_init(&(pool->is_empty), NULL)) {
        mutex_destroy(&(pool->lock));
        free(pool);
        return NULL;
    }

    for (i = 0, thread_args = NULL, thread_node = NULL; i < size; i++) {
        if (!thread_node) {
            thread_node = (struct _thread_node *) malloc(
                    sizeof(struct _thread_node));
            if (!thread_node) {
                log_error("Malloc error!, _thread_node");
                continue;
            }
        }

        if (!thread_args) {
            thread_args = (struct _thread_args *) malloc(
                    sizeof(struct _thread_args));
            if (!thread_args) {
                log_error("Malloc error!, _thread_args");
                continue;
            }
        }
        thread_args->pool = pool;
        thread_args->thread_node = thread_node;

        if (!(thread = thread_create(_main, thread_args, NULL)))
            continue;

        thread_detach(thread);

        thread_node->thread = thread;
        list_add(&(thread_node->node), &(pool->thread_list));
        ++pool->active_cnts;
        thread_node = NULL;
        thread_args = NULL;
    }

    return pool;
}


int thread_pool_push(struct thread_pool * pool, struct task_struct * task)
{

    struct _task_node * task_node;

    task_node = (struct _task_node *) malloc(sizeof(struct _task_node));
    if (!task_node) {
        log_error("Malloc error!, task_node.");
        return -1;
    }
    task_node->task = task;

    mutex_lock(&(pool->lock));

    list_add_tail(&(task_node->node), &(pool->task_list));

    mutex_unlock(&(pool->lock));
    cond_signal(&(pool->is_empty));

    return 0;
}

void thread_pool_destroy(struct thread_pool * pool)
{
    struct list_head * pos, * n;
    struct _task_node * task_node;

    if (!pool || pool->status == POOL_STATUS_EXITED)
        return;

    mutex_lock(&(pool->lock));

    pool->status = POOL_STATUS_EXITED;

    list_for_each_safe(pos, n, &(pool->task_list)) {
        list_del(pos);
        task_node = list_entry(pos, struct _task_node, node);
        free(task_node->task);
        free(task_node);
    }

    mutex_unlock(&(pool->lock));
    cond_broadcast(&(pool->is_empty));
}

void thread_pool_run(struct thread_pool * pool)
{
    struct list_head * pos;
    struct _thread_node * thread_node;

    mutex_lock(&(pool->lock));
    if (pool->status == POOL_STATUS_RUNNING)
        return;
    pool->status = POOL_STATUS_RUNNING;
    mutex_unlock(&(pool->lock));

    list_for_each(pos, &(pool->thread_list)) {
        thread_node = list_entry(pos, struct _thread_node, node);
        thread_run(thread_node->thread);
    }
}
