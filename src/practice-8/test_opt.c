/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：test_opt.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月26日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>

#include "../../lib/opt.h"

int main(int argc, char ** argv)
{
    struct opt_def options[] = {
        {
            .long_opt = "help",
            .short_opt = "h",
            .flags = OPT_OPTIONAL | OPT_VALUE_OMITTED,
            .desc = "null"
        },
        {
            .long_opt = NULL, 
            .short_opt = "c",
            .flags = OPT_OPTIONAL | OPT_VALUE_OMITTED,
            .desc = "null"
        },
        {
            .long_opt = NULL, 
            .short_opt = "d",
            .flags = OPT_OPTIONAL | OPT_VALUE_REUIRED,
            .desc = "null"
        },
        {
            .long_opt = NULL,
            .short_opt = "e",
            .flags = OPT_OPTIONAL | OPT_VAULE_OPTIONAL,
            .desc = "null"
        },

        {
            .long_opt = "count", 
            .short_opt = NULL, 
            .flags = OPT_OPTIONAL | OPT_VALUE_OMITTED,
            .desc = "null"
        },
        {
            .long_opt = "ip", 
            .short_opt = NULL,
            .flags = OPT_OPTIONAL | OPT_VALUE_REUIRED,
            .desc = "null"
        },
        {
            .long_opt = "port",
            .short_opt = NULL,
            .flags = OPT_OPTIONAL | OPT_VAULE_OPTIONAL,
            .desc = "null"
        },
    };

    opt_map_t * opt_map = get_opt_map(argc, argv, options, 
            sizeof options / sizeof(struct opt_def));

    if (!opt_map)
        return 1;

    print_otions(opt_map);

    return 0;
}
