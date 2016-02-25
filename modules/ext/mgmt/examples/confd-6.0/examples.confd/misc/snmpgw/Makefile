######################################################################
# SNMP Gateway Example Makefile
# (C) 2012 Tail-f Systems
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

# Include standard ConfD build definitions and rules
include $(CONFD_DIR)/src/confd/build/include.mk

# In case CONFD_DIR is not set (correctly), this rule will trigger
$(CONFD_DIR)/src/confd/build/include.mk:
	@echo 'Where is ConfD installed? Set $$CONFD_DIR to point it out!'
	@echo ''

######################################################################
# Example specific definitions and rules

CONFD_FXS_DIR = $(CONFD_DIR)/etc/confd
CONFD_SNMP_FXS_DIR = $(CONFD_DIR)/etc/confd/snmp
CONFD_SNMP_YANG_DIR = $(CONFD_DIR)/src/confd/snmp/yang

CONFD_FLAGS = --addloadpath $(CONFD_FXS_DIR) \
	      --addloadpath $(CONFD_SNMP_FXS_DIR)

CONFD_CONF ?= confd.conf

######################################################################

ALL_MIBS = IF-MIB.mib IANAifType-MIB.mib HOST-RESOURCES-MIB.mib

ALL_FXS = $(ALL_MIBS:.mib=.fxs)

all:	$(ALL_FXS) $(CDB_DIR)
	@echo "Build complete"

# Keep all YANG files
.SECONDARY: $(ALL_MIBS:.mib=.yang)

# Dependecies for our MIBs
IF-MIB.fxs: IANAifType-MIB.yang
HOST-RESOURCES-MIB.fxs: IF-MIB.yang
HOST-RESOURCES-MIB.fxs: HOST-RESOURCES-MIB-ann.yang

%.yang: %.mib
	$(CONFDC) --mib2yang-std -o $@ $<

# add CONFD_SNMP_YANG_DIR to the YANG path, so that confdc can find
#  the standard MIBs in YANG format
%.fxs: %.yang %-ann.yang
	$(CONFDC) -c --snmpgw -o $@ --annotate $*-ann.yang \
	  -f . --yangpath $(CONFD_SNMP_YANG_DIR) $<

%.fxs: %.yang
	$(CONFDC) -c --snmpgw -o $@ -f . \
	  --yangpath $(CONFD_SNMP_YANG_DIR) $<


# Create an annotation file for the YANG module
%-ann.yang: %.mib %.miba
	$(CONFDC) --mib2yang-mods --keep-readonly \
	   --mib-annotation $*.miba -o $@ $<

######################################################################
clean:	iclean
	-rm -rf *.yang *.fxs global.data logs *.d 2>/dev/null || true

start:  stop 
	mkdir -p logs || true	
	$(CONFD) -c $(CONFD_CONF) $(CONFD_FLAGS)

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

query-if:
	$(CONFD_DIR)/bin/netconf-console-tcp --get -x /IF-MIB

query-host:
	$(CONFD_DIR)/bin/netconf-console-tcp --get -x /HOST-RESOURCES-MIB
