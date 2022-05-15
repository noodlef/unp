/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：tests.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月05日
 * 描    述：
 * 
 *===============================================================
 */
#include <assert.h>
#include <stdio.h>

#include "timer_manager.h"

void callback(struct timer_struct * timer, void * args)
{
    printf("Timer [%ld], expiration: %d\n", (long)args, timer->expiration); 
}

int main(int argc, char ** argv)
{
    int i;
    struct timer_struct * timer;
    struct timer_manager * manager;

    printf("################## Tests begin...\n");
    manager = timer_manager_create();
    assert(manager != NULL);

    for (i = 0; i < 10; i++) {
       timer = timer_create(callback, ) 
    }

    timer_manager_destory(manager);
    printf("################## Tests end...\n");
    return 0;
}
