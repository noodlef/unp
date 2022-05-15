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
#include <string.h>

#include "../../lib/lib.h"

int echo_client(const char * ip, int port)
{
    int sockfd, n;
    char buf[1024];
    socklen_t addr_len;
    struct sockaddr_in server_addr;

    sockfd = udp_socket_ipv4();

    init_sockaddr_ipv4(&server_addr, ip, port);

    while (1) {
        if (fgets(buf, sizeof buf, stdin) == NULL)
            break;

        sendto(sockfd, buf, strlen(buf), 0, 
                (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in));
        addr_len = sizeof(struct sockaddr_in);
        n = recvfrom(sockfd, buf, sizeof buf, 0, 
                (struct sockaddr *)&server_addr, &addr_len);
        
        if (n < 0)
            continue;

        buf[n] = '\0';
        fputs(buf, stdout);
    }


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

