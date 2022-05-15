/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：test.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月02日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>

#include "coroutine.h"

cid_t co1, co2;
void work1(void * args)
{
    int i = 0;

    for (; i < 5; i++) {
        printf("work1: ...%d\n", i);
        coroutine_switch(co2);
    }
}

void work2(void * args)
{
    int i = 0;

    for (; i < 5; i++) {
        printf("work2: ...%d\n", i);
        coroutine_switch(co1);
    }
}

int main(int argc, char ** argv)
{
    printf("Test begin...\n");

    co1 = coroutine_create(work1, NULL, NULL);
    co2 = coroutine_create(work2, NULL, NULL);
    coroutine_switch(co1);

    printf("Test end...\n");

    return 0;
}

