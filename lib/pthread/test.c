/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：test.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月11日
 * 描    述：
 * 
 *===============================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread.h"
#include "thread_pool.h"

void do_task(void * args)
{
    long i;
    struct thread_struct * current;

    i = (long) args;
    current = thread_current();
    printf("Task %ld handled by thread %p...\n", i, current);
    sleep(10);
}

int main(int argc, char ** argv)
{
    int i;
    struct thread_pool * pool;
    struct task_struct * task;

    pool = thread_pool_create(3);
    thread_pool_run(pool);

    for (i = 0; i < 6; i++) {
        task = malloc(sizeof(struct task_struct));
        task->args = (void *)i;
        task->func = do_task;
        thread_pool_push(pool, task);
    }

    sleep(10);
    thread_pool_destroy(pool);
    sleep(60);

    return 0;
}

