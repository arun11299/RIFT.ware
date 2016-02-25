
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */



/**
 * @file rw_namespace.cpp
 * @date 2014/09/02
 * @brief RiftWare general namespace utilities
 */

#include <rw_namespace.h>
#include <rw_yang_mutex.hpp>
#include <functional>

using namespace rw_yang;

void NamespaceManager::init()
{
  // ATTN: Need to register prefixes?

  /*
     ATTN: Should all these integer constants also have global enum
     values?  That seems like it would be a good idea.  And, for that
     matter, should the strings also be global constants?  We already
     use them in several places in the CLI code.
   */

  /***************************************************************************/
  /**                                                                       **/
  /** DO NOT ADD OR REMOVE IN THE MIDDLE!  NAMESPACES ARE FOREVER!!         **/
  /**                                                                       **/
  /***************************************************************************/
  register_fixed(   1, "http://netconfcentral.org/ns/yangdump" );
  register_fixed(   2, "http://netconfcentral.org/ns/yuma-app-common" );
  register_fixed(   3, "http://netconfcentral.org/ns/yuma-ncx" );
  register_fixed(   4, "http://netconfcentral.org/ns/yuma-types" );
  register_fixed(   5, "http://riftio.com/ns/core/util/rwutcli/yang/rw-utcli-base.yang" );
  register_fixed(   6, "http://riftio.com/ns/core/util/yangtools/tests/person" );
  register_fixed(   7, "http://riftio.com/ns/riftware-1.0/config_root" );
  register_fixed(   8, "http://riftio.com/ns/riftware-1.0/rw-base" );
  register_fixed(   9, "http://riftio.com/ns/riftware-1.0/rw-cli-ext" );
  //register_fixed(  10, "http://riftio.com/ns/riftware-1.0/rw-fpath" );
  register_fixed(  11, "http://riftio.com/ns/riftware-1.0/rw-hello" );
  register_fixed(  12, "http://riftio.com/ns/riftware-1.0/rw-pb-ext" );
  register_fixed(  13, "http://riftio.com/ns/riftware-1.0/rw-yang-types" );
  register_fixed(  14, "http://riftio.com/ns/riftware-1.0/rw-utcli-ext" );
  register_fixed(  15, "http://riftio.com/ns/riftware-1.0/rwcmdargs-cli" );
  register_fixed(  16, "http://riftio.com/ns/riftware-1.0/rwcmdargs-ext" );
  register_fixed(  17, "http://riftio.com/ns/riftware-1.0/rwcmdargs-min" );
  register_fixed(  18, "http://riftio.com/ns/riftware-1.0/rw-manifest" );
  register_fixed(  19, "http://riftio.com/ns/riftware-1.0/rwmsg-data" );
  register_fixed(  20, "http://riftio.com/ns/riftware-1.0/rw-mgmtagt" );
  register_fixed(  21, "http://riftio.com/ns/riftware-1.0/rwvcs" );
  register_fixed(  22, "http://riftio.com/ns/riftware-1.0/rwzk" );
  register_fixed(  23, "http://tail-f.com/ns/aaa/1.1" );
  register_fixed(  24, "http://tail-f.com/ns/cli-builtin/1.0" );
  register_fixed(  25, "http://tail-f.com/ns/confd_dyncfg/1.0" );
  register_fixed(  26, "http://tail-f.com/ns/netconf/inactive/1.0" );
  register_fixed(  27, "http://tail-f.com/ns/netconf/transactions/1.0" );
  register_fixed(  28, "urn:ietf:params:xml:ns:netconf:base:1.0" );
  register_fixed(  29, "urn:ietf:params:xml:ns:netconf:notification:1.0" );
  register_fixed(  30, "urn:ietf:params:xml:ns:netmod:notification" );
  register_fixed(  31, "urn:ietf:params:xml:ns:yang:iana-afn-safi" );
  register_fixed(  32, "urn:ietf:params:xml:ns:yang:iana-crypt-hash" );
  register_fixed(  33, "urn:ietf:params:xml:ns:yang:iana-if-type" );
  register_fixed(  34, "urn:ietf:params:xml:ns:yang:iana-timezones" );
  register_fixed(  35, "urn:ietf:params:xml:ns:yang:ietf-inet-types" );
  register_fixed(  36, "urn:ietf:params:xml:ns:yang:ietf-interfaces" );
  register_fixed(  37, "urn:ietf:params:xml:ns:yang:ietf-ip" );
  register_fixed(  38, "urn:ietf:params:xml:ns:yang:ietf-ipfix-psamp" );
  register_fixed(  39, "urn:ietf:params:xml:ns:yang:ietf-ipv4-unicast-routing" );
  register_fixed(  40, "urn:ietf:params:xml:ns:yang:ietf-ipv6-unicast-routing" );
  register_fixed(  41, "urn:ietf:params:xml:ns:yang:ietf-netconf-acm" );
  register_fixed(  42, "urn:ietf:params:xml:ns:yang:ietf-netconf-monitoring" );
  register_fixed(  43, "urn:ietf:params:xml:ns:yang:ietf-netconf-with-defaults" );
  register_fixed(  44, "urn:ietf:params:xml:ns:yang:ietf-routing" );
  register_fixed(  45, "urn:ietf:params:xml:ns:yang:ietf-snmp" );
  register_fixed(  46, "urn:ietf:params:xml:ns:yang:ietf-system" );
  register_fixed(  47, "urn:ietf:params:xml:ns:yang:ietf-yang-smiv2" );
  register_fixed(  48, "urn:ietf:params:xml:ns:yang:ietf-yang-types" );
  register_fixed(  49, "http://riftio.com/ns/riftware-1.0/rwappmgr");
  register_fixed(  50, "http://riftio.com/ns/riftware-1.0/config_ipsec" );
  register_fixed(  51, "http://riftio.com/ns/riftware-1.0/config_strongswan" );
  register_fixed(  52, "urn:ietf:params:xml:ns:yang:smiv2:AGENTX-MIB" );
  register_fixed(  53, "urn:ietf:params:xml:ns:yang:smiv2:DIFFSERV-CONFIG-MIB" );
  register_fixed(  54, "urn:ietf:params:xml:ns:yang:smiv2:DIFFSERV-DSCP-TC" );
  register_fixed(  55, "urn:ietf:params:xml:ns:yang:smiv2:DIFFSERV-MIB" );
  register_fixed(  56, "urn:ietf:params:xml:ns:yang:smiv2:DISMAN-EVENT-MIB" );
  register_fixed(  57, "urn:ietf:params:xml:ns:yang:smiv2:DISMAN-SCHEDULE-MIB" );
  register_fixed(  58, "urn:ietf:params:xml:ns:yang:smiv2:DISMAN-SCRIPT-MIB" );
  register_fixed(  59, "urn:ietf:params:xml:ns:yang:smiv2:ENTITY-MIB" );
  register_fixed(  60, "urn:ietf:params:xml:ns:yang:smiv2:ENTITY-SENSOR-MIB" );
  register_fixed(  61, "urn:ietf:params:xml:ns:yang:smiv2:ENTITY-STATE-MIB" );
  register_fixed(  62, "urn:ietf:params:xml:ns:yang:smiv2:ENTITY-STATE-TC-MIB" );
  register_fixed(  63, "urn:ietf:params:xml:ns:yang:smiv2:EtherLike-MIB" );
  register_fixed(  64, "urn:ietf:params:xml:ns:yang:smiv2:HCNUM-TC" );
  register_fixed(  65, "urn:ietf:params:xml:ns:yang:smiv2:HOST-RESOURCES-MIB" );
  register_fixed(  66, "urn:ietf:params:xml:ns:yang:smiv2:HOST-RESOURCES-TYPES" );
  register_fixed(  67, "urn:ietf:params:xml:ns:yang:smiv2:IANA-ADDRESS-FAMILY-NUMBERS-MIB" );
  register_fixed(  68, "urn:ietf:params:xml:ns:yang:smiv2:IANA-LANGUAGE-MIB" );
  register_fixed(  69, "urn:ietf:params:xml:ns:yang:smiv2:IANA-RTPROTO-MIB" );
  register_fixed(  70, "urn:ietf:params:xml:ns:yang:smiv2:IANAifType-MIB" );
  register_fixed(  71, "urn:ietf:params:xml:ns:yang:smiv2:IF-INVERTED-STACK-MIB" );
  register_fixed(  72, "urn:ietf:params:xml:ns:yang:smiv2:IF-MIB" );
  register_fixed(  73, "urn:ietf:params:xml:ns:yang:smiv2:INET-ADDRESS-MIB" );
  register_fixed(  74, "urn:ietf:params:xml:ns:yang:smiv2:INTEGRATED-SERVICES-MIB" );
  register_fixed(  75, "urn:ietf:params:xml:ns:yang:smiv2:IP-MIB" );
  register_fixed(  76, "urn:ietf:params:xml:ns:yang:smiv2:RFC-1215" );
  register_fixed(  77, "urn:ietf:params:xml:ns:yang:smiv2:RMON-MIB" );
  register_fixed(  78, "urn:ietf:params:xml:ns:yang:smiv2:SNMP-FRAMEWORK-MIB" );
  register_fixed(  79, "urn:ietf:params:xml:ns:yang:smiv2:SNMP-MPD-MIB" );
  register_fixed(  80, "urn:ietf:params:xml:ns:yang:smiv2:SNMP-NOTIFICATION-MIB" );
  register_fixed(  81, "urn:ietf:params:xml:ns:yang:smiv2:SNMP-TARGET-MIB" );
  register_fixed(  82, "urn:ietf:params:xml:ns:yang:smiv2:SNMP-USM-AES-MIB" );
  register_fixed(  83, "urn:ietf:params:xml:ns:yang:smiv2:SNMPv2-MIB" );
  register_fixed(  84, "urn:ietf:params:xml:ns:yang:smiv2:SNMPv2-TC" );
  register_fixed(  85, "urn:ietf:params:xml:ns:yang:smiv2:SNMPv2-TM" );
  register_fixed(  86, "urn:ietf:params:xml:ns:yang:smiv2:TRANSPORT-ADDRESS-MIB" );
  register_fixed(  87, "urn:ietf:params:xml:ns:yang:yang-smi" );
  register_fixed(  88, "urn:ietf:params:xml:ns:yang:yang-smiv2" );
  register_fixed(  89, "http://riftio.com/ns/core/util/yangtools/tests/test-tag-generation" );
  register_fixed(  90, "http://riftio.com/ns/core/util/yangtools/tests/test-tag-generation-base" );
  register_fixed(  91, "http://riftio.com/ns/yangtools/rift-cli-test" );
  register_fixed(  92, "http://riftio.com/ns/core/rwvx/rwdts/test/yang/dtstest");
  register_fixed(  93, "http://riftio.com/ns/riftware-1.0/rwdts" );
  register_fixed(  94, "http://riftio.com/ns/core/mgmt/uagent/test/vehicle" );
  register_fixed(  95, "http://riftio.com/ns/riftware-1.0/rw-vcs" );
  register_fixed(  96, "http://riftio.com/ns/core/util/yangtools/tests/testy2p-search-node1" );
  register_fixed(  97, "http://riftio.com/ns/core/util/yangtools/tests/testy2p-search-node2" );
  register_fixed(  98, "http://riftio.com/ns/core/util/yangtools/tests/testy2p-top1" );
  register_fixed(  99, "http://riftio.com/ns/core/util/yangtools/tests/testy2p-top2" );
  register_fixed( 100, "http://riftio.com/ns/core/util/yangtools/tests/test-rwapis" );
  register_fixed( 101, "http://riftio.com/ns/core/util/yangtools/tests/document" );
  register_fixed( 102, "http://riftio.com/ns/core/util/yangtools/tests/company" );
  register_fixed( 103, "http://riftio.com/ns/riftware-1.0/rwlog-mgmt" );
  register_fixed( 104, "http://riftio.com/ns/riftware-1.0/rw-3gpp-types" );
  //register_fixed( 105, "http://riftio.com/ns/riftware-1.0/rw-ifmgr-data" );
  register_fixed( 106, "http://riftio.com/ns/riftware-1.0/rwvnfd" );
  register_fixed( 107, "http://riftio.com/ns/riftware-1.0/rwnsd" );
  register_fixed( 108, "http://riftio.com/ns/riftware-1.0/rw-composite" );
  register_fixed( 109, "http://riftio.com/ns/riftware-1.0/rwuagent-cli-annotation" );
  register_fixed( 110, "http://riftio.com/ns/riftware-1.0/rw-notify-ext" );
  register_fixed( 111, "http://riftio.com/ns/riftware-1.0/rw-dist-classifier-data" );
  register_fixed( 112, "http://riftio.com/ns/riftware-1.0/rw-fpath-annotation" );
  register_fixed( 113, "http://riftio.com/ns/riftware-1.0/rw-fpath-appmgr/annotations" );
  register_fixed( 114, "http://riftio.com/ns/riftware-1.0/rw-log" );
  register_fixed( 115, "http://riftio.com/ns/riftware-1.0/rw-log-ext" );
  //register_fixed( 116, "http://riftio.com/ns/riftware-1.0/rw-ncmgr-data" );
  register_fixed( 117, "http://riftio.com/ns/riftware-1.0/rwcloud" );
  register_fixed( 118, "urn:TBD:params:xml:ns:yang:network-topology" );
  register_fixed( 119, "urn:opendaylight:yang:extension:yang-ext" );
  register_fixed( 120, "urn:opendaylight:model:topology:general" );
  //register_fixed( 121, "http://riftio.com/ns/riftware-1.0/rw-fpctrl-data" );
  register_fixed( 122, "http://riftio.com/ns/riftware-1.0/rwappmgr-log");
  register_fixed( 123, "http://riftio.com/ns/riftware-1.0/core/rwvx/rwdts/test/yang/testdts-rw-fpath");
  register_fixed( 124, "http://riftio.com/ns/riftware-1.0/core/rwvx/rwdts/test/yang/testdts-rw-base");
  register_fixed( 125, "http://riftio.com/ns/riftware-1.0/rwdtsapilog");
  register_fixed( 126, "http://riftio.com/ns/riftware-1.0/rwdtsrouterlog");
  register_fixed( 127, "http://riftio.com/ns/riftware-1.0/rwnetns-log");
  register_fixed( 128, "http://riftio.com/ns/riftware-1.0/rwfpath-log");
  register_fixed( 129, "http://riftio.com/ns/riftware-1.0/rwlogger-log");
  register_fixed( 130, "http://riftio.com/ns/riftware-1.0/rwvm-heartbeat-log");
  register_fixed( 131, "http://riftio.com/ns/riftware-1.0/rwcal");
  register_fixed( 132, "http://riftio.com/ns/riftware-1.0/rw-fpath-data" );
  register_fixed( 133, "http://riftio.com/ns/riftware-1.0/rwvcs-mgmtagent-log");
  register_fixed( 134, "http://riftio.com/ns/core/util/yangtools/tests/conversion");
  register_fixed( 135, "http://riftio.com/ns/riftware-1.0/slb-dns" );
  register_fixed( 136, "http://riftio.com/ns/riftware-1.0/rw-fastflow-data" );
  register_fixed( 137, "http://riftio.com/ns/riftware-1.0/rwdts-cfg" );
  register_fixed( 138, "http://riftio.com/ns/riftware-1.0/rwncmgr-log");
  register_fixed( 139, "http://riftio.com/ns/riftware-1.0/rwifmgr-log");
  register_fixed( 140, "http://riftio.com/ns/riftware-1.0/rw-fastpath");
  register_fixed( 141, "http://riftio.com/ns/riftware-1.0/rwdtstoytasklet");
  register_fixed( 142, "http://riftio.com/ns/riftware-1.0/rwiaas-cmp");
  register_fixed( 143, "http://riftio.com/ns/riftware-1.0/rwiaas-hosts");
  register_fixed( 144, "http://riftio.com/ns/riftware-1.0/rwiaas-zones");
  register_fixed( 145, "http://riftio.com/ns/riftware-1.0/rwiaas-workspaces");
  register_fixed( 146, "http://riftio.com/ns/riftware-1.0/rwpaas-users");
  register_fixed( 147, "http://riftio.com/ns/riftware-1.0/rwpaas-environments");
  register_fixed( 148, "http://riftio.com/ns/riftware-1.0/rwpaas-networks");
  register_fixed( 149, "http://riftio.com/ns/riftware-1.0/rwpaas-vms");
  register_fixed( 150, "http://riftio.com/ns/riftware-1.0/rwvld" );
  register_fixed( 151, "http://riftio.com/ns/riftware-1.0/rwpnfd" );
  register_fixed( 152, "http://riftio.com/ns/riftware-1.0/rwmano-types" );
  register_fixed( 153, "http://riftio.com/ns/riftware-1.0/rwvnffgd" );
  register_fixed( 154, "http://riftio.com/ns/riftware-1.0/rwpaas-vdus" );
  register_fixed( 155, "http://riftio.com/ns/riftware-1.0/rw-ipsec" );
  register_fixed( 156, "http://riftio.com/ns/riftware-1.0/rwstrongswantasklet");
  register_fixed( 157, "http://riftio.com/ns/riftware-1.0/rw-c-types");
  register_fixed( 158, "http://riftio.com/ns/riftware-1.0/rw-system-test");
  register_fixed( 159, "http://riftio.com/ns/riftware-1.0/rw-iot" );
  register_fixed( 160, "http://riftio.com/ns/riftware-1.0/rw-iot-data" );
  register_fixed( 161, "http://riftio.com/ns/riftware-1.0/rw-dtsperf");
  register_fixed( 162, "http://riftio.com/ns/riftware-1.0/rwshell-mgmt" );
  register_fixed( 163, "http://riftio.com/ns/riftware-1.0/rw-netconf" );
  register_fixed( 164, "http://riftio.com/ns/riftware-1.0/rw-debug" );
  register_fixed( 165, "http://riftio.com/ns/riftware-1.0/rwfpctrl-log" );
  register_fixed( 166, "http://riftio.com/ns/riftware-1.0/rw-keyspec-stats" );
  register_fixed( 167, "http://riftio.com/ns/riftware-1.0/rw-pbc-stats" );
  register_fixed( 168, "http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a" );
  register_fixed( 169, "http://riftio.com/ns/riftware-1.0/rwvnfmgr" );
  register_fixed( 170, "http://riftio.com/ns/core/mgmt/uagent/test/vehicle-a" );
  register_fixed( 171, "http://riftio.com/ns/core/mgmt/uagent/test/vehicle-a-augment" );
  register_fixed( 172, "http://riftio.com/ns/riftware-1.0/rw-dtsperfmgr" );
  register_fixed( 173, "http://riftio.com/ns/core/mgmt/rwrestconf/test/vehicle-a-augment" );
  register_fixed( 174, "http://riftio.com/ns/riftware-1.0/rw-nnlatencytasklet" );
  register_fixed( 175, "http://riftio.com/ns/riftware-1.0/rw-mc" );
  register_fixed( 176, "http://riftio.com/ns/riftware-1.0/rw-restconf" );
  register_fixed( 177, "http://riftio.com/ns/riftware-1.0/rw-reststream" );
  register_fixed( 178, "http://riftio.com/ns/riftware-1.0/rw-mgmt-schema" );
  register_fixed( 179, "http://riftio.com/ns/riftware-1.0/rw-vnf-base-types");
  //register_fixed( 180, "http://riftio.com/ns/riftware-1.0/rw-vrf");
  register_fixed( 181, "http://riftio.com/ns/riftware-1.0/rw-dest-nat");
  register_fixed( 182, "http://riftio.com/ns/riftware-1.0/rw-destnat-data");
  register_fixed( 183, "http://riftio.com/ns/riftware-1.0/rw-ip-receiver-app");
  register_fixed( 184, "http://riftio.com/ns/riftware-1.0/rw-ipreceiverapp-data");
  register_fixed( 185, "http://riftio.com/ns/riftware-1.0/rw-trafgen-data");
  register_fixed( 186, "http://riftio.com/ns/riftware-1.0/rw-trafgen");
  register_fixed( 187, "http://riftio.com/ns/riftware-1.0/rw-vfabric");
  register_fixed( 188, "http://riftio.com/ns/riftware-1.0/rw-external-app");
  register_fixed( 189, "http://riftio.com/ns/riftware-1.0/rw-ip-classifier");
  register_fixed( 190, "http://riftio.com/ns/riftware-1.0/rw-ip-rx-app");
  register_fixed( 191, "http://riftio.com/ns/riftware-1.0/rw-iprxapp-data");
  register_fixed( 192, "http://riftio.com/ns/riftware-1.0/rw-ipfp");
  register_fixed( 193, "http://riftio.com/ns/riftware-1.0/rw-scriptable-lb");
  register_fixed( 194, "http://riftio.com/ns/riftware-1.0/rw-scriptablelb-data");
  //register_fixed( 195, "http://riftio.com/ns/riftware-1.0/rw-ncconfig");
  //register_fixed( 196, "http://riftio.com/ns/riftware-1.0/rw-portconfig");
  register_fixed( 197, "http://riftio.com/ns/riftware-1.0/rw-memlog" );
  register_fixed( 198, "http://riftio.com/ns/riftware-1.0/rw-restconf-annotation" );
  register_fixed( 199, "http://riftio.com/ns/riftware-1.0/rw-sfmgr" );
  register_fixed( 200, "http://riftio.com/ns/riftware-1.0/rw-sfmgr-data" );
  register_fixed( 201, "http://riftio.com/ns/riftware-1.0/rw-memlog-annotation" );
  register_fixed( 202, "http://riftio.com/ns/riftware-1.0/rw-cc-toy" );
  register_fixed( 203, "http://riftio.com/ns/riftware-1.0/rw-mc-log" );
  register_fixed( 204, "http://riftio.com/ns/riftware-1.0/rw-restconf-perf-test" );
  register_fixed( 205, "http://riftio.com/ns/riftware-1.0/mano-base" );
  register_fixed( 206, "http://riftio.com/ns/riftware-1.0/toy-vnf-config" );
  register_fixed( 207, "http://riftio.com/ns/riftware-1.0/toy-vnf-opdata" );
  register_fixed( 208, "http://riftio.com/ns/riftware-1.0/rw-vnf-base-config" );
  register_fixed( 209, "http://riftio.com/ns/riftware-1.0/rwvcs-inventory" );
  register_fixed( 210, "http://riftio.com/ns/riftware-1.0/mano-types" );
  register_fixed( 211, "http://riftio.com/ns/riftware-1.0/vnfd" );
  register_fixed( 212, "http://riftio.com/ns/riftware-1.0/nsd" );
  register_fixed( 213, "http://riftio.com/ns/riftware-1.0/vld" );
  register_fixed( 214, "http://riftio.com/ns/riftware-1.0/nsd-catalog" );
  register_fixed( 215, "http://riftio.com/ns/riftware-1.0/vnfd-catalog" );
  register_fixed( 216, "http://riftio.com/ns/riftware-1.0/vld-catalog" );
  register_fixed( 217, "http://riftio.com/ns/riftware-1.0/vnffgd" );
  register_fixed( 218, "http://riftio.com/ns/riftware-1.0/pnfd" );
  register_fixed( 219, "http://riftio.com/ns/riftware-1.0/pnfd-catalog" );
  register_fixed( 220, "http://riftio.com/ns/riftware-1.0/rw-vnf-base-opdata" );
  register_fixed( 221, "http://riftio.com/ns/riftware-1.0/rwvcs-dynschema-log");
  register_fixed( 222, "urn:ietf:params:xml:ns:yang:nfvo:mano-types" );
  register_fixed( 223, "urn:ietf:params:xml:ns:yang:nfvo:vnfd" );
  register_fixed( 224, "urn:ietf:params:xml:ns:yang:nfvo:nsd" );
  register_fixed( 225, "urn:ietf:params:xml:ns:yang:nfvo:vld" );
  register_fixed( 226, "urn:ietf:params:xml:ns:yang:nfvo:nsd-catalog" );
  register_fixed( 227, "urn:ietf:params:xml:ns:yang:nfvo:vnfd-catalog" );
  register_fixed( 228, "urn:ietf:params:xml:ns:yang:nfvo:vld-catalog" );
  register_fixed( 229, "urn:ietf:params:xml:ns:yang:nfvo:vnffgd" );
  register_fixed( 230, "urn:ietf:params:xml:ns:yang:nfvo:pnfd" );
  register_fixed( 231, "urn:ietf:params:xml:ns:yang:nfvo:pnfd-catalog" );
  register_fixed( 232, "urn:ietf:params:xml:ns:yang:nfvo:nsr" );
  register_fixed( 233, "urn:ietf:params:xml:ns:yang:nfvo:vnffgr" );
  register_fixed( 234, "urn:ietf:params:xml:ns:yang:nfvo:vlr" );
  register_fixed( 235, "urn:ietf:params:xml:ns:yang:nfvo:vnfr" );
  register_fixed( 236, "urn:ietf:params:xml:ns:yang:nfvo:pnfr" );
  register_fixed( 237, "http://riftio.com/ns/riftware-1.0/rw-launchpad" );
  register_fixed( 238, "http://riftio.com/ns/riftware-1.0/rw-launchpad-log" );
  register_fixed( 239, "http://riftio.com/ns/riftware-1.0/rw-vnfd" );
  register_fixed( 240, "http://riftio.com/ns/riftware-1.0/rw-iwp" );
  register_fixed( 241, "http://riftio.com/ns/riftware-1.0/rw-vnfr" );
  register_fixed( 242, "http://riftio.com/ns/riftware-1.0/rw-nsr" );
  register_fixed( 243, "http://riftio.com/ns/riftware-1.0/rw-vlr" );
  register_fixed( 244, "http://riftio.com/ns/riftware-1.0/rw-vns" );
  register_fixed( 245, "http://riftio.com/ns/riftware-1.0/rw-external-app-data");

  RW_ASSERT(current_nsid_ <= NSID_RUNTIME_BASE);
  current_nsid_ = NSID_RUNTIME_BASE;
}

