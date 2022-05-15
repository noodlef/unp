#================================================================
#  Copyright (C) 2021 noodles. All rights reserved.
#  
#  文件名称：Makefile
#  创 建 者：noodles
#  邮    箱：peizezhong@163.com
#  创建日期：2021年11月06日
#  描    述：
#  
#================================================================

# specify the default goal, should be ahead of 'include ../makeconfig'
.DEFAULT_GOAL = lib 

include ../../makeconfig

LIBS 			:= -lpthread -lm

LIB_OBJS		:= $(wildcard ./$(OBJ_DIR)/*.o)
LIB_OBJS		+= $(wildcard ../../3rd/$(OBJ_DIR)/*.o)
LIB_OBJS		+= $(wildcard ../$(OBJ_DIR)/*.o)

lib: $(BUILD_DIR) $(OBJ_FILES)

tests: $(LIB_OBJS) 
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS) 
