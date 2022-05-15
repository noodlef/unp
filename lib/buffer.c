/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：buffer.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月18日
 * 描    述：
 * 
 *===============================================================
 */
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "buffer.h"
#include "log.h"

#define IOVEC_SZ        8
#define BLOCK_SZ        (128 - sizeof(void *))
#define MIN_FREE_SZ     4 * BLOCK_SZ 

#define INSPECT_PEEK    0
#define INSPECT_REPLACE 1

#define BUFFER_TAIL 0
#define BUFFER_HEAD 1

#define CAPACITY(buffer) ((buffer)->block_sz * (buffer)->block_nr)

#define max(a, b) ((a) > (b) ? (a) : (b)) 
#define min(a, b) ((a) < (b) ? (a) : (b)) 

struct _buffer_block {
    struct _buffer_block * next;
    char data[0];
};

struct socket_buffer {
    size_t block_sz;
    size_t block_nr;
    size_t r_pos, w_pos;
    struct _buffer_block * first_block;
    struct _buffer_block * last_block;
};

static void _debug_check_buffer(struct socket_buffer * buffer)
{
    int i;
    struct _buffer_block * t, * n; 

    t = buffer->first_block;
    n = buffer->last_block;

    i = 0;
    while (t) {
        if (t == n)
            assert((i + 1) == buffer->block_nr);
            //log_info("Find last buffer, idx: %d", i);
        t = t->next;
        ++i;
    }

    assert(i == buffer->block_nr);
    //log_info("Buffer: nr: %d, actual nr: %d", buffer->block_nr, i);
}

void socket_buffer_free(struct socket_buffer * socket_buffer)
{
    struct _buffer_block * block, * next_block;

    if (!socket_buffer)
        return;

    block = socket_buffer->first_block;
    for (; block; ) {
        next_block = block->next;
        free(block);
        block = next_block;
    }

    free(socket_buffer);
}

struct socket_buffer * socket_buffer_new(void)
{
    size_t i, block_nr;
    struct socket_buffer * socket_buffer;
    struct _buffer_block * block;

    socket_buffer = (struct socket_buffer *) malloc(sizeof(struct socket_buffer));
    if (!socket_buffer)
        goto failed;

    block_nr = 4; /* 4 blocks */
    socket_buffer->block_sz = BLOCK_SZ;
    socket_buffer->r_pos = 0;
    socket_buffer->w_pos = 0;
    socket_buffer->first_block = NULL;
    socket_buffer->last_block = NULL;
    socket_buffer->block_nr = 0;

    for (i = 0; i < block_nr; i++) {
        block = (struct _buffer_block *) malloc(
                sizeof(struct _buffer_block) + socket_buffer->block_sz);
        if (!block)
            goto failed;
        block->next = NULL;

        if (!socket_buffer->first_block)
            socket_buffer->first_block = block;

        if (socket_buffer->last_block) {
            socket_buffer->last_block->next = block;
        }
        socket_buffer->last_block = block;

        ++socket_buffer->block_nr;
    }

    return socket_buffer;

failed:
    log_warning("New socket_buffer failed!");
    socket_buffer_free(socket_buffer);
    return NULL;
}

static void _copy_range(struct socket_buffer * dst, size_t dst_start, 
        struct socket_buffer * src, size_t src_start, size_t len)
{
    size_t src_offset, dst_offset, i;
    struct _buffer_block * src_block, * dst_block;

    i = src_start / src->block_sz;
    src_block = src->first_block;
    while (i-- > 0) {
        assert(src_block != NULL); 
        src_block = src_block->next;
    }

    i = dst_start / dst->block_sz;
    dst_block = dst->first_block;
    while (i-- > 0) {
        assert(dst_block != NULL); 
        dst_block = dst_block->next;
    }

    while (len > 0) {
        assert(src_block != NULL); 
        assert(dst_block != NULL); 

        src_offset = src_start % src->block_sz;
        dst_offset = dst_start % dst->block_sz;
        i = min(len, min((src->block_sz - src_offset), 
                    (dst->block_sz - dst_offset)));
        memcpy(dst_block->data + dst_offset, src_block->data + src_offset, i);

        if ((i + src_offset) == src->block_sz)
            src_block = src_block->next;

        if ((i + dst_offset) == dst->block_sz)
            dst_block = dst_block->next;

        len -= i;
        src_start += i;
        dst_start += i;
    }
}

