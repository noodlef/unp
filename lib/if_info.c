/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：if_info.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月08日
 * 描    述：
 * 
 *===============================================================
 */
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <arpa/inet.h>

#include "log.h"
#include "socket.h"
#include "if_info.h"
#include "utils.h"

/**
 * Liunx:
 *
 * struct ifreq {
 *   char ifr_name[IFNAMSIZ]; // Interface name
 *   union {
 *       struct sockaddr ifr_addr;
 *       struct sockaddr ifr_dstaddr;
 *       struct sockaddr ifr_broadaddr;
 *       struct sockaddr ifr_netmask;
 *       struct sockaddr ifr_hwaddr;
 *       short           ifr_flags;
 *       int             ifr_ifindex;
 *       int             ifr_metric;
 *       int             ifr_mtu;
 *       struct ifmap    ifr_map;
 *       char            ifr_slave[IFNAMSIZ];
 *       char            ifr_newname[IFNAMSIZ];
 *       char           *ifr_data;
 *   };
 * };
 *
 * struct ifconf {
 *   int                 ifc_len; // size of buffer
 *   union {
 *      char           *ifc_buf;  // buffer address
 *      struct ifreq   *ifc_req;  // array of structures
 *   };
 * };
 */

static int _get_all_ifs(int sockfd, struct ifreq ** ifreq)
{
    struct ifconf ifconf;

    /* NOTE: should allocate a larger buffer??? */
    ifconf.ifc_len = IF_BUF_SIZE * sizeof(struct ifreq);
    ifconf.ifc_buf = malloc(ifconf.ifc_len);
    if (!ifconf.ifc_buf) {
        log_error("Malloc buffer failed!");
        return -1;
    }

    /** 
     * interfaces that are not running or have not address 
     * will not be returned 
     */
    if (ioctl(sockfd, SIOCGIFCONF, &ifconf) < 0) {
        log_error_sys("Ioctl error!");
        free(ifconf.ifc_buf);
        return -1;
    }
    *ifreq = (struct ifreq *)ifconf.ifc_buf;

    return ifconf.ifc_len;
}


static int _get_info(int sockfd, struct if_info * ifi, 
        struct ifreq * ifreq, int opt)
{
    struct sockaddr_in * addr;

    if (ioctl(sockfd, opt, ifreq) < 0) {
        log_error_sys("Ioctl error!, opt: %d", opt);
        return -1;
    }

    switch (opt) {
        case SIOCGIFFLAGS:
            ifi->flags = ifreq->ifr_flags;     
            break;
        case SIOCGIFMTU:
            ifi->mtu = ifreq->ifr_mtu;
            break;
        case SIOCGIFHWADDR:
            memcpy(ifi->hwaddr, ifreq->ifr_hwaddr.sa_data, IF_HADDR);
            break;
        case SIOCGIFINDEX:
            ifi->index = ifreq->ifr_ifindex;
            break;

        case SIOCGIFBRDADDR:
            addr = (struct sockaddr_in *) &(ifreq->ifr_broadaddr);
            inet_ntop(AF_INET, &addr->sin_addr, ifi->broadaddr, IF_ADDR);
            break;
        case SIOCGIFDSTADDR:
            addr = (struct sockaddr_in *) &(ifreq->ifr_dstaddr);
            inet_ntop(AF_INET, &addr->sin_addr, ifi->dstaddr, IF_ADDR);
            break;
        case SIOCGIFNETMASK:
            addr = (struct sockaddr_in *) &(ifreq->ifr_netmask);
            inet_ntop(AF_INET, &addr->sin_addr, ifi->netmask, IF_ADDR);
            break;
    }

    return 0;
}

struct if_info * get_if_info(void)
{
    int sockfd, len;
    struct sockaddr_in * addr;
    struct ifreq * ifreq, * ifr, ifrbuf;
    struct if_info * ifi, * ifi_head, ** ifi_next;

    if((sockfd = udp_socket_ipv4()) < 0)
        return NULL;

    if ((len = _get_all_ifs(sockfd, &ifreq)) < 0) {
        close(sockfd);
        free(ifreq);
        return NULL;
    }

    ifi_head = NULL;
    ifi_next = &ifi_head;

    for (ifr = ifreq; (char *)ifr < ((char *)ifreq + len); ++ifr) { 
        ifrbuf = *ifr;

        ifi = calloc(1, sizeof(struct if_info)); 
        if (!(ifi = calloc(1, sizeof(struct if_info)))) {
            log_error("Calloc failed!");
            break;
        }
        *ifi_next = ifi;
        ifi_next = &(ifi->next);

        memcpy(ifi->name, ifr->ifr_name, IF_NAME);
        ifi->name[IF_NAME - 1] = '\0';

        addr = (struct sockaddr_in *) &(ifr->ifr_addr);
        inet_ntop(AF_INET, &addr->sin_addr, ifi->addr, IF_ADDR);

        _get_info(sockfd, ifi, &ifrbuf, SIOCGIFFLAGS);
        _get_info(sockfd, ifi, &ifrbuf, SIOCGIFINDEX);
        _get_info(sockfd, ifi, &ifrbuf, SIOCGIFMTU);
        _get_info(sockfd, ifi, &ifrbuf, SIOCGIFHWADDR);
        _get_info(sockfd, ifi, &ifrbuf, SIOCGIFBRDADDR);
        _get_info(sockfd, ifi, &ifrbuf, SIOCGIFNETMASK);

        if (ifi->flags & IFF_POINTOPOINT)
            _get_info(sockfd, ifi, &ifrbuf, SIOCGIFDSTADDR);
    }
    close(sockfd);
    free(ifreq);

    return ifi_head;
}

void free_if_info(struct if_info * if_info)
{
    struct if_info * next;

    while (if_info) {
        next = if_info->next;
        free(if_info);
        if_info = next;
    }

    return;
}

