######################################################################
# SNMP Agent example "simple" Makefile
# (C) 2006-2009 Tail-f Systems
#
# See the README files for more information
######################################################################

usage:
	@echo "See README file for more instructions"
	@echo "make all      Build all example files"
	@echo "make clean    Remove all built and intermediary files"
	@echo "make start    Start ConfD daemon and example agent"

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

all:			snmpaudit
	@echo "Build complete"

snmpaudit:	snmpaudit.o
	$(CC) -o $@ $< $(LIBS) 

start:
	./snmpaudit
stop:
	$(KILLALL) snmpaudit 2> /dev/null || true

######################################################################
clean:	
	-rm -rf *.o snmpaudit 2>/dev/null || true

