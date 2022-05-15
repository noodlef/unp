/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：tftp.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年04月15日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "tftp.h"
#include "../lib/utils.h"

static int G_retrans_nr = 0;

extern char * format_str(char * fmt, ...);

static int recv_data_ack(struct eventloop * loop, int fd, void * user_data)
{
    int block_nr;
    struct tftp_ack_pkt * pkt;
    struct tftp_error_pkt * err_pkt;
    struct tftp_block_hdr * block;

    pkt = tftp_recv_pkt();
    if (pkt->code != TFTP_CODE_ACK) {
        if (pkt->code == TFTP_CODE_ERROR) {
            err_pkt = (struct tftp_error_pkt *) pkt;
            die("TFTP quit, recv error msg from the perr, error_code[%d], "
                "error_msg: %s", err_pkt->error_code, err_pkt->error_msg);
        } else {
            tftp_relpy_error_msg(1, "ERROR: not receiving ack!");
            die("TFTP quit, recv invalid packet, code[%d]", pkt->code);
        }
    }

    block_nr = pkt->block_nr;
    if (dict_find(G_tftp_config.blocks_waiting_ack, 
                INT2VOIDP(block_nr), (void **)&block) < 0) {
        log_warning("block[%d] not found in blocks_waiting_ack queue!", block_nr);
        return 0;
    }

    log_info("Recv ack of block[%d].", block_nr);
    eventloop_time_event_del(loop, block->timer_id);
    dict_delete(G_tftp_config.blocks_waiting_ack, INT2VOIDP(block_nr));
    ++G_tftp_config.stat_sent_blocks;

    if ((G_tftp_config.block_size * G_tftp_config.next_block_nr) 
            > G_tftp_config.filesize 
         && !dict_size(G_tftp_config.blocks_waiting_ack)) {
        /* transmission finished */
        log_info("Transmission finished successfully!");
        eventloop_stop(loop);
        return 0;
    }

    return 0;
}

static int _retransmit_block(struct eventloop * loop, 
        time_event_id id, void * user_data)
{
    int block_nr;
    struct tftp_block_hdr * block;

    block_nr = VOIDP2INT(user_data);
    if (dict_find(G_tftp_config.blocks_waiting_ack, 
                INT2VOIDP(block_nr), (void **)&block) < 0) {
        log_warning("Retransmit block[%d] error, block not found!", block_nr);
        return 0;
    }

    if (block->retrans_nr >= G_tftp_config.max_retry_nr) {
        tftp_relpy_error_msg(0, "ERROR: Quit retransmit block");
        die("TFTP quit, quit retransmit block[%d]", block->block_nr);
    }

    log_info("Begin to retransmit block[%d]", block->block_nr);
    tftp_send_data_block(block);

    G_retrans_nr = 1; /* retransmission, mark it */
    ++G_tftp_config.stat_sent_cnt;
    ++G_tftp_config.stat_retrans_cnt;

    return 0;
}

static int _send_block(struct eventloop * loop, 
        time_event_id id, void * user_data);
static void _update_sent_rate_if_needed(
        struct eventloop * loop, time_event_id event_id)
{
    struct poll_time_event timer;
    struct timeval tmp = {0, 100 * 1000 }, tv1 = {0, 1 * 1000};

    if (G_retrans_nr || timeval_compare(
                &G_tftp_config.sent_rate, &G_tftp_config.max_sent_rate) > 0) {
        if (G_retrans_nr) {
            G_tftp_config.sent_rate = G_tftp_config.min_sent_rate;
            G_retrans_nr = 0;
        } else {
            if (timeval_compare(&G_tftp_config.sent_rate, &tmp) > 0) {
                G_tftp_config.sent_rate = timeval_sub(&G_tftp_config.sent_rate, &tmp);              
            } else {
                G_tftp_config.sent_rate = timeval_sub(&G_tftp_config.sent_rate, &tv1);              
            }
            if (timeval_compare(&G_tftp_config.sent_rate, &G_tftp_config.max_sent_rate) < 0)
                G_tftp_config.sent_rate = G_tftp_config.max_sent_rate;
        }

        eventloop_time_event_del(loop, event_id);

        INIT_POLL_TIME_EVENT(&timer);
        timer.interval = G_tftp_config.sent_rate;
        timer.perodic = 1;
        timer.on_expired = _send_block;
        if (eventloop_time_event_create(loop, &timer) < 0) {
            tftp_relpy_error_msg(0, "Internal error!");
            die("TFTP quit, create time event error!");
        }

    }
}

