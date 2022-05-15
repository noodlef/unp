/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：web.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月07日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>

#include "web.h"
#include "../../../lib/lib.h"

static char * G_log_file = "/log/web.log";

struct file file[MAXFILES];
int G_nconn;
int G_nfiles;
int G_nleftconn;
int G_nlefttoread;
int G_maxfd;

fd_set G_rset;
fd_set G_wset;

int main(int argc, char ** argv)
{
    char buf[1024];
    fd_set rset, wset;
    int i, sockfd, n, max_conn, flags, error;

    if (argc < 5) {
        printf("usage: web <#conns> <hostname> <homepage> <file1> ...\n");
        return EXIT_FAILURE;
    }

    if (init_log(G_log_file, 1) < 0) {
        printf("Init log failed");
        return EXIT_FAILURE;
    }
    
    max_conn = atoi(argv[1]);
    G_nfiles = min(argc - 4, MAXFILES);

    for (i = 0; i < G_nfiles; i++) {
        file[i].f_name = argv[i + 4];
        file[i].f_host = argv[2];
        file[i].flags = 0;
    }

    log_info("G_nfiles = %d", G_nfiles);

    home_page(argv[2], argv[3]);
    
    FD_ZERO(&G_rset);
    FD_ZERO(&G_wset);
    G_maxfd = -1;
    G_nleftconn = G_nlefttoread = G_nfiles;
    G_nconn = 0;

    while (G_nlefttoread > 0) {
        
        while (G_nconn < max_conn && G_nleftconn > 0) {
            for (i = 0; i < G_nfiles; i++) {
                if (file[i].flags == 0)
                    break;
            }

            if (i == G_nfiles)
                log_error("G_nleftconn = %d but nothing found", G_nleftconn);
            
            start_connect(&file[i]);
            G_nconn++;
            G_nleftconn--;
        }

        rset = G_rset;
        wset = G_wset;
        n = select(G_maxfd + 1, &rset, &wset, NULL, NULL);

        for (i = 0; i < G_nfiles; i++) {
            flags = file[i].flags;
            if (flags == 0 || flags & F_DONE)
                continue;

            sockfd = file[i].sockfd;
            if (flags & F_CONNECTING && 
                    (FD_ISSET(sockfd, &rset) || FD_ISSET(sockfd, &wset))) {
                n = sizeof error;
                if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &n) < 0 || 
                        error != 0) {
                    log_error_sys("Nonblocking connect failed for %s", file[i].f_name);
                    FD_CLR(sockfd, &G_rset);
                    FD_CLR(sockfd, &G_wset);
                    G_nconn--;
                    G_nlefttoread--;
                    continue;
                }
                log_info("Connection established for %s", file[i].f_name);
                FD_CLR(sockfd, &G_wset);
                write_get_cmd(&file[i]);
            }

            if (flags & F_READING && FD_ISSET(sockfd, &rset)) {
                if ((n = read(sockfd, buf, sizeof buf)) < 0) {
                    if (errno == EINTR)
                        continue;
                    log_error_sys("Read error for %s", file[i].f_name);
                    FD_CLR(sockfd, &G_rset);
                    G_nconn--;
                    G_nlefttoread--;
                } else if (n == 0) {
                    log_info("End-of-file on %s", file[i].f_name);
                    close(sockfd);
                    FD_CLR(sockfd, &G_rset);
                    file[i].flags = F_DONE;
                    G_nconn--;
                    G_nlefttoread--;
                } else {
                    log_info("Read %d bytes from %s", n, file[i].f_name);
                    log_info("==================================================");
                    buf[n] = '\0';
                    log_info("Response: \n%s", buf);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
