#================================================================
#  Copyright (C) 2022 noodles. All rights reserved.
#  
#  文件名称：Makefile
#  创 建 者：noodles
#  邮    箱：peizezhong@163.com
#  创建日期：2022年04月20日
#  描    述：
#  
#================================================================
# specify the default goal, should be ahead of 'include ../makeconfig'
.DEFAULT_GOAL = all 

include ../makeconfig

LIBS := -lm

SRC_FILES := $(filter-out tftp_server.c tftp_client.c, $(SRC_FILES))
OBJ_FILES := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

DEP_OBJS := $(wildcard ../lib/$(OBJ_DIR)/*.o ../lib/eventloop/$(OBJ_DIR)/*.o)
DEP_OBJS += $(wildcard ../3rd/$(OBJ_DIR)/*.o)

all: $(BUILD_DIR) $(BIN_DIR)/tftp-server $(BIN_DIR)/tftp-cli 

$(BIN_DIR)/tftp-cli: $(OBJ_DIR)/tftp_client.o $(OBJ_FILES) $(DEP_OBJS) 
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS) 

$(BIN_DIR)/tftp-server: $(OBJ_DIR)/tftp_server.o $(OBJ_FILES) $(DEP_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS) 

install:
	cp	$(BIN_DIR)/tftp-server /usr/bin/
	cp	$(BIN_DIR)/tftp-cli /usr/bin/

