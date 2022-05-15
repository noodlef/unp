#================================================================
#  Copyright (C) 2022 noodles. All rights reserved.
#  
#  文件名称：Makefile
#  创 建 者：noodles
#  邮    箱：peizezhong@163.com
#  创建日期：2022年03月02日
#  描    述：
#  
#================================================================
# specify the default goal, should be ahead of 'include ../makeconfig'
.DEFAULT_GOAL = ping 

include ../makeconfig

LIBS := -lpthread -lm

DEP_OBJS := $(wildcard ../lib/$(OBJ_DIR)/*.o)
DEP_OBJS += $(wildcard ../3rd/$(OBJ_DIR)/*.o)

ping: $(BUILD_DIR) $(OBJ_FILES) $(DEP_OBJS) 
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $(OBJ_FILES) $(DEP_OBJS) $(LIBS)

