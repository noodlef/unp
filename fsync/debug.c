/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：debug.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年12月18日
 * 描    述：
 * 
 *===============================================================
 */
#include <string.h>
#include <assert.h>

#include "fsync.h"
#include "packet.h"

#include "../lib/log.h"
#include "../lib/buffer.h"

char * str_cmd(uint8_t cmd)
{
    char * cmd_str;

    switch (cmd) {
    case CMD_UPLOAD_REQ:
        cmd_str = "UPLOAD-REQ";
        break;
    case CMD_UPLOAD_ACK:
        cmd_str = "UPLOAD-ACK";
        break;
    case CMD_DOWNLOAD_REQ:
        cmd_str = "DOWNLOAD-REQ";
        break;
    case CMD_DOWNLOAD_ACK:
        cmd_str = "DOWNLOAD-ACK";
        break;
    case CMD_SYNCINFO_REQ:
        cmd_str = "SYNCINFO-REQ";
        break;
    case CMD_SYNCINFO_ACK:
        cmd_str = "SYNCINFO-ACK";
        break;
    default:
        cmd_str = "UNKOWN-CMD";
    }

    return cmd_str;
}

char * str_md5(char * md5)
{
    int i, idx, n;
    static char md5_str[32];
    char hex[16] = {'0', '1', '2', '3', '4', '5', '6', 
                    '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

    i = MD5_SIZE;
    idx = 0;
    while (i-- > 0) {
        n = md5[i] & 0x0f;
        md5_str[idx++] = hex[n];
        n = (md5[i] & 0xf0) >> 4;
        md5_str[idx++] = hex[n];
    }
    md5_str[idx] = '\0';

    return md5_str;
}

inline void log_cmd_payload_req(struct cmd_payload_req * req)
{
    log_info("============================================");
    log_info("len:      %d", req->len);
    log_info("filename: %s", req->filename); 
    log_info("md5:      %s", str_md5(req->md5));
    log_info("============================================");
}

inline void log_cmd_payload_ack(struct cmd_payload_ack * ack)
{
    log_info("============================================");
    log_info("len:      %d", ack->len);
    log_info("filename: %s", ack->filename); 
    log_info("============================================");
}

inline void log_file_hdr(struct filesync_file_header * hdr)
{
    log_info("============================================");
    log_info("filesync file header:");
    log_info("file_type:        %d", hdr->file_type);
    log_info("file_mode:        %d", hdr->file_mode);
    log_info("filename_len:     %d", hdr->filename_len);
    log_info("file-content-len: %d", hdr->length);
    log_info("============================================");
}

inline void log_cmd_hdr(struct filesync_cmd_header * hdr)
{
    log_info("============================================");
    log_info("filesync cmd header:");
    log_info("version: %d", hdr->version);
    log_info("cmd:     %s", str_cmd(hdr->cmd));
    log_info("length:  %d", hdr->length);
    log_info("============================================");
}

void log_full_cmd_hdr(struct socket_buffer * buffer)
{
    int idx, len, i;
    struct filesync_cmd_header hdr;
    struct cmd_payload_req entry_req;
    struct cmd_payload_ack entry_ack;

    read_cmd_hdr_from_buffer(buffer, &hdr);
    log_cmd_hdr(&hdr);

    idx = 0;
    len = hdr.length; 
    while (len > 0) {
        log_info("\n=========== IDX<%d>", idx++);
        if (hdr.cmd == CMD_UPLOAD_REQ || hdr.cmd == CMD_DOWNLOAD_REQ 
                || hdr.cmd == CMD_SYNCINFO_REQ) {
            i = read_cmd_payload_req_from_buffer(buffer, &entry_req);
            log_cmd_payload_req(&entry_req);
        } else if (hdr.cmd == CMD_UPLOAD_ACK || hdr.cmd == CMD_DOWNLOAD_ACK 
                || hdr.cmd == CMD_SYNCINFO_ACK) {
            i = read_cmd_payload_ack_from_buffer(buffer, &entry_ack);
            log_cmd_payload_ack(&entry_ack);
        } else {
            log_info("Unkown CMD type!!!", entry_ack.filename); 
            break;
        }

        len -= i;
    }

    log_info("\n");
    assert(len == 0);
}