static int _send_block(struct eventloop * loop, 
        time_event_id id, void * user_data)
{
    int i;
    struct poll_time_event event;
    struct tftp_block_hdr * block;

    if ((G_tftp_config.block_size * G_tftp_config.next_block_nr) 
            > G_tftp_config.filesize) {
        if (!dict_size(G_tftp_config.blocks_waiting_ack)) {
            /* transmission finished */
            log_info("Transmission finished successfully!");
            eventloop_stop(loop);
        } else {
            eventloop_time_event_del(loop, id);
        }
        log_info("No more data to send, quit!");
        return 0;
    }

    if (dict_size(G_tftp_config.blocks_waiting_ack) 
            > G_tftp_config.max_sent_block_nr) {
        log_info("Number of blocks waiting for ack has reach max limit!");
        return 0; /* we will try again later */
    }

    /* finally, we can send another packet */
    i = sizeof(struct tftp_block_hdr) +  4 + G_tftp_config.block_size;
    if (!(block = malloc(i))) {
        tftp_relpy_error_msg(0, "ERROR: OOM when sending data");
        die("TFTP quit, OOM when sending data!");
    }

    block->len = -1; /* it will be filled later */
    block->block_nr = G_tftp_config.next_block_nr++;
    block->retrans_nr = 0;

    log_info("Begin to send block[%d].", block->block_nr);
    tftp_send_data_block(block);
    ++G_tftp_config.stat_sent_cnt;
    dict_add(G_tftp_config.blocks_waiting_ack, 
            INT2VOIDP(block->block_nr), block);

    /**
     * retrasmit the block if it is not confirmed by ack 
     */
    INIT_POLL_TIME_EVENT(&event);
    event.interval = G_tftp_config.retrans_timeout;
    event.perodic = 1;
    event.on_expired = _retransmit_block;
    event.user_data = INT2VOIDP(block->block_nr);
    event.free_user_data = NO_FREE_USE_DATA_HANDLE; /* NOTE */
    if ((block->timer_id = eventloop_time_event_create(
                    loop, &event)) < 0) {
        tftp_relpy_error_msg(0, "Internal error!");
        die("TFTP quit, create time event error!");
    }

    _update_sent_rate_if_needed(loop, id);

    return 0;
}

void rrq_init(struct eventloop * loop)
{
    int fd;
    char * filename;
    struct stat stat;
    struct poll_file_event event;
    struct poll_time_event timer;

    filename = G_tftp_config.filename;
    if ((fd = open(filename, O_RDONLY)) < 0) {
        tftp_relpy_error_msg(errno, format_str("Open file(%s) error!, err: %s.", 
                    filename, strerror(errno)));
        die_with_errno("TFTP quit, open file(%s) error!", filename);
    }

    if (fstat(fd, &stat) < 0) {
        tftp_relpy_error_msg(errno, format_str("Fstat file(%s) error!, err: %s.", 
                    filename, strerror(errno)));
        die_with_errno("TFTP quit, stat file(%s) error!", filename);
    }
    G_tftp_config.fd = fd;
    G_tftp_config.filesize = stat.st_size;

    /**
     * register file event, recv ack
     */
    INIT_POLL_FILE_EVENT(&event);
    event.events = EVENT_READABLE;
    event.on_read = recv_data_ack;
    if (eventloop_file_event_create(loop, G_tftp_config.sockfd, &event) < 0) {
        tftp_relpy_error_msg(0, "Internal error!");
        die("TFTP quit, create file event error!");
    }

    /**
     * register timer event, send a packet every 100 ms 
     */
    INIT_POLL_TIME_EVENT(&timer);
    timer.interval = G_tftp_config.sent_rate;
    timer.perodic = 1;
    timer.on_expired = _send_block;
    if (eventloop_time_event_create(loop, &timer) < 0) {
        tftp_relpy_error_msg(0, "Internal error!");
        die("TFTP quit, create time event error!");
    }
}

