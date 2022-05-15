#================================================================
#  Copyright (C) 2022 noodles. All rights reserved.
#  
#  文件名称：Makefile
#  创 建 者：noodles
#  邮    箱：peizezhong@163.com
#  创建日期：2022年03月17日
#  描    述：
#  
#================================================================

# specify the default goal, should be ahead of 'include ../makeconfig'
.DEFAULT_GOAL = tests

include ../makeconfig

LIB_OBJS		:= $(wildcard ../lib/$(OBJ_DIR)/*.o)
LIB_OBJS		+= $(wildcard ../lib/eventloop/$(OBJ_DIR)/*.o)
LIB_OBJS		+= $(wildcard ../3rd/$(OBJ_DIR)/*.o)
TEST_EXES		:= $(patsubst %.c, $(BIN_DIR)/%, $(SRC_FILES))

tests: $(BUILD_DIR) $(TEST_EXES) 

$(TEST_EXES): $(BIN_DIR)/%: $(OBJ_DIR)/%.o $(LIB_OBJS) 
	$(CC) $(CFLAGS) $(INCLUDE) -o $@ $^ $(LIBS) 
