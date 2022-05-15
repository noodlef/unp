/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：daytime_server.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月05日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include "../../lib/lib.h"

int main(int argc, char ** argv)
{
    socklen_t len;
    time_t ticks;
    char buf[1024];
    struct sockaddr_in from;

    len = sizeof (struct sockaddr_in);
    if (init_log("daytime-server.log", 1) < 0)
        return EXIT_FAILURE;

    recvfrom(0, buf, sizeof(buf), 0, (struct sockaddr *)&from, &len);

    ticks = time(NULL);
    snprintf(buf, sizeof(buf), "%.24s\r", ctime(&ticks));
    sendto(0, buf, strlen(buf), 0, (struct sockaddr *)&from, len);

    return 0;
}
