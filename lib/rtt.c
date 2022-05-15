/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：rtt.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月12日
 * 描    述：
 * 
 *===============================================================
 */
#include <time.h>

#include "rtt.h"

#define RTT_MIN_RXMT_TIMEOUT 2
#define RTT_MAX_RXMT_TIMEOUT 60 
#define RTT_MAX_RXMT_NUM     3

#define RTT_RTOCALC(rtt_info) (rtt_info->srtt + 4.0 * rtt_info->rttval)

static int _round_rto(float rto)
{
    if (rto < RTT_MIN_RXMT_TIMEOUT)
        return RTT_MIN_RXMT_TIMEOUT;

    if (rto > RTT_MAX_RXMT_TIMEOUT)
        return RTT_MAX_RXMT_TIMEOUT;

    return rto;
}

time_t rtt_timestamp(void)
{
    struct timespec tp;

    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec;
}

void rtt_init(struct rtt_info * rtt_info)
{
    rtt_info->base = rtt_timestamp(); 
    rtt_info->nrxmt = 0;
    rtt_info->rtt = 0;
    rtt_info->srtt = 0;
    rtt_info->rttval = 0.75;
    rtt_info->rto = _round_rto(RTT_RTOCALC(rtt_info));
}

void rtt_new_packet(struct rtt_info * rtt_info)
{
    rtt_info->nrxmt = 0;
}

int rtt_get(struct rtt_info * rtt_info)
{
    return rtt_info->rto; 
}

void rtt_update(struct rtt_info * rtt_info, time_t last_rtt)
{
    float delta;

    rtt_info->rtt = last_rtt;
    delta = rtt_info->rtt - rtt_info->srtt;
    rtt_info->srtt += delta / 8;

    if (delta < 0.0)
        delta = -delta;

    rtt_info->rttval += (delta - rtt_info->rttval) / 4;

    rtt_info->rto = _round_rto(RTT_RTOCALC(rtt_info));
}

int rtt_timeout(struct rtt_info * rtt_info)
{
    rtt_info->rto *= 2;

    if (++rtt_info->nrxmt > RTT_MAX_RXMT_NUM)
        return -1;

    return 0;
}

