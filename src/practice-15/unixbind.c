/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：unixbind.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月07日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <unistd.h>

#include "../../lib/lib.h"

int main(int argc, char ** argv)
{
    int sockfd, len;
    char * pathname;
    struct sockaddr_un addr, addr1;

    if (argc != 2) {
        printf("usage: unixbind <pathname>\n");
        return EXIT_FAILURE;
    }

    pathname = argv[1];
    sockfd = unix_stream_socket();

    unlink(pathname);

    unix_init_sockaddr(&addr, pathname);
    unix_bind(sockfd, &addr);

    len = sizeof(addr1);
    getsockname(sockfd, (struct sockaddr *)&addr1, (socklen_t *)&len);
    printf("bound name = %s, returned len = %d\n", addr1.sun_path, len); 

    return EXIT_SUCCESS;
}
