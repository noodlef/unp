/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：file_lock.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年01月05日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __FILE_LOCK_H
#define __FILE_LOCK_H
#include <fcntl.h>

#define __lock_register(fd, cmd, type, offset, whence, len) \
({ \
    int ret;\
    struct flock lock; \
    lock.l_type = type; \
    lock.l_start = offset; \
    lock.l_whence = whence; \
    lock.l_len = len; \
    ret = fcntl(fd, cmd, &lock); \
    ret;\
})

#define file_rlock(fd, offset, whence, len) \
    __lock_register(fd, F_SETLK, F_RDLCK, offset, whence, len)

#define file_rlock_wait(fd, offset, whence, len) \
    __lock_register(fd, F_SETLKW, F_RDLCK, offset, whence, len)

#define file_wlock(fd, offset, whence, len) \
    __lock_register(fd, F_SETLK, F_WRLCK, offset, whence, len)

#define file_wlock_wait(fd, offset, whence, len) \
    __lock_register(fd, F_SETLKW, F_WRLCK, offset, whence, len)

#define file_unlock(fd, offset, whence, len) \
    __lock_register(fd, F_SETLK, F_UNLCK, offset, whence, len)
#endif //FILE_LOCK_H
