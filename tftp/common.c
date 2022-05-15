/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：common.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年04月04日
 * 描    述：
 * 
 *===============================================================
 */
#include <assert.h>
#include <errno.h>
#include <unistd.h>

#include "tftp.h"
#include "../lib/socket.h"

static struct dict_type blocks_dict_type = {
    .key_destroy = NULL,
    .val_destroy = free,
    .key_dup = NULL,
    .val_dup = NULL,
    .key_compare = NULL 
};

void init_tftp_config(struct tftp_struct * tftp)
{
    memset(tftp, 0, sizeof(struct tftp_struct));
    tftp->mode = "octet";
    tftp->connected = 0;
    tftp->sockfd = -1;
    tftp->fd = -1;
    tftp->filesize = -1;
    tftp->block_size = 512;
    tftp->next_block_nr = 0;
    tftp->max_sent_block_nr = 20;
    tftp->max_retry_nr = 3;
    tftp->retrans_timeout.tv_sec = 3;  /* 3 s */
    if (!(tftp->blocks_waiting_ack = dict_create(&blocks_dict_type))) {
        log_error("Create dict error!");
        die("TFTP quit, init tftp error!");
    }
    tftp->sent_rate.tv_sec = 1; /* 1 s*/
    tftp->min_sent_rate.tv_sec = 1; /* 1 s */
    tftp->max_sent_rate.tv_usec = 1 * 1000; /* 1 ms */
    tftp->keepalive_timeout.tv_sec = 6; /* 6 s */

    tftp->stat_sent_blocks = 0;
    tftp->stat_sent_cnt = 0;
    tftp->stat_recv_cnt = 0;
    tftp->stat_retrans_cnt = 0;
}

static int _recv(int fd, char * data, int len)
{
    int i;
    socklen_t addr_len;
    struct sockaddr_in cli_addr;

again:
    if (G_tftp_config.connected) {
        i = read(fd, data, len);
    } else {
        /**
         * not connected
         */
        addr_len = sizeof(struct sockaddr_in);
        i = recvfrom(G_tftp_config.sockfd, data, len,
                0, (struct sockaddr *)&cli_addr, &addr_len);
        G_tftp_config.peer_addr = cli_addr;
    }

    if (i < 0) {
        if (errno == EINTR)
            goto again;
        log_error_sys("TFTP recv packet error!");
        return -1;
    }
    assert(i > 2);

    return i;
}

static char G_recv_buffer[1024];
static char G_pkt[128];
void * tftp_recv_pkt(void)
{
    char * buf;
    int code, len;
    struct tftp_ack_pkt * ack_pkt;
    struct tftp_req_pkt * req_pkt;
    struct tftp_data_pkt * data_pkt;
    struct tftp_error_pkt * error_pkt;

    buf = G_recv_buffer;
    if ((len = _recv(G_tftp_config.sockfd, buf, 1024)) < 0)
        die("TFTP quit, recv packet error!");
    
    code = ntohs(*((uint16_t *)buf)); 
    switch (code) {
    case TFTP_CODE_RRQ:
    case TFTP_CODE_WRQ:
        req_pkt = (struct tftp_req_pkt *)G_pkt;
        req_pkt->code = code;
        req_pkt->filename = buf + 2; 
        req_pkt->mode = req_pkt->filename + strlen(req_pkt->filename) + 1; 
        return req_pkt;
    case TFTP_CODE_ACK:
        ack_pkt = (struct tftp_ack_pkt *)G_pkt;
        ack_pkt->code = code;
        ack_pkt->block_nr = ntohs(*((uint16_t *)(buf + 2)));
        return ack_pkt;
    case TFTP_CODE_DATA:
        data_pkt = (struct tftp_data_pkt *)G_pkt;
        data_pkt->code = code;
        data_pkt->block_nr = ntohs(*((uint16_t *)(buf + 2)));

        /**
         * TODO: convert '/r/n' to '/n' if text file
         */
        data_pkt->data = buf + 4;
        data_pkt->len = len - 4;
        return data_pkt;
    case TFTP_CODE_ERROR:
        error_pkt = (struct tftp_error_pkt *)G_pkt;
        error_pkt->code = code;
        error_pkt->error_code = ntohs(*((uint16_t *)(buf + 2)));
        error_pkt->error_msg = buf + 4; 
        return error_pkt;
    default:
        log_error("Recv packet with unkown code[%d]!", code);
        die("TFTP quit, receive invlaid packet!");
    }

    return NULL; /* should never reach here */
}

static int _send(int fd, char * data, int len)
{
    int i, addr_len;

again:
    if (G_tftp_config.connected) {
        i = write(fd, data, len);
    } else {
        /**
         * not connected
         */
        addr_len = sizeof(struct sockaddr_in);
        i = sendto(G_tftp_config.sockfd, data, len, 0, 
                (struct sockaddr *)&G_tftp_config.peer_addr, addr_len);
    }

    if (i < 0) {
        if (errno == EINTR)
            goto again;
        log_error_sys("TFTP send packet error!");
        return -1;
    } else if (i < len) {
        log_error("Packet truncate!");
        return -1;
    }
    assert(i == len);

    return 0;
}

void tftp_reply_ack(int block_nr)
{
    char buf[4];

    *((uint16_t *)buf) = htons(TFTP_CODE_ACK);
    *((uint16_t *)(buf + 2)) = htons(block_nr);
    if (_send(G_tftp_config.sockfd, buf, 4) < 0)
        die("TFTP quit, send ack error!");
}

void tftp_relpy_error_msg(int error_code, char * msg)
{
    int len;
    static char buf[1024];

    if (error_code < 0) error_code = -error_code;
    *((uint16_t *)buf) = htons(TFTP_CODE_ERROR);
    *((uint16_t *)(buf + 2)) = htons(error_code);
    len = strlen(msg) > (sizeof buf - 5) ? (sizeof buf - 5): strlen(msg);
    memcpy(buf + 4, msg, len);
    buf[4 + len] = '\0';

    if (_send(G_tftp_config.sockfd, buf, 5 + len) < 0)
        die("TFTP quit, send error msg error!");
}

void tftp_send_req(int code, char * filename, char * mode)
{
    int len;
    static char buf[1024];

    if ((4 + strlen(filename) + strlen(mode)) > sizeof buf)
        die("TFTP quit, buffer overflow when send req!");

    *((uint16_t *)buf) = htons(code);
    len = strlen(filename) + 1;
    memcpy(buf + 2, filename, len);
    memcpy(buf + 2 + len, mode, strlen(mode) + 1);

    len = 4 + strlen(filename) + strlen(mode);
    if (_send(G_tftp_config.sockfd, buf, len) < 0)
        die("TFTP quit, send req error!");
}

void tftp_send_data_block(struct tftp_block_hdr * block)
{
    int i;
    char * buf;

    if (block->len < 0) {
        buf = block->data;
        *((uint16_t *)buf) = htons(TFTP_CODE_DATA);
        *((uint16_t *)(buf + 2)) = htons(block->block_nr);
        i = read(G_tftp_config.fd, buf + 4, G_tftp_config.block_size);
        if (i < 0) {
            tftp_relpy_error_msg(errno, strerror(errno));
            die_with_errno("TFTP quit, send block [%d] error!", block->block_nr);
        } else {
            block->len = i + 4;
            block->retrans_nr = 0;
        }
    } else {
        /* retransmission */
        ++block->retrans_nr;
    }

    /**
     * TODO: convert '/r/n' to '/n' if text file
     */
    if (_send(G_tftp_config.sockfd, 
                block->data, block->len) < 0)
        die("TFTP quit, send block [%d] error!", block->block_nr);
}