void NamespaceManager::register_fixed(
  rw_namespace_id_t nsid,
  const char* ns)
{
  GlobalMutex::guard_t guard(GlobalMutex::g_mutex);
  RW_ASSERT(nsid >= current_nsid_);

  // fill in nullptrs to deleted/missing namespaces.  std::vector cannot have holes
  while(current_nsid_ < nsid) {
    id2ns_fixed_.emplace_back(nullptr);
    ++current_nsid_;
  }

  // Save in the vector first; if the map insert blows up, things are safe (although broken).
  RW_ASSERT(id2ns_fixed_.size() == nsid);
  id2ns_fixed_.emplace_back(new Namespace(ns, nullptr/*prefix*/, nsid));
  ++current_nsid_;

  // Now update the map; if this blows up, the namespace leaks, but things otherwise work
  auto status = ns2id_.emplace(ns, nsid);
  RW_ASSERT(status.second);
}

rw_namespace_id_t NamespaceManager::register_runtime_locked(
  const char* ns,
  const char* prefix)
{
  RW_ASSERT(ns);
  RW_ASSERT(ns[0] != '\0');
  UNUSED(prefix);
  // ATTN: Is there a maximum namespace id we should assert on?

  RW_ASSERT((id2ns_runtime_.size() + NSID_RUNTIME_BASE) == current_nsid_);
  rw_namespace_id_t nsid = current_nsid_;

  // Save in the vector first; if the map insert blows up, things are safe (although broken).
  id2ns_runtime_.emplace_back(new Namespace(ns, nullptr/*prefix*/, nsid));
  ++current_nsid_;

  // Now update the map; if this blows up, the namespace leaks, but things otherwise work
  auto status = ns2id_.emplace(ns, nsid);
  RW_ASSERT(status.second);
  return nsid;
}


