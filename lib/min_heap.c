/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：min_heap.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月15日
 * 描    述：
 * 
 *===============================================================
 */
#include <assert.h>
#include <stdlib.h>

#include "log.h"

#include "min_heap.h"

struct heap_struct {
    struct heap_type * type;
    int len;
    int capacity;
    void ** vec;
};

struct heap_struct * heap_create(struct heap_type * type)
{
    struct heap_struct * heap;
    assert(type != NULL);    

    if (!(heap = malloc(sizeof(struct heap_struct)))) {
        log_error("Malloc error!"); 
        return NULL;
    }

    heap->len = 0;
    heap->capacity = 0;
    heap->type = type;
    heap->vec = NULL;

    return heap;
}

void heap_destory(struct heap_struct * heap)
{
    int i;

    assert(heap != NULL);    

    if (heap->type->val_destroy) {
        for (i = 0; i < heap->len; i++)
            heap->type->val_destroy(heap->vec[i]);
    }

    free(heap->vec);
    free(heap);
}

static int _resize(struct heap_struct * heap, int cnt)
{
    int i;

    if ((heap->capacity - heap->len) >= cnt)
        return 0;

    /* increased by the power of 2 */
    i = 2 * (heap->len + cnt);
    if (!(heap->vec = realloc(heap->vec, i * sizeof(void *)))) {
        log_error("Realloc error!");
        return -1; /* OOM */
    }

    heap->capacity = i;
    return 0;
}

static void _swap(void ** val1, void ** val2)
{
    void * tmp;

    tmp = *val1;
    *val1 = *val2;
    *val2 = tmp;
}

#define VALUE(idx) (heap->vec[idx - 1])
static void _shift_up(struct heap_struct * heap, int idx)
{
    int parent, child;

    child = idx;
    parent = idx / 2;
    while (child > 1) {
        /**
         * 如果当前节点的权重比其父节点的权重小则已经
         * 满足了大顶堆的性质, 否则需要‘上浮’
         */
        if (!heap->type->val_compare(VALUE(child), VALUE(parent)))
            break;

        _swap(&VALUE(child), &VALUE(parent));
        child = parent;
        parent = child / 2;
    }
}

static void _shift_down(struct heap_struct * heap, int idx)
{
    int parent, lchild, rchild, mark;

    parent = idx;
    lchild = 2 * parent;
    rchild = lchild + 1;
    mark = lchild;

    while (parent <= heap->len / 2) {
        /**
         * 找出当前节点与其左右子节点之间的最大值，如果当前 
         * 节点不是最大值，则需进行交换并‘下沉’ 
         */
        if (heap->type->val_compare(VALUE(lchild), VALUE(parent)))
            mark = lchild;
        else 
            mark = parent;

        if (rchild <= heap->len &&
                heap->type->val_compare(VALUE(rchild), VALUE(mark)))
            mark = rchild;

        if (mark == parent)
            break;

        _swap(&VALUE(mark), &VALUE(parent));
        parent = mark;
        lchild = parent * 2;
        rchild = lchild + 1;
    }
}

int heap_push(struct heap_struct * heap, void * val)
{
    assert(val != NULL);    
    assert(heap != NULL);    

    if (_resize(heap, 1) < 0) {
        log_error("OOM when heap push!");
        return -1;
    }

    heap->vec[heap->len++] = val;

    _shift_up(heap, heap->len);
    return 0;
}

void * heap_pop(struct heap_struct * heap)
{
    assert(heap != NULL);    

    if (!heap->len)
        return NULL;

    _swap(&VALUE(1), &VALUE(heap->len--));

    _shift_down(heap, 1);

    return VALUE(heap->len + 1);
}

void * heap_top(struct heap_struct * heap)
{
    assert(heap != NULL);    

    if (!heap->len)
        return NULL;

    return VALUE(1);
}

int heap_size(struct heap_struct * heap)
{
    return heap->len;
}

