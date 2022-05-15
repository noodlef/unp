/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：defs.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年10月18日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __DEFS_H
#define __DEFS_H

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);  \
    (type *)( (char *)__mptr - offsetof(type,member) );})

#endif //DEFS_H
