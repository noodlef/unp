#================================================================
#  Copyright (C) 2021 noodles. All rights reserved.
#  
#  文件名称：Makefile
#  创 建 者：noodles
#  邮    箱：peizezhong@163.com
#  创建日期：2021年10月08日
#  描    述：
#  
#================================================================

# specify the default goal, should be ahead of 'include ../makeconfig'
.DEFAULT_GOAL = $(BUILD_DIR)/web 

include ../../../makeconfig

LIB_OBJS		:= $(wildcard ../../../lib/$(OBJ_DIR)/*.o)
LIB_OBJS		+= $(wildcard ../../../3rd/$(OBJ_DIR)/*.o)

$(BUILD_DIR)/web: $(BUILD_DIR) $(OBJ_FILES) $(LIB_OBJS)
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $(OBJ_FILES) $(LIB_OBJS) $(LIBS)

