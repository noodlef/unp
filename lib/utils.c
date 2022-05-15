/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：utils.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月11日
 * 描    述：
 * 
 *===============================================================
 */
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <netinet/in.h>

#include "log.h"
#include "utils.h"
#include "socket.h"
#include "file_lock.h"

size_t readn(int fd, void * buf, size_t n)
{
    int i, left;

    assert(buf != NULL);
    
    left = n;
    while (left > 0) {
       if ((i = read(fd, buf, left)) < 0) {
           if (errno == EINTR)
               continue;
           log_warning_sys("Readn failed, fd: %d", fd);
           return (n - left);
       } else if (i == 0) {
           log_warning("Read EOF, fd: %s", fd);
           break; /* EOF */
       }

       left -= i;
       buf = (char *)buf + i;
    }

    return (left == 0) ? n : 0;
}

size_t writen(int fd, void * buf, size_t n)
{
    int i, left;

    assert(buf != NULL);

    left = n;
    while (left > 0) {
        if ((i = write(fd, buf, left)) < 0) {
            if (errno == EINTR)
                continue;
            log_warning_sys("Writen failed, fd: %d", fd);
            return (n - left);
        }

        left -= i;
        buf = (char *)buf + i;
    }

    return n;
}

sig_func_t signal_act(int signo, sig_func_t func) 
{
    struct sigaction act, oldact;
    assert(func != NULL);

    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(signo, &act, &oldact) < 0) {
        log_warning_sys("Sigaction error!, signo: %d", signo);
        return SIG_ERROR;
    }

    return oldact.sa_handler;
}

int daemon_init(void)
{
    int i;
    pid_t pid;

    if ((pid = fork()) < 0)
        return -1;
    else if (pid > 0)
        _exit(0);

    if (setsid() < 0)
        return -1;

    signal(SIGHUP, SIG_IGN);

    if ((pid = fork()) < 0)
        return -1;
    else if (pid > 0)
        _exit(0);

    chdir("/");

    for (i = 0; i < 128; i++)
        close(i);

    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);
    open("/dev/null", O_RDWR);

    return 0;
}

extern char ** environ;
static int _init_set_proctitle(char ** argv, char ** saved_argv)
{
    int i; 
    size_t size;
    char * p, * argv_last, * environ_last;

    /* check if argv[] and environ[] are stored continuously */
    argv_last = argv[0];
    for (i = 0, size = 0; argv[i]; i++) {
        if (argv_last == argv[i]) {
            argv_last  = argv[i] + strlen(argv[i]) + 1;
        }
        size += strlen(argv[i]) + 1;
    }

    environ_last = argv_last;
    for (i = 0; environ[i]; i++) {
        if (environ_last == environ[i])
            environ_last = environ[i] + strlen(environ[i]) + 1;
        size += strlen(environ[i]) + 1;
    }

    p = environ[i - 1] + strlen(environ[i - 1]) + 1;
    if (environ_last != p)
        return -1;

    /* allocating space and copy environmental variables */
    p = malloc(size);
    if (!(p = malloc(size)))
        return -1;

    *saved_argv = p;
    for (i = 0; argv[i]; i++) {
        size = strlen(argv[i]) + 1;
        strcpy(p, argv[i]);
        p += size;
        *(p - 1) = ' '; /* repalce '\0' with ' ' */
    }
    *(p - 1) = '\0';

    size = environ_last - environ[0];
    memcpy(p, environ[0], size);
    for (i = 0; environ[i]; i++) {
        environ[i] = p;
        p += strlen(environ[i]) + 1;
    }

    /* space size for new process titile */
    return environ_last - argv[0];
}

int set_proctitile(char ** argv, char * new_title)
{
    static int size = 0;
    static char is_init = 0;
    static char * saved_argv;

    if (!is_init) {
        if ((size = _init_set_proctitle(argv, &saved_argv)) < 0)
            return -1;
        is_init = 1;
    }

    snprintf(argv[0], size, "%s <%s>", new_title, saved_argv);
    argv[1] = NULL;

    return 0;
}

static void _sig_func(int signo)
{
    log_info("Receive signal(%d)!", signo);
}

int connect_timeo(int sockfd, struct sockaddr_in * server_addr, int nsec)
{
    int ret;
    sig_func_t old_sigfunc;

    old_sigfunc = signal_act(SIGALRM, _sig_func);
    if (alarm(nsec))
        log_warning("Alarm was alreday set!");

    if ((ret = connect_ipv4(sockfd, server_addr)) < 0) {
        if (errno == EINTR)
            errno = ETIMEDOUT;
    }

    alarm(0); /* turn off the alarm */
    signal_act(SIGALRM, old_sigfunc);

    return ret;
}

