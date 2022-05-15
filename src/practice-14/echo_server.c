/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：echo_server.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月06日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "../../lib/lib.h"

static char * G_log_file = "/log/echo_server.log";

int get_sockaddr_in(int argc, char ** argv,
        struct sockaddr_in * server, struct sockaddr_in * client);

void process(int conn_fd)
{
    int n;
    uint16_t port;
    char buf[1024];
    char ip[32];
    struct sockaddr_in client_addr;
    socklen_t addr_len;

    addr_len = sizeof(struct sockaddr_in);

    while (1) {
        n = recvfrom(conn_fd, buf, sizeof buf, 0, 
                (struct sockaddr *)&client_addr, &addr_len);
        if (n < 0) {
            log_warning_sys("Recv error!");
            continue;
        }

        sockaddr_to_ipport_ipv4(&client_addr, ip, &port);
        log_info("Recv from IP: %s, port: %d", ip, port);

        sleep(8);
        n = sendto(conn_fd, buf, n, 0, 
                (struct sockaddr *)&client_addr, addr_len);
        if (n < 0)
            log_warning_sys("Sendto error!");
    }
}

int echo_server(struct sockaddr_in * server_addr, 
        struct sockaddr_in * client_addr)
{
    int sockfd;

    if ((sockfd = udp_socket_ipv4()) < 0) {
        log_error("Get socket failed");
        return -1;
    }
    
    bind_ipv4(sockfd, server_addr);

    connect_ipv4(sockfd, client_addr);

    process(sockfd);

    return 0;
}

int main(int argc, char ** argv)
{
    struct sockaddr_in server_addr, client_addr;

    if (get_sockaddr_in(argc, argv, &server_addr, &client_addr) < 0)
        return EXIT_FAILURE;

    daemon_init();

    if (init_log(G_log_file, 1) < 0) {
        printf("Init log failed!\n");
        return EXIT_FAILURE;
    }

    set_proctitile(argv, "echo-server");

    log_info("==================================================\n");
    log_info("Starting the echo server...\n");

    echo_server(&server_addr, &client_addr);

    log_info("Echo server has been terminated!\n");
    log_info("==================================================\n");

    return 0;
}
