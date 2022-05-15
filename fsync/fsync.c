/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：tmp.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月28日
 * 描    述：
 * 
 *===============================================================
 */
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/stat.h>

#include "../lib/log.h"
#include "../lib/buffer.h"

#include "debug.h"
#include "fsync.h"
#include "utils.h"
#include "packet.h"
#include "network.h"

static int G_dirfd = -1;
static char G_dir_path[FILENAME_SIZE];

static int parse_upload_req(struct filesync_cmd_header * hdr, 
        struct cmd_payload_req * req)
{
    int fd;
    char md5[MD5_SIZE];
    struct cmd_payload_ack ack;

    fd = openat(G_dirfd, req->filename, O_RDONLY);
    /* file does not exist */
    if (fd < 0 && errno != ENOENT) {
        log_warning_sys("Open file(%s) error!", req->filename);
        return -1;
    }

    if (fd > 0) {
        compute_file_md5(fd, (unsigned char *)&md5);
        if (!memcmp(req->md5, &md5, MD5_SIZE))
            return 0;
        log_info("Fsync: MD5(%s) of file(%s) does not equal with MD5(%s)!", 
                req->filename, str_md5(req->md5), str_md5(md5));
    }

    if (fd < 0)
        log_info("Fsync: file(%s) does not exist!", req->filename);

    ack.len = req->len;
    memcpy(ack.filename, req->filename, req->len);
    write_cmd_payload_ack_to_buffer(G_send_buffer, &ack);

    return 0;
}

static void recv_and_parse_cmd_req(struct filesync_cmd_header * cmd_hdr, 
        parse_cmd_req parse_req)
{
    int i, k;
    struct cmd_payload_req payload_req;

    /**
     * 根据报文头部的长度字段，读出完整的报文体, 然后首先读
     * 出报文体的第一项: 同步目录名信息
     */
    i = cmd_hdr->length;
    if ((k = socket_buffer_size(G_recv_buffer)) < i) {
        socket_read_at_least(G_conn_fd, G_recv_buffer, i - k); 
    }

    read_cmd_payload_req_from_buffer(G_recv_buffer, &payload_req);
    memcpy(G_dir_path, payload_req.filename, payload_req.len + 1);
    log_info("Fsync with dir: %s", G_dir_path);

#ifdef __FSYNC_DEBUG_ON__
    log_info("Recv req-entry: >>>>>>");
    log_cmd_payload_req(&payload_req);
#endif

    if ((G_dirfd = open(payload_req.filename, 
                    O_RDONLY | O_DIRECTORY)) < 0) {
        log_error_sys("Open dir(%s) error!", payload_req.filename);
        die_unexpectedly(NULL);
    }

    /**
     * 接下来依次解析报文体的每一项，在解析的同时也构造应答报问体 
     */
    i = socket_buffer_size(G_recv_buffer); 
    while (i > 0) {
        i -= read_cmd_payload_req_from_buffer(G_recv_buffer, &payload_req);

#ifdef __FSYNC_DEBUG_ON__
        log_info("Recv req-entry: >>>>>>");
        log_cmd_payload_req(&payload_req);
#endif

        if (parse_req && parse_req(cmd_hdr, &payload_req) < 0) {
            log_warning("Parse request error!");
            die_unexpectedly(NULL);
        }
    }
}

static void do_sync_info_req(struct filesync_cmd_header * cmd_hdr)
{
    uint32_t i;

    /* recv and parse req */
    recv_and_parse_cmd_req(cmd_hdr, NULL);

    prepare_cmd_request(CMD_SYNCINFO_ACK, NULL, G_dir_path);

    /**
     * 计算出请求体的长度，然后更新缓冲区中该字段的值
     */
    i = socket_buffer_size(G_send_buffer) - FILESYNC_CMD_HEADER_LEN;
    i = htonl(i);
    socket_buffer_replace(G_send_buffer, 2, (char *)&i, 4);

    /* send reply */
    i = socket_buffer_size(G_send_buffer);
    socket_write_exactly(G_conn_fd, G_send_buffer, i);
}

static struct dict * G_file_info_map = NULL;
static int parse_download_req(struct filesync_cmd_header * hdr, 
        struct cmd_payload_req * req)
{
    dict_add(G_file_info_map, req->filename, req->md5);
    return 0;
}

/**
 * eg: fullpath: /home/mydir/test.c, len: 11(/home/mydir)
 */
