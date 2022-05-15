/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：dict.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年12月25日
 * 描    述：
 * 
 *===============================================================
 */
#include <stdlib.h>

#include "log.h"
#include "dict.h"

#include "../3rd/rbtree.h"

struct dict_entry {
    void * key;
    void * val;
    struct rb_node node;
};

struct dict {
    int size;
    struct dict_type * type;
    struct rb_root root;
};

inline int dict_size(struct dict * dict)
{
    return dict->size;
}

struct dict * dict_create(struct dict_type * type)
{
    struct dict * dict;

    if (!(dict = malloc(sizeof(struct dict)))) {
        log_warning_sys("Create dict: OOM!!!");
        return NULL;
    }

    dict->size = 0;
    dict->type = type;
    dict->root = RB_ROOT;

    return dict;
}

static void _free_dict_entry(struct dict * dict, 
        struct dict_entry * entry)
{
    struct dict_type * type;

    type = dict->type;

    /* in case of 'key' == NULL */
    if (entry->key && type->key_destroy)
        type->key_destroy(entry->key);

    /* in case of 'val' == NULL */
    if (entry->val && type->val_destroy)
        type->val_destroy(entry->val);

    free(entry);
}

static struct dict_entry * _newdict_entry(void)
{
    struct dict_entry * entry;

    if (!(entry = malloc(sizeof(struct dict_entry)))) {
        log_warning_sys("Dict add: OOM!!!");
        return NULL;
    }

    entry->key = 0;
    entry->val = 0;

    return entry;
}

static void _rb_tree_destory(struct dict * dict, struct rb_node * root)
{
    struct dict_entry * entry;
    struct rb_node * left, * right;

    if (!root) return;
    left = root->rb_left;
    right = root->rb_right;

    entry = container_of(root, struct dict_entry, node);
    _free_dict_entry(dict, entry);

    if (left)
        _rb_tree_destory(dict, left);

    if (right)
        _rb_tree_destory(dict, right );
}

static struct dict_entry * _dict_find(struct dict * dict, void * key)
{
    int cmp;
    struct dict_entry * entry;
    struct rb_node * node = dict->root.rb_node;

    while (node) {
        entry = container_of(node, struct dict_entry, node);
        if (dict->type->key_compare) {
            cmp = dict->type->key_compare(entry->key, key);
        } else {
            if (entry->key > key)
                cmp = 1;
            else if (entry->key == key)
                cmp = 0;
            else 
                cmp = -1;
        }

        if (cmp > 0)
            node = node->rb_left;
        else if (cmp < 0)
            node = node->rb_right;
        else 
            return entry;
    }

    return NULL;
}

static void  _dict_insert(struct dict * dict, struct dict_entry * entry)
{
    int cmp;
    struct dict_entry * this;
	struct rb_node ** new, * parent;

    parent = NULL;
    new = &(dict->root.rb_node);

	while (*new) {
		this = container_of(*new, struct dict_entry, node);
        if (dict->type->key_compare) {
            cmp = dict->type->key_compare(this->key, entry->key);
        } else {
            if (this->key > entry->key)
                cmp = 1;
            else if (this->key == entry->key)
                cmp = 0;
            else 
                cmp = -1;
        }

  	    parent = *new;
		if (cmp > 0)
			new = &((*new)->rb_left);
		else if (cmp < 0)
			new = &((*new)->rb_right);
		else
			return;
	}

    ++dict->size;
	rb_link_node(&(entry->node), parent, new);
	rb_insert_color(&(entry->node), &dict->root);
}

void dict_destroy(struct dict * dict)
{
    _rb_tree_destory(dict, dict->root.rb_node);
    free(dict);
}

int dict_find(struct dict * dict, void * key, void ** val)
{
    struct dict_entry * entry;

    entry = _dict_find(dict, key);
    if (!entry)
        return -1;

    *val = entry->val;
    return 0;
}

void dict_delete(struct dict * dict, void * key)
{
    struct dict_type * type;
    struct dict_entry * entry;

    type = dict->type;
    entry = _dict_find(dict, key);
    if (!entry)
        return;
    
    --dict->size;
    rb_erase(&entry->node, &dict->root);
    if (type->key_destroy)
        type->key_destroy(entry->key);

    if (type->val_destroy)
        type->val_destroy(entry->val);
    free(entry);
}

static int _dict_add(struct dict * dict, void * key, void * val)
{
    struct dict_entry * entry;

    if (!(entry = _newdict_entry()))
        return -1;

    /* in case of 'key' == NULL */
    if (key && dict->type->key_dup) {
        if (!(entry->key = dict->type->key_dup(key))) {
            _free_dict_entry(dict, entry);
            return -1;
        }
    } else {
        entry->key = key;
    }

    /* in case of 'val' == NULL */
    if (val && dict->type->val_dup) {
        if (!(entry->val = dict->type->val_dup(val))) {
            _free_dict_entry(dict, entry);
            return -1;
        }
    } else {
        entry->val = val;
    }

    _dict_insert(dict, entry);
    return 0;
}

int dict_add(struct dict * dict, void * key, void * val)
{
    struct dict_entry * entry;

    entry = _dict_find(dict, key);
    if (entry)
        return -1;

    return _dict_add(dict, key, val) < 0 ? -2 : 0;
}

int dict_update(struct dict * dict, void * key, void * val)
{
    void * old_val;
    struct dict_entry * entry;

    entry = _dict_find(dict, key);
    if (entry) {
        old_val = entry->val;
        /* in case of 'val' == NULL */
        if (val && dict->type->val_dup) {
            if (!(entry->val = dict->type->val_dup(val))) {
                entry->val = old_val;
                return -1;
            }
        } else {
            entry->val = val;
        }
        
        if (old_val && dict->type->val_destroy)
            dict->type->val_destroy(old_val);

        return 0;
    }

    return _dict_add(dict, key, val);
}

inline void * dict_fetch_key(struct dict_entry * entry)
{
    return entry->key;
}

inline void * dict_fetch_val(struct dict_entry * entry)
{
    return entry->val;
}

inline struct dict_entry * dict_first(struct dict * dict)
{
    struct rb_node * node;

    if (!(node = rb_first(&dict->root)))
        return NULL;

    return container_of(node, struct dict_entry, node);
}

inline struct dict_entry * dict_next(struct dict_entry * entry)
{
    struct rb_node * node;

    if (!(node = rb_next(&entry->node)))
        return NULL;

    return container_of(node, struct dict_entry, node);
}

/**
 * 键和值都是字符串，且都是在栈上分配的
 */
struct dict_type hstr_2_hstr_map = {
    .key_destroy = free, 
    .val_destroy = free,
    .key_compare = (key_compare_t)strcmp,
    .key_dup = (key_dup_t)strdup,
    .val_dup = (val_dup_t)strdup,
};

