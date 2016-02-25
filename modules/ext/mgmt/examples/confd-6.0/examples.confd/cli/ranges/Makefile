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

CONFD_FLAGS = --addloadpath $(CONFD_DIR)/etc/confd 
START_FLAGS ?=

OSTYPE	= $(shell uname -s)
SRC	= actions.c
OBJS	= $(SRC:.c=.o) 
CFLAGS	+= -D$(OSTYPE)

all:	range.fxs  \
        $(CDB_DIR) \
        $(CDB_DIR)/range.xml \
	ssh-keydir \
	confd.c.ccl \
	confd.j.ccl \
	actions
	@echo "Build complete"

actions: range.h $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

$(CDB_DIR)/range.xml :$(CDB_DIR)
	-cp range.xml $(CDB_DIR)

######################################################################
clean:	iclean
	-rm -rf config.h range.h actions 2> /dev/null || true

start:  stop
	$(CONFD) -c confd.conf $(CONFD_FLAGS) 
	./actions $(START_FLAGS)


######################################################################
stop:
	$(CONFD) --stop || true

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

