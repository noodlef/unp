/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：ifinfo.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月10日
 * 描    述：
 * 
 *===============================================================
 */
#include "../../lib/lib.h"

int main(int argc, char ** argv)
{
    struct if_info * if_info;

    if_info = get_if_info();

    return EXIT_SUCCESS;
}
