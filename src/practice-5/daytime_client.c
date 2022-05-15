/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：daytime_client.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月11日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>

#include "../../lib/lib.h"

int main(int argc, char ** argv)
{
    int i;
    int client_fd;
    char buf[1024];
    struct sockaddr_in server_addr;

    client_fd = tcp_socket_ipv4(); 

    init_sockaddr_ipv4(&server_addr, "127.0.0.1", 32000);
    connect_ipv4(client_fd, &server_addr);

    i = readn(client_fd, buf, sizeof(buf));
    buf[i] = '\0';
    printf("From daytime-server: %s\n", buf);

    return i;
}

