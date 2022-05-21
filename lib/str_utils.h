/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：str_utils.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年05月18日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __STR_UTILS_H
#define __STR_UTILS_H

#define streq(s1, s2) !strcmp((s1), (s2))

/**
 * 获取路径中的文件名和文件名后缀 
 * 参数：
 *     path：路径，/path/file.txt
 *     file：文件名, 值结果, 不存在返回NULL 
 *     suffix: 后缀，值结果, 不存在返回NULL，可传NULL 
 */
void basename_path(char * path, char ** file, char ** suffix);

#endif //STR_UTILS_H