size_t socket_buffer_size(struct socket_buffer * socket_buffer)
{
    assert(socket_buffer != NULL);
    assert(socket_buffer->w_pos >= socket_buffer->r_pos);

    return (socket_buffer->w_pos - socket_buffer->r_pos);
}

static size_t _free_size_at_tail(struct socket_buffer * socket_buffer)
{
    assert(socket_buffer != NULL);

    return CAPACITY(socket_buffer) - socket_buffer->w_pos; 
}

static size_t _free_size_at_head(struct socket_buffer * socket_buffer)
{
    assert(socket_buffer != NULL);

    return socket_buffer->r_pos; 
}

static void _enlarge_capacity_at(struct socket_buffer * socket_buffer, 
        size_t sz, int pos)
{
    size_t block_nr;
    struct _buffer_block * block;

    assert(socket_buffer != NULL);
    assert(pos == BUFFER_HEAD || pos == BUFFER_TAIL);

    block_nr = sz / socket_buffer->block_sz + 1;
    while (block_nr-- > 0) {
        block = (struct _buffer_block *) malloc(
                sizeof(struct _buffer_block) + socket_buffer->block_sz);
        if (!block) {
            log_warning("Enlarge capacity failed!");
            return;
        }
        block->next = NULL;

        if (pos == BUFFER_TAIL) {
            /* at tail */
            if (socket_buffer->last_block) {
                socket_buffer->last_block->next = block;
                socket_buffer->last_block = block;
            } else {
                /* empty list */
                socket_buffer->first_block = block;
                socket_buffer->last_block = block;
            }
        } else {
            /* at head */
            block->next = socket_buffer->first_block;
            socket_buffer->first_block = block;

            if (!socket_buffer->last_block)
                socket_buffer->last_block = block;
        }

        ++socket_buffer->block_nr;
    }

    _debug_check_buffer(socket_buffer);
}

struct socket_buffer * socket_buffer_copy(struct socket_buffer * socket_buffer)
{
    size_t len, start_idx;
    struct socket_buffer * copyed_buffer;

    assert(socket_buffer != NULL);

    len = sizeof(struct socket_buffer);
    copyed_buffer = (struct socket_buffer *) malloc(len);
    if (!copyed_buffer)
        goto failed;

    /**
     * only copy used blocks
     */
    start_idx = socket_buffer->r_pos / socket_buffer->block_sz;
    len = start_idx * socket_buffer->block_sz;

    copyed_buffer->block_sz = socket_buffer->block_sz;
    copyed_buffer->r_pos = socket_buffer->r_pos - len;
    copyed_buffer->w_pos = socket_buffer->w_pos - len;
    copyed_buffer->first_block = NULL;
    copyed_buffer->last_block = NULL;
    copyed_buffer->block_nr = 0;

    _enlarge_capacity_at(copyed_buffer, copyed_buffer->w_pos, BUFFER_TAIL);
    if (copyed_buffer->block_nr <= (copyed_buffer->w_pos / copyed_buffer->block_sz))
        goto failed;

    _copy_range(copyed_buffer, copyed_buffer->r_pos, socket_buffer, 
            socket_buffer->r_pos, (socket_buffer->w_pos - socket_buffer->r_pos));
    return copyed_buffer;

failed:
    log_warning("New socket_buffer failed!");
    socket_buffer_free(copyed_buffer);
    return NULL;
}

