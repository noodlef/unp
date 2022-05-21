/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：utils.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月27日
 * 描    述：
 * 
 *===============================================================
 */
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "../3rd/md5.h"
#include "../lib/log.h"
#include "../lib/str_utils.h"

#include "utils.h"

void die_unexpectedly(const char * error)
{
    if (!error)
        log_error("Filesync quit unexpected!!!");
    else 
        log_error("Filesync quit unexpected, error_msg: %s!!!", error);
    exit(EXIT_FAILURE);
}

static int _is_dir_filtered(char * dirname)
{
    if (*dirname == '.')
        return 1;

    if (streq(dirname, "build"))
        return 1;

    if (streq(dirname, "bins"))
        return 1;

    if (streq(dirname, "deps"))
        return 1;

    return 0;
}

static int _search_dir_recursively(char * dirpath, int len, int size, 
        search_dir_callback callback, void * callback_args)
{
    int i;
    DIR * dir;
    struct stat statbuf;
    struct dirent * dir_entry;

    if (!(dir = opendir(dirpath))) {
        log_warning_sys("Open dir(%s) errno!", dirpath);
        return -1;
    }

    while ((dir_entry = readdir(dir))) {
        /* skip '.' and '..' */
        if (!strcmp(dir_entry->d_name, "."))
            continue;
        if (!strcmp(dir_entry->d_name, ".."))
            continue;
        
        i = strlen(dir_entry->d_name) + 1;
        if ((size  - len) <= i)
            goto out_of_buffer;

        dirpath[len] = '/';
        memcpy(dirpath + len + 1, dir_entry->d_name, i);
        len += i;

        if (stat(dirpath, &statbuf) < 0) {
            log_warning_sys("Stat file(%s) errno!", dirpath);
            goto failed;
        }

        if (S_ISDIR(statbuf.st_mode)) {
            if (!_is_dir_filtered(dir_entry->d_name) 
                    && _search_dir_recursively(dirpath, len, size, 
                        callback, callback_args) < 0)
                goto failed;
            len -= i;
            continue;
        }

        if (!S_ISREG(statbuf.st_mode)) {
            len -= i;
            continue;
        }

        if (callback && callback(dirpath, callback_args) < 0)
            goto failed;

        len -= i;
    }

    closedir(dir);
    return 0;

out_of_buffer:
    log_warning_sys("Too long filepath for buffer(size: %d bytes)", size);
    return -1;

failed:
    closedir(dir);
    return -1;
}

int search_dir_recursively(char * dirpath, search_dir_callback callback, void * callback_args)
{
    size_t len;
    char fullpath[1024];

    len = strlen(dirpath);
    if (sizeof(fullpath) <= strlen(dirpath)) {
        log_warning_sys("Too long filepath for buffer(size: %d bytes)", 
                sizeof(fullpath));
        return -1;
    }
    strcpy(fullpath, dirpath);

    return _search_dir_recursively(fullpath, len, 
            sizeof(fullpath), callback, callback_args);
}

int compute_file_md5(int fd, unsigned char * md5_buf) 
{
    int i;
    MD5_CTX md5_ctx;
    unsigned char data[1024];

    MD5Init(&md5_ctx);
    while (1) {
        i = read(fd, data, sizeof data);
        if (i < 0)
            return -1;

        MD5Update(&md5_ctx, data, i);
        if (i == 0 || (size_t)i < sizeof data)
            break;
    }
    MD5Final(&md5_ctx, md5_buf);

    return 0;
}

char * join_n_str(char * strs[])
{
    size_t len;
    char * new_str, * pos, ** str;

    len = 0;
    for (str = strs; *str; ++str) {
        len += strlen(*str);
    }
    ++len;

    if (!(new_str = malloc(len))) {
        log_warning_sys("Malloc error!");
        return NULL;
    }

    pos = new_str;
    for (str = strs; *str; ++str) {
        memcpy(pos, *str, strlen(*str));
        pos += strlen(*str);
    }
    *pos = '\0';

    return new_str;
}

