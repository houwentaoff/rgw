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

INCLUDES 	=

include ../lib.rules

LOCAL_LIBS 	=

LD = $(SYS_LIBS) -pthread 

all:
	@echo -e "\033[5;41;32m Building rgw ... \033[0m"

rgw: $(LOCAL_LIBS)
		@$(CXX) $^ -o $@ $(LD)

.PHONY:clean

clean:
	-@echo -e "\033[5;41;32m Cleaning $(MODULE_NAME) ... \033[0m"
	-rm *.o $(MODULE_TARGETS) $(LIB_OBJS)
