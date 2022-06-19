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

static int _strarrayeq(char ** strs1, char ** strs2)
{
    int i;

    if (!strs1 || !strs2) {
        if (!strs1 && !strs2)
            return 1;
        else 
            return 0;
    }

    for (i = 0; strs1[i] && strs2[i]; i++) {
        if (!streq(strs1[i], strs2[i]))
            break;
    }

    if (!strs1[i] && !strs2[i])
        return 1;
    else 
        return 0;
}

void test_split_str(void)
{
    char * str, * delim;
    char * expected_res[10], ** res;

    str = delim = NULL;
    TEST_COND("str=<NULL>, delim=<NULL>", split_str(str, delim) == NULL);

    str = "hi noodles";
    delim = " ";
    expected_res[0] = "hi";
    expected_res[1] = "noodles";
    expected_res[2] = NULL;
    res = split_str(str, delim);
    TEST_COND("str=<hi noodles>, delim=< >", _strarrayeq(res, expected_res));
    free_str_array(res);

    str = "hi noodles";
    delim = NULL;
    expected_res[0] = "hi noodles";
    expected_res[1] = NULL;
    res = split_str(str, delim);
    TEST_COND("str=<hi noodles>, delim=NULL", _strarrayeq(res, expected_res));
    free_str_array(res);

    str = "h i n o o d l e";
    delim = " ";
    expected_res[0] = "h";
    expected_res[1] = "i";
    expected_res[2] = "n";
    expected_res[3] = "o";
    expected_res[4] = "o";
    expected_res[5] = "d";
    expected_res[6] = "l";
    expected_res[7] = "e";
    expected_res[8] = NULL;
    res = split_str(str, delim);
    TEST_COND("str=<h i n o o d l e>, delim=< >", 
            _strarrayeq(res, expected_res));
    free_str_array(res);

    str = "  h i n o o d l e";
    delim = " ";
    res = split_str(str, delim);
    TEST_COND("str=<  h i n o o d l e>, delim=< >", 
            _strarrayeq(res, expected_res));
    free_str_array(res);

    str = "  h i n o o d l e  ";
    delim = " ";
    res = split_str(str, delim);
    TEST_COND("str=<  h i n o o d l e  >, delim=< >", 
            _strarrayeq(res, expected_res));
    free_str_array(res);

    str = "xx h i x nxo o d l e x";
    delim = " x";
    res = split_str(str, delim);
    TEST_COND("str=<xx h i x nxo o d l e x>, delim=< >", 
            _strarrayeq(res, expected_res));
    free_str_array(res);
}

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
    test_split_str();
    TEST_REPORT();
    
    return 0;
}
