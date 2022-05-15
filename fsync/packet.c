/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：buffer.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月20日
 * 描    述：
 * 
 *===============================================================
 */
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../lib/log.h"
#include "../lib/buffer.h"
#include "../lib/utils.h"

#include "fsync.h"
#include "packet.h"
#include "debug.h"
#include "network.h"
#include "utils.h"


struct socket_buffer * G_recv_buffer = NULL; 
struct socket_buffer * G_send_buffer = NULL; 

/* read exactly len bytes from socket buffer */
void read_from_buffer_exactly(
        struct socket_buffer * socket_buffer, char * buf, int len)
{
    int i;

    i = socket_buffer_read(socket_buffer, buf, len);
    if (i != len)
        die_unexpectedly(NULL);
}

/* write exactly len bytes to socket buffer */
void write_to_buffer_exactly(
        struct socket_buffer * socket_buffer, char * buf, int len)
{
    int i;

    i = socket_buffer_write(socket_buffer, buf, len);
    if (i != len)
        die_unexpectedly("OOM when write to buffer");
}

/**
 * ==============================================================================
 */

int read_cmd_hdr_from_buffer(struct socket_buffer * buffer, 
        struct filesync_cmd_header * hdr)
{
    read_from_buffer_exactly(buffer, (char *)&(hdr->version), 1);
    read_from_buffer_exactly(buffer, (char *)&(hdr->cmd), 1);
    read_from_buffer_exactly(buffer, (char *)&(hdr->length), 4);

    hdr->length = ntohl(hdr->length); /* network byte order */

    return (1 + 1 + 4);
}

int write_cmd_hdr_to_buffer(struct socket_buffer * buffer, 
        struct filesync_cmd_header * hdr)
{
    uint32_t n;

    write_to_buffer_exactly(buffer, (char *)&(hdr->version), 1);
    write_to_buffer_exactly(buffer, (char *)&(hdr->cmd), 1);

    n = htonl(hdr->length);
    write_to_buffer_exactly(buffer, (char *)&n, 4);

    return (1 + 1 + 4);
}

int read_cmd_payload_req_from_buffer(
        struct socket_buffer * buffer, struct cmd_payload_req * payload)
{
    /* read out filename len(2 bytes) */
    read_from_buffer_exactly(buffer, (char *)&(payload->len), 2);
    payload->len = ntohs(payload->len); /* network byte order */

    /* read out filename (len bytes) */
    assert(payload->len < sizeof(payload->filename));
    read_from_buffer_exactly(buffer, payload->filename, payload->len);
    payload->filename[payload->len] = '\0';

    /* read out md5(16 bytes) */
    read_from_buffer_exactly(buffer, payload->md5, MD5_SIZE);

    return (2 + payload->len + MD5_SIZE);
}

int write_cmd_payload_ack_to_buffer(
        struct socket_buffer * buffer, struct cmd_payload_ack * ack)
{
    uint16_t len;

    len = htons(ack->len); /* network byte order */
    write_to_buffer_exactly(buffer, (char *)&len, 2);

    write_to_buffer_exactly(buffer, ack->filename, ack->len);

    return (2 + ack->len);
}

int read_cmd_payload_ack_from_buffer(struct socket_buffer * buffer, 
        struct cmd_payload_ack * payload)
{
    /* read out filename len(2 bytes) */
    read_from_buffer_exactly(buffer, (char *)&(payload->len), 2);
    payload->len = ntohs(payload->len); /* network byte order */

    /* read out filename (len bytes) */
    assert(payload->len < sizeof(payload->filename));
    read_from_buffer_exactly(buffer, payload->filename, payload->len);
    payload->filename[payload->len] = '\0';

    return (2 + payload->len);
}

int write_file_hdr_to_buffer(struct socket_buffer * buffer, 
        struct filesync_file_header * hdr)
{
    uint32_t length;
    uint16_t filename_len;

    assert(hdr->filename_len < sizeof(hdr->filename));

    write_to_buffer_exactly(buffer, (char *)&(hdr->file_type), 1);
    write_to_buffer_exactly(buffer, (char *)&(hdr->file_mode), 1);

    length = htonl(hdr->length);
    write_to_buffer_exactly(buffer, (char *)&length, 4);

    filename_len = htons(hdr->filename_len);
    write_to_buffer_exactly(buffer, (char *)&filename_len, 2);

    write_to_buffer_exactly(buffer, hdr->filename, hdr->filename_len);

    return length = (1 + 1 + 2 + 4 + hdr->filename_len);
}

int read_file_hdr_from_buffer(
        struct socket_buffer * buffer, struct filesync_file_header * hdr)
{
    read_from_buffer_exactly(buffer, (char *)&(hdr->file_type), 1);
    read_from_buffer_exactly(buffer, (char *)&(hdr->file_mode), 1);

    read_from_buffer_exactly(buffer, (char *)&(hdr->length), 4);
    hdr->length = ntohl(hdr->length);

    read_from_buffer_exactly(buffer, (char *)&(hdr->filename_len), 2);
    hdr->filename_len = ntohs(hdr->filename_len);

    return (1 + 1 + 2 + 4);
}
/**
 * =============================================================================
 */
#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

