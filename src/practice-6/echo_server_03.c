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
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>

#include "../../lib/lib.h"

int G_conn_fd_set[1024];
struct socket_buffer * G_fd_buffer_map[1024];
fd_set G_read_set, G_write_set, G_exc_set;

int init_fd_set(void) 
{
    int i, new_maxfd;

    new_maxfd = -1;
    FD_ZERO(&G_read_set);
    FD_ZERO(&G_write_set);
    FD_ZERO(&G_exc_set);

    for (i = 0; i < 1024; i++) {
        if (G_conn_fd_set[i] > 0) {
            FD_SET(i, &G_read_set);
            FD_SET(i, &G_write_set);
            FD_SET(i, &G_exc_set);
            new_maxfd = max(new_maxfd, i);
        }
    }

    return new_maxfd + 1;
}

void process_fd(int fd)
{
    int n;
    size_t len;
    struct socket_buffer * buffer;
    struct iovec * iov;

    buffer = G_fd_buffer_map[fd];
    if (FD_ISSET(fd, &G_exc_set)) {
        log_warning("Got exception, fd: %d", fd);
        goto shutdown;
    }

#if 0
    if (FD_ISSET(fd, &G_read_set)) {
        socket_buffer_get(buffer, &iov, &len);
        if ((n = readv(fd, iov, len)) < 0) {
            log_warning("Read error, fd: %d", fd);
            goto shutdown;
        } else if (n == 0) {
            /* EOF(n == 0), shutdown by the peer */
            log_warning("Connection close by the client!");
            goto shutdown;
        }
        socket_buffer_seek(buffer, n, SEEK_WRITE);
    }

    n = socket_buffer_size(buffer);
    if (n && FD_ISSET(fd, &G_write_set)) {
        socket_buffer_peek(buffer, n, &iov, &len);
        if ((n = writev(fd, iov, len)) < 0) {
            log_warning("Read error, fd: %d", fd);
            goto shutdown;
        }
        socket_buffer_seek(buffer, n, SEEK_READ);
    }
#endif

    return;

shutdown:
    close(fd);
    socket_buffer_free(buffer);
    G_conn_fd_set[fd] = -1;
    return;
}

int echo_server_01(const char * ip, int port)
{
    int i, listen_fd, max_fd, conn_fd, fd;
    struct sockaddr_in server_addr, conn_addr;

    if ((listen_fd = tcp_socket_ipv4()) < 0)
        return -1;

    if (init_sockaddr_ipv4(&server_addr, ip, port) < 0)
        return -1;

    if (bind_ipv4(listen_fd, &server_addr) < 0)
        return -1;

    if (listen_ipv4(listen_fd, 5) < 0)
        return -1;
    
    for (i = 0; i < 1024; i++)
        G_conn_fd_set[i] = -1;

    G_conn_fd_set[listen_fd] = 1;
    max_fd = listen_fd + 1;
    while (1) {
        max_fd = init_fd_set();
        if ((select(max_fd, &G_read_set, 
                        &G_write_set, &G_exc_set, NULL))< 0) {
            if (errno == EINTR)
                continue;

            log_warning_sys("Select error!");
            return -1;
        }

        for(fd = 0; fd < 1024; fd++) {
            if (G_conn_fd_set[fd] < 0)
                continue;

            /* accept new connection */
            if (fd == listen_fd) {
                if (FD_ISSET(listen_fd, &G_read_set)) {
                   conn_fd = accept_ipv4(listen_fd, &conn_addr);
                   if (conn_fd >= 0) {
                       if ((G_fd_buffer_map[conn_fd] = socket_buffer_new())) 
                           G_conn_fd_set[conn_fd] = 1;
                   }
                }
                continue;
            }

            process_fd(fd);
        }
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

    echo_server_01(ip, port);

    printf("Echo server has been terminated!\n");
    printf("==================================================\n");

    return 0;
}

