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

MODULE_NAME 	:=  include

MODULE_LIBS 	 =  lib$(MODULE_NAME).a

INCLUDES 		 =  -I../ 

include ../lib.rules

LIB_OBJS = 

MODULE_TARGETS = $(MODULE_LIBS)

all:$(MODULE_TARGETS)
#	@echo -e "\033[5;41;32m Building $(MODULE_NAME) ... \033[0m"

$(MODULE_LIBS): $(LIB_OBJS)	
	@$(AR) rucs $@ $^

.PHONY:clean

clean:
	-@echo -e "\033[5;41;32m Cleaning $(MODULE_NAME) ... \033[0m"
	-$(RM) *.o $(MODULE_TARGETS) $(LIB_OBJS) 

