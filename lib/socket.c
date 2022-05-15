/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：sockaddr.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月09日
 * 描    述：
 * 
 *===============================================================
 */
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <strings.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#include "log.h"

void sockaddr_to_ipport_ipv4(
        const struct sockaddr_in * addr, char * ip, uint16_t * port);

int init_sockaddr_ipv4(struct sockaddr_in * addr, const char * ip, uint16_t port)
{
    assert(port >= 0);
    assert(addr != NULL);

    bzero(addr, sizeof(struct sockaddr_in));
    if (!ip) {
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
    } else {
        if (inet_aton(ip, &(addr->sin_addr)) <= 0) {
            log_warning("Invalid IP: %s", ip);
            return -1;
        }
    }
    addr->sin_port = htons(port);
    addr->sin_family = AF_INET;

    return 0;
}

int tcp_socket_ipv4(void)
{
    int ret;

    if ((ret = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        log_error_sys("Create TCP socket failed");

    return ret; 
}

int udp_socket_ipv4(void)
{
    int ret;

    if ((ret = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        log_error_sys("Create UDP socket failed");

    return ret; 
}

//========================================================================
int unix_stream_socket(void)
{
    int ret;

    if ((ret = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
        log_error_sys("Create unix stream socket failed");

    return ret; 
}

int unix_datagram_socket(void)
{
    int ret;

    if ((ret = socket(AF_LOCAL, SOCK_DGRAM, 0)) < 0)
        log_error_sys("Create unix datagram socket failed");

    return ret; 
}

int unix_init_sockaddr(struct sockaddr_un * addr, char * pathname)
{
    size_t len;

    assert(addr != NULL);
    assert(pathname != NULL);

    bzero(addr, sizeof(struct sockaddr_un));
    addr->sun_family = AF_LOCAL;

    len = sizeof(addr->sun_path) - 1;
    strncpy(addr->sun_path, pathname, len);
    if (strlen(pathname) > len)
        log_warning("Pathname(%s) truncated(%s)!!!", 
                pathname, addr->sun_path);

    return 0;
}

int unix_bind(int sockfd, const struct sockaddr_un * addr)
{
    int ret;

    assert(addr != NULL);

    ret = bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_un));
    if (ret < 0)
        log_error_sys("Bind to pathname(%s) failed, sockfd: %d",
               addr->sun_path, sockfd);

    return ret;
}
//========================================================================

int connect_ipv4(int sockfd, struct sockaddr_in * addr)
{
    char ip[32];
    int ret, port;

    assert(addr != NULL);

    ret = connect(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        sockaddr_to_ipport_ipv4(addr, ip, &port);
        log_error_sys("Connect to %s:%d failed, sockfd: %d", ip, port, sockfd);
    }

    return ret;
}

int bind_ipv4(int sockfd, const struct sockaddr_in * addr)
{
    char ip[32];
    int ret, port;

    assert(addr != NULL);

    ret = bind(sockfd, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
    if (ret < 0) {
        sockaddr_to_ipport_ipv4(addr, ip, &port);
        log_error_sys("Bind to %s:%d failed, sockfd: %d", ip, port, sockfd);
    }

    return ret;
}

int listen_ipv4(int sockfd, int backlog)
{
    int ret;
    char * env_backlog;

    if ((env_backlog = getenv("TCP_BACKLOG")) != NULL)
        backlog = atoi(env_backlog);

    if ((ret = listen(sockfd, backlog)) < 0)
        log_error_sys("Listen failed, sockfd: %d", sockfd);

    return ret;
}

int accept_ipv4(int sockfd, struct sockaddr_in * addr)
{
    int ret;
    socklen_t no_use;

again:
    no_use = sizeof(struct sockaddr_in);
    if ((ret = accept(sockfd, (struct sockaddr *)addr, &no_use)) < 0) {
        if (errno == EINTR)
            goto again;
        log_error_sys("Accept failed, sockfd: %d", sockfd);
    }

    return ret;
}

void sockaddr_to_ipport_ipv4(
        const struct sockaddr_in * addr, char * ip, uint16_t * port)
{
   assert(addr != NULL);

   if (ip)
        inet_ntop(AF_INET, &(addr->sin_addr), ip, 16);

   if (port)
        *port = ntohs(addr->sin_port);
}

int getsockname_ipv4(int sockfd, struct sockaddr_in * addr)
{
    int ret;
    socklen_t no_use;

    assert(addr != NULL);

    no_use = sizeof(struct sockaddr_in);
    if ((ret = getsockname(sockfd, (struct sockaddr *)addr, &no_use)) < 0)
        log_warning("Getsockname failed, sockfd: %d", sockfd);

    return ret;
}

int getpeername_ipv4(int sockfd, struct sockaddr_in * addr)
{
    int ret;
    socklen_t no_use;

    assert(addr != NULL);

    no_use = sizeof(struct sockaddr_in);
    if ((ret = getpeername(sockfd, (struct sockaddr *)addr, &no_use)) < 0)
        log_warning("Getpeername failed, sockfd: %d", sockfd);

    return ret;
}

int set_sock_nonblocking(int sockfd)
{
    int flags;

    if ((flags = fcntl(sockfd, F_GETFL, 0)) < 0) {
        log_warning_sys("Set sock nonblocking failed, sockfd: %d", sockfd);
        return -1;
    }

    flags |= O_NONBLOCK; 
    if (fcntl(sockfd, F_SETFL, flags) < 0) {
        log_warning_sys("Set sock nonblocking failed, sockfd: %d", sockfd);
        return -1;
    }

    return 0;
}

int set_sock_reuseaddr(int sockfd)
{
    int on = 1, ret;

    ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (ret < 0)
        log_warning_sys("Set sock reuseaddr failed, sockfd: %d", sockfd);

    return ret;
}
