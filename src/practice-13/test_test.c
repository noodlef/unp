/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：test_test.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年12月25日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <unistd.h> #include "../../lib/utils.h"

int main(int argc, char ** argv)
{
    int ret;

    ret = write_pid_file("/var/run/test.pid");
    if (ret < 0)
        return 1;

    while(1)
        sleep(60);
}
