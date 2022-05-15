/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：arg.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月25日
 * 描    述：
 * 
 *===============================================================
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <getopt.h>

#include "log.h"
#include "opt.h"

struct _opt_node {
    char found;
    char * opt;
    char * opt_value;
    struct opt_def * opt_def;
    struct rb_node node;
};

static int _parse_opt_def(struct opt_def * opt_defs, int len, 
        char * optstring, struct option * long_options)
{
    int i, j, flags; 
    struct opt_def * def;

    assert(opt_defs != NULL);
    assert(optstring != NULL);
    assert(long_options != NULL);
     
    i = 0;
    j = 0;
    def = opt_defs;
    optstring[i++] = ':';

    while (len-- > 0) {
        if (!def->short_opt && !def->long_opt) {
            printf("Invalid arg definition!\n");
            return -1;
        }

        flags = def->flags;
        /* short option */
        if (def->short_opt) {
            optstring[i++] = *def->short_opt;
            if (flags & OPT_VALUE_REUIRED) {
                optstring[i++] = ':';
            } else if (flags & OPT_VAULE_OPTIONAL) {
                optstring[i++] = ':';
                optstring[i++] = ':';
            } 
            /* OPT_VALUE_OMITTED */
        } 

        /* long option */
        if (def->long_opt) {
            long_options[j].name = def->long_opt;
            long_options[j].flag = NULL;
            if (flags & OPT_VALUE_REUIRED)
                long_options[j].has_arg = required_argument;
            else if (flags & OPT_VAULE_OPTIONAL) long_options[j].has_arg = optional_argument;
            else 
                long_options[j].has_arg = no_argument;

            long_options[j].val = 0;
            ++j;
        }

        ++def;
    }

    optstring[i] = '\0';
    long_options[j].name = NULL;
    long_options[j].flag = NULL;
    long_options[j].has_arg = 0;
    long_options[j].val = 0;

    return 0;
}

static void _insert_into_map(struct rb_root * opt_map, struct _opt_node * opt)
{
    int i;
    struct _opt_node * this;
	struct rb_node ** new, * parent;

    assert(opt_map != NULL);
    assert(opt != NULL);

    parent = NULL;
    new = &(opt_map->rb_node);

	while (*new) {
		this = container_of(*new, struct _opt_node, node);

  	    parent = *new;
        i = strcmp(this->opt, opt->opt); 
		if (i > 0)
			new = &((*new)->rb_left);
		else if (i < 0)
			new = &((*new)->rb_right);
		else
            /* ignore when find a same option */
			return;
	}

	rb_link_node(&(opt->node), parent, new);
	rb_insert_color(&(opt->node), opt_map);
}

static struct _opt_node * _search_map(struct rb_root * opt_map, char * opt)
{
    
    int i;
    struct _opt_node * this;
	struct rb_node * node = opt_map->rb_node;

    assert(opt_map != NULL);
    assert(opt != NULL);

  	while (node) {
        this = container_of(node, struct _opt_node, node);
        
        i = strcmp(this->opt, opt);
		if (i > 0)
  			node = node->rb_left;
		else if (i < 0) 
  			node = node->rb_right;
		else
  			return this;
	}

    return NULL;
}

static void _free_opt_node(struct rb_node * node)
{
    struct _opt_node * opt_node;

    if (!node)
        return;

    if (node->rb_left)
        _free_opt_node(node->rb_left);
        
    if (node->rb_right)
        _free_opt_node(node->rb_right);

    opt_node = container_of(node, struct _opt_node, node);
    free(opt_node);
}

void free_opt_map(struct rb_root * opt_map)
{
	struct rb_node * root = opt_map->rb_node;
    
    _free_opt_node(root);
}

static struct rb_root * _opt_map_new(struct opt_def * opt_defs, int len)
{
    int i;
    struct rb_root * opt_map;
    struct _opt_node * opt_node;

    assert(opt_defs != NULL);
    
    opt_map = (struct rb_root *) malloc(sizeof(struct rb_root));
    if (!opt_map)
        return NULL;
    
