/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：tftp_server.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年04月15日
 * 描    述：
 * 
 *===============================================================
 */
#include "tftp.h"
#include "../lib/utils.h"

static char * G_log_file = "/log/today/tftp-server.log";
struct tftp_struct G_tftp_config;

int main(int argc, char * argv[])
{
    if (init_log(G_log_file, LOG_WITH_PREFIX) < 0)
        return EXIT_FAILURE;

    if (set_proctitile(argv, "tftp-server")< 0) {
        log_error("Set process titile Failed!\n");
        return EXIT_FAILURE;
    }

    init_tftp_config(&G_tftp_config);
    G_tftp_config.sockfd = 0;
    log_info("========================================================");
    log_info("TFTP-SERVER starting...");

    tftp_server();

    log_info("TFTP-SERVER finished.");
    log_info("========================================================");
    return EXIT_SUCCESS;
}
