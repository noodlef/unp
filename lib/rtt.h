/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：rtt.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月12日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __RTT_H
#define __RTT_H
#include <time.h>

struct rtt_info {
    int rtt;
    float srtt;
    float rttval;
    int rto;
    int nrxmt;
    int base;
};


void rtt_init(struct rtt_info * rtt_info);

void rtt_new_packet(struct rtt_info * rtt_info);

int rtt_get(struct rtt_info * rtt_info);

void rtt_update(struct rtt_info * rtt_info, time_t last_rtt);

int rtt_timeout(struct rtt_info * rtt_info);

time_t rtt_timestamp(void);

void rtt_debug(struct rtt_info * rtt_info);

#endif //RTT_H
