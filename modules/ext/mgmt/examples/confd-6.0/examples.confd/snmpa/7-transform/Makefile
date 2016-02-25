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

FXSS	= ospf.fxs OSPF-MIB.fxs IP-MIB.fxs IF-MIB.fxs
SRC	= transform.c dlist.c
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

all:	$(FXSS) ssh-keydir transform \
		OSPF-MIB.bin IP-MIB.bin IF-MIB.bin \
		$(CDB_DIR) $(CDB_DIR)/vacm_init.xml
	@echo "Build complete"

transform: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

transform.o: INET-ADDRESS-MIB.h OSPF-MIB.h IP-MIB.h IF-MIB.h SNMPv2-MIB.h \
		ospf.h interface.h

$(OBJS): $(FXSS:.fxs=.h)

$(CDB_DIR)/vacm_init.xml:
	-mkdir -p $(CDB_DIR)
	cp *_init.xml $(CDB_DIR)

SNMPv2-MIB.h: $(CONFD_DIR)/etc/confd/snmp/SNMPv2-MIB.fxs
	$(CONFDC) --emit-h $@ $<

%.bin: %.mib %.fxs
	$(CONFDC) -c -f . -f $(CONFD_FXS_DIR) -f $(CONFD_SNMP_FXS_DIR) \
	-- $< $*.fxs 

%-MIB.yang:	%-MIB.mib
	$(CONFDC) --mib2yang -o $@ $<

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
	-rm -rf transform ospf.h interface.h *.d \
		OSPF-MIB.yang OSPF-MIB.h OSPF-MIB.bin \
		IP-MIB.yang IP-MIB.h IP-MIB.bin \
		IF-MIB.yang IF-MIB.h IF-MIB.bin \
		INET-ADDRESS-MIB.yang \
		INET-ADDRESS-MIB.h INET-ADDRESS-MIB.bin \
		SNMPv2-MIB.h SNMPv2-TC.yang interface.h \
		log/* \
		2> /dev/null || true

start:  stop 
	$(CONFD) -c confd.conf $(CONFD_FLAGS) 
	./transform $(START_FLAGS) &

######################################################################
stop:
	$(CONFD) --stop    || true
	$(KILLALL) transform || true

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
	MIBS=./IP-MIB.mib:./IF-MIB.mib:./OSPF-MIB.mib snmpwalk -c public -v2c localhost:4000 mib-2


.PRECIOUS:	IF-MIB.yang IP-MIB.yang INET-ADDRESS-MIB.yang OSPF-MIB.yang 
