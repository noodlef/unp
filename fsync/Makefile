#================================================================
#  Copyright (C) 2021 noodles. All rights reserved.
#  
#  文件名称：Makefile
#  创 建 者：noodles
#  邮    箱：peizezhong@163.com
#  创建日期：2021年12月16日
#  描    述：
#  
#================================================================
# specify the default goal, should be ahead of 'include ../makeconfig'
.DEFAULT_GOAL = all 

include ../makeconfig

LIBS := -lm

SRC_FILES := $(filter-out main.c fsync.c cli_main.c fsync_cli.c, $(SRC_FILES))
OBJ_FILES := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

DEP_OBJS := $(wildcard ../lib/$(OBJ_DIR)/*.o)
DEP_OBJS += $(wildcard ../3rd/$(OBJ_DIR)/*.o)

all: $(BUILD_DIR) $(BIN_DIR)/fsync-server $(BIN_DIR)/fsync-cli 

$(BIN_DIR)/fsync-cli: $(OBJ_DIR)/cli_main.o $(OBJ_DIR)/fsync_cli.o $(OBJ_FILES) $(DEP_OBJS) 
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS) 

$(BIN_DIR)/fsync-server: $(OBJ_DIR)/main.o $(OBJ_DIR)/fsync.o $(OBJ_FILES) $(DEP_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS) 

install:
	cp	$(BIN_DIR)/fsync-server /usr/bin/
	cp	$(BIN_DIR)/fsync-cli /usr/bin/
