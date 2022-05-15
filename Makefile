#================================================================
#  Copyright (C) 2021 noodles. All rights reserved.
#  
#  文件名称：Makefile
#  创 建 者：noodles
#  邮    箱：peizezhong@163.com
#  创建日期：2021年09月12日
#  描    述：
#  
#================================================================
SUB_DIRS = `find ./src -maxdepth 1 -type d | awk '{if (NR > 1) print}'`
MAKE_PRACTICE	:= ../Makefile

target:	
	@make -C lib
	@make -C 3rd 
	@for dir in $(SUB_DIRS); do \
		make -C $$dir -f $(MAKE_PRACTICE); \
    done
	@make -C src/practice-16/web
	@make -C inetd
	@make -C fsync

install:
	@make -C inetd install
	@make -C fsync install

.PHONY: clean
clean:
	@make -C lib clean
	@make -C 3rd clean
	@for dir in $(SUB_DIRS); do \
		make -C $$dir -f $(MAKE_PRACTICE) clean; \
    done
	@make -C src/practice-16/web clean
	@make -C inetd clean
	@make -C fsync clean
