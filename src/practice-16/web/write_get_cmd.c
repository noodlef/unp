/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：write_get_cmd.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月07日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>

#include "web.h"
#include "../../../lib/lib.h"

void write_get_cmd(struct file * file)
{
    int n;
    char line[1024];

    n = snprintf(line, sizeof line, GET_CMD, file->f_name);
    writen(file->sockfd, line, n);

    log_info("Wrote %d bytes for %s", n, file->f_name);

    file->flags = F_READING;
    FD_SET(file->sockfd, &G_rset);
    if (file->sockfd > G_maxfd)
        G_maxfd = file->sockfd;
}
