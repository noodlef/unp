/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：str_utils.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年04月23日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "str_utils.h"

static char _is_delim(const char * delim, char c)
{
    const char * p;

    if (!delim)
        return 0;
    for (p = delim; p; p++)
        if (c == *p)
            return 1;

    return 0;
}

char ** split_str(const char * str, const char * delim)
{
    int len, i, k;
    const char * p, * start;
    char ** vec;

    if (!str) 
        return NULL;
    p = str;
    vec = NULL;
    i = len = 0;
    while (*p) {
        if (i >= len) {
            len = len ? 2 * len : 8;
            if (!(vec = realloc(vec, len)))
                goto err;
        }

        while (*p && _is_delim(delim, *p))
            ++p;
        if (!*p) break;
        start = p++;

        while (*p && !_is_delim(delim, *p))
            ++p;

        k = p - start;
        if (!(vec[i] = malloc(k + 1)))  
            goto err;
        memcpy(vec[i], start, k);
        vec[i++][k] = '\0';
    }
    vec[i] = NULL;

    return vec;

err:
    while (i-- > 0) 
        free(vec[i]);
    free(vec);
    return NULL;
}

char * format_str(char * fmt, ...)
{
    va_list ap;
    static char buf[512];

    va_start(ap, fmt);
    (void) vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);

    return buf;
}

void basename_path(char * path, char ** file, char ** suffix)
{
    int len;
    char * pos;

    *file = NULL;
    if (suffix)
        *suffix = NULL;
    if (!path)
        return;

    /* find suffix and filename */
    len = strlen(path);
    pos = path + len;
    while (len-- >= 0) {
        if (suffix && !*suffix && *pos == '.') {
            *suffix = pos + 1;
        } else if (!*file && *pos == '/') {
            *file = pos + 1;
            break;
        }
        --pos;
    }

    if (!*file)
        *file = path;

    if (!strcmp(*file, ""))
        *file = NULL;

    if (suffix && *suffix && streq(*suffix, ""))
        *suffix = NULL;
}

