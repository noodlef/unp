/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：echo_server.c
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
#include <errno.h>

#include "../../lib/lib.h"

void process(int conn_fd)
{
    int n;
    uint16_t port;
    char buf[1024];
    char ip[32];
    struct sockaddr_in * srcaddr, * dstaddr;
    struct msg_info msg_info;
    socklen_t addr_len;

    addr_len = sizeof(struct sockaddr_in);

    msg_info.flags = 0;
    msg_info.recvbuf = buf;
    msg_info.bufsize = sizeof buf;

    while (1) {
        n = recvmsginfo(conn_fd, &msg_info);
        /*
        n = recvfrom(conn_fd, buf, sizeof buf, 0, 
                (struct sockaddr *)&client_addr, &addr_len);
         */
        if (n < 0) {
            log_warning_sys("Recv error!");
            continue;
        }

        dstaddr = &msg_info.dstaddr;
        srcaddr = &msg_info.srcaddr;
        sockaddr_to_ipport_ipv4(srcaddr, ip, &port);
        log_info("Recv from src IP: %s, port: %d", ip, port);

        sockaddr_to_ipport_ipv4(dstaddr, ip, &port);
        log_info("Recv from dst IP: %s, port: %d", ip, port);

        n = sendto(conn_fd, buf, n, 0, 
                (struct sockaddr *)srcaddr, addr_len);
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
    
    if (server_addr)
        bind_ipv4(sockfd, server_addr);

    if (client_addr)
        connect_ipv4(sockfd, client_addr);

    process(sockfd);

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

    daemon_init();

    init_log("/log/udp-echo-server.log", 1);

    set_proctitile(argv, "udp-echo-server");

    log_info("==================================================");
    log_info("Starting the echo server...");

    echo_server(server, client);

    log_info("Echo server has been terminated!");
    log_info("==================================================");

    return 0;
}