    *opt_map = RB_ROOT;
    for (i = 0; i < len; i++) {
        if (opt_defs[i].short_opt) {
            opt_node = malloc(sizeof(struct _opt_node));
            if (!opt_node)
                goto failed;

            opt_node->found = 0;
            opt_node->opt_def = opt_defs + i;
            opt_node->opt_value = NULL;
            opt_node->opt = opt_defs[i].short_opt;
            _insert_into_map(opt_map, opt_node);
        }

        if (opt_defs[i].long_opt) {
            opt_node = malloc(sizeof(struct _opt_node));
            if (!opt_node)
                goto failed;
            
            opt_node->found = 0;
            opt_node->opt_def = opt_defs + i;
            opt_node->opt_value = NULL;
            opt_node->opt = opt_defs[i].long_opt;
            _insert_into_map(opt_map, opt_node);
        }
    }

    return opt_map;

failed:
    free_opt_map(opt_map);
    return NULL;
}

static struct _opt_node *  _set_opt(struct rb_root * opt_map, char * opt)
{
    struct _opt_node * opt_node;

    assert(opt_map != NULL);
    assert(opt != NULL);

    opt_node = _search_map(opt_map, opt);
    if (!opt_node) {
        printf("Undefined option '-%s'\n", opt);
        return NULL;
    }

    opt_node->found = 1;
    if (optarg) {
        /* option argumnt startwith - or -- */
        if (optarg[0] == '-') {
            printf("Option '-%s' missing argument!\n", opt); 
            return NULL;
        }

        opt_node->opt_value = optarg;
    } else {
        /* no argument option */
        opt_node->opt_value = OPT_NO_VALUE;
    }

    return opt_node;
}

struct rb_root * get_opt_map(int argc, char ** argv, 
        struct opt_def * opt_defs, int len)
{
    int opt_idx, ret;
    struct rb_root * opt_map;
    struct _opt_node * opt_node;
    struct option long_options[128];
    char optstring[512], opt_name[8], * arg;
    
    opt_map = _opt_map_new(opt_defs, len); 
    if (!opt_map)
        return NULL;

    ret = _parse_opt_def(opt_defs, len, optstring, long_options);
    if (ret < 0)
        return NULL;

    while((ret = getopt_long(argc, argv, 
                optstring, long_options, &opt_idx)) != -1) {
        arg = argv[optind -1];
        switch (ret) {
            case '?':
                /* invalid option */
                printf("Unkown option '%s'!\n", arg);
                return NULL;
            case ':':
                /* missing option argument */
                printf("Option '%s' missing argument!\n", arg); 
                return NULL;
            case 0:
                /* long option */
                opt_node = _set_opt(opt_map, (char *)long_options[opt_idx].name);
                if (!opt_node)
                    return NULL;
                if (opt_node->opt_def->short_opt) {
                    opt_node = _set_opt(opt_map, opt_node->opt_def->short_opt);
                    if (!opt_node)
                        return NULL;
                }
                break;
            default:
                opt_name[0] = ret;
                opt_name[1] = '\0';
                if (!strstr(optstring, opt_name)) {
                    printf("??? getopt returned character code '%c'\n", ret);
                    return NULL;
                }

                /* short option */
                opt_node = _set_opt(opt_map, opt_name);
                if (!opt_node)
                    return NULL;
                if (opt_node->opt_def->long_opt) {
                    opt_node = _set_opt(opt_map, opt_node->opt_def->long_opt);
                    if (!opt_node)
                        return NULL;
                }
        }
    }

    if (optind < argc) {
        printf("Non-option arguments: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
        return NULL;
    }

    /* TODO(noodles): check required otions */

    return opt_map;
}

char * get_opt_value(struct rb_root * opt_map, char * opt)
{
    struct _opt_node * opt_node;

    opt_node = _search_map(opt_map, opt);
    if (!opt_node)
        return OPT_NOT_FOUND;

    if (opt_node->found)
        return opt_node->opt_value;

    return OPT_NOT_FOUND;
}

void print_otions(struct rb_root * opt_map)
{
    struct rb_node * node;
    struct _opt_node * opt_node;

    printf("=============================================\n");
    printf("Debug: list all options:\n");
    for (node = rb_first(opt_map); node; 
            node = rb_next(node)) {
        opt_node = container_of(node, struct _opt_node, node);

        printf("++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("opt: %s\n", opt_node->opt);
        if (opt_node->found) {
            if (opt_node->opt_value == OPT_NO_VALUE)
                printf("opt_value: %p\n", opt_node->opt_value);
            else 
                printf("opt_value: %s\n", opt_node->opt_value);
        }
        printf("found:    %d\n", opt_node->found);
        printf("\n");
    }
    printf("============================================\n");
}
