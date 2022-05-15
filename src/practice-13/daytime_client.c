/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：daytime_client.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月06日
 * 描    述：
 * 
 *===============================================================
 */
#include <unistd.h>
#include <stdio.h>

#include "../../lib/lib.h"

int get_sockaddr_in(int argc, char ** argv,
        struct sockaddr_in * server, struct sockaddr_in * client);

int main(int argc, char ** argv)
{
    int i, sockfd;
    char buf[1024];
    struct sockaddr_in local_addr, remote_addr;

    if (get_sockaddr_in(argc, argv, &remote_addr, &local_addr) < 0)
        return EXIT_FAILURE;

    sockfd = udp_socket_ipv4();
    if (sockfd < 0)
        return EXIT_FAILURE;

    if (bind_ipv4(sockfd, &local_addr) < 0)
        return EXIT_FAILURE;

    if (connect_ipv4(sockfd, &remote_addr) < 0)
        return EXIT_FAILURE;

    buf[0] = '\n';
    write(sockfd, buf, 1);

    i = read(sockfd, buf, sizeof(buf) - 1);
    buf[i] = '\0';
    printf("From daytime-server: %s\n", buf);

    return EXIT_SUCCESS;
}

