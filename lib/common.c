/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：common.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月06日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include "opt.h"
#include "socket.h"

static struct opt_def options[] = {
    {
        .long_opt = "server-ip",
        .short_opt = NULL,
        .flags = OPT_OPTIONAL | OPT_VALUE_REUIRED,
        .desc = "server addr"
    },
    {
        .long_opt = "server-port",
        .short_opt = NULL,
        .flags = OPT_OPTIONAL | OPT_VALUE_REUIRED,
        .desc = "server port"
    },
    {
        .long_opt = "client-ip",
        .short_opt = NULL,
        .flags = OPT_OPTIONAL | OPT_VALUE_REUIRED,
        .desc = "client addr"
    },
    {
        .long_opt = "client-port",
        .short_opt = NULL,
        .flags = OPT_OPTIONAL | OPT_VALUE_REUIRED,
        .desc = "client port"
    },
};

int get_sockaddr_in(int argc, char ** argv,
        struct sockaddr_in * server, struct sockaddr_in * client)
{
    uint16_t port_num;
    char * ip, * port;
    opt_map_t * opt_map;

    opt_map = get_opt_map(argc, argv, options, 
            sizeof(options) / sizeof(struct opt_def));
    if (!opt_map) {
        printf("Get opt-map failed!\n");
        return -1;
    }

    ip = get_opt_value(opt_map, "server-ip");
    if (ip == OPT_NOT_FOUND)
        ip = (char *)ADDR_ANY_IPV4;

    port_num = PORT_ANY_IPV4;
    port = get_opt_value(opt_map, "server-port");
    if (port != OPT_NOT_FOUND)
        port_num = atoi(port);
    if (init_sockaddr_ipv4(server, ip, port_num) < 0)
        goto failed;

    if (client) {
        ip = get_opt_value(opt_map, "client-ip");
        if (ip == OPT_NOT_FOUND)
            ip = (char *)ADDR_ANY_IPV4;

        port_num = PORT_ANY_IPV4;
        port = get_opt_value(opt_map, "client-port");
        if (port != OPT_NOT_FOUND)
            port_num = atoi(port);
        if (init_sockaddr_ipv4(client, ip, port_num) < 0)
            goto failed;
    }

    free_opt_map(opt_map);
    return 0;

failed:
    free_opt_map(opt_map);
    return -1;
}

int get_sockaddr_params(int argc, char ** argv,
        struct sockaddr_in ** server, struct sockaddr_in ** client)
{
    char * ip, * port;
    opt_map_t * opt_map;

    opt_map = get_opt_map(argc, argv, options, 
            sizeof(options) / sizeof(struct opt_def));
    if (!opt_map) {
        printf("Get opt-map failed!\n");
        goto failed;
    }

    if (server && *server) {
        ip = get_opt_value(opt_map, "server-ip");
        port = get_opt_value(opt_map, "server-port");

        if (ip == OPT_NOT_FOUND && port == OPT_NOT_FOUND)
            *server = NULL;
        else {
            if (ip == OPT_NOT_FOUND)
                ip = (char *)ADDR_ANY_IPV4;

            if (port == OPT_NOT_FOUND)
                port = "0";

            if (init_sockaddr_ipv4(*server, ip, atoi(port)) < 0)
                goto failed;
        }
    }

    if (client && *client) {
        ip = get_opt_value(opt_map, "client-ip");
        port = get_opt_value(opt_map, "client-port");

        if (ip == OPT_NOT_FOUND && port == OPT_NOT_FOUND)
            *client = NULL;
        else {
            if (ip == OPT_NOT_FOUND)
                ip = (char *)ADDR_ANY_IPV4;

            if (port == OPT_NOT_FOUND)
                port = "0";

            if (init_sockaddr_ipv4(*client, ip, atoi(port)) < 0)
                goto failed;
        }
    }

    free_opt_map(opt_map);
    return 0;

failed:
    free_opt_map(opt_map);
    return -1;
}
