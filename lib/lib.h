/**
 *===============================================================
 * Copyright (C) 2021 noodles. All rights reserved.
 * 
 * 文件名称：lib.h
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2021年09月11日
 * 描    述：
 * 
 *===============================================================
 */
#pragma once
#ifndef __LIB_H
#define __LIB_H

#include "log.h"
#include "socket.h"
#include "utils.h"
#include "buffer.h"
#include "opt.h"
#include "if_info.h"

int inetd(char * config_file);

#endif //LIB_H