Namespace* NamespaceManager::get_nsobj_locked(
  rw_namespace_id_t nsid)
{
  Namespace* nsobj = nullptr;
  if (nsid < NSID_RUNTIME_BASE) {
    if (nsid < id2ns_fixed_.size()) {
      nsobj = id2ns_fixed_[nsid].get();
    }
  } else {
    nsid -= NSID_RUNTIME_BASE;
    if (nsid < id2ns_runtime_.size()) {
      nsobj = id2ns_runtime_[nsid].get();
    }
  }
  return nsobj;
}


void NamespaceManager::unittest_runtime_clear()
{
  GlobalMutex::guard_t guard(GlobalMutex::g_mutex);
  for (auto& nsuptr: id2ns_runtime_) {
    auto count = ns2id_.erase(nsuptr->get_ns());
    RW_ASSERT(1 == count); // really should have found it - otherwise bugs lurk elsewhere
  }

  id2ns_runtime_.clear();
  current_nsid_ = NSID_RUNTIME_BASE;
}

const char* NamespaceManager::nsid_to_string(
  rw_namespace_id_t nsid)
{
  RW_ASSERT(nsid > NSID_NULL);
  GlobalMutex::guard_t guard(GlobalMutex::g_mutex);
  Namespace* nsobj = get_nsobj_locked(nsid);
  if (nsobj) {
    return nsobj->get_ns();
  }
  return nullptr;
}

