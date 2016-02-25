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

JAVAC   = javac
JAVA    = java

all:	not_sunos \
	range.fxs  \
	test.fxs \
	jdemo.fxs \
	jdemo.java \
	Demo.class \
	callbacks \
        $(CDB_DIR) \
        $(CDB_DIR)/range.xml \
        $(CDB_DIR)/test.xml \
	ssh-keydir \
	confd.c.ccl \
	confd.j.ccl \
	actions \
	more_a_than_b
	@echo "Build complete"

%.class:	%.java
	$(JAVAC) -classpath $(CLASSPATH) $*.java

%.java: %.fxs
	$(CONFDC) --emit-java $*.java $<

callbacks: callbacks.o
	$(CC) -o $@ $< $(LIBS)

actions: range.h $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

more_a_than_b: test.h more_a_than_b.o
	$(CC) -o $@ more_a_than_b.o $(LIBS)

######################################################################
$(CDB_DIR)/%.xml: %.xml
	rm -f $@ ; cp $< $@
######################################################################
clean:	iclean
	-rm -rf *.class actions more_a_than_b callbacks 2> /dev/null || true
	-rm -rf *.class config.h range.h test.h 2> /dev/null || true
	-rm -rf jdemo.java 2> /dev/null || true

start:  stop
	$(CONFD) -c confd.conf --start-phase0 $(CONFD_FLAGS)
	$(CONFD) --wait-phase0
	./more_a_than_b $(START_FLAGS) &
	sleep 2
	$(CONFD) --start-phase1 $(CONFD_FLAGS); \
	$(CONFD) --start-phase2 $(CONFD_FLAGS); 
	./actions $(START_FLAGS) &
	./callbacks $(START_FLAGS) &
	$(MAKE) jdp

jdp:
	PATH=/usr/sbin:/sbin:$${PATH} java -classpath $(CLASSPATH):$(JLIB):. Demo


######################################################################
stop:
	$(CONFD) --stop || true
	$(KILLALL) actions || true
	$(KILLALL) more_a_than_b || true


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