int recvmsginfo(int sockfd, struct msg_info * msg_info)
{
    int n, on = 1;
    struct msghdr msg;
    struct cmsghdr * cmg;
    struct iovec iov[1];
    struct in_pktinfo pktinfo;
    char msg_control[512]; /* may be insufficient ??? */
    
    memset(&msg, 0, sizeof(struct msghdr));
    msg.msg_name = &(msg_info->srcaddr);
    msg.msg_namelen = sizeof(struct sockaddr_in);
    iov[0].iov_base = msg_info->recvbuf;
    iov[0].iov_len = msg_info->bufsize;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = msg_control;
    msg.msg_controllen = sizeof(msg_control);

    setsockopt(sockfd, IPPROTO_IP, IP_PKTINFO, &on, sizeof(on));
    if ((n = recvmsg(sockfd, &msg, msg_info->flags)) < 0) {
        log_error_sys("Recvmsg error, sockfd: %d!", sockfd);
        return n;
    }

    msg_info->recvflags = msg.msg_flags;
    
    for(cmg = CMSG_FIRSTHDR(&msg); cmg != NULL; 
            CMSG_NXTHDR(&msg, cmg)) {
        if (cmg->cmsg_level == IPPROTO_IP && 
                cmg->cmsg_type == IP_PKTINFO) {
            memcpy(&pktinfo, CMSG_DATA(cmg), sizeof(struct in_pktinfo));
            msg_info->ifindx = pktinfo.ipi_ifindex;
            msg_info->dstaddr.sin_addr = pktinfo.ipi_addr;
            msg_info->dstaddr.sin_port = msg_info->srcaddr.sin_port;
            break;
        }
    }

    if (cmg == NULL) 
        log_error("No pktinfo found in msg!, sockfd: %d", sockfd);

    return n;
}

int tcp_connect(struct sockaddr_in * local_addr, 
        struct sockaddr_in * remote_add)
{
    int sockfd;

    if ((sockfd = tcp_socket_ipv4()) < 0)
        return -1;

    set_sock_reuseaddr(sockfd);
    if (local_addr) {
        if (bind_ipv4(sockfd, local_addr) < 0) {
            close(sockfd);
            return -1;
        }
    }

    if (connect_ipv4(sockfd, remote_add) < 0) {
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int tcp_listen(struct sockaddr_in * server_addr, int backlog)
{
    int sockfd;

    if ((sockfd = tcp_socket_ipv4()) < 0)
        return -1;

    set_sock_reuseaddr(sockfd);
    if (bind_ipv4(sockfd, server_addr) < 0) {
        close(sockfd);
        return -1;
    }

    if (listen_ipv4(sockfd, backlog) < 0) {
        close(sockfd);
        return -1;
    }

    return sockfd;
}

int write_pid_file(const char * pid_file)
{
    int fd, ret;
    char pid[32];

    if ((fd = open(pid_file, O_RDWR | O_CREAT, 
                    FILE_MODE)) < 0) {
        log_error_sys("Open pid file(%s) error!", pid_file);
        return -1;
    }

    ret = file_wlock(fd, 0, SEEK_SET, 0);
    if (ret < 0) {
        if (errno == EACCES || errno == EAGAIN)
            log_error_sys("Unable to lock file(%s)!", pid_file);
        else 
            log_error_sys("Acquire write lock error, file: %s!", pid_file);
        return -1;
    }

    if (truncate(pid_file, 0) < 0) {
        log_error_sys("Truncate file(%s) error!", pid_file);
        return -1;
    }

    snprintf(pid, sizeof(pid), "%ld\n", (long)getpid());
    if (writen(fd, pid, strlen(pid)) < strlen(pid)) {
        log_error_sys("Write pid to file(%s) error!", pid_file);
        return -1;
    }

    return 0;
}

int get_real_file_size(int fd)
{
    int real_size;
    struct stat stat;

    if (fstat(fd, &stat) < 0) {
        log_error_sys("Fstat fd(%d) error!", fd);
        return -1;
    }

    /* for Linux */
    real_size = stat.st_blocks * 512;
    if (real_size > stat.st_size)
        real_size = stat.st_size;

    return real_size;
}

/* =================================================================== */
struct timeval timeval_add(struct timeval * tv1, struct timeval * tv2)
{
    struct timeval res;

    res.tv_sec = tv1->tv_sec + tv2->tv_sec;
    res.tv_usec = tv1->tv_usec + tv2->tv_usec;

    if (res.tv_usec >= 1000000) {
        res.tv_usec -= 1000000;
        ++res.tv_sec;
    }

    return res;
}

struct timeval timeval_sub(struct timeval * tv1, struct timeval * tv2)
{
    struct timeval res;

    res.tv_sec = tv1->tv_sec - tv2->tv_sec;
    res.tv_usec = tv1->tv_usec - tv2->tv_usec;

    if (res.tv_usec < 0) {
        res.tv_usec += 1000000;
        --res.tv_sec;
    }

    return res;
}

int timeval_compare(struct timeval * tv1, struct timeval * tv2)
{
    if (tv1->tv_sec > tv2->tv_sec) {
        return 1;
    } else if (tv1->tv_sec == tv2->tv_sec) {
        if (tv1->tv_usec > tv2->tv_usec)
            return 1;
        else if (tv1->tv_usec == tv2->tv_usec)
            return 0;
    }

    return -1;
}

/* =================================================================== */

