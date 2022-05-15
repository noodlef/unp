/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：tftp.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年04月03日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __TFTP_H
#define __TFTP_H
#include <inttypes.h>
#include <sys/time.h> 
#include <netinet/in.h>

#include "../lib/dict.h"
#include "../lib/log.h"
#include "../lib/eventloop/eventloop.h"

#define TFTP_CODE_RRQ   0
#define TFTP_CODE_WRQ   1
#define TFTP_CODE_DATA  2
#define TFTP_CODE_ACK   3
#define TFTP_CODE_ERROR 4

/**
 * TFTP protocols:
 *
 * 1. read or write request:
 *    ++++++++++++++++++++++++++++++++++++++++ 
 *    +       +                +             + 
 *    +  code +    filename \0 +    mode \0  + 
 *    +       +                +             + 
 *    ++++++++++++++++++++++++++++++++++++++++ 
 *       2           N                N
 *
 * 2. data:
 *    +++++++++++++++++++++++++++++++++++++++++++++++++++ 
 *    +       +           +                             + 
 *    +  code +  block_nr +            data ...         +
 *    +       +           +                             + 
 *    +++++++++++++++++++++++++++++++++++++++++++++++++++ 
 *       2           2               0 - 512 
 *
 * 3. ack:
 *    +++++++++++++++++++++
 *    +       +           +
 *    +  code +  block_nr +
 *    +       +           +
 *    +++++++++++++++++++++
 *       2           2    
 *
 * 4. error messages:
 *    +++++++++++++++++++++++++++++++++++++++++++++++++++ 
 *    +       +            +                            + 
 *    +  code + error_code +           err_msg \0       + 
 *    +       +            +                            + 
 *    +++++++++++++++++++++++++++++++++++++++++++++++++++ 
 *       2           2               N 
 *
 */
struct tftp_req_pkt {
    uint16_t code;
    char * filename;
    char * mode;
};

struct tftp_data_pkt {
    uint16_t code;
    uint16_t block_nr;
    int len;
    char * data;
};

struct tftp_ack_pkt {
    uint16_t code;
    uint16_t block_nr;
};

struct tftp_error_pkt {
    uint16_t code;
    uint16_t error_code;
    char * error_msg;
};

struct tftp_block_hdr {
    int len; /* block size */
    int block_nr;
    time_event_id timer_id;
    int retrans_nr; /* retransmission counts */
    char data[0];
};

struct tftp_struct {
    /**
     * Text file that every line end up with '/r/n' 
     * if mode equals 'netascii', binary file if mode 
     * equals 'octet'. 
     */
    char * mode;
    char * filename;

    char connected; /* already invoked 'connect' on socket if True */
    int sockfd; /* udp socket fd */
    struct sockaddr_in peer_addr;

    int fd; /* file fd */
    int filesize;
    int block_size;
    /* number of next block to be sent */
    int next_block_nr;

    /**
     * max number of blocks that have not been confirmed by ack 
     */
    int max_sent_block_nr;
    int max_retry_nr; /* max retransmission number before quit */
    struct timeval retrans_timeout; /* retransmission timeout */
    struct dict * blocks_waiting_ack;

    struct timeval sent_rate; /* interval of sending packet */
    struct timeval min_sent_rate;
    struct timeval max_sent_rate;
    struct timeval last_recv_time;
    struct timeval keepalive_timeout;

    int stat_sent_blocks;
    int stat_sent_cnt;
    int stat_retrans_cnt;
    int stat_recv_cnt;
};

extern struct tftp_struct G_tftp_config;

extern void init_tftp_config(struct tftp_struct * tftp);
extern void tftp_server(void);
extern void tftp_cli(int download_or_upload, char * file);
extern void * tftp_recv_pkt(void);
extern void tftp_reply_ack(int block_nr);
extern void tftp_relpy_error_msg(int error_code, char * msg);
extern void tftp_send_req(int code, char * filename, char * mode);
extern void tftp_send_data_block(struct tftp_block_hdr * block);
#endif
