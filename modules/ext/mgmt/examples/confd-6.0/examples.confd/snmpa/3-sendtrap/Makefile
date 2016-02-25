######################################################################
# SNMP Agent example "sendtrap" Makefile
# (C) 2006-2009 Tail-f Systems
#
# See the README file for more information
######################################################################

usage:
	@echo "See README file for more instructions"
	@echo "make all      Build all example files"
	@echo "make clean    Remove all built and intermediary files"
	@echo "make start    Start ConfD daemon and example agent"
	@echo "make stop     Stop any ConfD daemon and example agent"
	@echo "make cli      Start the ConfD Command Line Interface"

######################################################################
# Where is ConfD installed? Make sure CONFD_DIR points it out
CONFD_DIR ?= ../../..

# Include standard ConfD build definitions and rules
include $(CONFD_DIR)/src/confd/build/include.mk

# In case CONFD_DIR is not set (correctly), this rule will trigger
$(CONFD_DIR)/src/confd/build/include.mk:
	@echo 'Where is ConfD installed? Set $$CONFD_DIR to point it out!'
	@echo ''

# Include common SNMP rules
include ../Makefile.incl

######################################################################
# Example specific definitions and rules

START_FLAGS ?=

OSTYPE    = $(shell uname -s)
SRC	  = sendtrap.c
OBJS	  = $(SRC:.c=.o) 
CFLAGS	  += -Wall -D$(OSTYPE)

CONFD_FXS_DIR = $(CONFD_DIR)/etc/confd
CONFD_SNMP_FXS_DIR = $(CONFD_DIR)/etc/confd/snmp

CONFD_FLAGS = --addloadpath $(CONFD_FXS_DIR) \
	      --addloadpath $(CONFD_SNMP_FXS_DIR)

SBIN_DIRS = /usr/sbin:/usr/local/sbin:/usr/pkg/sbin:/opt/csw/sbin

######################################################################
all:	SIMPLE-MIB.fxs SIMPLE-MIB.bin $(CDB_DIR) $(CDB_DIR)/vacm_init.xml \
	sendtrap
	@echo "Build complete"

sendtrap: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

%.yang:	%.mib
	$(CONFDC) --mib2yang -o $@ $<

%.fxs: %.yang
	$(CONFDC) $(EXTRA_LINK_FLAGS) -c -o $@ $<

SIMPLE-MIB.bin: SIMPLE-MIB.fxs SIMPLE-MIB.mib
	$(CONFDC) -c  \
	-f . -f $(CONFD_FXS_DIR) -f $(CONFD_SNMP_FXS_DIR) -- \
	SIMPLE-MIB.mib SIMPLE-MIB.fxs

$(CDB_DIR)/vacm_init.xml:
	-mkdir -p $(CDB_DIR)
	cp *_init.xml $(CDB_DIR)

######################################################################
clean:	iclean
	-rm -rf *.yang *.bin global.data *.d *.o sendtrap 2>/dev/null || true

start:  stop 
	$(CONFD) -c ./confd.conf $(CONFD_FLAGS)

start_snmptrapd:
	PATH=$(SBIN_DIRS):$$PATH snmptrapd -Lo -m ./SIMPLE-MIB.mib \
	     -c snmptrapd.conf localhost:5000 localhost:6000 localhost:7000

######################################################################
stop:
	$(CONFD) --stop || true
	$(KILLALL) snmptrapd || true

######################################################################
cli:
	$(CONFD_DIR)/bin/confd_cli --user=admin --groups=admin \
		--interactive || echo Exit

cli-c:
	$(CONFD_DIR)/bin/confd_cli -C --user=admin --groups=admin \
		--interactive || echo Exit
######################################################################
