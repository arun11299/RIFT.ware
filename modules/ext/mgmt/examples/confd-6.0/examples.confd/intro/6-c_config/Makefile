######################################################################
# Introduction example 6-c_config
# (C) 2006 Tail-f Systems
#
# See the README file for more information
######################################################################

usage:
	@echo "See README file for more instructions"
	@echo "make all     Build all example files"
	@echo "make py-all     Build all example files for Python"
	@echo "make clean   Remove all built and intermediary files"
	@echo "make start   Start CONFD daemon and example agent"
	@echo "make py-start   Start CONFD daemon and python example agent"
	@echo "make stop    Stop any CONFD daemon and example agent"
	@echo "make query   Run query against CONFD"
	@echo "make cli     Start the CONFD Command Line Interface, J-style"
	@echo "make cli-c   Start the CONFD Command Line Interface, C-style"



######################################################################
# Where is ConfD installed? Make sure CONFD_DIR points it out
CONFD_DIR ?= ../../..

# Include standard ConfD build definitions and rules
include $(CONFD_DIR)/src/confd/build/include.mk

# In case CONFD_DIR is not set (correctly), this rule will trigger
$(CONFD_DIR)/src/confd/build/include.mk:
	@echo 'Where is ConfD installed? Set $$CONFD_DIR to point it out!'
	@echo ''

######################################################################
# Example specific definitions and rules

CONFD_FLAGS = --addloadpath $(CONFD_DIR)/etc/confd 
START_FLAGS ?=

######################################################################
# Example specific definitions and rules

CONFD_FLAGS ?=
START_FLAGS ?=

SRC	= hosts.c dlist.c
OBJS	= $(SRC:.c=.o)

all:	hst.fxs hst.h hosts $(CDB_DIR) ssh-keydir
	@echo "Build complete"

py-all:	hst.fxs hst_ns.py $(CDB_DIR) ssh-keydir
	@echo "Build complete"

hosts: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)


######################################################################
clean:	iclean
	-rm -rf hst.h hst_ns.py *.pyc hosts RUNNING.ckp 2> /dev/null || true

start:  stop 
	$(CONFD)  -c confd.conf $(CONFD_FLAGS)
	### * In one terminal window, run: tail -f ./confd.log
	### * In another terminal window, run queries
	###   (try 'make query' for an example)
	### * In this window, the HOSTS confd daemon now starts:
	./hosts $(START_FLAGS)

py-start:  stop
	$(CONFD)  -c confd.conf $(CONFD_FLAGS)
	### * In one terminal window, run: tail -f ./confd.log
	### * In another terminal window, run queries
	###   (try 'make query' for an example)
	### * In this window, the HOSTS confd daemon now starts:
	python hosts.py --debuglevel t

# Like py_start, but with log-files
py-log-start:.
	$(CONFD)  -c ../../meta_common/confd.conf.confdconf $(CONFD_FLAGS)
	### * In one terminal window, run: tail -f ./confd.log
	### * In another terminal window, run queries
	###   (try 'make query' for an example)
	### * In this window, the HOSTS confd daemon now starts:
	python hosts.py --debuglevel t

######################################################################
stop:
	### Killing any confd daemon and HOSTS confd agents
	$(CONFD) --stop || true
	$(KILLALL) hosts || true

######################################################################
cli:
	$(CONFD_DIR)/bin/confd_cli --user=admin --groups=admin \
		--interactive || echo Exit

cli-c:
	$(CONFD_DIR)/bin/confd_cli -C --user=admin --groups=admin \
		--interactive || echo Exit

######################################################################
query:
	$(CONFD_DIR)/bin/netconf-console-tcp cmd-get-buzz.xml

######################################################################
