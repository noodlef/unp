/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：test_mkdir.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月21日
 * 描    述：
 * 
 *===============================================================
 */
#include <errno.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdio.h>

#include "../../lib/log.h"
#include "../../lib/utils.h"

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
    char * abspath, * strs[4];

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
    if ((fd = open(abspath, O_WRONLY | O_APPEND | O_CREAT | O_TRUNC, 
                    mode)) < 0) {
        /* Directory does not exist */
        if (errno == ENOENT) {
            len = strlen(filename);
            while (len > 0 && filename[len - 1] != '/')
                --len;
            if (!len) {
                log_error("Create file(%s) failed, directory(%s) \
                        does not exist!", abspath, dirpath);
                goto failed;
            }
             
            filename[len - 1] = '\0';
            if (mkdir_recursively(dirpath, filename, DIR_MODE) < 0)
                goto failed;

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

int main(int argc, char ** argv)
{
    if (argc != 3) {
        printf("Usage: mkdir dirpath relative_path\n");
        return -1;
    }

    if (create_file(argv[1], argv[2], FILE_MODE) < 0) {
        printf("mkdir failed!!!\n");
        return -1;
    }

    printf("mkdir success\n");
    return 0;
}
