/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：inetd_server.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月05日
 * 描    述：
 * 
 *===============================================================
 */
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#include "../lib/opt.h"
#include "../lib/utils.h"

struct opt_def G_options[] = {
    {
        .long_opt = "config-file",
        .short_opt = "c",
        .flags = OPT_REQUIRED | OPT_VALUE_REUIRED,
        .desc = "config file"
    },
    {
        .long_opt = "help",
        .short_opt = "h",
        .flags = OPT_OPTIONAL | OPT_VALUE_OMITTED,
        .desc = "config file"
    }
};

static char G_file_buf[512];

extern int inetd(char * config_file);

void print_usage(void)
{
    printf("Usage: inetd --config-file [FILE]\n"); 
}

int main(int argc, char ** argv)
{
    int fd;
    char * config_file, link[32];
    opt_map_t * opt_map;

    if (write_pid_file("/var/run/inetd.pid") < 0) {
        return EXIT_FAILURE;
    }

    opt_map = get_opt_map(argc, argv, 
            G_options, sizeof(G_options) / sizeof(struct opt_def));
    if (!opt_map)
        goto parse_arg_failed;

    config_file = get_opt_value(opt_map, "config-file"); 
    if (config_file == OPT_NOT_FOUND) {
        config_file = get_opt_value(opt_map, "help"); 
        if (config_file == OPT_NOT_FOUND)
            goto parse_arg_failed;
        else {
            print_usage();
            return EXIT_SUCCESS;
        }
    }

    /**
     * must save the fullpath of config file before we become a
     * daemon, cause we will change our 'pwd'.
     */
    if (config_file[0] != '/') {
        /* not abs path */
        if ((fd = open(config_file, O_RDONLY)) < 0) {
            printf("Open file failed, config_file: %s, errno: %s\n.", 
                    config_file, strerror(errno));
            return EXIT_FAILURE;
        }
        snprintf(link, sizeof(link), "/proc/self/fd/%d", fd);
        if (readlink(link, G_file_buf, sizeof(G_file_buf)) < 0) {
            printf("Readlink failed, config_file: %s, errno: %s\n.", 
                    config_file, strerror(errno));
            return EXIT_FAILURE;
        }
        close(fd);
    } else {
        memcpy(G_file_buf, config_file, strlen(config_file));
    }
    free_opt_map(opt_map);

    if (set_proctitile(argv, "inetd-server")< 0) {
        printf("Set process titile Failed!\n");
        return EXIT_FAILURE;
    }

    return inetd(G_file_buf);

parse_arg_failed:
    printf("Inetd: invalid option\n");
    printf("Try 'Inetd --help for more information.\n");
    return EXIT_FAILURE;
}