void recv_and_save_files(char * dir_path)
{
    int i, file_fd, counts;
    struct filesync_file_header file_hdr;
    
    i = socket_buffer_size(G_recv_buffer);    
    assert(i == 0);

    while (1) {
        /**
         * 先读出头部的8字节，然后根据头部的文件名长度字段读出
         * 文件名，之后就是实际的文件数据
         */
        i = socket_buffer_size(G_recv_buffer);
        if (i < FILESYNC_FILE_HEADER_LEN)
            socket_read_at_least(G_conn_fd, G_recv_buffer, FILESYNC_FILE_HEADER_LEN - i);
        read_file_hdr_from_buffer(G_recv_buffer, &file_hdr);
        counts = file_hdr.length;

#ifdef __FSYNC_DEBUG_ON__
        log_file_hdr(&file_hdr);
#endif

        /* 文件名长度为0是结束标记 */
        if (file_hdr.filename_len == 0) {
            log_info("Fsync done!!!");
            exit(EXIT_SUCCESS);
        }

        /**
         * 把文件名读到缓冲区，然后打开文件，将接收到数据写入到文件中，如果
         * 文件不存在，则需要先创建出文件
         */
        i = socket_buffer_size(G_recv_buffer);
        if (i < file_hdr.filename_len)
            socket_read_at_least(G_conn_fd, G_recv_buffer, file_hdr.filename_len - i);
        read_from_buffer_exactly(G_recv_buffer, file_hdr.filename, file_hdr.filename_len);
        file_hdr.filename[file_hdr.filename_len] = '\0';

        /* 递归创建文件路径中不存在的目录 */
        if ((file_fd = create_file(dir_path, file_hdr.filename, FILE_MODE)) < 0) {
            log_error("Create file(%s) error!", file_hdr.filename);
            die_unexpectedly(NULL);
        }

        /* in case of empty files */
        assert(counts >= 0);
        if (counts == 0)
            continue;

        /**
         * 好了现在可以接收文件数据，并将其直接写入文件中 
         */
        socket_transfer_to(G_recv_buffer, file_fd, G_conn_fd, counts);
        log_info("%-64s>>>>>>         %-12d bytes transfered", 
                file_hdr.filename, file_hdr.length);
    }
}

/**
 * eg: fullpath: /home/mydir/test.c, len: 11(/home/mydir)
 */
static int write_cmd_payload_req_to_buffer(char * fullpath, void * args)
{
    int fd, len;
    uint16_t nlen, i;
    char md5[MD5_SIZE], * filename;

    if (is_file_filtered(fullpath))
        return 0;

    if ((fd = open(fullpath, O_RDONLY)) < 0) {
        log_warning_sys("Open file(%s) error!", fullpath);
        return -1;
    }

    /**
     * 按照以下格式将文件名和其md5值信息写入缓冲区
     *     [len][filename][md5]
     * 文件名为相对路径，相对于远端目录
     */
    len = *((int *)args);
    i = (strlen(fullpath) - len - 1);
    nlen = htons(i);
    write_to_buffer_exactly(G_send_buffer, (char *)&nlen, 2);

    filename = fullpath + len + 1;
    write_to_buffer_exactly(G_send_buffer, filename, i);

    compute_file_md5(fd, (unsigned char *)md5);
    write_to_buffer_exactly(G_send_buffer, md5, MD5_SIZE);

    return 0;
}

void prepare_cmd_request(int cmd, char * remote_dir, char * local_dir)
{
    uint32_t length;
    uint16_t nlen, i;
    struct filesync_cmd_header hdr;

    hdr.version = 1;
    hdr.cmd = cmd;
    hdr.length = 0; 
    write_cmd_hdr_to_buffer(G_send_buffer, &hdr);

    /**
     * 写入第一项, 同步的远端目录名:  [len][filename][MD5]
     * md5 字段未使用
     */
    if (cmd == CMD_UPLOAD_REQ || cmd == CMD_DOWNLOAD_REQ 
            || cmd == CMD_SYNCINFO_REQ) {
        i = strlen(remote_dir);
        nlen = htons(i);
        write_to_buffer_exactly(G_send_buffer, (char *)&nlen, 2);

        write_to_buffer_exactly(G_send_buffer, remote_dir, i);
        write_to_buffer_exactly(G_send_buffer, NULL, MD5_SIZE);
    }
    //i = socket_buffer_size(G_send_buffer) - FILESYNC_CMD_HEADER_LEN;

    /**
     * 上传或下载请求时需要递归遍历本地目录，把每一个文件
     * 名 + md5 写入缓冲区
     */
    if (cmd == CMD_UPLOAD_REQ || cmd == CMD_DOWNLOAD_REQ 
            || cmd == CMD_SYNCINFO_ACK) {
        length = strlen(local_dir);
        if (search_dir_recursively(local_dir, write_cmd_payload_req_to_buffer, 
                    &length) < 0)
            die_unexpectedly("preapre request error!!!");
    }

    /**
     * 如果在上传时当前目录内没有需要同步的文件则退出，否则就
     * 计算出请求体的长度，然后更新请求头中该字段的值
     */
    length = socket_buffer_size(G_send_buffer) - FILESYNC_CMD_HEADER_LEN;
    /*
    if (length == i && cmd == CMD_UPLOAD_REQ) {
        log_info("No files need to synchronize, Fsync done...");
        exit(EXIT_SUCCESS);
    }
    */
    length = htonl(length);
    socket_buffer_replace(G_send_buffer, 2, (char *)&length, 4);
}

inline void * val_dup(void * val)
{
    void * new_val;

    if (!(new_val = malloc(MD5_SIZE)))
        return NULL;
    memcpy(new_val, val, MD5_SIZE);

    return new_val;
}

struct dict_type file_info_map_type = {
    .key_destroy = free, 
    .val_destroy = free,
    .key_compare = (key_compare_t)strcmp,
    .key_dup = (key_dup_t)strdup,
    .val_dup = val_dup,
};

