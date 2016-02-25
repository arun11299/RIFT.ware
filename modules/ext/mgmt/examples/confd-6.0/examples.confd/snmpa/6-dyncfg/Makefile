######################################################################
# SNMP Agent example "dyncfg" Makefile
# (C) 2006-2009 Tail-f Systems
#
# See the README files for more information
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

# In case CONFD_DIR is not set (correctly), this rule will trigger
$(CONFD_DIR)/src/confd/build/include.mk:
	@echo 'Where is ConfD installed? Set $$CONFD_DIR to point it out!'
	@echo ''

# Include common SNMP rules
include ../Makefile.incl

######################################################################
# Example specific definitions and rules

CONFD_FXS_DIR = $(CONFD_DIR)/etc/confd
CONFD_SNMP_FXS_DIR = $(CONFD_DIR)/etc/confd/snmp

CONFD_FLAGS = --addloadpath $(CONFD_FXS_DIR)

SNMP_SRC_DIR = $(CONFD_DIR)/src/confd/snmp

MIB	= SNMPv2-TC.mib \
	SNMPv2-SMI.mib \
	SNMPv2-MIB.mib \
	SNMP-FRAMEWORK-MIB.mib \
	SNMP-TARGET-MIB.mib \
	SNMP-MPD-MIB.mib \
	SNMP-NOTIFICATION-MIB.mib \
	SNMP-USER-BASED-SM-MIB.mib \
	SNMP-COMMUNITY-MIB.mib \
	SNMP-VIEW-BASED-ACM-MIB.mib

MIBFXS     = $(MIB:%.mib=%.fxs)
CDB_DIR		 = ./confd-cdb

######################################################################

all: simple.fxs system.fxs TAIL-F-TEST-MIB.bin \
	confd_dyncfg.fxs $(MIBFXS) \
	$(CDB_DIR) $(CDB_DIR)/vacm_init.xml ssh-keydir
	@echo "Build complete"

TAIL-F-TEST-MIB.bin: simple.fxs TAIL-F-TEST-MIB.mib
	$(CONFDC) -c -f $(CONFD_FXS_DIR) -- TAIL-F-TEST-MIB.mib simple.fxs 

$(CDB_DIR)/vacm_init.xml:
	-mkdir -p $(CDB_DIR)
	cp *_init.xml $(CDB_DIR)

confd_dyncfg.fxs: $(CONFD_DIR)/src/confd/dyncfg/confd_dyncfg.fxs
	cp $< $@

system.fxs: system.yang confd_dyncfg.fxs SNMP-COMMUNITY-MIB.fxs
	$(CONFDC) -c -o $@ \
	--yangpath $(SNMP_SRC_DIR)/yang \
	--yangpath $(CONFD_DIR)/src/confd/dyncfg \
	-f confd_dyncfg.fxs SNMP-COMMUNITY-MIB.fxs -- $<

simple.fxs: simple.yang 
	$(CONFDC) -c $< -o $@

######################################################################

%.fxs:	$(SNMP_SRC_DIR)/yang/%.yang
	a=$(SNMP_SRC_DIR)/yang/$*-ann.yang; \
	$(CONFDC) -c -o $@ `[ -e $$a ] && echo "-a $$a"` \
	--export snmp \
	--yangpath $(SNMP_SRC_DIR)/yang \
	-f . $(CONFD_FXS_DIR) -- $<

# Include standard ConfD build definitions and rules
include $(CONFD_DIR)/src/confd/build/include.mk

######################################################################
clean:	iclean
	-rm -rf *.bin SNMP*.yang *.d global.data 2>/dev/null || true

start:  stop 
	$(CONFD) -c ./confd.conf $(CONFD_FLAGS)

######################################################################
stop:
	$(CONFD) --stop    || true

######################################################################
cli:
	$(CONFD_DIR)/bin/confd_cli --user=admin --groups=admin \
		--interactive || echo Exit

cli-c:
	$(CONFD_DIR)/bin/confd_cli -C --user=admin --groups=admin \
		--interactive || echo Exit
######################################################################

COMMUNITY = public
SNMPPORT  = 4000

walk:
	snmpwalk -v2c -c $(COMMUNITY) 127.0.0.1:$(SNMPPORT)

######################################################################
