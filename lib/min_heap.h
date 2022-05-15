/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：min_heap.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月15日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __MIN_HEAP_H
#define __MIN_HEAP_H

struct heap_struct;
typedef void (* val_destroy_t)(void * val);
typedef int (* val_compare_t)(void * val1, void * val2);

struct heap_type {
    val_compare_t val_compare;
    val_destroy_t val_destroy;
};

struct heap_struct * heap_create(struct heap_type * type);

void heap_destory(struct heap_struct * heap);

int heap_push(struct heap_struct * heap, void * val);

void * heap_pop(struct heap_struct * heap);

void * heap_top(struct heap_struct * heap);

int heap_size(struct heap_struct * heap);

#endif //MIN_HEAP_H
