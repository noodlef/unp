/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：inetd.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月28日
 * 描    述：
 * 
 *===============================================================
 */
#include <pwd.h>
#include <wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/signal.h>

#include "../lib/opt.h"
#include "../lib/log.h"
#include "../lib/utils.h"
#include "../lib/socket.h"
#include "../3rd/ini.h"
#include "../3rd/rbtree.h"

struct server_conf {
    char * service_name;
    char * socket_type;
    char * protocol;
    char * wait_flag;
    char * login_name;
    char * server_program;
    char * server_program_arguments[32];
};

struct wait_queue {
    pid_t pid;
    int sockfd;
    struct server_conf * conf;
    struct rb_node node;
};

static int G_server_conf_cnt = 0;

static struct server_conf G_server_confs[128];

static char * G_last_section = NULL;

static struct server_conf * G_server_map[1024];

static char G_reload_config_file = 0;

static char  * G_config_file;

static char * G_log_file = NULL;

fd_set G_read_set;

static struct rb_root G_wait_queue = RB_ROOT;

static void _insert_into_queue(
        struct rb_root * wait_queue, struct wait_queue * wait)
{
    struct wait_queue * this;
	struct rb_node ** new, * parent;

    parent = NULL;
    new = &(wait_queue->rb_node);

	while (*new) {
		this = container_of(*new, struct wait_queue, node);

  	    parent = *new;
		if (this->pid > wait->pid)
			new = &((*new)->rb_left);
		else if (this->pid < wait->pid)
			new = &((*new)->rb_right);
		else
			return;
	}

	rb_link_node(&(wait->node), parent, new);
	rb_insert_color(&(wait->node), wait_queue);
}

static struct wait_queue * _search_queue(
        struct rb_root * wait_queue, int pid)
{
    
    struct wait_queue * this;
	struct rb_node * node = wait_queue->rb_node;

  	while (node) {
        this = container_of(node, struct wait_queue, node);
        
		if (this->pid > pid)
  			node = node->rb_left;
		else if (this->pid < pid) 
  			node = node->rb_right;
		else
  			return this;
	}

    return NULL;
}

static void _remove_from_queue(
        struct rb_root * wait_queue, struct wait_queue * wait)
{
    rb_erase(&(wait->node), wait_queue);
}

static void _free_node(struct rb_node * node)
{
    struct wait_queue * wait_node;

    if (!node)
        return;

    if (node->rb_left)
        _free_node(node->rb_left);
        
    if (node->rb_right)
        _free_node(node->rb_right);

    wait_node = container_of(node, struct wait_queue, node);
    free(wait_node);
}

void _free_queue(struct rb_root * wait_queue)
{
	struct rb_node * root = wait_queue->rb_node;
   
    _free_node(root); 
    *wait_queue =  RB_ROOT;
}

static int _create_sockfd(struct server_conf * conf)
{
    int sockfd;
    struct sockaddr_in server_addr;

    if (!strcmp(conf->protocol, "udp")) {
        sockfd = udp_socket_ipv4(); 
        set_sock_reuseaddr(sockfd);
    } else if (!strcmp(conf->protocol, "tcp")) {
        sockfd = tcp_socket_ipv4(); 
    } else {
        log_error("Invalid protocol: %s!", conf->protocol);
        return -1;
    }

    if (sockfd < 0)
        return -1;

    init_sockaddr_ipv4(&server_addr, ADDR_ANY_IPV4, 
            atoi(conf->service_name));
    if (bind_ipv4(sockfd, &server_addr) < 0)
        return -1;

    if (!strcmp(conf->protocol, "tcp")) {
        if (listen_ipv4(sockfd, 5) < 0)
            return -1;
    }

    return sockfd;
}

static int _init_sockfd_set(struct server_conf ** servers) 
{
    int i, new_maxfd;

    new_maxfd = -1;
    FD_ZERO(&G_read_set);

    for (i = 0; i < 1024; i++) {
        if (servers[i]) {
            FD_SET(i, &G_read_set);
            new_maxfd = max(new_maxfd, i);
        }
    }

    return new_maxfd + 1;
}

