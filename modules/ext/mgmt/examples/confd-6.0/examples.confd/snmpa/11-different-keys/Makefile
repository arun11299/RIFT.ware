######################################################################
# SNMP Agent example "Transform"
# (C) 2008-2009 Tail-f Systems
#
# See the README files for more information
######################################################################

usage:
	@echo "See README file for more instructions"
	@echo "make all      Build all example files"
	@echo "make clean    Remove all built and intermediary files"
	@echo "make start    Start ConfD daemon and example agent"
	@echo "make stop     Stop any ConfD daemon and example agent"
	@echo "make query    Run NETCONF query against ConfD"
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

CONFD_FLAGS = --addloadpath $(CONFD_FXS_DIR) \
	      --addloadpath $(CONFD_SNMP_FXS_DIR)

CDB_DIR		 = ./confd-cdb
######################################################################
# Example specific definitions and rules

FXSS	= servers.fxs
SRC	= hook.c
OBJS	= $(SRC:.c=.o)

#CFLAGS		+= -pedantic-errors
CWARNINGS	= \
		-Werror \
	        -Wall	\
		-Wstrict-prototypes \
		-Wpointer-arith \
		-Wcast-align \
		-Wwrite-strings \
		-Wnested-externs

#		-Wsign-compare \

CFLAGS	+= $(CWARNINGS)

all:	$(FXSS) ssh-keydir SERVERS-MIB.bin $(CDB_DIR) hook \
    $(CDB_DIR)/vacm_init.xml
	@echo "Build complete"

SERVERS-MIB.mib: servers.fxs
	$(CONFDC) --emit-mib $@ --top serversMIB -f $(CONFD_FXS_DIR) -- $<

%.fxs: %.yang
	$(CONFDC) $(EXTRA_LINK_FLAGS) -c -o $@ $<

SERVERS-MIB.bin: servers.fxs SERVERS-MIB.mib
	$(CONFDC) -c -f $(CONFD_FXS_DIR) -- SERVERS-MIB.mib servers.fxs


hook: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

hook.o: servers.h

$(CDB_DIR)/vacm_init.xml:
	-mkdir -p $(CDB_DIR)
	cp *_init.xml $(CDB_DIR)

SNMPv2-MIB.h: $(CONFD_DIR)/etc/confd/snmp/SNMPv2-MIB.fxs
	$(CONFDC) --emit-h $@ $<

%.bin: %.mib
	$(CONFDC) -c -f . -f $(CONFD_FXS_DIR) -f $(CONFD_SNMP_FXS_DIR) \
	-- $< $*.fxs 

%-MIB.yang:	%-MIB.mib
	$(CONFDC) --mib2yang -o $@ $<

%-MIB.bin: %-MIB.mib %-MIB.fxs
	$(CONFDC) -c -f $(CONFD_SNMP_FXS_DIR) -- $*-MIB.mib $*-MIB.fxs 

OSPF-MIB.bin: ospf.fxs interface.fxs

ospf.fxs: interface.fxs IF-MIB.fxs 

# Include standard ConfD build definitions and rules
include $(CONFD_DIR)/src/confd/build/include.mk

%.fxs:	%.yang
	a=$*-ann.yang; \
	$(CONFDC) -c -o $@ `[ -e $$a ] && echo "-a $$a"` -f . \
		-f $(CONFD_FXS_DIR) -- $<
######################################################################
clean:	iclean
	-rm -rf hook servers.h *.d \
	    SERVERS-MIB.mib SERVERS-MIB.bin \
	    log/* \
	    2> /dev/null || true

start:  stop 
	$(CONFD) -c confd.conf $(CONFD_FLAGS) 
	./hook $(START_FLAGS) &

######################################################################
stop:
	$(CONFD) --stop    || true
	$(KILLALL) hook || true

######################################################################
cli:
	$(CONFD_DIR)/bin/confd_cli --user=admin --groups=admin \
		--interactive || echo Exit

cli-c:
	$(CONFD_DIR)/bin/confd_cli -C --user=admin --groups=admin \
		--interactive || echo Exit
######################################################################
query:
	$(CONFD_DIR)/bin/netconf-console-tcp --get-config -x /interface

######################################################################

mibwalk:
	MIBS=./SERVERS-MIB.mib snmpwalk -c public -v2c localhost:4000 mib-2


.PRECIOUS:	IF-MIB.yang IP-MIB.yang INET-ADDRESS-MIB.yang OSPF-MIB.yang 
