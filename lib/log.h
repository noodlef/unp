/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：log.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月12日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __LOG_H
#define __LOG_H

#define LOG_WITH_PREFIX 1

#define LOG_DEBUG   0
#define LOG_INFO    1
#define LOG_WARN    2
#define LOG_ERROR   3
#define LOG_FATAL   4

#define log_debug(fmt, ...) \
    log_to(LOG_DEBUG, 0, 1, __FILE__, __LINE__, __func__, fmt"\n", ##__VA_ARGS__)
#define log_info(fmt, ...) \
    log_to(LOG_INFO, 0, 1,  __FILE__, __LINE__, __func__, fmt"\n", ##__VA_ARGS__)
#define log_warning(fmt, ...) \
    log_to(LOG_WARN, 0, 1, __FILE__, __LINE__, __func__, fmt"\n", ##__VA_ARGS__)
#define log_error(fmt, ...) \
    log_to(LOG_ERROR, 0, 1, __FILE__, __LINE__, __func__, fmt"\n", ##__VA_ARGS__)
#define log_fatal(fmt, ...) \
    log_to(LOG_FATAL, 0, 1, __FILE__, __LINE__, __func__, fmt"\n", ##__VA_ARGS__)

#define log_warning_sys(fmt, ...) \
    log_to(LOG_WARN, 1, 1, __FILE__, __LINE__, __func__, fmt"\n", ##__VA_ARGS__)
#define log_error_sys(fmt, ...) \
    log_to(LOG_ERROR, 1, 1, __FILE__, __LINE__, __func__, fmt"\n", ##__VA_ARGS__)
#define log_fatal_sys(fmt, ...) \
    log_to(LOG_FATAL, 1, 1, __FILE__, __LINE__, __func__, fmt"\n", ##__VA_ARGS__)

#define die(fmt, ...) \
do { \
    log_to(LOG_ERROR, 0, 1, __FILE__, \
                       __LINE__,  __func__, fmt"\n", ##__VA_ARGS__); \
    exit(1); \
} while (0)

#define die_with_errno(fmt, ...) \
do { \
    log_to(LOG_ERROR, 1, 1, __FILE__, \
                       __LINE__,  __func__, fmt"\n", ##__VA_ARGS__); \
    exit(1); \
} while (0)

/* no warnings for casting to void * from int */
#define INT2VOIDP(i) ((void *)(size_t)(i))

/* no warnings for casting to int from void * */
#define VOIDP2INT(i) ((intptr_t)(i))

void set_log_level(int level);

int init_log(char * log_file, int log_with_prefix);

void log_to(
        int log_level, 
        int log_with_errno, 
        int log_with_prefix, 
        const char * file_name, 
        int file_no, 
        const char * func_name, 
        const char * fmt, 
        ...);

void dump_trace(unsigned char max_depth);
#endif //LOG_H