rw_namespace_id_t NamespaceManager::string_to_nsid(
  const char* ns)
{
  RW_ASSERT(ns);
  RW_ASSERT(ns[0] != '\0');
  GlobalMutex::guard_t guard(GlobalMutex::g_mutex);
  auto idi = ns2id_.find(ns);
  if (idi == ns2id_.end()) {
    return NSID_NULL;
  }
  Namespace* nsobj = get_nsobj_locked(idi->second);
  RW_ASSERT(nsobj);
  return nsobj->get_nsid();
}

rw_namespace_id_t NamespaceManager::find_or_register(
  const char* ns,
  const char* prefix)
{
  RW_ASSERT(ns);
  RW_ASSERT(ns[0] != '\0');
  UNUSED(prefix);

  GlobalMutex::guard_t guard(GlobalMutex::g_mutex);
  auto idi = ns2id_.find(ns);
  if (idi == ns2id_.end()) {
    return register_runtime_locked(ns, prefix);
  }
  Namespace* nsobj = get_nsobj_locked(idi->second);
  RW_ASSERT(nsobj);
  return nsobj->get_nsid();
}

Namespace NamespaceManager::get_new_dynamic_schema_ns()
{
  GlobalMutex::guard_t guard(GlobalMutex::g_mutex);
  // ATTN: Check for maximun number of dynamic schemas??
  std::string sn = std::to_string(++runtime_dyns_counter_);
  std::string ns(dyn_schema_ns+sn);

  auto hashi = ns2hash_.find(ns);
  RW_ASSERT(hashi == ns2hash_.end());

  std::string prefix(dyn_schema_prefix+sn);
  rw_namespace_id_t nsid = get_ns_hash(ns.c_str());

  std::string mn(dyn_schema_mn+sn);

  Namespace nsobj(ns.c_str(), nullptr, nsid);
  nsobj.set_module(mn);

  return nsobj;
}