static int _process_connection(int sockfd)
{
    uint16_t port;
    int pid, i, drop;
    char * user, ip[32];
    struct sockaddr_in conn_addr;
    struct passwd * passwd;
    struct server_conf * server;
    struct wait_queue * wait;

    server = G_server_map[sockfd];

    if (!strcmp(server->protocol, "tcp")) {
        sockfd = accept_ipv4(sockfd, &conn_addr); 
        if (sockfd < 0)
            return -1;

        sockaddr_to_ipport_ipv4(&conn_addr, ip, &port);
        log_info("Connection from IP: %s, Port: %d", ip, port);
    }
    
    if ((pid = fork()) > 0) {
        /* the parent */
        if (!strcmp(server->protocol, "tcp"))
            close(sockfd);
        
        /* wait util the child exits for UDP servers */
        if (!strcmp(server->protocol, "udp")) {
            wait = malloc(sizeof(struct wait_queue));
            if (!wait) {
                log_error("Malloc failed, sockfd: %d", sockfd);
                return 0;
            }

            wait->pid = pid;
            wait->conf = server;
            wait->sockfd = sockfd;
            _insert_into_queue(&G_wait_queue, wait);

            G_server_map[sockfd] = NULL;
        }
        return 0;
    } else if (pid < 0) {
        log_error_sys("Fork error!");
        return -1;
    }
    
    /* the child */
    close(0);
    close(1);
    close(2);
    for (i = 0; G_server_map[i]; i++) {
        if (i != sockfd)
            close(i);
    }

    dup2(sockfd, 0);
    dup2(sockfd, 1);
    dup2(sockfd, 2);
    close(sockfd);

    user = server->login_name;
    if (!(passwd = getpwnam(user))) {
        log_error_sys("Get user(%s) passwd failed!", user);
        goto failed;
    }

    if (passwd->pw_uid) {
        /* for no root user */
        setuid(passwd->pw_uid); 
        setgid(passwd->pw_gid);
        chdir(passwd->pw_dir);
    }

    log_info("Begin to execute OBJ-FILE: %s", server->server_program);
    log_info("Args are as follow:");
    for (i = 0; server->server_program_arguments[i]; i++)
        log_info("\t\t#%d\t\t%s", i, server->server_program_arguments[i]);

    if (execvp(server->server_program, 
                server->server_program_arguments) < 0){
        log_error_sys("Exec program(%s) failed!", server->server_program);
        goto failed;
    }

    /* should never get here */
    _exit(EXIT_FAILURE);

failed:

    /**
     * must drop the datagram in recvbuf if 'UDP', or the parent 
     * will fork another child again to handle the reqeust. 
     */
    if (!strcmp(server->protocol, "udp")) {
        if (read(0, (char *)&drop, sizeof(drop)) < 0) {
            log_warning_sys("Read error when drop udp data!");
        }
    }
    _exit(EXIT_FAILURE);
}

static void _create_socket_fds(void)
{
    int i, sockfd;
    struct server_conf * conf;

    conf = G_server_confs;
    for (i = 0; i < G_server_conf_cnt; i++) {
        conf = G_server_confs + i;
        if (!conf->service_name)
            continue;

        sockfd = _create_sockfd(conf);
        if (sockfd < 0) {
            log_error("Create sockfd for service: %s, protocol: %s failed!", 
                    conf->service_name, conf->protocol);
            continue;
        }
        G_server_map[sockfd] = conf;
        log_info("Create sockfd for service: %s, protocol: %s, sockfd: %d", 
                conf->service_name, conf->protocol, sockfd);
    }
}

static int _reload_config_file(void);
static int _start_server(void)
{
    int sockfd, maxfd;
    sigset_t old_set, blocked_set;

    _create_socket_fds();

    if (sigemptyset(&blocked_set) < 0)
        log_error_sys("Sigemptyset error!");

    if (sigaddset(&blocked_set, SIGHUP) < 0)
        log_error_sys("Sigaddset error!");

    if (sigprocmask(SIG_BLOCK, &blocked_set, &old_set) < 0) {
        log_error_sys("Sigprocmask error!");
        sigemptyset(&old_set);
    }

    while (1) {
        if (G_reload_config_file) {
            log_info("Reload config file, file: %s", G_config_file);
            if (_reload_config_file() < 0)
                return -1;
            G_reload_config_file = 0;
        }

        maxfd = _init_sockfd_set(G_server_map);
        if ((sockfd = pselect(maxfd, &G_read_set, NULL, NULL, NULL, 
                        &old_set)) < 0) {
            if (errno != EINTR)
                log_warning_sys("Select error!");
            continue;
        }

        for (sockfd = 0; sockfd < 1024; sockfd++) {
            if (FD_ISSET(sockfd, &G_read_set))
                _process_connection(sockfd);
        }
    }

    return 0;
}

