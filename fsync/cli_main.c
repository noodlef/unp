/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：main.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月27日
 * 描    述：
 * 
 *===============================================================
 */
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "../lib/opt.h"
#include "../lib/utils.h"
#include "../lib/buffer.h"
#include "../lib/socket.h"

#include "fsync.h"

static void print_usage(void)
{
    printf("Usage: filesync_cli --remote-ip [IP] --remote-port [PORT]\n"); 
    printf("                    --remote-dir [DIR] --local-dir [DIR] --cmd [CMD]\n"); 
}

char * G_cmd = NULL;
char * G_remote_dir = NULL;
char * G_local_dir = NULL;
int G_conn_fd = -1;

static int G_remote_port = 0;
static char * G_remote_ip = NULL;

static void parse_args(int argc, char ** argv)
{
    char * option_value;  
    opt_map_t * opt_map;
    struct opt_def option_defs[] = {
        {
            .long_opt = "remote-ip",
            .short_opt = NULL, 
            .flags = OPT_REQUIRED | OPT_VALUE_REUIRED,
            .desc = "remote server ip address"
        },
        {
            .long_opt = "remote-port",
            .short_opt = NULL, 
            .flags = OPT_REQUIRED | OPT_VALUE_REUIRED,
            .desc = "remote server port number"
        },
        {
            .long_opt = "remote-dir",
            .short_opt = NULL, 
            .flags = OPT_REQUIRED | OPT_VALUE_REUIRED,
            .desc = "remote server dir path"
        },
        {
            .long_opt = "local-dir",
            .short_opt = NULL, 
            .flags = OPT_REQUIRED | OPT_VALUE_REUIRED,
            .desc = "local dir path"
        },
        {
            .long_opt = "cmd",
            .short_opt = NULL, 
            .flags = OPT_REQUIRED | OPT_VALUE_REUIRED,
            .desc = "command: upload, download, info"
        },
        {
            .long_opt = "help",
            .short_opt = "h", 
            .flags = OPT_REQUIRED | OPT_VALUE_OMITTED,
            .desc = "help information"
        },
    };

    opt_map = get_opt_map(argc, argv, 
            option_defs, sizeof(option_defs) / sizeof(struct opt_def));
    if (!opt_map)
        goto failed;

    option_value = get_opt_value(opt_map, "help"); 
    if (option_value != OPT_NOT_FOUND) {
        print_usage();
        exit(EXIT_SUCCESS);
    }

    option_value = get_opt_value(opt_map, "remote-ip"); 
    if (option_value == OPT_NOT_FOUND || option_value == OPT_NO_VALUE)
        goto failed;
    G_remote_ip = option_value;

    option_value = get_opt_value(opt_map, "remote-port"); 
    if (option_value == OPT_NOT_FOUND || option_value == OPT_NO_VALUE)
        goto failed;
    G_remote_port = atoi(option_value);

    option_value = get_opt_value(opt_map, "remote-dir"); 
    if (option_value == OPT_NOT_FOUND || option_value == OPT_NO_VALUE)
        goto failed;
    G_remote_dir = option_value;

    option_value = get_opt_value(opt_map, "local-dir"); 
    if (option_value == OPT_NOT_FOUND || option_value == OPT_NO_VALUE)
        goto failed;
    G_local_dir = option_value;

    option_value = get_opt_value(opt_map, "cmd"); 
    if (option_value == OPT_NOT_FOUND || option_value == OPT_NO_VALUE)
        goto failed;
    G_cmd = option_value;

    free_opt_map(opt_map);
    return;

failed:
    printf("filesync_cli: invalid option\n");
    printf("Try 'filesync_cli --help for more information.\n");
    exit(EXIT_FAILURE);
}

extern int fsync_cli(void);
int main(int argc, char ** argv)
{
    struct sockaddr_in server;

    /* log with no prefix */
    init_log(NULL, 0);

    parse_args(argc, argv);

    init_sockaddr_ipv4(&server, G_remote_ip, G_remote_port);
    if ((G_conn_fd = tcp_connect(NULL, &server)) < 0) {
        printf("filesync: connect to remote server(%s:%d) failed\n",
                G_remote_ip, G_remote_port);
        exit(EXIT_FAILURE);
    }

    if (!(G_recv_buffer = socket_buffer_new())) {
        printf("Get recv_buffer failed!!!");
        return EXIT_FAILURE;
    }

    if (!(G_send_buffer = socket_buffer_new())) {
        printf("Get send_buffer failed!!!");
        return EXIT_FAILURE;
    }

    fsync_cli();
    printf("Fsync done...\n");

    return EXIT_SUCCESS;
}
