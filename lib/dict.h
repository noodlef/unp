/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：dict.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年12月24日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __DICT_H
#define __DICT_H
#include <stdlib.h>
#include <string.h>

typedef void (* key_destroy_t)(void * key);
typedef void (* val_destroy_t)(void * val);
typedef void * (* key_dup_t)(void * key);
typedef void * (* val_dup_t)(void * val);
typedef int (* key_compare_t)(void * key1, void * key2);

struct dict_type {
    key_destroy_t key_destroy;
    val_destroy_t val_destroy;
    key_dup_t key_dup;
    val_dup_t val_dup;
    key_compare_t key_compare;
};

struct dict;
struct dict_entry;

extern struct dict_type hstr_2_hstr_map;
/**
 * 创建字典，返回值如下：
 *     返回     NULL：失败 
 *     返回 NOT NULL：成功
 */
struct dict * dict_create(struct dict_type * type);

inline int dict_size(struct dict * dict);

/**
 * 删除字典，释放占用的空间  
 */
void dict_destroy(struct dict * dict);

/**
 * 查找键值为 key 的元素的值，返回值存放在 val 中 
 * 返回值如下：
 *     返回 -1：字典中不存在 key
 *     返回  0：成功找到
 */
int dict_find(struct dict * dict, void * key, void ** val);

/**
 * 删除字段中 key 的健，不存在不报错 
 */
void dict_delete(struct dict * dict, void * key);

/**
 * 字典中添加新的键，已存在则失败 
 * 返回值如下：
 *     返回  0：成功添加新的键值对
 *     返回 -1：键已存在
 *     返回 -2：其它错误
 */
int dict_add(struct dict * dict, void * key, void * val);

/**
 * 向字典中添加新键或更新已有的键的值 
 * 返回值如下：
 *     返回  0：成功
 *     返回 -1: 失败 
 */
int dict_update(struct dict * dict, void * key, void * val);

/*=============================================================================*/
/**
 * 遍历字典，方式如下： 
 * for (entry = dict_first(dict); entry; 
 *                      entry = dict_next(entry)) {
 *      key = dict_fetch_key(entry);
 *      val = dict_fetch_val(entry);
 *      ...
 *  }
 */
inline void * dict_fetch_key(struct dict_entry * entry);

inline void * dict_fetch_val(struct dict_entry * entry);

inline struct dict_entry * dict_first(struct dict * dict);

inline struct dict_entry * dict_next(struct dict_entry * entry);
/*=============================================================================*/

#endif //DICT_H
