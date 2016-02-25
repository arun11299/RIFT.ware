######################################################################
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

CONFD_FXS_DIR = $(CONFD_DIR)/etc/confd

CONFD_FLAGS = --addloadpath $(CONFD_FXS_DIR) 

CONFD_FLAGS ?=
START_FLAGS ?=

OSTYPE	= $(shell uname -s)
CFLAGS	+= -D$(OSTYPE)

all:	completion.fxs config.fxs config.h callbacks $(CDB_DIR) ssh-keydir \
		confd.ccl
	@echo "Build complete"

callbacks: callbacks.o
	$(CC) -o $@ $< $(LIBS)

######################################################################
clean:	iclean
	-rm -rf config.h confd.ccl callbacks 2> /dev/null || true

start:  stop
	$(CONFD) -c confd.conf $(CONFD_FLAGS) 
	### * In one terminal window, run: tail -f ./confd.log
	### * Start a CLI in another terminal window, run make cli
	./callbacks $(START_FLAGS)

######################################################################
stop:
	### Killing any confd daemon and ARP confd agents
	$(CONFD) --stop || true
	killall callbacks || true

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
