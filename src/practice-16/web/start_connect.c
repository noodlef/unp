/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：start_connect.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月07日
 * 描    述：
 * 
 *===============================================================
 */
#include <errno.h>

#include "web.h"
#include "../../../lib/lib.h"

void start_connect(struct file * file)
{
    int sockfd, n;
    struct sockaddr_in server_addr;
    
    sockfd = tcp_socket_ipv4();
    file->sockfd = sockfd;

    log_info("Start connect for %s, sockfd: %d", file->f_name, sockfd);

    set_sock_nonblocking(sockfd);

    init_sockaddr_ipv4(&server_addr, file->f_host, SERVER_PORT);
    if ((n = connect_ipv4(sockfd, &server_addr)) < 0) {
        if (errno != EINPROGRESS) {
            log_error_sys("Nonblocking connect error!");
            return;
        }

        file->flags = F_CONNECTING;
        FD_SET(sockfd, &G_rset);
        FD_SET(sockfd, &G_wset);
        if (sockfd > G_maxfd)
            G_maxfd = sockfd;
    } else {
        /* connect is already done */
        write_get_cmd(file);
    }
}
