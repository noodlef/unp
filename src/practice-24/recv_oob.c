/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：recv_oob.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月13日
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
#include <fcntl.h>
#include <errno.h>

#include "../../lib/lib.h"

int G_connfd = -1;

void sig_urg(int signo)
{
    int n;
    char buf[36];

    log_info("SIGURG received!");
    if ((n = recv(G_connfd, buf, sizeof(buf) - 1, MSG_OOB)) < 0) {
        log_error("Recv OOB error!");
        return;
    }

    buf[n] = '\0';
    log_info("Read %d OOB byte: %s", n, buf);
}

void recv_oob(struct sockaddr_in * server)
{
    char buf[36];
    int listenfd, n;
    struct sockaddr_in client;

    if ((listenfd = tcp_listen(server, 5)) < 0)
        return;

    if ((G_connfd = accept_ipv4(listenfd, &client)) < 0)
        return;

    signal_act(SIGURG, sig_urg);
    fcntl(G_connfd, F_SETOWN, getpid());

    sleep(3);
    for(;;) {
        if ((n = read(G_connfd, buf, sizeof(buf) - 1)) == 0) {
            log_info("Recv EOF!");
            return;
        } else if (n < 0) {
            if (errno == EINTR)
                continue;

            log_error_sys("Recv error!");
            return;
        }

        buf[n] = '\0';
        log_info("Read %d bytes: %s", n, buf);
    }
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

    recv_oob(server);

    return EXIT_SUCCESS; 
}

