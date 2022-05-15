/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：socket.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月20日
 * 描    述：
 * 
 *===============================================================
 */
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/sendfile.h>

#include "../lib/log.h"
#include "../lib/buffer.h"
#include "../lib/utils.h"

#include "fsync.h"
#include "network.h"

void socket_read_at_least(int fd, struct socket_buffer * buffer, int min_bytes)
{
    int i, left, readn;

    readn = 0;
    left = min_bytes;

again:
    readn = left;

    /* blocking IO */
    i = socket_buffer_recv(buffer, fd, &readn);
    left -= readn;

    if (i == 0) {
        assert(left <= 0);
        return;
    }

    if (i == SBUF_RECV_OOM)
        die_unexpectedly("OOM when write to buffer");

    if (i == SBUF_RECV_ERR) {
        if (errno == EINTR)
            goto again;

        /* SOL_RCVTIMEO flag already set */
        if (errno == EAGAIN)
            die_unexpectedly("Read socket timeout");

        log_warning_sys("Read socket error!");
        die_unexpectedly(NULL);
    }

    /**
     * receive EOF when trying to read more bytes
     */
    if (i == SBUF_RECV_EOF && left > 0)
        die_unexpectedly("Receive EOF when read from socket");
}

void socket_write_exactly(int fd, struct socket_buffer * buffer, int bytes)
{
    int i, left, readn;

    i = socket_buffer_size(buffer);
    if (i < bytes)
        die_unexpectedly("Not enough data in buffer");

    left = bytes;

again:
    readn = left;

    /* blocking IO */
    i = socket_buffer_send(buffer, fd, &readn);
    left -= readn;
    if (i == 0)
        return;
    
    /**
     * must have some write errors if we got here, check the 'errno'
     * to find out why
     */ 
    if (errno == EINTR)
        goto again;

    /* SOL_RCVTIMEO flag already set */
    if (errno == EAGAIN)
        die_unexpectedly("write socket timeout");

    log_error_sys("write socket error!");
    die_unexpectedly(NULL);
}

void socket_send_file(int socket_fd, int file_fd)
{
    int len, i;
    struct stat file_stat;

    if (fstat(file_fd, &file_stat) < 0) {
        log_warning_sys("Fstat error!");
        die_unexpectedly(NULL);
    }
    len = file_stat.st_size;

    if (lseek(file_fd, 0, SEEK_SET) < 0) {
        log_error_sys("Lseek error!");
        die_unexpectedly(NULL);
    }

    while (len > 0) {
        /* zero-copy using sendfile() in linux platform */
        if ((i = sendfile(socket_fd, file_fd, NULL, len)) < 0) {
            /* SOL_SNDTIMEO flag already set */
            if (errno == EAGAIN)
                log_error("Sendfile timeout!");
            else 
                log_warning_sys("Sendfile error!");
            die_unexpectedly(NULL);
        }
        len -= i;
    }
}

/**
 * transfer 'len' bytes of data from 'in_fd' to 'out_fd' 
 */
void socket_transfer_to(struct socket_buffer * buffer, 
        int out_fd, int in_fd, int len)
{
    int left, readn, i;

    left = len;
    while (left > 0) {
        readn = min(left, 4 * 1024);
        i = socket_buffer_size(buffer);
        if (i < readn)
            socket_read_at_least(in_fd, buffer, readn - i);

        socket_write_exactly(out_fd, buffer, readn);
        left -= readn;
    }
}

int set_socket_timeout(int socket_fd, time_t ms)
{
    socklen_t len;
    struct timeval timeout;

    len = sizeof(struct timeval);
    timeout.tv_sec = ms / 1000;
    timeout.tv_usec = (ms % 1000) * 1000;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, 
                &timeout, len) < 0) {
        log_warning_sys("Set socket SO_SNDTIMEO error!");
        return -1;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, 
                &timeout, len) < 0) {
        log_warning_sys("Set socket SO_RCVTIMEO error!");
        return -1;
    }

    return 0;
}
