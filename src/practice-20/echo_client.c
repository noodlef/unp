/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：echo_client.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月11日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>

#include "../../lib/lib.h"

int echo_client(
        struct sockaddr_in * server_addr, struct sockaddr_in * client_addr)
{
    char buf[1024];
    int sockfd, n, len;
    int broadcast = 1;

    len = sizeof(struct sockaddr_in);
    sockfd = udp_socket_ipv4();

    if (client_addr)
        bind_ipv4(sockfd, client_addr);

    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, 
            &broadcast, sizeof(broadcast));

    while (1) {
        if (fgets(buf, sizeof buf, stdin) == NULL)
            break;

        sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)server_addr, len);
        if (n < 0) {
            log_error_sys("Sendto error!");
            continue;
        }

        n = recvfrom(sockfd, buf, sizeof buf, 0, 
                (struct sockaddr *)server_addr, (socklen_t *)&len);
        if (n < 0) {
            log_error_sys("Recvfrom error!");
            continue;
        }

        buf[n] = '\0';
        fputs(buf, stdout);
    }

    return 0;
}

int get_sockaddr_params(int argc, char ** argv,
        struct sockaddr_in ** server, struct sockaddr_in ** client);

int main(int argc, char ** argv)
{
    struct sockaddr_in server_addr, client_addr;
    struct sockaddr_in * server, * client;

    server = &server_addr;
    client = &client_addr;
    if (get_sockaddr_params(argc, argv, &server, &client) < 0)
        return EXIT_FAILURE;

    echo_client(server, client);

    return EXIT_SUCCESS; 
}
