/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：fsync_utils.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月27日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __FSYNC_UTILS_H
#define __FSYNC_UTILS_H
#include "../lib/utils.h"

typedef int (*search_dir_callback) (char * fullpath, void * callback_args); 
int search_dir_recursively(char * dirpath, search_dir_callback callback, void * callback_args);

int compute_file_md5(int fd, unsigned char * md5_buf);

char * join_n_str(char * strs[]);

int create_file(char * dirpath, char * filename, mode_t mode);

int is_file_filtered(char * filepath);
#endif //FSYNC_UTILS_H
