#================================================================
#  Copyright (C) 2021 noodles. All rights reserved.
#  
#  文件名称：Makefile
#  创 建 者：noodles
#  邮    箱：peizezhong@163.com
#  创建日期：2021年12月15日
#  描    述：
#  
#================================================================
# specify the default goal, should be ahead of 'include ../makeconfig'
.DEFAULT_GOAL = all

include ../makeconfig

LIBS := -lpthread -lm

DEP_OBJS := $(wildcard ../lib/$(OBJ_DIR)/*.o)
DEP_OBJS += $(wildcard ../3rd/$(OBJ_DIR)/*.o)

all: $(BIN_DIR)/inetd-server

$(BIN_DIR)/inetd-server: $(BUILD_DIR) $(OBJ_FILES) $(DEP_OBJS) 
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $(OBJ_FILES) $(DEP_OBJS) $(LIBS)

install:
	cp $(BIN_DIR)/inetd-server /usr/bin/
	cp ./inetd.sh /usr/bin/
	cp ../etc/inetd-conf.ini /etc/inetd/
