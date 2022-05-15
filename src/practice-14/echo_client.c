/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：echo_client.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月07日
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

int get_sockaddr_in(int argc, char ** argv,
        struct sockaddr_in * server, struct sockaddr_in * client);

static void sig_alarm(int signo)
{
    return;
}

int echo_client(struct sockaddr_in * server_addr, struct sockaddr_in * client_addr)
{
    int sockfd, n;
    char buf[1024];

    signal_act(SIGALRM, sig_alarm);

    sockfd = udp_socket_ipv4();

    bind_ipv4(sockfd, client_addr);

    connect_ipv4(sockfd, server_addr);

    while (1) {
        if (fgets(buf, sizeof buf, stdin) == NULL)
            break;

        sendto(sockfd, buf, strlen(buf), 0, NULL, 0);

        alarm(5);
        n = recvfrom(sockfd, buf, sizeof buf, 0, NULL, 0);
        
        if (n < 0)
            continue;

        alarm(0);
        buf[n] = '\0';
        fputs(buf, stdout);
    }


    return 0;
}

int main(int argc, char ** argv)
{
    struct sockaddr_in server_addr, client_addr;

    if (get_sockaddr_in(argc, argv, &server_addr, &client_addr) < 0)
        return EXIT_FAILURE;


    echo_client(&server_addr, &client_addr);

    return 0;
}
