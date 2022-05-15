/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：socket.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月14日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __SOCKET_H
#define __SOCKET_H

#include <netinet/in.h>
#include <sys/un.h>

#define ADDR_ANY_IPV4 (const char *)0 
#define PORT_ANY_IPV4 0 

int init_sockaddr_ipv4(struct sockaddr_in * addr, const char * ip, uint16_t port);

int tcp_socket_ipv4(void);

int udp_socket_ipv4(void);

int connect_ipv4(int sockfd, struct sockaddr_in * addr);

int bind_ipv4(int sockfd, const struct sockaddr_in * addr);

int listen_ipv4(int sockfd, int backlog);

int accept_ipv4(int sockfd, struct sockaddr_in * addr);

void sockaddr_to_ipport_ipv4(const struct sockaddr_in * addr, 
        char * ip, uint16_t * port);

int getsockname_ipv4(int sockfd, struct sockaddr_in * addr);

int set_sock_nonblocking(int sockfd);

int getpeername_ipv4(int sockfd, struct sockaddr_in * addr);

int set_sock_reuseaddr(int sockfd);

int unix_stream_socket(void);

int unix_datagram_socket(void);

int unix_init_sockaddr(struct sockaddr_un * addr, char * pathname);

int unix_bind(int sockfd, const struct sockaddr_un * addr);

#endif //SOCKET_H
