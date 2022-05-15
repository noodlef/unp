/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：echo_server.c
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
#include <wait.h>

#include "../../lib/lib.h"

void sig_child(int signo)
{
    int stat, pid;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        log_warning("Child(%d) terminated by signal(%d)",
                pid, signo);

    return;
}

void process_connection(int conn_fd)
{
    int n;
    char buf[1024];

again:
    while ((n = read(conn_fd, buf, sizeof buf)) > 0) {
        if (writen(conn_fd, buf, n) < 0)
            return;
    }

    if (n < 0 && errno == EINTR)
        goto again;
    else if (n < 0)
        log_error("Process connection: read error!");

    /* EOF(n == 0), shutdown by the peer */
}

int echo_server(const char * ip, int port)
{
    int listen_fd, conn_fd, ret;
    struct sockaddr_in server_addr, client_addr;

    signal_act(SIGCHLD, sig_child);

    if ((listen_fd = tcp_socket_ipv4()) < 0) {
        log_error("Get socket failed");
        return -1;
    }

    if (init_sockaddr_ipv4(&server_addr, ip, port) < 0) {
        log_error("Invalid IP: %s, Port: %d", ip, port);
        return -1;
    }

    bind_ipv4(listen_fd, &server_addr);
    listen_ipv4(listen_fd, 5);
    while(1) {
        conn_fd = accept_ipv4(listen_fd, &client_addr);
        if ((ret = fork()) == 0) {
            close(listen_fd);
            process_connection(conn_fd);
            exit(0);
        } else if (ret < 0) {
            log_warning("Fork error!");
        }
        close(conn_fd);
    }

    return 0;
}

int main(int argc, char ** argv)
{
    int port;
    const char * ip;

    ip = ADDR_ANY_IPV4;
    port = PORT_ANY_IPV4;
    if (argc >= 3) {
        ip = argv[1];
        port = atoi(argv[2]);
    } else if (argc == 2) {
        ip = argv[1];
    }

    printf("==================================================\n");
    printf("Starting the echo server...\n");

    echo_server(ip, port);

    printf("Echo server has been terminated!\n");
    printf("==================================================\n");

    return 0;
}

