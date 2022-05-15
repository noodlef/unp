/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：opt.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月26日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __OPT_H
#define __OPT_H
#include "../3rd/rbtree.h"

#define OPT_REQUIRED        (1 << 0)
#define OPT_OPTIONAL        (1 << 1)

#define OPT_VALUE_REUIRED   (1 << 2)
#define OPT_VAULE_OPTIONAL  (1 << 3)
#define OPT_VALUE_OMITTED   (1 << 4) 

#define OPT_TYPE_INT        (1 << 5)
#define OPT_TYPE_FLOAT      (1 << 6) 
#define OPT_TYPE_STRING     (1 << 7) 

#define OPT_NOT_FOUND       NULL 
#define OPT_NO_VALUE        ((void *) 1)

/**
 * long otions:
 *     required argument option: --arg=param or --arg param
 *     optional argument option: --arg=param
 * short options:
 *     required argument option: -dparam or -d param 
 *     optional argument option: -dparam 
 */
struct opt_def {
    char * long_opt;
    char * short_opt;
    unsigned int flags;
    char * desc;
};

typedef struct rb_root opt_map_t;

opt_map_t * get_opt_map(int argc, char ** argv, 
        struct opt_def * opt_defs, int len);

/**
 * OPT_NOT_FOUND: option is not found
 * OPT_NO_VALUE: no argument option
 */
char * get_opt_value(opt_map_t * opt_map, char * opt);

void free_opt_map(opt_map_t * opt_map);

void print_otions(opt_map_t * opt_map);

#endif //OPT_H
