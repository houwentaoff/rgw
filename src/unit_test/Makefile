###############################################
#
# code/s3/src/unit_test
#
# 2015/12/17 - [Sean Hou] created file
#
# 
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Sobey, Inc.
# 
# ###############################################

default:all

MODULE_NAME := unit_test

INCLUDES 	 = -I../

include ../lib.rules

#LOCAL_LIBS 	 = ../include/libinclude.a ../common/libcommon.a ../json_spirit/libjson_spirit.a \
			   ../cls/libcls.a #../global/libglobal.a ../include/libinclude.a \
			    ../auth/libauth.a    \
			   ../log/liblog.a ../msg/libmsg.a 

LD = $(SYS_LIBS) -pthread -lfcgi -lcurl

TEST_FCGI 	:= test_fcgi.fcgi

all:ECHO_BUILD $(TEST_FCGI) 

ECHO_BUILD:
	@echo -e "\033[5;41;32m Building $(MODULE_NAME) ... \033[0m"

TEST_FCGI_OBJS 	  =  test_fcgi.o

$(TEST_FCGI): $(TEST_FCGI_OBJS) 
		$(CXX) $^ -o $@ $(LD)

.PHONY:clean

clean:
	-@echo -e "\033[5;41;32m Cleaning $(MODULE_NAME) ... \033[0m"
	-$(RM) *.o  $(TEST_FCGI_OBJS) $(TEST_FCGI)
