


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
	@echo "make query   Run query against CONFD"
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

JAVAC   = javac
JAVA    = java


all:	not_sunos \
	jdemo.fxs  confd.cisco.ccl \
	jdemo.java \
        Demo.class \
        $(CDB_DIR) ssh-keydir 
	@echo "Build complete"

%.class:	%.java
	$(JAVAC) -classpath $(CLASSPATH) $*.java

%.java: %.fxs
	$(CONFDC) --emit-java $*.java $<


confd.cisco.ccl: confd.cisco.cli
	${CONFDC} -c confd.cisco.cli

######################################################################
clean:	iclean
	-rm -rf jdemo.java *.class *.ccl 2> /dev/null || true

start:  stop
	$(CONFD) -c confd.conf $(CONFD_FLAGS) 
	$(MAKE) jdp

jdp:
	PATH=/usr/sbin:/sbin:$${PATH} java -classpath $(CLASSPATH):$(JLIB):. Demo



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

query:
	$(CONFD_DIR)/bin/netconf-console --get -x /arp