static void _update_last_recv_time(void)
{
    gettimeofday(&G_tftp_config.last_recv_time, NULL);
}

static char G_last_block_received = 0;
static int _recv_data_block(struct eventloop * loop, int fd, void * user_data)
{
    int offset;
    struct tftp_data_pkt * pkt;


    pkt = tftp_recv_pkt();

#if 0
    static int loops = 1;
    if (loops++ == 30) {
        return 0;
    }
#endif

    _update_last_recv_time();

    if (pkt->code == TFTP_CODE_DATA) {
        ++G_tftp_config.stat_recv_cnt;

        if (pkt->len < G_tftp_config.block_size)
            G_last_block_received = 1;
        /**
         * not guranteed to receive blocks in order, so we
         * must set the proper offset of 'file' before every write
         */
        offset = pkt->block_nr * G_tftp_config.block_size;
        if (lseek(G_tftp_config.fd, offset, SEEK_SET) < 0) {
            tftp_relpy_error_msg(errno, "ERROR: lseek file error!");
            die_with_errno("Recv data: lseek error!");
        }
        if (write(G_tftp_config.fd, pkt->data, pkt->len) != pkt->len) {
            tftp_relpy_error_msg(errno, "ERROR: write file error!");
            die_with_errno("Recv data: write error!");
        }
        tftp_reply_ack(pkt->block_nr);
    } else if (pkt->code == TFTP_CODE_ERROR) {
        die("TFTP quit, recv error: %s!", 
                ((struct tftp_error_pkt *)pkt)->error_msg);
    } else {
        tftp_relpy_error_msg(1, "ERROR: recv packet with invalid code!");
        die("TFTP quit, recv packet with no ack, code[%d]", pkt->code);
    }

    return 0;
}

static int _check_sender_alive(struct eventloop * loop, 
        time_event_id id, void * user_data)
{
    struct timeval now, last_recv_time, timeout; 

    gettimeofday(&now, NULL);
    timeout = G_tftp_config.keepalive_timeout;
    last_recv_time = G_tftp_config.last_recv_time;

    if (((now.tv_sec - last_recv_time.tv_sec) > timeout.tv_sec) ||
            (((now.tv_sec - last_recv_time.tv_sec) == timeout.tv_sec) && 
            ((now.tv_usec - last_recv_time.tv_usec) > timeout.tv_usec))) {
        if (G_last_block_received) {
            /**
             * not for sure if we are really done, but since there
             * is no other way to check if the transmission is finished,
             * here it's the best place to stop
             */
            log_info("Transmission finished successfully.");
            eventloop_stop(loop);
            return 0;
        } else {
            die("Check alive timeout!");
        }
    }

    log_info("Check sender alive...");
    return 0;
}

void wrq_init(struct eventloop * loop)
{
    int fd;
    char * filename;
    struct poll_file_event event;
    struct poll_time_event timer;

    filename = G_tftp_config.filename;
    if ((fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, FILE_MODE)) < 0) {
        tftp_relpy_error_msg(errno, format_str("Open file(%s) error!, err: %s.", 
                    filename, strerror(errno)));
        die_with_errno("TFTP quit, open file(%s) error!", filename);
    }
    G_tftp_config.fd = fd;

    /**
     * register file event, recv data block 
     */
    INIT_POLL_FILE_EVENT(&event);
    event.events = EVENT_READABLE;
    event.on_read = _recv_data_block;
    if (eventloop_file_event_create(loop, G_tftp_config.sockfd, &event) < 0) {
        tftp_relpy_error_msg(0, "Internal error!");
        die("TFTP quit, create file event error!");
    }

    /**
     * register timer event, check if the sender alive 
     */
    INIT_POLL_TIME_EVENT(&timer);
    timer.interval = G_tftp_config.keepalive_timeout;
    timer.perodic = 1;
    timer.on_expired = _check_sender_alive;
    if (eventloop_time_event_create(loop, &timer) < 0) {
        tftp_relpy_error_msg(0, "Internal error!");
        die("TFTP quit, create time event error!");
    }

    _update_last_recv_time(); /* init */
}

