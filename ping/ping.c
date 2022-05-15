/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：ping.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月14日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include "../lib/log.h" 
#include "../lib/utils.h"

static int G_request_cnt = 0;
static int G_total_request_cnt = 0;
static int G_total_reply_cnt = 0;
static int G_send_interval = 1;

static void _get_timestamp(struct timespec * tp)
{
    clock_gettime(CLOCK_MONOTONIC, tp);
}

static uint16_t _calc_cksum(uint16_t * addr, int len)
{
    int nleft = len;
    uint32_t sum = 0;
    uint16_t * w = addr, answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1) {
        *(unsigned char *) (&answer) = *(unsigned char *)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return answer;
}

/**
 * ICMP echo request:
 * -------------------------------------------------------
 * | icmp_type | imcp_code(0)|        icmp_cksum         | 
 * -------------------------------------------------------
 * |      icmp_id            |       icmp_seq            | 
 * -------------------------------------------------------
 * |                                                     | 
 * |                 icmp_data(optional)                 | 
 * ~                                                     ~ 
 * |                                                     | 
 * -------------------------------------------------------
 * NOTE: any data in 'icmp_data' will be returned untouched.
 */
static int _get_icmp_datagram(void ** icmp_buf)
{
    int buflen;
    struct icmp * icmp;
    static char * buffer[64];
    static int is_init = 0;
    static uint16_t seqnum = 0;

    buflen = sizeof(buffer);
    icmp = (struct icmp *)buffer;
    *icmp_buf = buffer;

    if (!is_init) {
        is_init = 1;
        icmp->icmp_type = ICMP_ECHO;
        icmp->icmp_code = 0;
        icmp->icmp_id = getpid() & 0xffff; /* pid */
        memset(icmp->icmp_data, 0xa5, buflen - 8);
    }

    icmp->icmp_seq = seqnum++;
    _get_timestamp((struct timespec *)icmp->icmp_data);

    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = _calc_cksum((uint16_t *)icmp, buflen);

    return buflen;
}

static int _calc_tto(
        struct timespec * at_send, struct timespec * at_arrived)
{
    int tto; // ms
    double delta;

    tto = (at_arrived->tv_sec - at_send->tv_sec) * 1000;
    delta = (at_arrived->tv_nsec - at_send->tv_nsec) / 1000000;
    
    return tto + delta;
}

static int _get_sockfd(void)
{
    int sockfd, recvbuf_size;

    /* must be privileged user */
    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
        log_error_sys("Socket error!"); 
        return -1;
    }

    /* don't need speical perimissions any more */
    setuid(getuid());

    /** 
     * enlarge recvbuf size in case of an muticast or 
     * boradcast destination address
     */
    recvbuf_size = 60 * 1024; 
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, 
                &recvbuf_size, sizeof(recvbuf_size)) < 0)
        log_warning_sys("set rcvbuf size failed!");

    return sockfd;
}

void process_ip_packet(struct ip * ippacket, 
        int packet_len, struct timespec * at_arrived)
{
    float tto;
    char src_ip[16];
    int hlen, pid, icmp_len;
    struct icmp * icmp_reply;
    struct timespec * at_send;

    if (ippacket->ip_p != IPPROTO_ICMP)
        return;

    pid = getpid() & 0xffff;
    hlen = ippacket->ip_hl << 2;
    icmp_reply = (struct icmp *) ((char *)ippacket + hlen);

    /* icmp header length must be greater than 8 bytes */
    if ((icmp_len = packet_len - hlen) < 8)
        return;

    if (icmp_reply->icmp_type != ICMP_ECHOREPLY)
        return;

    if (icmp_reply->icmp_id != pid)
        return;

    if ((unsigned long)icmp_len < sizeof(struct timespec))
        return;

    at_send = (struct timespec *) icmp_reply->icmp_data;
    tto = _calc_tto(at_send, at_arrived);
    
    inet_ntop(AF_INET, &(ippacket->ip_src), src_ip, sizeof(src_ip));
    printf("%d bytes from %s: seq = %d, ttl = %d, rrt = %.3f ms\n", 
            icmp_len, src_ip, icmp_reply->icmp_seq, ippacket->ip_ttl, tto);
}

static void _sig_alarm(int signo)
{
    ++G_request_cnt;
    alarm(G_send_interval);
}

void ping(char * dst_ip)
{
    int sockfd, n;
    char * icmp_request, ippacket[1600]; 
    struct sockaddr_in dstaddr, srcaddr;
    struct timespec at_arrived;

    if ((sockfd = _get_sockfd()) < 0)
        return;

    init_sockaddr_ipv4(&dstaddr, dst_ip, 0);
    signal_act(SIGALRM, _sig_alarm);

    alarm(G_send_interval); // start the timer
    for(;;) {
        if (G_request_cnt > 0) {
            n = _get_icmp_datagram(&icmp_request);
            sendto(sockfd, icmp_request, n, 0, 
                    &dstaddr, sizeof(dstaddr)); 
            --G_request_cnt;
        }

        if ((n = recvfrom(sockfd, &ippacket, sizeof(ippacket), 
                        0, NULL, 0)) < 0) {
            if (errno == EINTR)
                continue;
            printf("Recv error!, errno: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }
        
        _get_timestamp(&at_arrived);
        process_ip_packet(&ippacket, n, &at_arrived);
    }

    return;
}
