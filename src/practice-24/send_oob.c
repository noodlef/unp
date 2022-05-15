/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：send_oob.c
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

#include "../../lib/lib.h"

void send_oob(struct sockaddr_in * client, struct sockaddr_in * server)
{
    int sockfd;

    if ((sockfd = tcp_connect(server, client)) < 0) {
        log_error("Connect failed!");
        return;
    }

    write(sockfd, "123", 3);
    log_info("Wrote 3 bytes of normal data");
//    sleep(1);

    send(sockfd, "4", 1, MSG_OOB);
    log_info("Wrote 1 bytes of OOB data");
 //   sleep(1);

    write(sockfd, "uuu", 3);
    log_info("Wrote 3 bytes of normal data");
  //  sleep(1);

    send(sockfd, "7", 1, MSG_OOB);
    log_info("Wrote 1 bytes of OOB data");
   // sleep(1);

    write(sockfd, "999", 3);
    log_info("Wrote 3 bytes of normal data");
 //   sleep(1);
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

    send_oob(server, client);

    return EXIT_SUCCESS; 
}
