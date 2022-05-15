/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：socket.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月26日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __FSYNC_SOCKET_H
#define __FSYNC_SOCKET_H

struct socket_buffer;

void socket_read_at_least(int fd, struct socket_buffer * buffer, int min_bytes);

void socket_write_exactly(int fd, struct socket_buffer * buffer, int bytes);

void socket_send_file(int socket_fd, int file_fd);

void socket_transfer_to(struct socket_buffer * buffer, 
        int out_fd, int in_fd, int len);
#endif //FSYNC_SOCKET_H