static void _sig_child(int signo)
{
    int stat, pid, sockfd;
    struct wait_queue * wait;

    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        log_warning("Child(%d) terminated by signal(%d)",
                pid, signo);

        if ((wait = _search_queue(&G_wait_queue, pid))) {
            sockfd = wait->sockfd;
            G_server_map[sockfd] = wait->conf; 
            _remove_from_queue(&G_wait_queue, wait);
        }
    }

    return;
}

static int _parse_handler(void * server_confs, 
        const char * section, const char * cfg_name, const char * cfg_value)
{
    int i;
    char * opt, * copyed_cfg;
    struct server_conf * conf; 

    if (!strcmp(section, "log_file")) {
        if (!strcmp(cfg_name, "log_file"))
            G_log_file = strdup(cfg_value);
        else
            return 0;
        return 1;
    }

    /* server conf sections startwith 'server:' */
    if(strstr(section, "server:") != section)
        return 0;

    if (!G_last_section) {
        G_last_section = (char *) strdup(section);
        G_server_conf_cnt = 1;
    }

    if (strcmp(section, G_last_section)) {
        free(G_last_section);
        G_last_section = (char *) strdup(section);
        G_server_conf_cnt++;
    }
    conf = (struct server_conf *)server_confs + G_server_conf_cnt - 1; 

    /* (NOTE) need to be free */
    copyed_cfg = strdup(cfg_value);

    if (!strcmp(cfg_name, "service_name")) {
        conf->service_name = copyed_cfg; 
    } else if (!strcmp(cfg_name, "socket_type")) {
        conf->socket_type = copyed_cfg; 
    } else if (!strcmp(cfg_name, "protocol")) {
        conf->protocol = copyed_cfg; 
    } else if (!strcmp(cfg_name, "wait_flag")) {
        conf->wait_flag = copyed_cfg; 
    } else if (!strcmp(cfg_name, "login_name")) {
        conf->login_name = copyed_cfg; 
    } else if (!strcmp(cfg_name, "server_program")) {
        conf->server_program = copyed_cfg;
        /* (NOTE): program name should be the first argument of argv[] */
        conf->server_program_arguments[0] = copyed_cfg; 
    } else if (!strcmp(cfg_name, "server_program_arguments")) {
        i = 1;
        if ((opt = strtok(copyed_cfg, " "))) {
            conf->server_program_arguments[i++] = opt;
            while ((opt = strtok(NULL, " ")))
                conf->server_program_arguments[i++] = opt;
            conf->server_program_arguments[i++] = NULL; 
        }
    } else {
        return 0; /* ignore unknown cfg */
    }

    return 1;
}

static void _sig_hub(int signo)
{
    G_reload_config_file = 1;
    log_info("Caught signal: %d", signo);
}

static int _reload_config_file(void) 
{
    int i;

    /* load new confs */
    G_server_conf_cnt = 0;
    G_last_section = NULL;
    if ((i = ini_parse(G_config_file, _parse_handler, &G_server_confs)) < 0) {
        log_error("Parse config file failed, config_file: %s", G_config_file);
        return -1;
    }

    /* close all opend socket fds */
    for (i = 0; i < 1024; i++) {
        if (G_server_map[i]) {
            close(i);
            G_server_map[i] = NULL;
        }
    }

    _free_queue(&G_wait_queue);

    /* create socket fds */
    _create_socket_fds();

    return 0;
}

int inetd(char * config_file)
{
    G_config_file = config_file;
    if (ini_parse(config_file, _parse_handler, &G_server_confs) < 0) {
        printf("Parse config file failed, config_file: %s\n", config_file);
        return EXIT_FAILURE;
    }

    if (daemon_init() < 0) {
        printf("Init daemon failed!\n");
        return EXIT_FAILURE;
    }

    if (init_log(G_log_file, 1) < 0) {
        printf("Init log failed!\n");
        return EXIT_FAILURE;
    }

    if (write_pid_file("/var/run/inetd.pid") < 0) {
        return EXIT_FAILURE;
    }

    signal_act(SIGCHLD, _sig_child);

    signal_act(SIGHUP, _sig_hub);
    
    log_info("==========================================================");
    log_info("Starting inetd-server with config-file(%s)...", config_file);

    if (_start_server() < 0) {
        log_error("Inetd-server quit unexpectedly!!!");
        return EXIT_FAILURE;
    }

    log_info("Inetd-server exit!", config_file);
    log_info("==========================================================");

    return EXIT_SUCCESS;
}
