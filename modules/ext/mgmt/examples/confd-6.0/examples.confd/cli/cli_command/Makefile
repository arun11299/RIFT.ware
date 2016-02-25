######################################################################
# SNMP Agent example "simple" Makefile
# (C) 2006 Tail-f Systems
#
# See the README files for more information
######################################################################

usage:
	@echo "See README file for more instructions"
	@echo "make all      Build all example files"
	@echo "make clean    Remove all built and intermediary files"
	@echo "make start    Start ConfD daemon and example agent"
	@echo "make stop     Stop any ConfD daemon and example agent"
	@echo "make cli      Start the ConfD Command Line Interface)"
	@echo "make cli-c    Start the ConfD Command Line Interface, C-style"
	@echo "make cli-j    Start the ConfD Command Line Interface, J-style)"

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

START_FLAGS ?=

CONFD_FXS_DIR = $(CONFD_DIR)/etc/confd
CONFD_SNMP_FXS_DIR = $(CONFD_DIR)/etc/confd/snmp

CONFD_FLAGS = --addloadpath $(CONFD_FXS_DIR) \
	      --addloadpath $(CONFD_SNMP_FXS_DIR)

CONFD_FLAGS ?=
START_FLAGS ?=

all:	simple.fxs TAIL-F-TEST-MIB.bin show_snmp add_user \
	$(CDB_DIR) $(CDB_DIR)/vacm_init.xml confd.c.ccl confd.j.ccl confd.j2.ccl
	@echo "Build complete"

TAIL-F-TEST-MIB.bin: simple.fxs TAIL-F-TEST-MIB.mib TAIL-F-TEST-MIB.miba
	$(CONFDC) -c TAIL-F-TEST-MIB.mib -f $(CONFD_FXS_DIR) \
          --mib-annotation TAIL-F-TEST-MIB.miba simple.fxs

$(CDB_DIR)/vacm_init.xml: $(CDB_DIR)
	-mkdir -p $(CDB_DIR)
	cp *_init.xml $(CDB_DIR)

add_user: add_user.o
	$(CC) -o $@ add_user.o  $(LIBS)

show_snmp.o: show_snmp.c SNMPv2-MIB.h SNMP-FRAMEWORK-MIB.h

show_snmp: show_snmp.o 
	$(CC) -o $@ show_snmp.o $(LIBS)

confd.c.ccl: confd.c.cli
	${CONFDC} -c confd.c.cli

confd.j.ccl: confd.j.cli
	${CONFDC} -c confd.j.cli

confd.j2.ccl: confd.j2.cli
	${CONFDC} -c confd.j2.cli

SNMPv2-MIB.h: $(CONFD_DIR)/etc/confd/snmp/SNMPv2-MIB.fxs
	$(CONFDC) --emit-h $@ $<

SNMP-FRAMEWORK-MIB.h: $(CONFD_DIR)/etc/confd/snmp/SNMP-FRAMEWORK-MIB.fxs
	$(CONFDC) --emit-h $@ $<

######################################################################
clean:	iclean
	-rm -rf *.bin snmpa.state add_user show_snmp confd.c.ccl \
		confd.j.ccl confd.j2.ccl SNMPv2-MIB.h SNMP-FRAMEWORK-MIB.h \
		2>/dev/null || true

start:  stop 
	$(CONFD) -c ./confd.conf $(CONFD_FLAGS)

######################################################################
stop:
	### Killing any confd daemon
	$(CONFD) --stop    || true

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
