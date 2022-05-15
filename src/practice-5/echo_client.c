/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：echo_client.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月13日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../../lib/lib.h"

void process_connection(int conn_fd)
{
    int n;
    char buf[1024];

again:    
    if (fgets(buf, sizeof buf, stdin) == NULL) {
        log_warning("Fgets: input error!");
        return;
    }

    if (writen(conn_fd, buf, strlen(buf)) < 0)
        return;

    if ((n = read(conn_fd, buf, sizeof buf)) < 0) {
        log_warning("Read error!");
        return;
    }

    buf[n] = '\0';
    if (fputs(buf, stdout) == EOF) {
        log_warning("Fputs: input error!");
        return;
    }
    goto again;
}

int echo_client(const char * ip, int port)
{
    int conn_fd;
    struct sockaddr_in server_addr;

    conn_fd = tcp_socket_ipv4();

    init_sockaddr_ipv4(&server_addr, ip, port);
    connect_ipv4(conn_fd, &server_addr);

    process_connection(conn_fd);

    return 0;
}

int main(int argc, char ** argv)
{
    int port;
    const char * ip;

    ip = ADDR_ANY_IPV4;
    port = 0; /* decided by the system */
    if (argc >= 3) {
        ip = argv[1];
        port = atoi(argv[2]);
    } else if (argc == 2) {
        ip = argv[1];
    }

    echo_client(ip, port);

    return 0;
}

