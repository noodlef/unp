/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：test_ping.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月16日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>

extern void ping(char * dst_ip);

int main(int argc, char ** argv)
{
    char * dst_ip;
    
    if (argc != 2) {
        printf("Usage: ping <dst-ip>\n");
        return 1;
    }

    dst_ip = argv[1];
    ping(dst_ip);

    return 0;
}
