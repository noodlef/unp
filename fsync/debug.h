/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：debug.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年12月26日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __DEBUG_H
#define __DEBUG_H

#include "fsync.h"

char * str_md5(char * md5);

inline void log_cmd_payload_req(struct cmd_payload_req * req);

inline void log_cmd_payload_ack(struct cmd_payload_ack * ack);

inline void log_file_hdr(struct filesync_file_header * hdr);

inline void log_cmd_hdr(struct filesync_cmd_header * hdr);

void log_full_cmd_hdr(struct socket_buffer * buffer);

#endif //DEBUG_H