void tftp_server(void)
{
    struct tftp_req_pkt * pkt;
    struct eventloop * main_loop;

    if (!(main_loop = eventloop_create()))
        die("TFTP quit, create eventloop failed!");

    pkt = tftp_recv_pkt();
    G_tftp_config.filename = strdup(pkt->filename);  
    G_tftp_config.mode = strdup(pkt->mode);

    switch(pkt->code) {
    case TFTP_CODE_RRQ:
        rrq_init(main_loop);
        break;
    case TFTP_CODE_WRQ:
        wrq_init(main_loop);
        break;
    default:
        tftp_relpy_error_msg(0, "Invalid code of req!");
        die("TFTP quit unexpectedly!, recv unvalid req, code: %d", 
                pkt->code);
    }

    /**
     * Main-loop
     */
    eventloop_events_dispatch(main_loop);
    eventloop_destroy(main_loop);
}

static int _show_progress(struct eventloop * loop, 
        time_event_id id, void * user_data)
{
    static int last_stat_sent_cnt = 0;
    static int last_stat_recv_cnt = 0;
    double rate;
    int sent_bytes, recv_bytes;

    printf("\033[s"); /* save the cursor position */
    printf("\033[K"); /* clear current line */

    printf("%s", G_tftp_config.filename);
    if (G_tftp_config.filesize < 0) {
        /* download */
        recv_bytes = get_real_file_size(G_tftp_config.fd);
        rate = (G_tftp_config.stat_recv_cnt - last_stat_recv_cnt) * 
            G_tftp_config.block_size / 1024.0 * 2;

        if (recv_bytes > 1024 * 1024)
            printf("\t\t%.2f Mb", recv_bytes / (1024 * 1024.0));
        else
            printf("\t\t%.2f Kb", recv_bytes / 1024.0);

        last_stat_recv_cnt = G_tftp_config.stat_recv_cnt;
    } else {
        /* upload */
        sent_bytes = G_tftp_config.stat_sent_blocks * G_tftp_config.block_size;
        if (sent_bytes > G_tftp_config.filesize)
            sent_bytes = G_tftp_config.filesize;

        rate = (G_tftp_config.stat_sent_cnt - last_stat_sent_cnt) * 
            G_tftp_config.block_size / 1024.0 * 2;

        if (G_tftp_config.filesize > 1024 * 1024)
            printf("\t\t%.2f Mb/%.2f Mb", sent_bytes / (1024 * 1024.0), 
                    G_tftp_config.filesize / (1024 * 1024.0));
        else
            printf("\t\t%.2f Kb/%.2f Kb", sent_bytes / 1024.0, 
                    G_tftp_config.filesize / 1024.0);
        printf("\t\t%.2f%%", sent_bytes * 1.0 / G_tftp_config.filesize * 100);

        last_stat_sent_cnt = G_tftp_config.stat_sent_cnt;
    }

    if (rate > 1024)
        printf("\t\t%.2f Mb/s", rate / 1024.0);
    else
        printf("\t\t%.2f Kb/s", rate);

    fflush(NULL);
    printf("\033[u"); /* restore the cursor position */

    return 0;
}

void tftp_cli(int download_or_upload, char * file)
{
    struct eventloop * main_loop;
    struct poll_time_event timer;

    if (!(main_loop = eventloop_create()))
        die("TFTP quit, create eventloop failed!");

    if (download_or_upload) {
        /* upload */
        rrq_init(main_loop);
        tftp_send_req(TFTP_CODE_WRQ, file, G_tftp_config.mode);
    } else  {
        /* download */
        wrq_init(main_loop);
        tftp_send_req(TFTP_CODE_RRQ, file, G_tftp_config.mode); 
    }

    INIT_POLL_TIME_EVENT(&timer);
    timer.interval.tv_usec = 500 * 1000; // 500 ms
    timer.perodic = 1;
    timer.on_expired = _show_progress;
    if (eventloop_time_event_create(main_loop, &timer) < 0) {
        tftp_relpy_error_msg(0, "Internal error!");
        die("TFTP quit, create time event error!");
    }

    /**
     * Main-loop
     */
    eventloop_events_dispatch(main_loop);
    _show_progress(NULL, 0, NULL); /* 100 % finally */

    eventloop_destroy(main_loop);
}
