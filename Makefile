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
	@make -C 3rd 
	@make -C lib
	@make -C lib/eventloop 
	@make -C lib/coroutine
	@make -C lib/pthread
	@for dir in $(SUB_DIRS); do \
		make -C $$dir -f $(MAKE_PRACTICE); \
    done
	@make -C inetd
	@make -C fsync
	@make -C tftp
	@make -C ping 
	@make -C ping 

.PHONY: tests 
tests:
	@make -C tests

.PHONY: install 
install:
	@make -C inetd install
	@make -C fsync install

.PHONY: clean
clean:
	@make -C tests clean 
	@make -C 3rd clean
	@make -C lib clean
	@make -C lib/eventloop clean
	@make -C lib/coroutine clean
	@make -C lib/pthread clean
	@for dir in $(SUB_DIRS); do \
		make -C $$dir -f $(MAKE_PRACTICE) clean; \
    done
	@make -C inetd clean
	@make -C fsync clean
	@make -C tftp clean
	@make -C ping clean 
	@make -C ping clean 

