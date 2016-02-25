######################################################################
# Quagga example
# (C) 2007 Tail-f Systems
#
# See the README file for more information
######################################################################

usage:
	@echo "See README file for more instructions"
	@echo "make all    Build all example files"
	@echo "make clean  Remove all built and intermediary files"
	@echo "make start  Start ConfD daemon and example agent"
	@echo "make stop   Stop any ConfD daemon and example agent"
	@echo "make query  Run query against ConfD"
	@echo "make cli    Start the ConfD Command Line Interface"
	@echo "make http   Browse to http://127.0.0.1:8008"
	@echo "make https  Browse to https://127.0.0.1:8888"

######################################################################
# Where is ConfD installed? Make sure CONFD_DIR points it out
CONFD_DIR ?= ../../..

# Include standard ConfD build definitions and rules
include Makefile.inc

# In case Makefile.inc cannot be found, this rule will trigger
Makefile.inc:
	@echo 'Makefile.inc is missing, please copy it from the example.'
	@echo ''
	@exit 1

######################################################################
# Example specific definitions and rules

CONFD_HOST  ?= 127.0.0.1
CONFD_FLAGS = --addloadpath $(CONFD_DIR)/etc/confd
START_FLAGS ?=

COMMON =							   \
	bin/quagga.fxs confd-installation			   \
	db/aaa_init.xml db/quagga_init.xml db/system-info-init.xml \
	bin/commands-c.ccl bin/commands-j.ccl			   \
	bin/quagga-proxy bin/loadhist bin/loadhist.fxs	           \
	bin/syslog-helper bin/syslog.fxs			   \
	bin/confd_cli

AAA_FXS = $(CONFD_DIR)/etc/confd/tailf-aaa.fxs \
	  $(CONFD_DIR)/etc/confd/ietf-netconf-acm.fxs \
	  $(CONFD_DIR)/etc/confd/tailf-acm.fxs

all: $(COMMON) webui-cert confd_cli
	@echo "Build complete"

webui-cert:
#	@cp $(CONFD_DIR)/src/confd/webui_ng/cert/host.* ./bin
	@cp $(CONFD_DIR)/var/confd/webui/cert/host.* ./bin

bin/confd_cli:
	ln -s $(CONFD_DIR)/bin/confd_cli $@

confd_cli:
	ln -s $(CONFD_DIR)/bin/confd_cli .


######################################################################
# Authorization & Database setup

db/%.xml: src/cdb_init/%.xml
	cp $< $@
	@touch $@

confd-installation:
	ln -s $(CONFD_DIR) $@

######################################################################
# Loadhist build

bin/loadhist.fxs:  src/loadhist/yang/loadhist.yang
	$(CONFDC) -c -o $@ $<

bin/loadhist: obj/loadhist.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

obj/loadhist.o: gen/loadhist.h src/loadhist/c/loadhist.c
	$(CC) -c -o $@ $(CFLAGS) -I. src/loadhist/c/loadhist.c

gen/loadhist.h: bin/loadhist.fxs
	$(CONFDC) --emit-h $@ $<

######################################################################
# Syslog-helper build

bin/syslog.fxs: src/syslog/yang/syslog.yang
	$(CONFDC) -c -o $@ $<

bin/syslog-helper: obj/syslog-helper.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

obj/syslog-helper.o: gen/syslog-helper.h src/syslog/c/syslog-helper.c
	$(CC) -c -o $@ $(CFLAGS) -I. src/syslog/c/syslog-helper.c

gen/syslog-helper.h: bin/syslog.fxs
	$(CONFDC) --emit-h $@ $<

######################################################################
# CLI build

bin/%.ccl: src/cli/%.cli
	$(CONFDC) -o $@ -c $<

######################################################################
# CONFSPEC build

NS_PREFIX = quagga
NS_URI = http://tail-f.com/ns/example/$(NS_PREFIX)

QUAGGA_YANG_NAMES =							\
	quagga.yang confd_common.yang confd_top.yang			\
	zebra_confd_acts.yang zebra_confd.yang zebra_confd_if.yang 	\
	zebra_confd_if_ip.yang zebra_confd_if_ipv6.yang 		\
	zebra_confd_ip.yang zebra_confd_ipv6.yang 			\
	zebra_confd_stats_ip.yang zebra_confd_stats_memory.yang 	\
	rip_confd_if_ip.yang rip_confd_router.yang 			\
	rip_confd_stats_memory.yang 					\
	ospf_confd_if_ip.yang ospf_confd_router.yang 			\
	ospf_confd_stats_memory.yang 					\
	ipsec.yang