static void _adjust_buffer(struct socket_buffer * socket_buffer)
{
    int block_idx, i, cnt;
    struct _buffer_block * block, * tmp;

    /**
     * always remains 2 free blocks at buffer-head
     */
    block_idx = socket_buffer->r_pos / socket_buffer->block_sz;
    if (block_idx >= 2) {
        cnt = block_idx - 1;
        i = cnt * socket_buffer->block_sz;
        socket_buffer->r_pos -= i;
        socket_buffer->w_pos -= i;

        block = socket_buffer->first_block;
        while (--cnt > 0)
           block = block->next; 

        socket_buffer->last_block->next = socket_buffer->first_block;
        socket_buffer->last_block = block;
        socket_buffer->first_block = block->next;
        block->next = NULL;
    }

    block_idx = socket_buffer->w_pos / socket_buffer->block_sz;
    if ((socket_buffer->block_nr - block_idx) > IOVEC_SZ) {
        /* find the block where 'w_pos' lies in */
        block = socket_buffer->first_block;
        while (block_idx-- > 0)
            block = block->next;

        cnt = IOVEC_SZ;
        while (--cnt > 0)
           block = block->next; 
        socket_buffer->last_block = block;

        tmp = block->next;
        block->next = NULL;
        while (tmp) {
            block = tmp->next;
            free(tmp);
            --socket_buffer->block_nr;
            tmp = block;
        }
    }

    _debug_check_buffer(socket_buffer);
}

static size_t _get_buffer_vector(struct socket_buffer * socket_buffer, 
        size_t start, size_t size, struct iovec * iov, size_t * len)
{
    int block_idx, left;
    size_t iov_len, offset, i;
    struct _buffer_block * block;

    assert(iov != NULL);
    assert(len != NULL);
    assert(socket_buffer != NULL);

    block_idx = start / socket_buffer->block_sz;
    offset = start % socket_buffer->block_sz;

    block = socket_buffer->first_block;
    while (block_idx-- > 0 && block)
       block = block->next;

    iov_len = *len;
    *len = 0;
    left = size;
    while (left > 0 && block) {
        iov[*len].iov_base = block->data + offset;

        i = min((size_t)left, socket_buffer->block_sz - offset);
        iov[*len].iov_len = i;
        if (offset) 
            offset = 0;

        left -= i;
        block = block->next;

        if (++(*len) >= iov_len)
            break;
    }

    return (size - (size_t)left);
}

static int _socket_buffer_get(struct socket_buffer * socket_buffer, 
        struct iovec ** iov, size_t * len)
{
    size_t size;
    static struct iovec iovec[IOVEC_SZ] = {0};

    assert(iov != NULL);
    assert(socket_buffer != NULL);

    *len = IOVEC_SZ;
    *iov = iovec;
    size = _free_size_at_tail(socket_buffer);

    if (size < MIN_FREE_SZ) {
        _enlarge_capacity_at(socket_buffer, MIN_FREE_SZ - size, BUFFER_TAIL); 
        size = _free_size_at_tail(socket_buffer);
    }
    if (size < MIN_FREE_SZ)
        return 0;

    return _get_buffer_vector(socket_buffer, socket_buffer->w_pos, size, *iov, len);
}

static int _socket_buffer_peek(
        struct socket_buffer * socket_buffer, size_t offset, size_t size, 
        struct iovec ** iov, size_t * len)
{
    size_t buf_size;
    static struct iovec iovec[IOVEC_SZ] = {0};

    assert(iov != NULL);
    assert(len != NULL);
    assert(offset >= 0);
    assert(socket_buffer != NULL);

    buf_size = socket_buffer_size(socket_buffer);
    assert(offset < buf_size);

    *len = IOVEC_SZ;
    *iov = iovec;
    size = min(buf_size - offset, size);

    return _get_buffer_vector(socket_buffer, socket_buffer->r_pos + offset, 
            size, *iov, len);
}

static int _socket_buffer_inspect(struct socket_buffer * socket_buffer, 
        int offset, char * buf, int len, int op)
{
    size_t iovcnt;
    struct iovec * iov;
    int i, left, cnt, check;

    assert(len >= 0);
    assert(offset >= 0);
    assert(socket_buffer != NULL);

    i = socket_buffer_size(socket_buffer); 
    assert(offset < i);
    left = min(i - offset, len);

    cnt = 0;
    while (left > 0) {
        i = _socket_buffer_peek(socket_buffer, offset, left, &iov, &iovcnt);

        check = 0;
        while (iovcnt-- > 0) {
            /**
             * INSPECT_PEEK: just peek 
             * INSPECT_REPLACE: replace with data in buf 
             */
            if (op == INSPECT_PEEK) {
                memcpy(buf, iov->iov_base, iov->iov_len);
                buf += iov->iov_len;
            } else {
                if (buf) {
                    memcpy(iov->iov_base, buf, iov->iov_len);
                    buf += iov->iov_len;
                }
            }
            check += iov->iov_len;
            ++iov;
        }
        assert(i == check);

        cnt += i;
        left -= i;
        offset += i;
    }

    return cnt;
}

