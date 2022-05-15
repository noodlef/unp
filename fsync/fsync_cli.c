/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：filesync_client.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月24日
 * 描    述：
 * 
 *===============================================================
 */
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../lib/log.h"
#include "../lib/utils.h"
#include "../lib/buffer.h"
#include "../lib/dict.h"

#include "fsync.h"
#include "debug.h"
#include "utils.h"
#include "packet.h"
#include "network.h"

extern char * G_cmd;
extern char * G_remote_dir;
extern char * G_local_dir;

static int G_current_dir_fd = 0;

static void recv_cmd_reply(parse_cmd_reply parse_reply)
{
    int i, cmd;
    void * payload_entry;
    struct filesync_cmd_header reply_hdr;
    struct cmd_payload_ack payload_ack;
    struct cmd_payload_req payload_req;

    /**
     * 接收应答，先读出头部，根据长度字段，读出完整的应答包
     */
    socket_read_at_least(G_conn_fd, G_recv_buffer, FILESYNC_CMD_HEADER_LEN);
    read_cmd_hdr_from_buffer(G_recv_buffer, &reply_hdr);

#ifdef __FSYNC_DEBUG_ON__
    log_cmd_hdr(&reply_hdr);
#endif

    cmd = reply_hdr.cmd;
    assert(cmd == CMD_UPLOAD_ACK || cmd == CMD_SYNCINFO_ACK);

    i = reply_hdr.length - socket_buffer_size(G_recv_buffer);
    socket_read_at_least(G_conn_fd, G_recv_buffer, i);

    /**
     * 依次解析处理应答体中的每一项，应答体格式如下： 
     *     upload-reply: [len][filename1][len][filename2]...
     *     info-reply:   [len][filename1][md5][len][filename2][md5]...
     */
    i = reply_hdr.length;
    if (i == 0 && cmd == CMD_UPLOAD_ACK) {
        printf("Already synchronized with the remote!\n");
        return;
    }

    while (i > 0) {
        if (cmd == CMD_UPLOAD_ACK) {
            i -= read_cmd_payload_ack_from_buffer(G_recv_buffer, &payload_ack);
            payload_entry = &payload_ack;
#ifdef __FSYNC_DEBUG_ON__
            log_cmd_payload_ack(&payload_ack);
#endif
        } else {
            i -= read_cmd_payload_req_from_buffer(G_recv_buffer, &payload_req);
            payload_entry = &payload_req;
#ifdef __FSYNC_DEBUG_ON__
            log_cmd_payload_req(&payload_req);
#endif
        }

        if (parse_reply && parse_reply(&reply_hdr, payload_entry) < 0) {
            printf("parse reply payload error!\n");
            die_unexpectedly(NULL);
        }
    }

    i = socket_buffer_size(G_recv_buffer);
    assert(i == 0);
}

static int parse_upload_reply(struct filesync_cmd_header * hdr, 
        void * payload)
{
    int fd, i;
    char * filename;
    struct stat statbuf;
    struct cmd_payload_ack * ack; 
    struct filesync_file_header file_hdr;

    ack = (struct cmd_payload_ack *)payload;
    filename = ack->filename;
    printf("%-64s>>>>>>         ", filename);
    
    if ((fd = openat(G_current_dir_fd, filename, O_RDONLY)) < 0) {
        log_warning_sys("Open file(%s) error!", filename);
        return -1;
    }

    if (fstat(fd, &statbuf) < 0) {
        log_warning_sys("Stat file(%s) error!", filename);
        close(fd);
        return -1;
    }

    file_hdr.file_type = 0;
    file_hdr.file_mode = 0;
    file_hdr.filename_len = ack->len;
    file_hdr.length = statbuf.st_size;
    memcpy(file_hdr.filename, filename, ack->len);

    /* send file hdr */
    write_file_hdr_to_buffer(G_send_buffer, &file_hdr);
    i = socket_buffer_size(G_send_buffer);
    socket_write_exactly(G_conn_fd, G_send_buffer, i);

    socket_send_file(G_conn_fd, fd);
    printf("%-12d bytes transfered\n", (int)statbuf.st_size);

    return 0;
}

void do_upload(void)
{
    int count;
    struct filesync_file_header file_hdr;

    /**
     * 打开目录，然后遍历目录下的文件，把文件名和其md5值等信息
     * 添加到请求体中，最后把准备好的请求体发送出去
     */
    if ((G_current_dir_fd = open(G_local_dir, 
                    O_RDONLY | O_DIRECTORY)) < 0) {
        printf("Open dir(%s) error, errno: %s\n", G_local_dir, strerror(errno));
        die_unexpectedly(NULL);
    }

    /* prepare the request */
    prepare_cmd_request(CMD_UPLOAD_REQ, G_remote_dir, G_local_dir); 

#ifdef __FSYNC_DEBUG_ON__
    struct socket_buffer * copyed;
    copyed = socket_buffer_copy(G_send_buffer);

    log_info("Upload REQ info:");
    log_full_cmd_hdr(copyed);
    socket_buffer_free(copyed);
#endif

    /* send the request to the peer */
    count = socket_buffer_size(G_send_buffer);
    socket_write_exactly(G_conn_fd, G_send_buffer, count);

    /**
     * 接收应答，根据应答中给出的信息，逐个发送文件到远端
     */
    recv_cmd_reply(parse_upload_reply);

    /**
     * 发送结束包文，使用 filename_len 字段为0来标记
     */
    file_hdr.file_type = 0;
    file_hdr.file_mode = 0;
    file_hdr.length = 0;
    file_hdr.filename_len = 0;
    write_file_hdr_to_buffer(G_send_buffer, &file_hdr);
    count = socket_buffer_size(G_send_buffer);
    socket_write_exactly(G_conn_fd, G_send_buffer, count);
}

