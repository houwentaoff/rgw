###############################################
#
# code/s3/src/common
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

MODULE_NAME 	:= common

MODULE_LIBS 	 =  lib$(MODULE_NAME).a

INCLUDES 		 =  -I../ 

include ../lib.rules

BASE_OBJS = signal.o io_priority.o simple_spin.o buffer.o safe_io.o str_list.o ceph_json.o mime.o utf8.o armor.o WorkQueue.o  Thread.o page.o Mutex.o  Formatter.o Clock.o lockdep.o environment.o code_environment.o PrebufferedStreambuf.o strtol.o dout.o BackTrace.o # WorkQueue.o Thread.o Clock.o# entity_name.o ceph_context.o ceph_crypto.o admin_socket.o \
		   signal.o Thread.o code_environment.o admin_socket_client.o errno.o safe_io.o armor.o Clock.o  \
		   

LIB_OBJS  =
LIB_OBJS += $(BASE_OBJS)

MODULE_TARGETS = $(MODULE_LIBS)

all:$(MODULE_TARGETS)
#	@echo -e "\033[5;41;32m Building $(MODULE_NAME) ... \033[0m"

$(MODULE_LIBS): $(LIB_OBJS)	
	@$(AR) rucs $@ $^

.PHONY:clean

clean:
	-@echo -e "\033[5;41;32m Cleaning $(MODULE_NAME) ... \033[0m"
	-$(RM) *.o $(MODULE_TARGETS) $(LIB_OBJS) 

