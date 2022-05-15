/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：tftp_client.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年04月15日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>

#include "tftp.h"
#include "../lib/socket.h"

struct tftp_struct G_tftp_config;

void print_usage(void)
{
    printf("usage: tftp [-T] <-[U|D]> <src-file> <dst-file> <ip> <port>\n");
    printf("upload or download file from the remote host specified by <IP:PORT>\n\n");
    printf("The options supported are:\n");
    printf("  -U        present when upload file\n");
    printf("  -D        present when download file\n");
    printf("  -T        specify the file mode, which is text file when it is present,\n");
    printf("            otherwise it is binary file by default.\n");
}

int main(int argc, char * argv[])
{
    int sockfd;
    struct sockaddr_in server_addr;
    char * option, * src_file, * dst_file, * ip, * port; 
    if (argc != 6 && argc != 7) {
        print_usage();
        return EXIT_FAILURE;
    }

    /* log to stdout with no prefix */
    if (init_log(NULL, 0) < 0)
        return EXIT_FAILURE;
    set_log_level(LOG_WARN);

    port = argv[argc - 1];
    ip = argv[argc - 2];
    dst_file = argv[argc - 3];
    src_file = argv[argc - 4];
    option = argv[argc - 5];
    if (argc == 7) {
        G_tftp_config.mode = "netascii";
    }
    
    if ((sockfd = udp_socket_ipv4()) < 0)
        return EXIT_FAILURE;

    if (init_sockaddr_ipv4(&server_addr,  ip, atoi(port)) < 0)
        return EXIT_FAILURE;

    /* (NOTE) connect on udp scoket, so we can use 'write' and 'read' */
    if (connect_ipv4(sockfd, &server_addr) < 0)
        return EXIT_FAILURE;

    init_tftp_config(&G_tftp_config);
    G_tftp_config.connected = 1;
    G_tftp_config.sockfd = sockfd;

    switch(option[1]) {
    case 'U':
        G_tftp_config.filename = strdup(src_file);
        tftp_cli(1, dst_file);
        break;
    case 'D':
        G_tftp_config.filename = strdup(dst_file);
        tftp_cli(0, src_file);
        break;
    default:
        printf("Invalid option!\n");
        print_usage();
    }
    printf("\n");

    return EXIT_SUCCESS;
}
