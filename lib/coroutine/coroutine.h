/**
 ===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：coroutine.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年08月31日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __COROUTINE_H
#define __COROUTINE_H

#define __COROUTINE_DEBUG

#define COROUTINE_READY    0
#define COROUTINE_RUNNING  1
#define COROUTINE_STOPPED  2
#define COROUTINE_CANCELED 3
#define COROUTINE_DEAD     4

struct coroutine_struct;

typedef int cid_t;
typedef void (* coroutine_func_t)(void * args);

typedef struct {
    cid_t parent; 
} coroutine_attr_t;

cid_t coroutine_create(
        coroutine_func_t run, void * args, coroutine_attr_t * attr);

int coroutine_switch(cid_t cid);

int coroutine_cancel(cid_t cid);

cid_t coroutine_get_current(void);

#ifdef __COROUTINE_DEBUG
void print_coroutines(void);
#endif

#endif //COROUTINE_H
