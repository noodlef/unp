/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：echo_server.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月24日
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
    
    if (server_addr)
        bind_ipv4(sockfd, server_addr);

    if (client_addr)
        connect_ipv4(sockfd, client_addr);

    process(sockfd);

    return 0;
}

struct opt_def options[] = {
    {
        .long_opt = "server-ip",
        .short_opt = NULL,
        .flags = OPT_OPTIONAL | OPT_VALUE_REUIRED,
        .desc = "server addr"
    },
    {
        .long_opt = "server-port",
        .short_opt = NULL,
        .flags = OPT_OPTIONAL | OPT_VALUE_REUIRED,
        .desc = "server port"
    },
    {
        .long_opt = "client-ip",
        .short_opt = NULL,
        .flags = OPT_OPTIONAL | OPT_VALUE_REUIRED,
        .desc = "client addr"
    },
    {
        .long_opt = "client-port",
        .short_opt = NULL,
        .flags = OPT_OPTIONAL | OPT_VALUE_REUIRED,
        .desc = "client port"
    },
};


int main(int argc, char ** argv)
{
    uint16_t num;
    char * ip, * port;
    opt_map_t * opt_map;
    struct sockaddr_in * server_addr, * client_addr;
    struct sockaddr_in  server, client;

    opt_map = get_opt_map(argc, argv, options, 
            sizeof(options) / sizeof(struct opt_def));

    server_addr = NULL;
    client_addr = NULL;

    ip = get_opt_value(opt_map, "server-ip");
    port = get_opt_value(opt_map, "server-port");
    if (ip != OPT_NOT_FOUND || port != OPT_NOT_FOUND) {
        server_addr = &server;
        if (ip == OPT_NOT_FOUND)
            ip = (char *)ADDR_ANY_IPV4;
        if (port == OPT_NOT_FOUND)
            num = PORT_ANY_IPV4;
        else 
            num = atoi(port);
        init_sockaddr_ipv4(server_addr, ip, num);
    }

    ip = get_opt_value(opt_map, "client-ip");
    port = get_opt_value(opt_map, "client-port");
    if (ip != OPT_NOT_FOUND || port != OPT_NOT_FOUND) {
        client_addr = &client;
        if (ip == OPT_NOT_FOUND)
            ip = (char *)ADDR_ANY_IPV4;
        if (port == OPT_NOT_FOUND)
            num = PORT_ANY_IPV4;
        else 
            num = atoi(port);
        init_sockaddr_ipv4(server_addr, ip, num);
    }

    daemon_init();

    printf("==================================================\n");
    printf("Starting the echo server...\n");

    echo_server(server_addr, client_addr);

    printf("Echo server has been terminated!\n");
    printf("==================================================\n");

    return 0;
}

