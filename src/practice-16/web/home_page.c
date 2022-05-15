/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：home_page.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月07日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "web.h"
#include "../../../lib/lib.h"

void home_page(char * host, char * fname)
{
    int sockfd, n;
    char line[1024];
    struct sockaddr_in server_addr;

    sockfd = tcp_socket_ipv4();
    init_sockaddr_ipv4(&server_addr, host, SERVER_PORT);
    connect_ipv4(sockfd, &server_addr);

    n = snprintf(line, sizeof line, GET_CMD, fname); 
    writen(sockfd, line, n);

    while (1) {
        if (!(n = read(sockfd, line, sizeof line)))
            break; /* EOF */
        else if (n < 0) {
            if (n == EINTR)
                continue;
            log_error_sys("Read error!");
            break;
        }

        log_info("Read %d bytes of home page", n);
    }
    log_info("End-of-file on home page");
    close(sockfd);
}