static struct dict * G_remote_file_info_map = NULL;
static struct dict * G_local_file_info_map = NULL;

int get_local_files_info(char * fullpath, void * args)
{
    int fd, len;
    char md5[MD5_SIZE], * filename;

    len = *((int *)args);
    filename = fullpath + len + 1;

    if (is_file_filtered(fullpath))
        return 0;
    
    if ((fd = open(fullpath, O_RDONLY)) < 0) {
        log_warning_sys("Open file(%s) error!", fullpath);
        return -1;
    }
    compute_file_md5(fd, (unsigned char *)md5);

    dict_add(G_local_file_info_map, filename, md5);

    return 0;
}

inline void repeat_print_str(char * s, unsigned int count, char * end)
{
    while (count-- > 0)
        printf("%s", s);

    if (end)
        printf("%s", end);
}

void show_pretty_sync_info(void)
{
    int length;
    struct dict_entry * entry;
    char * filename, * md51, * md52;

    /**
     * 获取本地目录文件的信息并存方到字典中
     */
    length = strlen(G_local_dir);
    if (search_dir_recursively(G_local_dir, get_local_files_info, 
                &length) < 0)
        die_unexpectedly("Search local dir error!!!");

    /**
     * 本地目录存在，远端目录不存在的文件
     */
    printf("\nFiles only exist in local dir:\n");
    for (entry = dict_first(G_local_file_info_map); entry;
            entry = dict_next(entry)) {
        filename = dict_fetch_key(entry);
        md51 = dict_fetch_val(entry);

        if (dict_find(G_remote_file_info_map, filename, 
                    (void**)&md52) >= 0)
            continue;

        printf("  %s\n", filename);
    }

    /**
     * 本地目录不存在，远端目录存在的文件
     */
    printf("\nFiles only exist in remote dir:\n");
    for (entry = dict_first(G_remote_file_info_map); entry;
            entry = dict_next(entry)) {
        filename = dict_fetch_key(entry);
        md51 = dict_fetch_val(entry);

        if (dict_find(G_local_file_info_map, filename, 
                    (void**)&md52) >= 0)
            continue;

        printf("  %s\n", filename);
    }

    /**
     * 本地目录和远端目录都存在但是文件 md5 不同的
     */
    printf("\nFiles need to synchronize:\n");
    repeat_print_str("=", 132, "\n");
    printf("  %-48s%-48s%-48s\n", "Filename", "Local-MD5", "Remote-MD5");
    repeat_print_str("=", 132, "\n");
    for (entry = dict_first(G_local_file_info_map); entry;
            entry = dict_next(entry)) {
        filename = dict_fetch_key(entry);
        md51 = dict_fetch_val(entry);

        if (dict_find(G_remote_file_info_map, filename, 
                    (void**)&md52) < 0)
            continue;
        
        if (!memcmp(md51, md52, MD5_SIZE))
            continue;

        printf("  %-48s%-48s", filename, str_md5(md51));
        printf("%-48s\n", str_md5(md52));
    }
    repeat_print_str("=", 132, "\n\n");
}

int parse_sync_info_reply(struct filesync_cmd_header * hdr, 
        void * payload)
{
    struct cmd_payload_req * req; 

    req = (struct cmd_payload_req *)payload;
    dict_add(G_remote_file_info_map, req->filename, req->md5);
    return 0;
}

void do_sync_info(void)
{
    int count;

    /* prepare the request */
    prepare_cmd_request(CMD_SYNCINFO_REQ, G_remote_dir, G_local_dir); 

#ifdef __FSYNC_DEBUG_ON__
    struct socket_buffer * copyed;
    copyed = socket_buffer_copy(G_send_buffer);

    log_info("sync-info:");
    log_full_cmd_hdr(copyed);
    socket_buffer_free(copyed);
#endif

    /* send the request to the peer */
    count = socket_buffer_size(G_send_buffer);
    socket_write_exactly(G_conn_fd, G_send_buffer, count);

    /**
     * 解析应答并把文件信息保存在字典中
     */
    G_remote_file_info_map = dict_create(&file_info_map_type);
    G_local_file_info_map = dict_create(&file_info_map_type);
    recv_cmd_reply(parse_sync_info_reply);

    show_pretty_sync_info();
}

void do_download(void)
{
    int count;

    /**
     * 打开目录，然后遍历目录下的文件，把文件名和其md5值等信息
     * 添加到请求体中，最后把准备好的请求体发送出去
     */
    prepare_cmd_request(CMD_DOWNLOAD_REQ, G_remote_dir, G_local_dir); 

#ifdef __FSYNC_DEBUG_ON__
    struct socket_buffer * copyed;
    copyed = socket_buffer_copy(G_send_buffer);

    log_info("Download REQ info:");
    log_full_cmd_hdr(copyed);
    socket_buffer_free(copyed);
#endif

    /* send the request to the peer */
    count = socket_buffer_size(G_send_buffer);
    socket_write_exactly(G_conn_fd, G_send_buffer, count);

    /**
     * 接收应答，保存文件
     */
    recv_and_save_files(G_local_dir);
}

int fsync_cli(void)
{
    if (!strcmp(G_cmd, "upload")) 
        do_upload();
    else if (!strcmp(G_cmd, "info"))
        do_sync_info();
    else if (!strcmp(G_cmd, "download"))
        do_download();
    else 
        printf("Unknow cmd: %s\n", G_cmd);

    return EXIT_SUCCESS;
}
