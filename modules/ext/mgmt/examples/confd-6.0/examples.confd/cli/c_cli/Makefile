######################################################################
# Introduction example 8-c_actions
# (C) 2006 Tail-f Systems
#
# See the README file for more information
######################################################################

usage:
	@echo "See README file for more instructions"
	@echo "make all     Build all example files"
	@echo "make clean   Remove all built and intermediary files"
	@echo "make start   Start CONFD daemon and example agent"
	@echo "make stop    Stop any CONFD daemon and example agent"
	@echo "make cli     Start the CONFD Command Line Interface"
	@echo "make cli-c   Start the CONFD Command Line Interface, C-style"
	@echo "make cli-j   Start the CONFD Command Line Interface, J-style"

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

OSTYPE	= $(shell uname -s)
CFLAGS	+= -D$(OSTYPE)

all:	data.fxs data.h actions callpoints $(CDB_DIR) ssh-keydir \
		confd.ccl confd.cisco.ccl
	@echo "Build complete"

actions: actions.o
	$(CC) -o $@ $< $(LIBS)

callpoints: callpoints.o
	$(CC) -o $@ $< $(LIBS)

######################################################################
clean:	iclean
	-rm -rf callpoints data.h confd.ccl actions 2> /dev/null || true

start:  stop
	$(CONFD) -c confd.conf $(CONFD_FLAGS) 
	### * In one terminal window, run: tail -f ./confd.log
	### * In another terminal window, run make cisco_cli and follow the
	### * instriuctions in the README file.
	./actions $(START_FLAGS) &
	./callpoints $(START_FLAGS)

######################################################################
stop:
	### Killing any confd daemon and ARP confd agents
	$(CONFD) --stop || true
	killall actions || true

######################################################################
cli:
	$(CONFD_DIR)/bin/confd_cli --user=admin --groups=admin \
		--interactive || echo Exit

cli-c:
	$(CONFD_DIR)/bin/confd_cli -C --user=admin --groups=admin \
		--interactive || echo Exit

cli-j:
	$(CONFD_DIR)/bin/confd_cli -J --user=admin --groups=admin \
		--interactive || echo Exit

######################################################################
