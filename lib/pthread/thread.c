/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：thread.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月18日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include "thread.h"
#include "cond.h"
#include "mutex.h"
#include "../log.h"

#define TH_READY    0
#define TH_RUNNING  1
#define TH_EXITED   2

#define F_FREE_MEM  (1 << 0)

struct thread_struct {
    th_func_t run;
    void * args;
    pthread_t tid;    
    int status;
    int flags;
    sem_t running;
    pthread_mutex_t lock;
};

static pthread_key_t G_current_key;
static pthread_once_t G_init_flag = PTHREAD_ONCE_INIT;

static void _init(void)
{
    log_info("Phthread init.");
    pthread_key_create(&G_current_key, NULL);
}

static void _cleanup(void * arg)
{
    struct thread_struct * th;

    th = (struct thread_struct *)arg;
    if (!th)
        return;

    mutex_lock(&(th->lock));
    th->status = TH_EXITED;
    mutex_unlock(&(th->lock));

    sem_destroy(&(th->running));
    mutex_destroy(&(th->lock));

    if (th->flags & F_FREE_MEM)
        free(th);
}

static void * _main(void * args)
{
    void * exit_code;
    struct thread_struct * th;

    th = (struct thread_struct *)args;

    pthread_once(&G_init_flag, _init);
    pthread_setspecific(G_current_key, th);

    pthread_cleanup_push(_cleanup, th);

    sem_wait(&(th->running));

    log_info("Pthread(%ld) start running.", th->tid);

    pthread_testcancel();
    exit_code = (th->run)(th->args);
    pthread_testcancel();

    log_info("Pthread(%ld) exit with code: %ld.", 
            th->tid, (long)exit_code);

    pthread_cleanup_pop(1);

    return exit_code;
}

struct thread_struct * thread_create(
        th_func_t func, void * args, pthread_attr_t * attr)
{
    struct thread_struct * th;

    if (!(th = malloc(sizeof(struct thread_struct)))) {
        log_error("Malloc error!");
        return NULL;
    }

    th->args = args;
    th->run = func;
    th->status = TH_READY;
    th->flags = 0;

    if (sem_init(&(th->running), 0, 0) < 0) {
        log_error("Sem init error!.");
        return NULL;
    }

    if (mutex_init(&(th->lock), NULL)) {
        sem_destroy(&(th->running));
        free(th);
        return NULL;
    }

    if ((pthread_create(&(th->tid), attr, _main, th))) {
        sem_destroy(&(th->running));
        mutex_destroy(&(th->lock));
        free(th);
        return NULL;
    }
    log_info("Create pthread (%ld) success.", (long)th->tid);

    return th;
}

void thread_run(struct thread_struct * th)
{
    mutex_lock(&(th->lock));
    if (th->status == TH_READY)
        th->status = TH_RUNNING;
    mutex_unlock(&(th->lock));

    sem_post(&(th->running));
}

void thread_exit(void * exit_code)
{
    pthread_t current_tid;

    current_tid = pthread_self();
    log_info("Pthread (%ld) call exit.", current_tid);

    pthread_exit(exit_code);
}

int thread_join(struct thread_struct * th, void ** retval)
{
    int ret; 

    if ((ret = pthread_join(th->tid, retval)))
        log_info("Pthread join error, code: %d", ret);

    return ret;
}

int thread_cancel(struct thread_struct * th)
{
    int ret;

    if (th->status == TH_EXITED)  {
        log_warning("Pthread alreay exited, no need to cancel.");
        return 0;
    }

    if (th->status == TH_READY)
        thread_run(th);

    log_info("Pthread (%ld) call cancel.", th->tid);
    if ((ret = pthread_cancel(th->tid))) {
        log_info("Pthread cancel error, code: %d", ret);
    }

    return ret;
}

void thread_destroy(struct thread_struct * th)
{
    if (!th)
        return; 

    if (th->status != TH_EXITED) {
        log_warning("Destroy active pthread(%ld), status: %d.", 
                th->tid, th->status);
        th->flags |= F_FREE_MEM; /* call free() in cleanup routine */
        thread_cancel(th);
        return;
    }

    free(th);
}

int thread_detach(struct thread_struct * th)
{
    int ret; 

    if ((ret = pthread_detach(th->tid)))
        log_info("Pthread detach error, code: %d", ret);

    return ret;
}

inline struct thread_struct * thread_current(void)
{
    return (struct thread_struct *) pthread_getspecific(G_current_key);
}
