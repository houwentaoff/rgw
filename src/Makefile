###############################################
#
# code/s3/src
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
SUBDIRS=common global rgw
MODULE_NAME  	:= s3
#MAKE=make

default:all

include lib.rules

.PHONY: clean
all:
	-@$(ECHO) -e "\033[5;41;32m  Building $(MODULE_NAME) ...   \033[0m"
	for d in $(SUBDIRS); do [ -d $$d ] && $(MAKE) -C $$d; done

clean:
	-@$(ECHO) -e "\033[5;41;32m  Cleaning $(MODULE_NAME) ...   \033[0m"
	for d in $(SUBDIRS); do [ -d $$d ] && $(MAKE) -C $$d clean; done
	rm -f *~

install:
	for d in $(SUBDIRS); do [ -d $$d ] && $(MAKE) -C $$d install; done