static size_t _socket_buffer_seek(struct socket_buffer * socket_buffer, 
        int sz, int seek_type)
{
    int i;

    assert(socket_buffer != NULL);
    assert(seek_type == SEEK_HEAD || seek_type == SEEK_TAIL);
    
    if (seek_type == SEEK_HEAD) {
        if (sz > 0) {
            sz = min(sz, (int)socket_buffer_size(socket_buffer));
        } else {
            i = 0 - (int)_free_size_at_head(socket_buffer);
            if (i > sz) 
                _enlarge_capacity_at(socket_buffer, i - sz, BUFFER_HEAD);
            sz = max(sz, (int)(0 - _free_size_at_head(socket_buffer))); }

        socket_buffer->r_pos += sz;
    } else {
        if (sz > 0) {
            i = _free_size_at_tail(socket_buffer);
            if (i < sz)
                _enlarge_capacity_at(socket_buffer, sz - i, BUFFER_TAIL);
            sz = min(sz, (int)_free_size_at_tail(socket_buffer));
        } else {
            sz = max(sz, (int)(0 - socket_buffer_size(socket_buffer)));
        }
        socket_buffer->w_pos += sz;
    }
        
    _adjust_buffer(socket_buffer);

    return sz;
}

int socket_buffer_seek(struct socket_buffer * socket_buffer, 
        int  sz, int seek_type)
{
    int i;

    i = _socket_buffer_seek(socket_buffer, sz, seek_type);
    return i > 0 ? i: -i;
}

static int _socket_buffer_write(struct socket_buffer * socket_buffer, 
        char * buf, int len)
{
    size_t iovcnt;
    int i, left, cnts;
    struct iovec * iov;

    assert(len >= 0);
    assert(socket_buffer != NULL);

    left = len;
    while (left > 0) {
        i = _socket_buffer_get(socket_buffer, &iov, &iovcnt);
        if (i == 0) {
            log_warning_sys("Socket buffer write: OOM!");
            break;
        }

        i = 0;
        while (iovcnt-- > 0) {
            cnts = min(iov->iov_len, (size_t)left);

            if (buf) {
                memcpy(iov->iov_base, buf, cnts);
                buf += cnts;
            }
            i += cnts;

            if ((left -= cnts) == 0)
                break;
            ++iov;
        }

        _socket_buffer_seek(socket_buffer, i, SEEK_TAIL);
    }

    return (len - left);
}

int socket_buffer_read(struct socket_buffer * socket_buffer, char * buf, int len)
{
    int i;

    i = _socket_buffer_inspect(socket_buffer, 0, buf, len, INSPECT_PEEK);
    _socket_buffer_seek(socket_buffer, i, SEEK_HEAD);

    return i;
}

int socket_buffer_peek(struct socket_buffer * socket_buffer, 
        int offset, char * buf, int len)
{
    return _socket_buffer_inspect(socket_buffer, offset, buf, len, INSPECT_PEEK);
}

int socket_buffer_replace(struct socket_buffer * socket_buffer, 
        int offset, char * buf, int len)
{
    return _socket_buffer_inspect(socket_buffer, offset, buf, len, INSPECT_REPLACE);
}

int socket_buffer_write(struct socket_buffer * socket_buffer, char * buf, int len)
{
    return _socket_buffer_write(socket_buffer, buf, len);
}

