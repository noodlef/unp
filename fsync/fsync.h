/*
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：filesync.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月17日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __FILESYNC_H
#define __FILESYNC_H

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "../lib/log.h" 
#include "../lib/dict.h"

//#define __FSYNC_DEBUG_ON__

#define CMD_UPLOAD_REQ      0
#define CMD_UPLOAD_ACK      1
#define CMD_DOWNLOAD_REQ    2
#define CMD_DOWNLOAD_ACK    3
#define CMD_SYNCINFO_REQ    4
#define CMD_SYNCINFO_ACK    5

#define FILETYPE_REGULAR    0

#define MD5_SIZE        16 
#define FILENAME_SIZE   512 

/**
 * payload format of REQ is as follow:
 *  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  +       +             +   ***   +       +             +         +                
 *  +  len  +     dir     +  no use +  len  +  filename1  +   md5   +
 *  +       +             +         +       +             +         +               
 *  ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *   2 byte    len bytes    16 bytes ...
 *
 * payload format of ACK is as follow:
 *  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  +       +             +       +             +                
 *  +  len  +  filename1  +  len  +  filename2  + len   filename3...   
 *  +       +             +       +             +               
 *  +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *   2 byte    len bytes   2 bytes ...
 */
#define FILESYNC_CMD_HEADER_LEN 6
struct filesync_cmd_header {
    uint8_t version; /* not used, v1 currently */
    uint8_t cmd;     /* command */ 
    uint32_t length; /* length of payload */
    char payload[];  
};

struct cmd_payload_req {
    uint16_t len;
    char filename[FILENAME_SIZE];
    char md5[MD5_SIZE];
};

struct cmd_payload_ack {
    uint16_t len;
    char filename[FILENAME_SIZE];
};

/**
 * payload format is as follow:
 *  +++++++++++++++++++++++++++++++++++++++++++++++++++++
 *  +             +                                     +
 *  +  filename   +              content                +
 *  +             +                                     +
 *  +++++++++++++++++++++++++++++++++++++++++++++++++++++
 *   filname_len bytes    length - filename_len bytes                              
 */
#define FILESYNC_FILE_HEADER_LEN 8
struct filesync_file_header {
    uint8_t file_type;      /* not used */
    uint8_t file_mode;      /* not used */
    uint32_t length;        /* length of payload */
    uint16_t filename_len;  /* length of filename */
    char filename[FILENAME_SIZE];
    char payload[];
};

/**
 * 连接描述符，socket 发送和接收缓冲区
 */
extern int G_conn_fd;
extern struct socket_buffer * G_recv_buffer;
extern struct socket_buffer * G_send_buffer;

extern struct dict_type file_info_map;

void die_unexpectedly(const char * error);
#endif //FILESYNC_H