bool NamespaceManager::ns_is_dynamic(const char* ns)
{
  std::string namesp(ns);
  std::size_t pos = namesp.find(dyn_schema_ns);
  if (pos != std::string::npos) {
    return true;
  }
  return false;
}

rw_namespace_id_t NamespaceManager::get_ns_hash(
    const char* ns)
{
  RW_ASSERT(ns);
  RW_ASSERT(ns[0] != '\0');
  GlobalMutex::guard_t guard(GlobalMutex::g_mutex);
  auto hashi = ns2hash_.find(ns);
  if (hashi == ns2hash_.end()) {
    rw_namespace_id_t hashv = NSID_NULL;

    std::size_t hash = std::hash<std::string>()(std::string(ns));
    hashv = hash & 0x1FFFFFFF; // Take lower 29 bits.

    auto ret = ns2hash_.emplace(ns, hashv);
    RW_ASSERT(ret.second);
    hashi = ret.first;

    // Insert in the hash to ns list as well.
    auto ret2 = hash2ns_.emplace(hashv, ns);
    RW_ASSERT(ret2.second);
  }

  return hashi->second;
}

const char* NamespaceManager::nshash_to_string(
    rw_namespace_id_t nsid)
{
  RW_ASSERT(nsid > NSID_NULL);
  GlobalMutex::guard_t guard(GlobalMutex::g_mutex);
  auto nsi = hash2ns_.find(nsid);
  if (nsi == hash2ns_.end()) {
    return nullptr;
  }
  return nsi->second.c_str();
}

void rw_namespace_manager_init(void)
{
  NamespaceManager::get_global();
}

void rw_namespace_manager_unittest_runtime_clear(void)
{
  NamespaceManager::get_global().unittest_runtime_clear();
}

const char* rw_namespace_nsid_to_string(
  rw_namespace_id_t nsid)
{
  return NamespaceManager::get_global().nsid_to_string(nsid);
}

rw_namespace_id_t rw_namespace_string_to_nsid(
  const char* ns)
{
  return NamespaceManager::get_global().string_to_nsid(ns);
}

rw_namespace_id_t rw_namespace_find_or_register(
  const char* ns,
  const char* prefix)
{
  return NamespaceManager::get_global().find_or_register(ns, prefix);
}

rw_namespace_id_t rw_namespace_string_to_hash(
    const char* ns)
{
  return NamespaceManager::get_global().get_ns_hash(ns);
}
