######################################################################
# REST API example
# (C) 2012 Tail-f Systems
#
# See the README files for more information
######################################################################

usage:
	@echo "See README files for more instructions"
	@echo "make all     Build all example files"
	@echo "make clean   Remove all built and intermediary files"
	@echo "make start   Start CONFD daemon"
	@echo "make stop    Stop any CONFD daemon"
	@echo "make cli     Start the CONFD Command Line Interface, J-style"
	@echo "make cli-c   Start the CONFD Command Line Interface, C-style"

######################################################################
# Where is ConfD installed? Make sure CONFD_DIR points it out
CONFD_DIR ?= ../..

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

ACTIONS_SRC = actions.c
ACTIONS_OBJS = $(ACTIONS_SRC:.c=.o) 

all:	dhcp.fxs $(CDB_DIR) ssh-keydir $(CDB_DIR)/dhcp_init.xml actions
	@echo "Build complete"

%.fxs:  %.yang
	a=$*-ann.yang; \
	$(CONFDC) -c -o $@ `[ -e $$a ] && echo "-a $$a"` -- $<

actions: dhcp.h $(ACTIONS_OBJS)
	$(CC) -o $@ $(ACTIONS_OBJS) $(LIBS)

######################################################################
clean:	iclean
	@rm -rf *.access saved-subnet patch-subnet actions *.h

######################################################################
start:  stop start_confd
	$(CONFD_DIR)/bin/confd_load -C dhcp_oper.xml
	./actions &

starti:  stop
	(sleep 3; $(CONFD_DIR)/bin/confd_load -C dhcp_oper.xml;	./actions &) &
	$(CONFD) -i -c confd.conf $(CONFD_FLAGS)


start_confd:
	$(CONFD) -c confd.conf $(CONFD_FLAGS)


######################################################################
stop:
	@ # Kill the confd daemon
	$(CONFD) --stop || true
	$(KILLALL) actions || true

######################################################################
cli:
	$(CONFD_DIR)/bin/confd_cli --user=admin --groups=admin \
		--interactive || echo Exit

cli-c:
	$(CONFD_DIR)/bin/confd_cli -C --user=admin --groups=admin \
		--interactive  || echo Exit

######################################################################
$(CDB_DIR)/%.xml: %.xml
	rm -f $@ ; cp $< $@
######################################################################
