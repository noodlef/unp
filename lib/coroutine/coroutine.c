/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：coroutine.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年08月31日
 * 描    述：
 * 
 *===============================================================
 */
#include <ucontext.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "coroutine.h"
#include "../../3rd/rbtree.h"

#define INIT_CID    0
#define COROUTINE_STK_SZ 8 * 1024 * 1024

#define COROUTINE_RUNNABLE(status) (((status) == COROUTINE_READY \
                                        || (status) == COROUTINE_STOPPED))

struct coroutine_struct {
    cid_t cid;
    cid_t parent;
    char status;
    void * args;
    size_t stack_size;
    char * stack;
    coroutine_func_t run;
    ucontext_t context;
    struct rb_node node;
};

static struct coroutine_struct G_main_coroutine;
static struct coroutine_struct * G_last_died_coroutine;
static struct coroutine_struct * G_current_coroutine = &G_main_coroutine;

/* organized all coroutines on an rb-tree for quick look-up */
static struct rb_root G_coroutine_map = RB_ROOT;
static struct coroutine_struct * __search_map(cid_t cid)
{
	struct rb_node * node = G_coroutine_map.rb_node;

  	while (node) {
        struct coroutine_struct * co = 
            container_of(node, struct coroutine_struct, node);

		if (cid < co->cid)
  			node = node->rb_left;
		else if (cid > co->cid) 
  			node = node->rb_right;
		else
  			return co;
	}

    return NULL;
}

static void __insert_into_map(struct coroutine_struct * co)
{
	struct rb_node ** new = &(G_coroutine_map.rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct coroutine_struct * this = 
            container_of(*new, struct coroutine_struct, node);

  	    parent = *new;
		if (co->cid < this->cid)
			new = &((*new)->rb_left);
		else if (co->cid > this->cid)
			new = &((*new)->rb_right);
		else
            /* ignore when find an node with the same cid */
			return;
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&(co->node), parent, new);
	rb_insert_color(&(co->node), &G_coroutine_map);
}

static void __remove_from_map(struct coroutine_struct * co)
{
    rb_erase(&co->node, &G_coroutine_map);
}

static void __set_coroutine_attr(
        struct coroutine_struct * co, coroutine_attr_t * attr) 
{
    assert(co != NULL);
    struct coroutine_struct * tmp;

    if (!attr)
        return;

    if (attr->parent >= INIT_CID) {
        tmp = __search_map(attr->parent);
        if (tmp)
            co->parent = attr->parent;
    }
}

inline static cid_t __get_next_cid() 
{
    static cid_t next_cid = INIT_CID;

    /* in case overflow */
    if (++next_cid < INIT_CID)
        next_cid = INIT_CID;

    return next_cid;
}

inline static void * __set_current_coroutine(struct coroutine_struct * co)
{
    assert(co != NULL);
    G_current_coroutine = co; 
}

inline static struct coroutine_struct * __get_current_coroutine()
{
    return G_current_coroutine;
}

static void __main(struct coroutine_struct *);
static void __init_context(struct coroutine_struct * co)
{
    assert(co != NULL);

    getcontext(&co->context);
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = co->stack_size; 

    /**
     * let it's parent resume when this coroutine exits, Note: 
     * the 'uc_link' member is used to determine the context 
     * that will be resumed when the context returns 
     */ 
    co->context.uc_link = &(G_main_coroutine.context);
    makecontext(&co->context, (void (*)(void))__main, 1, co);
}

inline static void __init_main_coroutine(void)
{
    G_main_coroutine.args = NULL;
    G_main_coroutine.run = (coroutine_func_t) NULL;
    G_main_coroutine.cid = INIT_CID;
    G_main_coroutine.parent = -1;
    G_main_coroutine.status = COROUTINE_RUNNING;
    G_main_coroutine.stack = NULL;
    G_main_coroutine.stack_size = 0;

    __insert_into_map(&G_main_coroutine);
}

