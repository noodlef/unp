/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：buffer.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月27日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __FSYNC_BUFFER_H
#define __FSYNC_BUFFER_H
#include "fsync.h"

extern struct dict_type file_info_map_type;

struct socket_buffer;
typedef int (*parse_cmd_req)(struct filesync_cmd_header * req_hdr, 
        struct cmd_payload_req * req);
typedef int (*parse_cmd_reply)(struct filesync_cmd_header * reply_hdr, 
        void * ack);

void read_from_buffer_exactly(
        struct socket_buffer * socket_buffer, char * buf, int len);

int write_cmd_hdr_to_buffer(struct socket_buffer * buffer, 
        struct filesync_cmd_header * hdr);

void write_to_buffer_exactly(
        struct socket_buffer * socket_buffer, char * buf, int len);

int read_cmd_hdr_from_buffer(struct socket_buffer * buffer, 
        struct filesync_cmd_header * hdr);

int read_cmd_payload_ack_from_buffer(struct socket_buffer * buffer, 
        struct cmd_payload_ack * payload);

int write_file_hdr_to_buffer(struct socket_buffer * buffer, 
        struct filesync_file_header * hdr);

int read_cmd_payload_req_from_buffer(
        struct socket_buffer * buffer, struct cmd_payload_req * payload);

int write_cmd_payload_ack_to_buffer(
        struct socket_buffer * buffer, struct cmd_payload_ack * ack);

int read_file_hdr_from_buffer(
        struct socket_buffer * buffer, struct filesync_file_header * hdr);

void recv_and_save_files(char * dir_path);

void prepare_cmd_request(int cmd, char * remote_dir, char * local_dir);

#endif //FSYNC_BUFFER_H