QUAGGA_YANG_DIR = src/quagga/yang
QUAGGA_DEPS     = $(QUAGGA_YANG_NAMES:%=$(QUAGGA_YANG_DIR)/%)
QUAGGA_H        = gen/quagga.h

bin/quagga.fxs: $(QUAGGA_DEPS)
	$(CONFDC) -c -o $@ --yangpath $(QUAGGA_YANG_DIR) $<

gen/quagga.h: bin/quagga.fxs
	$(CONFDC) --emit-h $@ --include-type $<

gen/ns_prefix.h: 
	echo '#define NS_PREFIX $(NS_PREFIX)' > $@


######################################################################
# QUAGGA-PROXY build

QUAGGA_C_NAMES =						\
	confd_alist.c confd_global.c confd_keychain.c		\
	confd_log.c confd_plist.c confd_rmap.c zconfd_subs.c	\
	main.c zconfd.c zebra_confd.c zebra_kernel_routes.c	\
	rip_confd.c ospf_confd.c cliLib.c cli_str.c trex.c
QUAGGA_OBJ	= $(QUAGGA_C_NAMES:%.c=obj/%.o)
QINCLUDES	= ./src/quagga/c ./gen $(CONFD_DIR)/include

$(QUAGGA_OBJ): $(QUAGGA_H) gen/ns_prefix.h gen/quagga.h

obj/%.o: src/quagga/c/%.c
	$(CC) -c -o $@ -fPIC -DLCC -D_REENTRANT $(CFLAGS)	\
		$(addprefix -I, $(QINCLUDES)) $<

bin/quagga-proxy: $(QUAGGA_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

######################################################################
clean:
	-rm -rf $(COMMON)		\
		confd_cli gen/* obj/* bin/* db/* log/*			\
		2> /dev/null || true

clean-db:
	-rm -f db/*.cdb db/*.db 2>/dev/null || true

######################################################################
start:  stop 
	$(CONFD) --start-phase0 -c confd.conf $(CONFD_FLAGS)	\
		--ignore-initial-validation
	$(CONFD) --start-phase1 
	LD_LIBRARY_PATH=$(CONFD_DIR)/lib bin/quagga-proxy 	\
		> log/quagga.log 2>&1 &
	LD_LIBRARY_PATH=$(CONFD_DIR)/lib bin/loadhist	 	\
		> log/loadhist.log 2>&1 &
	LD_LIBRARY_PATH=$(CONFD_DIR)/lib bin/syslog-helper 	\
		> log/syslog-helper.log 2>&1 &
	sleep 1 ### Give the quagga-proxy client 1 sec to start
	$(CONFD) --start-phase2
	-$(CONFD) --status

######################################################################
stop:
	: ### Killing any confd daemon and actions actions confd daemon
	$(CONFD) --stop >/dev/null 2>&1 || true
	killall quagga-proxy >/dev/null 2>&1 || true
	killall loadhist >/dev/null 2>&1 || true
	killall syslog-helper >/dev/null 2>&1 || true

######################################################################
cli cli-j:
	$(CONFD_DIR)/bin/confd_cli --user=admin --groups=admin	\
		--interactive --host $(CONFD_HOST) || echo Exit

cli-c:
	$(CONFD_DIR)/bin/confd_cli --user=admin --groups=admin	\
		--interactive --host $(CONFD_HOST) -C || echo Exit

cli-oper:
	$(CONFD_DIR)/bin/confd_cli --user=oper --groups=oper	\
		--interactive --host $(CONFD_HOST) || echo Exit

######################################################################
netconf-query query:
	$(CONFD_DIR)/bin/netconf-console-tcp --host		\
		$(CONFD_HOST) src/netconf/cmd-get-routes.xml

netconf-set set:
	$(CONFD_DIR)/bin/netconf-console-tcp --host		\
		$(CONFD_HOST) src/netconf/cmd-enable-detailed-log.xml

######################################################################
load:
	@echo "Generate some load on network element"
	@find /var -name "something-that-you-wont-find" >/dev/null 2>&1 || true

more-load:
	@echo "Generate more load on network element"
	@find /usr -name "something-that-you-wont-find" >/dev/null 2>&1 || true

heavy-load:
	@echo "Generate heavy load on network element"
	@find / -name "something-that-you-wont-find" >/dev/null 2>&1 || true

######################################################################
http:
	firefox http://$(CONFD_HOST):8008&

https:
	firefox https://$(CONFD_HOST):8888&

######################################################################
