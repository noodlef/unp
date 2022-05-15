/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：main.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月21日
 * 描    述：
 * 
 *===============================================================
 */
#include <signal.h>

#include "fsync.h"

#include "../lib/log.h"
#include "../lib/utils.h"
#include "../lib/socket.h"
#include "../lib/buffer.h"

int G_conn_fd = 0;

static char * G_log_file = "/log/today/filesync-server.log";

extern void filesync_server(void);
extern int set_socket_timeout(int socket_fd, time_t ms);

static void sig_hub(int signo)
{
    log_info("Caught signal SIGHUP");
    die_unexpectedly("Connection reset by the peer!!!");
}

int main(int argc, char ** argv)
{
    int ret;
    char ip[32];
    uint16_t port;
    struct sockaddr_in client;

    if (init_log(G_log_file, 1) < 0)
        return EXIT_FAILURE;

    log_info("========================================================");
    ret = getpeername_ipv4(G_conn_fd, &client);
    if (ret < 0)
        return EXIT_FAILURE;
    sockaddr_to_ipport_ipv4(&client, ip, &port);
    log_info("Filesync request from client(ip: %s, port: %d).", ip, port);

    signal_act(SIGHUP, sig_hub);

    if (!(G_recv_buffer = socket_buffer_new())) {
        log_error("Get recv_buffer failed!!!");
        return EXIT_FAILURE;
    }

    if (!(G_send_buffer = socket_buffer_new())) {
        log_error("Get send_buffer failed!!!");
        return EXIT_FAILURE;
    }

    /**
     * call to recv or send will timeout after 10 seconds
     */
    if (set_socket_timeout(G_conn_fd, 10 * 1000) <  0) {
        log_error("Set socket timeout error!!!");    
        return EXIT_FAILURE;
    }

    log_info("Begin filesync...");
    filesync_server();

    log_info("filesync done!!!.");
    log_info("========================================================");

    return EXIT_SUCCESS;
}
