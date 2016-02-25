######################################################################
# (C) 2006 Tail-f Systems
######################################################################

COMPONENT = ipmibs

AUX_MODS = IANAifType INET-ADDRESS
MODULES = UDP TCP IF IP
MIBS = $(addsuffix -MIB, $(AUX_MODS) $(MODULES))
FXS = $(MIBS:%=%.fxs)
BINS = $(MIBS:%=%.bin)
GENERATED = $(FXS) $(FXS_H) $(BINS)

COMPONENT_SRCS = ipmibs.c caches.c udp.c tcp.c if.c utils.c manufact.c ip.c

all: $(GENERATED) caches.o ipmibs.o

include ../include.mk
include ../component.mk

ifneq (,$(findstring LINUXCFG_INTERFACES, $(CFLAGS_LINUXCFG_FEATURES)))
	CFLAGS += -I../ietf_interfaces
endif
$(COMPONENT_SRCS:%.c=%.d): $(FXS_H)

ip.o if.o tcp.o udp.o: utils.h caches.h
ipmibs.o: utils.h caches.h udp.h tcp.h if.h ip.h

clean: clean-generated

clean-generated:
	-rm -f $(GENERATED) $(MIBS:%=%.d) $(COMPONENT_SRCS:%.c=%.d)

install: install-extra

install-extra:
	install -m 0644 $(BINS) $(CONFD_INSTALLDIR)
	install -m 0644 $(wildcard init_data/*_init.xml) $(CONFD_INSTALLDIR)/cdb

install_init_xml:
	install -m 0644 $(wildcard init_data/*_init.xml) $(INIT_XML_INSTALLDIR)

%.yang : %.miba