cid_t coroutine_create(
        coroutine_func_t run, void * args, coroutine_attr_t * attr)
{
    static char is_init = 0;
    struct coroutine_struct * co = malloc(sizeof(struct coroutine_struct));
    if (!co)
        return -1;

    if (!is_init) {
        __init_main_coroutine();
        is_init = 1;
    }

    memset(co, 0, sizeof(struct coroutine_struct));
    co->run = run;
    co->args = args;
    co->status = COROUTINE_READY;
    /* stack memory is not allocated untill it's running */
    co->stack_size = COROUTINE_STK_SZ;

    /* set it's parent to the current coroutine by default */
    co->parent = G_current_coroutine->cid; 
    co->cid = __get_next_cid();

    __set_coroutine_attr(co, attr);
    __insert_into_map(co);

    return co->cid;
}

cid_t coroutine_get_current()
{
    struct coroutine_struct * current;

    current = __get_current_coroutine();
    return current->cid; 
}

inline static void __destory_coroutine(struct coroutine_struct * co)
{
    if (!co)
        return;

    __remove_from_map(co);
    free(co->stack);
    free(co);
}

inline static struct coroutine_struct * __find_parent(
        struct coroutine_struct * co)
{
    assert(co != NULL);
    struct coroutine_struct * parent;

    /**
     * note that the main coroutine will adopt it and 
     * resume when it's parent is not runnable. 
     */ 
    parent = __search_map(co->parent);
    if (!parent || !COROUTINE_RUNNABLE(parent->status))
        parent = &G_main_coroutine;

    return parent;
}

static void __main(struct coroutine_struct * co)
{
    assert(co != NULL);
    struct coroutine_struct * parent;

    co->run(co->args);
    
    parent = __find_parent(co); 

    __set_current_coroutine(parent);
    parent->status = COROUTINE_RUNNING;
    co->status = COROUTINE_DEAD;

    if (G_last_died_coroutine)
        __destory_coroutine(G_last_died_coroutine);
    G_last_died_coroutine = co;

    /* no return */
    setcontext(&(parent->context));
}

int coroutine_switch(cid_t cid) 
{
    assert(cid >= INIT_CID);
    struct coroutine_struct * current, * co;

    co = __search_map(cid);
    current = G_current_coroutine; 
    if (!co || co == current)
        return -1;

    if (!COROUTINE_RUNNABLE(co->status))
        return -1;
    
    if (co->status == COROUTINE_READY) {
        co->stack = malloc(sizeof(char) * co->stack_size);
        if (!co->stack)
            return -1;
        __init_context(co);
    }

    current->status = COROUTINE_STOPPED;
    co->status = COROUTINE_RUNNING;
    __set_current_coroutine(co);

    /* yield execution to the coroutine pointed by 'co' */
    swapcontext(&(current->context), &(co->context));
    return 0;
}

int coroutine_cancel(cid_t cid)
{
    assert(cid >= INIT_CID);
    struct coroutine_struct * co, * parent;

    co = __search_map(cid);
    if (!co || co == &G_main_coroutine)
        return -1;

    if (co != G_current_coroutine) {
        /* do nothing if already died */
        if (co != G_last_died_coroutine)
            __destory_coroutine(co);
        return 0;
    }
        
    /* cancel self */
    co->status = COROUTINE_CANCELED;
    if (G_last_died_coroutine)
        __destory_coroutine(G_last_died_coroutine);
    G_last_died_coroutine = co;

    parent = __find_parent(co);
    setcontext(&(parent->context));
}

inline cid_t coroutine_get_main()
{
    return G_main_coroutine.cid;
}

char * coroutine_str_status(int status)
{
    char * str_status;
    
    if (status == COROUTINE_READY)
        str_status = "ready";
    else if (status == COROUTINE_RUNNING)
        str_status = "running";
    else if (status == COROUTINE_STOPPED)
        str_status = "stopped";
    else if (status == COROUTINE_DEAD)
        str_status = "died";
    else if (status == COROUTINE_CANCELED)
        str_status = "canceled";
    else 
        str_status = "unknown";

    return str_status;
}

#ifdef __COROUTINE_DEBUG
#include <stdio.h>
void print_coroutines(void)
{
    struct rb_node * node;
    struct coroutine_struct * co;

    printf("=============================================\n");
    printf("Debug: list all coroutines:\n");
    for (node = rb_first(&G_coroutine_map); node; 
            node = rb_next(node)) {
        co = container_of(node, struct coroutine_struct, node);

        printf("++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("cid:    %d\n", co->cid);
        printf("parent: %d\n", co->parent);
        printf("status: %s\n", coroutine_str_status(co->status));
        printf("\n");
    }
    printf("============================================\n");
}
#endif
