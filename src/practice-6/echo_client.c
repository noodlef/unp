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
#include <sys/select.h>
#include <sys/time.h>

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

void process(int sockfd)
{
    char buf[1024];
    fd_set read_set;
    int flag, stdin_fd, ret, maxfd;

    flag = 1;
    stdin_fd = fileno(stdin);
    while (1) {
        FD_ZERO(&read_set); 
        FD_SET(sockfd, &read_set);
        maxfd = sockfd + 1;
        if (flag) {
            FD_SET(stdin_fd, &read_set);
            maxfd = max(stdin_fd, sockfd) + 1;
        }

        ret = select(maxfd, &read_set, NULL, NULL, NULL); 
        if (ret < 0) {
            if (errno == EINTR)
                continue;
            log_warning_sys("Select error!");
            return;
        }

        if (FD_ISSET(sockfd, &read_set)) {
            if ((ret = read(sockfd, buf, sizeof buf)) < 0) {
                log_warning("Read error!");
                return;
            } else if (ret == 0) {
                log_warning("Connection close!");
                return;
            }

            buf[ret] = '\0';
            if (fputs(buf, stdout) == EOF) {
                log_warning("Fputs: input error!");
                return;
            }
        }

        if (FD_ISSET(stdin_fd, &read_set)) {
            if (fgets(buf, sizeof buf, stdin) == NULL) {
                log_warning_sys("Fgets: input error!");
                flag = 0;
                shutdown(sockfd, SHUT_WR);
                continue;
            }

            if (writen(sockfd, buf, strlen(buf)) < 0)
                return;
        }
    }
}

int echo_client(const char * ip, int port)
{
    int conn_fd;
    struct sockaddr_in server_addr;

    conn_fd = tcp_socket_ipv4();

    init_sockaddr_ipv4(&server_addr, ip, port);
    connect_ipv4(conn_fd, &server_addr);

    //process_connection(conn_fd);
    process(conn_fd);

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

