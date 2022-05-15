/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：test_search_dir.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年11月24日
 * 描    述：
 * 
 *===============================================================
 */

#include <dirent.h>
#include <string.h>
#include <sys/stat.h>

#include "../../lib/log.h"

typedef int (*search_dir_callback) (char * fullpath);

static int _search_dir_recursively(char * dirpath, int len, int size, 
        search_dir_callback callback)
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
            if (_search_dir_recursively(dirpath, len, size, callback) < 0)
                goto failed;
            len -= i;
            continue;
        }

        if (!S_ISREG(statbuf.st_mode)) {
            len -= i;
            continue;
        }

        if (callback && callback(dirpath) < 0)
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

int search_dir_recursively(char * dirpath, search_dir_callback callback)
{
    char fullpath[1024];

    if (sizeof(fullpath) <= strlen(dirpath)) {
        log_warning_sys("Too long filepath for buffer(size: %d bytes)", 
                sizeof(fullpath));
        return -1;
    }
    strcpy(fullpath, dirpath);

    return _search_dir_recursively(fullpath, strlen(dirpath), 
            sizeof(fullpath), callback);
}

int func(char * filepath)
{
    log_info("File: %s", filepath);
    return 0;
}

int main(int argc, char ** argv)
{
    char * dirpath;

    dirpath = argv[1];
    search_dir_recursively(dirpath, func);
    return 0;
}
