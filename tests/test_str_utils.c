/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：test_str_utils.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年05月18日
 * 描    述：
 * 
 *===============================================================
 */
#include <string.h>

#include "../lib/test.h"
#include "../lib/str_utils.h"

void test_basename_path(void)
{
    char * path, * file, * suffix;

    path = NULL;
    basename_path(path, &file, &suffix);
    TEST_COND("Basename(NULL)", !file && !suffix);

    path = "redis.conf";
    basename_path(path, &file, &suffix);
    TEST_COND("Basename(redis.conf)", 
            !strcmp(file, "redis.conf") && !strcmp(suffix, "conf"));

    path = ".conf";
    basename_path(path, &file, &suffix);
    TEST_COND("Basename(.conf)", 
            !strcmp(file, ".conf") && !strcmp(suffix, "conf"));

    path = ".";
    basename_path(path, &file, &suffix);
    TEST_COND("Basename(.)", 
            !strcmp(file, ".") && suffix == NULL); 

    path = "name.";
    basename_path(path, &file, &suffix);
    TEST_COND("Basename(name.)", 
            !strcmp(file, "name.") && suffix == NULL); 

    path = "/name.txt";
    basename_path(path, &file, &suffix);
    TEST_COND("Basename(name.txt)", 
            !strcmp(file, "name.txt") && !strcmp(suffix, "txt")); 

    path = "path/name.txt";
    basename_path(path, &file, &suffix);
    TEST_COND("Basename(path/name.txt)", 
            !strcmp(file, "name.txt") && !strcmp(suffix, "txt")); 

    path = "path/name..txt";
    basename_path(path, &file, &suffix);
    TEST_COND("Basename(path/name..txt)", 
            !strcmp(file, "name..txt") && !strcmp(suffix, "txt")); 

    path = "";
    basename_path(path, &file, &suffix);
    TEST_COND("Basename(null)", 
            file == NULL && suffix == NULL); 

    path = "/root/path/name..txt";
    basename_path(path, &file, NULL);
    TEST_COND("Basename(/root/path/name..txt)", 
            !strcmp(file, "name..txt")); 
}

int main(int argc, char ** argv)
{
    test_basename_path();
    TEST_REPORT();
    return 0;
}