int mkdir_recursively(char * dirpath, char * path, mode_t mode)
{
    int i;
    DIR * dirp;
    char * strs[4];
    char * abspath, * p, * t, s;

    assert(dirpath != NULL);
    assert(path != NULL);

    dirp = NULL;
    abspath = NULL;
    /* TODO: do more path checks */
    if (path[0] == '/') {
        log_error("Path(%s) must refer to relative directory path!", path);
        goto failed;
    }

    /* dir referred by 'dirpath' must exsit */
    if (!(dirp = opendir(dirpath))) {
        log_warning_sys("Mkdir failed, open directory(%s) failed!", dirpath);
        goto failed;
    }

    strs[0] = dirpath;
    strs[1] = "/";
    strs[2] = path;
    strs[3] = NULL;
    if (!(abspath = join_n_str(strs))) {
        log_error("Join_n_str error!");
        goto failed;
    }

    /* nothing to do if the dir already exsits */
    if ((dirp = opendir(abspath))) {
        free(abspath);
        return 0;
    }

    /* a directory component in 'path' does not exist */
    if (errno != ENOENT) {
        log_warning_sys("Opendir %s failed!", abspath);
        goto failed;
    }

    t = path;
    p = abspath + strlen(dirpath) + 1;
    while (*t != '\0') {
        while (*t != '/' && *t != '\0') {
            ++t;
            ++p;
        }

        s = *p;
        *p = '\0';
        
        /* failed except that the dir already exsits */
        if ((i = mkdir(abspath, mode)) < 0 && errno != EEXIST) {
            log_warning_sys("Mkdir %s error!", abspath);
            goto failed;
        }

        *p = s;
        if (*p == '/') {
            ++t;
            ++p;
        }
    }

    free(abspath);
    return 0;

failed:
    if (abspath)
        free(abspath);
    if (dirp)
        closedir(dirp);

    return -1;
}

int create_file(char * dirpath, char * filename, mode_t mode)
{
    int fd, len;
    char * abspath, * strs[4], c;

    abspath = NULL;

    strs[0] = dirpath;
    strs[1] = "/";
    strs[2] = filename;
    strs[3] = NULL;
    if (!(abspath = join_n_str(strs))) {
        log_error("Join_n_str error!");
        goto failed;
    }

AGAIN:
    if ((fd = open(abspath, O_WRONLY | O_CREAT | O_TRUNC, mode)) < 0) {
        /* Directory does not exist */
        if (errno == ENOENT) {
            //log_info("File(%s) does not exist!", filename);
            len = strlen(filename);
            while (len > 0 && filename[len - 1] != '/')
                --len;
            if (!len) {
                log_error("Create file(%s) failed, directory(%s) does not exist!", 
                        abspath, dirpath);
                goto failed;
            }
             
            c = filename[len -1];
            filename[len - 1] = '\0';
            if (mkdir_recursively(dirpath, filename, DIR_MODE) < 0) {
                filename[len -1] = c;
                goto failed;
            }

            filename[len -1] = c;
            goto AGAIN;
        }

        log_warning_sys("Create file(%s) failed!", abspath);
        goto failed;
    }

    free(abspath);
    return fd;

failed:
    if (abspath)
        free(abspath);

    return -1;
}

int is_file_filtered(char * filepath)
{
    char * file, * suffix;

    basename_path(filepath, &file, &suffix);
    if (!file)
        return 1;

    /* hidden files, eg: .example */
    if (*file == '.')
        return 1;

    if (!suffix)
        return 0;

    /* files with suffix as follow: .o, .d, .S, .i */
    if (!strcmp(suffix, "o") || !strcmp(suffix, "d") 
            || !strcmp(suffix, "S") || !strcmp(suffix, "i"))
        return 1;

    return 0;
}

