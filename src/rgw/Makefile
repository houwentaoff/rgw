###############################################
#
# code/s3/src/rgw
#
# 2015/12/07 - [Sean Hou] created file
#
# 
#
# All rights reserved. No Part of this file may be reproduced, stored
# in a retrieval system, or transmitted, in any form, or by any means,
# electronic, mechanical, photocopying, recording, or otherwise,
# without the prior consent of Sobey, Inc.
# 
# ###############################################


default:test_main

MODULE_NAME := rgw

INCLUDES 	 = -I../

include ../lib.rules

LOCAL_LIBS 	 = ../cgw/libcgw.a ../auth/libauth.a \
			   ../librados/librados.a ../include/libinclude.a ../common/libcommon.a \
			   ../json_spirit/libjson_spirit.a ../cls/libcls.a ../global/libglobal.a #../include/libinclude.a \
			    ../auth/libauth.a    \
			   ../log/liblog.a ../msg/libmsg.a 

LD = $(SYS_LIBS) -lcryptopp -lpthread -lfcgi -lcurl -lexpat -lboost_thread -lboost_filesystem  -lboost_system -static-libstdc++
RGW_SERVER  := rgw
TEST_RGW 	:= test_main

all:$(RGW_SERVER) 
	@echo -e "\033[5;41;32m Building $(MODULE_NAME) ... \033[0m"


RGW_OBJS  	  =  rgw_env.o rgw_fcgi.o rgw_client_io.o rgw_common.o \
			     rgw_op.o rgw_rest.o  rgw_rest_s3.o rgw_main.o 
TEST_OBJS 	  = rgw_multi.o \
				porting_quota.o rgw_cache.o porting_metadata.o porting_user.o\
				rgw_auth_s3.o porting_tools.o porting_rados.o porting_json_enc.o \
				porting_bucket.o porting_acl_s3.o porting_xml.o porting_acl.o \
				test_main.o porting_env.o rgw_client_io.o rgw_fcgi.o \
                porting_rest.o porting_rest_bucket.o\
				porting_rest_s3.o porting_op.o porting_common.o
				  #rgw_common.o #rgw_op.o

$(RGW_SERVER): $(RGW_OBJS) $(LOCAL_LIBS)
		@$(CXX) $^ -o $@ $(LD)

$(TEST_RGW): $(TEST_OBJS) $(LOCAL_LIBS) 
		$(CXX) $^ -o $@ $(LD)

.PHONY:clean

clean:
	-@echo -e "\033[5;41;32m Cleaning $(MODULE_NAME) ... \033[0m"
	-$(RM) *.o $(MODULE_TARGETS) $(LIB_OBJS) $(TEST_OBJS) $(TEST_RGW)
