/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：test_min_heap.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年03月18日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <stdlib.h>

#include "../lib/test.h"
#include "../lib/min_heap.h"

int int_compare(void * val1, void * val2)
{
    return (int)val1 > (int)val2 ? 1 : 0;
}

struct heap_type int_type = {
    .val_compare = int_compare,
    .val_destroy = NULL,
};

struct test_struct {
    int key;
    int val;
};

int struct_compare(void * val1, void * val2)
{
    return ((struct test_struct *)val1)->key < ((struct test_struct *)val2)->key 
        ? 1 : 0;
}

struct test_struct * new_test_struct(int key, int val)
{
    struct test_struct * test;

    test = malloc(sizeof(struct test_struct));
    test->key = key;
    test->val = val;
    return test;
}

struct heap_type struct_type = {
    .val_compare = struct_compare,
    .val_destroy = free,
};

int main(int argc, char ** argv)
{
    struct test_struct * test;
    struct heap_struct * int_heap, * struct_heap;

    printf("Test int type\n");
    int_heap = heap_create(&int_type);

    heap_push(int_heap, (void *)5);
    heap_push(int_heap, (void *)1);
    heap_push(int_heap, (void *)2);
    heap_push(int_heap, (void *)6);
    heap_push(int_heap, (void *)7);
    heap_push(int_heap, (void *)3);
    heap_push(int_heap, (void *)4);

    TEST_COND("Int heap size equals 7", heap_size(int_heap) == 7);
    
    TEST_COND("Int heap top; 7", (int)heap_pop(int_heap) == 7);
    TEST_COND("Int heap top; 6", (int)heap_pop(int_heap) == 6);
    TEST_COND("Int heap top; 5", (int)heap_pop(int_heap) == 5);
    TEST_COND("Int heap top; 4", (int)heap_pop(int_heap) == 4);
    TEST_COND("Int heap top; 3", (int)heap_pop(int_heap) == 3);
    TEST_COND("Int heap top; 2", (int)heap_pop(int_heap) == 2);
    TEST_COND("Int heap top; 1", (int)heap_pop(int_heap) == 1);

    heap_destory(int_heap);

    printf("\n\nTest struct type\n");
    struct_heap = heap_create(&struct_type);

    heap_push(int_heap, (void *)new_test_struct(7, 7));
    heap_push(int_heap, (void *)new_test_struct(1, 1));
    heap_push(int_heap, (void *)new_test_struct(2, 2));
    heap_push(int_heap, (void *)new_test_struct(6, 6));
    heap_push(int_heap, (void *)new_test_struct(4, 4));
    heap_push(int_heap, (void *)new_test_struct(4, 4));
    heap_push(int_heap, (void *)new_test_struct(5, 5));

    TEST_COND("Int heap size equals 7", heap_size(struct_heap) == 7);
    
    test = (struct test_struct *)heap_pop(struct_heap);
    TEST_COND("struct heap top; 1", test->val == 1);

    test = (struct test_struct *)heap_pop(struct_heap);
    TEST_COND("struct heap top; 2", test->val == 2);

    test = (struct test_struct *)heap_pop(struct_heap);
    TEST_COND("struct heap top; 4", test->val == 4);

    test = (struct test_struct *)heap_pop(struct_heap);
    TEST_COND("struct heap top; 4", test->val == 4);

    test = (struct test_struct *)heap_pop(struct_heap);
    TEST_COND("struct heap top; 5", test->val == 5);

    test = (struct test_struct *)heap_pop(struct_heap);
    TEST_COND("struct heap top; 6", test->val == 6);

    test = (struct test_struct *)heap_pop(struct_heap);
    TEST_COND("struct heap top; 7", test->val == 7);

    heap_destory(int_heap);
    TEST_REPORT();

    return 0;
}