int socket_buffer_send(struct socket_buffer * socket_buffer, int fd, int * len)
{
    int i, left;
    size_t iovcnt;
    struct iovec * iov;

    assert(socket_buffer != NULL);
    assert(len != NULL && *len >= 0);

    i = socket_buffer_size(socket_buffer);
    left = min(i, *len);

    *len = 0;
    while (left > 0) {
        i = _socket_buffer_peek(socket_buffer, 0, left, &iov, &iovcnt);

        i = writev(fd, iov, iovcnt);
        if (i < 0) {
            log_warning_sys("Writev fd: %d error!", fd);
            return -1;
        }

        *len += i;
        left -= i;
        _socket_buffer_seek(socket_buffer, i, SEEK_HEAD);
    }

    return 0;
}

int socket_buffer_recv(struct socket_buffer * socket_buffer, int fd, int * len)
{
    int i, left;
    size_t iovcnt;
    struct iovec * iov;

    assert(len != NULL && *len >= 0);
    assert(socket_buffer != NULL);

    left = *len;
    *len = 0;
    while (left > 0) {
        i = _socket_buffer_get(socket_buffer, &iov, &iovcnt);
        if (i == 0) {
            log_warning_sys("Socket buffer write: OOM!");
            return -1;
        }

        i = readv(fd, iov, iovcnt);
        if (i < 0) {
            log_warning_sys("Readv fd: %d error!", fd);
            return -2;
        } else if (i == 0) {
            /* EOF */
            return -3;
        }

        *len += i;
        left -= i;
        _socket_buffer_seek(socket_buffer, i, SEEK_TAIL);
    }

    return 0; 
}


#if 0
#include <stdio.h>
#include <fcntl.h>
int main(int argc, char ** argv)
{
    int i, len, fd;
    char buf[1024], check[1024]; 
    struct socket_buffer * buffer;

    buffer = socket_buffer_new();
    assert(buffer != NULL);
    
    i = socket_buffer_size(buffer);
    assert(i == 0);

    printf("========================================\n");
    printf("Test write\n");
    for (i = 0; i < 512; i++) {
        buf[i] = i;
    }
    i = socket_buffer_write(buffer, buf, 512);
    assert(i == 512);
    printf("pass\n\n");

    printf("========================================\n");
    printf("Test peek\n");
    i = socket_buffer_size(buffer);
    assert(i == 512);

    i = socket_buffer_peek(buffer, 0, check, 512);
    assert(i == 512);
    i = socket_buffer_size(buffer);
    assert(i == 512);

    i = memcmp(buf, check, 512);
    assert(i == 0);
    printf("pass\n\n");

    printf("========================================\n");
    printf("Test replace\n");
    i = socket_buffer_replace(buffer, 256, "noodles", 7);
    assert(i == 7);

    i = socket_buffer_peek(buffer, 256, check, 7);
    i = memcmp("noodles", check, 7);
    assert(i == 0);
    printf("pass\n\n");

    printf("========================================\n");
    printf("Test read\n");
    i = socket_buffer_read(buffer, check, 256);
    assert(i == 256);

    i = socket_buffer_size(buffer);
    assert(i == 256);
    i = memcmp(buf, check, 256);
    assert(i == 0);

    i = socket_buffer_read(buffer, check, 7);
    assert(i == 7);

    i = socket_buffer_size(buffer);
    assert(i == 249);
    i = memcmp("noodles", check, 7);
    printf("pass\n\n");

    printf("========================================\n");
    printf("Test seek\n");
    i = socket_buffer_seek(buffer, -242, SEEK_TAIL);
    assert(i == 242);

    i = socket_buffer_size(buffer);
    assert(i == 7);

    i = socket_buffer_read(buffer, check, 7);
    i = memcmp(buf + 7, check, 7);
    assert(i == 0);
    printf("pass\n\n");

    printf("========================================\n");
    printf("Test recv-send api\n");
    
    len = 1024;
    i = socket_buffer_recv(buffer, 0, &len);
    printf("ret: %d\n", i);
    assert(i == -3);

    char * str = "test for test\nnoodles\n";
    i = socket_buffer_seek(buffer, -strlen(str), SEEK_HEAD);
    i = socket_buffer_replace(buffer, 0, str, strlen(str));

    len += strlen(str);
    i = socket_buffer_send(buffer, 2, &len);
    assert(i == 0);
    printf("pass\n");
}
#endif
