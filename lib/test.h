/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：test.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年12月25日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __TEST_H
#define __TEST_H
#include <stdio.h>

/**
 * Examples:
 *   TEST_COND("Check if i == 0", (i == 0));
 *   TEST_COND("Check if i >= 0", (i >= 0));
 *   TEST_REPORT();
 */
#define __COLOR_NONE    "\033[0m"
#define __COLOR_RED     "\033[31m"
#define __COLOR_GREEN   "\033[32m"
#define __COLOR_YELLOW  "\033[33m"
#define __COLOR_BLUE    "\033[1;34m"

static int G_failed_tests = 0;
static int G_test_num = 0;

#define TEST_COND(desc, cond) \
do { \
    G_test_num++; \
    printf("- Case %d  %s: ", G_test_num, (desc)); \
    if (cond) { \
        printf(__COLOR_GREEN"PASSED\n\n"__COLOR_NONE); \
    } else { \
        printf(__COLOR_RED"FAILED\n\n"__COLOR_NONE); \
        G_failed_tests++; \
    } \
} while (0)

#define TEST_REPORT() \
do { \
    printf(__COLOR_BLUE"\n============== TEST RESULTS ===============\n"__COLOR_NONE); \
    printf("  %d test cases in total, " \
           __COLOR_GREEN"%d passed, "__COLOR_NONE \
           __COLOR_RED"%d failed\n\n"__COLOR_NONE, \
            G_test_num, G_test_num - G_failed_tests, G_failed_tests); \
    if (G_failed_tests) \
        printf(__COLOR_YELLOW"\n================= WARNING =================\n" \
               __COLOR_RED"  we have failed tests here...\n"__COLOR_NONE); \
} while (0)

#endif //TEST_H
