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

S3_TOP       = ../../

SDK_TOP      = $(S3_TOP)../../

PLATFORM 	 = FICS

ifeq ($(PLATFORM), FICS)
INCLUDES 	 = -I../ -I$(S3_TOP)../web/ -I$(SDK_TOP)/code/include_com -I$(S3_TOP)../web/web_include \
			   -I$(S3_TOP)../web/Json/include
endif

ifeq ($(PLATFORM), FISHARE)
INCLUDES 	 = -I../ -I/home/work/work/fishare2.0_MDS_Naturalize/code/windows/JNI
endif

include ../lib.rules

LOCAL_LIBS 	 = ../cgw/libcgw.a #../include/libinclude.a ../common/libcommon.a ../json_spirit/libjson_spirit.a \
			   ../cls/libcls.a #../global/libglobal.a ../include/libinclude.a \
			    ../auth/libauth.a    \
			   ../log/liblog.a ../msg/libmsg.a 

LD = $(SYS_LIBS) -lpthread -lfcgi -lcurl

LD_FICS = -L$(SDK_TOP)/bin/libs -lJson -lFicsJNI -lcomutility -lficsnet -lfiOsd -ljournal \
		  -lprtmds -lfistate -lficsNetSyn -lzookeeper_mt

LD_FISHARE = -L$(SDK_TOP)/bin/libs -lJson -lFicsJNI -lcomutility -lficsnet -lfiOsd -ljournal \
		  -lprtmds -lfistate -lficsNetSyn -lzookeeper_mt

TEST_FCGI 		:= test_fcgi.fcgi

TEST_GET_USERS  := test_getuserinf

CMD_SERVER 		:= cmd_server

CMD_SERVER_FISHARE 		:= cmd_server_fishare

all:ECHO_BUILD $(TEST_FCGI) 

ECHO_BUILD:
	@echo -e "\033[5;41;32m Building $(MODULE_NAME) ... \033[0m"

TEST_FCGI_OBJS 	  		=  test_fcgi.o

TEST_GET_USERS_OBJS     =  test_getuserinf.o

CMD_USERS_OBJS    		=  cmd_server.o

CMD_SERVER_FISHARE_OBJS    		=  cmd_server_fishare.o

SER_MANAGER_OBJS        =  ser_manager.o

$(TEST_FCGI): $(TEST_FCGI_OBJS) 
		$(CXX) $^ -o $@ 

$(TEST_GET_USERS): $(TEST_GET_USERS_OBJS) $(LOCAL_LIBS)
		$(CXX) $^ -o $@ $(LD_FICS)

$(CMD_SERVER): $(CMD_USERS_OBJS) $(LOCAL_LIBS)
		$(CXX) $^ -o $@ $(LD_FICS)

$(CMD_SERVER_FISHARE): $(CMD_SERVER_FISHARE_OBJS) $(LOCAL_LIBS)
		$(CXX) $^ -o $@ $(LD_FICS)

$(SER_MANAGER): $(SER_MANAGER_OBJS)
		$(CXX) $^ -o $@ 

.PHONY:clean

clean:
	-@echo -e "\033[5;41;32m Cleaning $(MODULE_NAME) ... \033[0m"
	-$(RM) *.o $(TEST_FCGI) $(CMD_SERVER) $(TEST_GET_USERS) $(SER_MANAGER)
