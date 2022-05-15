/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：log.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月12日
 * 描    述：
 * 
 *===============================================================
 */
#include <math.h>
#include <time.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <execinfo.h>

#include "log.h"

#define MAX_BUF_SZ  1024 
static char * G_log_level[] = {
    "DEBUG",
    "INFO",
    "WARNING",
    "ERROR",
    "FATAL",
};

static int G_level = LOG_INFO;
static int G_log_fd = -1;
static char * G_log_file = NULL;
static int G_log_with_prefix = 1;

/* eg: 2020:08:02 06:41:59.107 */
static char * get_formatted_time() {
    static char buffer[32];
    int millisec;
    struct timeval tv;
    time_t rawtime;
    struct tm * timeinfo;

    gettimeofday(&tv, NULL);
    millisec = lrint(tv.tv_usec / 1000.0);
    if (millisec >= 1000) {
        millisec -= 1000;
        tv.tv_sec++;
    }

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, sizeof buffer, "%Y:%m:%d %H:%M:%S", timeinfo);
    snprintf(buffer + strlen(buffer), 
            sizeof buffer - strlen(buffer), ".%03d", millisec);

    return buffer;
}

int init_log(char * log_file, int log_with_prefix)
{
    G_log_file = log_file;
    G_log_with_prefix = log_with_prefix;

    if (log_file) {
        G_log_fd = open(log_file, O_WRONLY | O_APPEND | O_CREAT, 
                S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

        if (G_log_fd < 0) {
            printf("Open log file failed, file: %s, errno: %s\n", 
                    log_file, strerror(errno));
            return -1;
        }
    }

    return 0;
}

void set_log_level(int level)
{
    assert(level >= LOG_DEBUG);
    G_level = level;
}

static void _log_to_file(char * log) 
{
    int i, left;

    left = strlen(log);
    while (left > 0) {
        if ((i = write(G_log_fd, log, left)) < 0) {
            if (errno == EINTR)
                continue;
            printf("Write to log file failed, file: %s, errno: %s", 
                    G_log_file, strerror(errno));
            return;
        }

        left -= i;
        log += i;
    }
}

static void _log(
        int level, 
        const char * file_name, 
        int line_no, 
        const char * func_name, 
        int log_with_prefix, 
        int log_with_errno, 
        const char * fmt, 
        va_list ap)
{
    int left, saved_errno, ret;
    char log[MAX_BUF_SZ], * buf, * formatted_time;

    assert(fmt != NULL);
    assert(file_name != NULL);

    buf = log;
    left = sizeof log;
    saved_errno = errno;

    /**
     * log with prefix: 
     *      [TIMESTAMP PID LOG_LEVEL <FILENAME>:LINE_NO:FUNC_NAME()]
     */
    if (log_with_prefix && G_log_with_prefix) {
        formatted_time = get_formatted_time();
        if ((ret = snprintf(buf, left, "[%s %d %s <%s>:%d:%s()] ", 
                        formatted_time, getpid(), G_log_level[level], 
                        file_name, line_no, func_name)) < 0)
            ret = 0; /* error */
        else if (ret >= left)
            ret = left - 1; /* output was truncated */
        buf += ret;
        if ((left -= ret) <= 1)
            goto out_of_buffer;
    }

    if ((ret = vsnprintf(buf, left, fmt, ap)) < 0)
        ret = 0; 
    else if (ret >= left)
        ret = left -1;
    buf += ret;
    if ((left -= ret) <= 1)
        goto out_of_buffer;

    /* append errno info */
    if (log_with_errno) {
        if (*(buf - 1) == '\n') {
            buf--;
            left++;
        }
        if ((ret = snprintf(buf, left, ", errno: %s.\n", strerror(saved_errno))) < 0)
            ret = 0;
        else if (ret >= left)
            ret = left - 1;
        buf += ret;
        left -= ret;
    }
    
out_of_buffer:
    *buf = '\0';
    
    if (G_log_fd < 0) {
        /* in case stdout and stderr are the same */
        fflush(stdout);
        fputs(log, stderr);
        /* flushes all stdio output streams */
        fflush(NULL);
    } else {
        _log_to_file(log);
    }

    return;
}

void log_to(
        int log_level, 
        int log_with_errno, 
        int log_with_prefix, 
        const char * file_name, 
        int file_no, 
        const char * func_name, 
        const char * fmt, 
        ...)
{
    va_list ap;

    if (log_level >= G_level) {
        va_start(ap, fmt);
        _log(log_level, 
             file_name, file_no, func_name, 
             log_with_prefix, log_with_errno, 
             fmt, ap);
        va_end(ap);
    }

    if (G_log_fd > 0)
        return;

    switch(log_level) {
        case LOG_DEBUG:
        case LOG_INFO:
        case LOG_WARN:
            break;
        
        /* terminate when log level is above 'LOG_ERROR' */
        case LOG_ERROR:
            exit(1);
        case LOG_FATAL:
            abort(); /* dump core */
            exit(1);
    }

    return;
}

void dump_trace(unsigned char max_depth)
{
    int i;
    void * traces[64];
    char ** strings;

    max_depth = (max_depth > 64) ? 64 : max_depth;

    max_depth = backtrace(traces, max_depth);
    strings = backtrace_symbols(traces, max_depth);
    if (strings == NULL) {
        log_error("Backtrace symbols error!");
        return;
    }

    log_info("Stack traces:");
    for (i = 0; i < max_depth; i++)
        log_info("    %s", strings[i]); 

    free(strings);
}
