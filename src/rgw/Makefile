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

default:all

MODULE_NAME := rgw

INCLUDES 	 = -I../

include ../lib.rules

LOCAL_LIBS 	 = ../common/libcommon.a ../global/libglobal.a ../include/libinclude.a \
			   ../cls/libcls.a ../auth/libauth.a ../json_spirit/libjson_spirit.a   \
			   ../log/liblog.a ../msg/libmsg.a 

LD = $(SYS_LIBS) -pthread -lfcgi 

RGW_SERVER  := rgw

all:$(RGW_SERVER) 
	@echo -e "\033[5;41;32m Building $(MODULE_NAME) ... \033[0m"


RGW_OBJS  	  =  rgw_env.o rgw_fcgi.o rgw_client_io.o rgw_common.o \
			     rgw_op.o rgw_rest.o  rgw_rest_s3.o rgw_main.o 

$(RGW_SERVER): $(LOCAL_LIBS) $(RGW_OBJS)
		@$(CXX) $^ -o $@ $(LD)

.PHONY:clean

clean:
	-@echo -e "\033[5;41;32m Cleaning $(MODULE_NAME) ... \033[0m"
	-$(RM) *.o $(MODULE_TARGETS) $(LIB_OBJS)