static int send_file(char * fullpath, void * args)
{
    int fd, i;
    struct stat statbuf;
    char md5[MD5_SIZE], * md51, * filename;
    struct filesync_file_header file_hdr;

    if (is_file_filtered(fullpath))
        return 0;

    if ((fd = open(fullpath, O_RDONLY)) < 0) {
        log_warning_sys("Open file(%s) error!", fullpath);
        return -1;
    }
    compute_file_md5(fd, (unsigned char *)md5);

    i = *((int *)args);
    filename = fullpath + i + 1;

    log_info("Begin to send file >>>>>> %s", filename);
    log_info("local file md5: %s", str_md5(md5));
    if (dict_find(G_file_info_map, filename, (void *)&md51) < 0) {
        log_info("remote file does not exist!");
    } else {
        if (!memcmp(md5, md51, MD5_SIZE)) {
            log_info("File content same with the remote!"); 
            return 0;
        }
        log_info("remote file md5: %s", str_md5(md51));
    }

    /* send file hdr */
    if (fstat(fd, &statbuf) < 0) {
        log_warning_sys("Stat file(%s) error!", filename);
        close(fd);
        return -1;
    }

    file_hdr.file_type = 0;
    file_hdr.file_mode = 0;
    file_hdr.filename_len = strlen(filename);
    file_hdr.length = statbuf.st_size;
    memcpy(file_hdr.filename, filename, strlen(filename));

    write_file_hdr_to_buffer(G_send_buffer, &file_hdr);
    i = socket_buffer_size(G_send_buffer);
    socket_write_exactly(G_conn_fd, G_send_buffer, i);

    socket_send_file(G_conn_fd, fd);

    return 0;
}

static void do_download_req(struct filesync_cmd_header * cmd_hdr)
{
    int length;
    struct filesync_file_header file_hdr;

    /**
     * 解析请求把其中的文件信息保存在字典中
     */
    G_file_info_map = dict_create(&file_info_map_type);
    recv_and_parse_cmd_req(cmd_hdr, parse_download_req); 

    /**
     * 遍历目录发送文件 
     */
    length = strlen(G_dir_path);
    if (search_dir_recursively(G_dir_path, send_file, 
                &length) < 0)
        die_unexpectedly("search dir and send file error!!!");

    /**
     * 发送结束包文，使用 filename_len 字段为0来标记
     */
    file_hdr.file_type = 0;
    file_hdr.file_mode = 0;
    file_hdr.length = 0;
    file_hdr.filename_len = 0;
    write_file_hdr_to_buffer(G_send_buffer, &file_hdr);
    length = socket_buffer_size(G_send_buffer);
    socket_write_exactly(G_conn_fd, G_send_buffer, length);
}

static void do_upload_req(struct filesync_cmd_header * cmd_hdr)
{
    uint32_t i;
    struct filesync_cmd_header reply_hdr;

    /* fill the header */
    reply_hdr.version = 1;
    reply_hdr.cmd = CMD_UPLOAD_ACK;
    reply_hdr.length = 0;
    write_cmd_hdr_to_buffer(G_send_buffer, &reply_hdr);

    /* recv and parse req */
    recv_and_parse_cmd_req(cmd_hdr, parse_upload_req); 

    /**
     * 计算出请求体的长度，然后更新缓冲区中该字段的值
     */
    i = socket_buffer_size(G_send_buffer) - FILESYNC_CMD_HEADER_LEN;
    i = htonl(i);
    socket_buffer_replace(G_send_buffer, 2, (char *)&i, 4);

#ifdef __FSYNC_DEBUG_ON__
    struct socket_buffer * copyed;
    copyed = socket_buffer_copy(G_send_buffer);

    log_info("Reply is as follow:");
    log_full_cmd_hdr(copyed);
    socket_buffer_free(copyed);
#endif

    /* send reply */
    i = socket_buffer_size(G_send_buffer);
    socket_write_exactly(G_conn_fd, G_send_buffer, i);

    /* recv files and save to the disks */
    recv_and_save_files(G_dir_path);
}

void filesync_server(void) 
{
    struct filesync_cmd_header cmd_hdr;

    /* read cmd_header */ 
    socket_read_at_least(G_conn_fd, G_recv_buffer, FILESYNC_CMD_HEADER_LEN);
    read_cmd_hdr_from_buffer(G_recv_buffer, &cmd_hdr);

    log_info("Recv filesync req as follow: ");
    log_cmd_hdr(&cmd_hdr);

    switch (cmd_hdr.cmd) {
    case CMD_DOWNLOAD_REQ:
        do_download_req(&cmd_hdr);
        break;
    case CMD_UPLOAD_REQ:
        do_upload_req(&cmd_hdr);
        break;
    case CMD_SYNCINFO_REQ:
        do_sync_info_req(&cmd_hdr);
        break;
    default:
        log_info("Invalid cmd code(%d)", cmd_hdr.cmd);
        exit(EXIT_FAILURE);
    }
}
