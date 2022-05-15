/**
 *===============================================================
 * Copyright (C) 2022 noodles. All rights reserved.
 * 
 * 文件名称：test_dict.c
 * 创 建 者：noodles
 * 邮    箱：peizezhong@163.com
 * 创建日期：2022年04月03日
 * 描    述：
 * 
 *===============================================================
 */
#include "../lib/dict.h"
#include "../lib/test.h"

void dict_for_each(struct dict * dict)
{
    int count;
    char * key, * val, * idx;
    struct dict_entry * entry;

    key = "key";
    val = "val";
    idx = "idx";
    printf("  %-32s%-32s%-32s\n", idx, key, val);

    count = 0;
    for (entry = dict_first(dict); entry; 
            entry = dict_next(entry)) {
        printf("  %-32d%-32s%-32s\n", ++count, 
                (char *)dict_fetch_key(entry), (char *)dict_fetch_val(entry));
    }
}

int main(int argc, char ** argv)
{
    int i;
    char * val;
    struct dict * dict;

    dict = dict_create(&hstr_2_hstr_map);
    TEST_COND("Test create", dict != NULL);

    i = dict_find(dict, "noodles", (void **)&val);
    TEST_COND("Test find noexistent key", i == -1);

    dict_delete(dict, "noodles");
    TEST_COND("Test delete noexistent key", 1);

    i = dict_add(dict, "noodles", "66666666");
    TEST_COND("Test add new key", i == 0);

    i = dict_add(dict, "noodles", "66666666");
    TEST_COND("Test add existent key", i == -1);

    i = dict_add(dict, "null", NULL); 
    TEST_COND("Test add new key: val == NULL", i == 0);
    
    i = dict_update(dict, "noodles", "777777");
    TEST_COND("Test update existent key", i == 0);

    i = dict_update(dict, "null", "noodles"); 
    TEST_COND("Test update key: val == NULL", i == 0);

    i = dict_update(dict, "root", "88888");
    TEST_COND("Test update noexistent key", i == 0);

    dict_for_each(dict);

    dict_destroy(dict);
    TEST_COND("Test delete", 1);

    TEST_REPORT();

    return 0;
}
