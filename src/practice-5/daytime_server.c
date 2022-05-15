/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：daytime_server.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月11日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "../../lib/lib.h"

void daytime_server(void)
{
    time_t ticks;
    char buf[32];
    int listen_fd, conn_fd, port;
    struct sockaddr_in server_addr, client_addr;

    listen_fd = tcp_socket_ipv4();
    init_sockaddr_ipv4(&server_addr, ADDR_ANY_IPV4, 13);
    bind_ipv4(listen_fd, &server_addr);

    listen_ipv4(listen_fd, 5);
    for (;;) {
        conn_fd = accept_ipv4(listen_fd, &client_addr);
        sockaddr_to_ipport_ipv4(&client_addr, buf, &port);
        printf("Connection from IP: %s, Port: %d\n", buf, port);

        getsockname_ipv4(conn_fd, &server_addr);
        sockaddr_to_ipport_ipv4(&server_addr, buf, &port);
        printf("Local IP: %s, Port: %d\n", buf, port);

        ticks = time(NULL);
        snprintf(buf, sizeof(buf), "%.24s\r\n", ctime(&ticks));
        writen(conn_fd, buf, strlen(buf));
        close(conn_fd);
    }
}

int main(int argc, char ** argv)
{
    printf("===========================================\n");
    printf("Daytime-server starting...\n");

    daytime_server();

    printf("Daytime-server stopped...\n");
    printf("===========================================\n");
}

