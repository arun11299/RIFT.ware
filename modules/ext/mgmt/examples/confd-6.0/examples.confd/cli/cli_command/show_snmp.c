
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#include <stdio.h>

#include "confd.h"
#include "confd_maapi.h"
#include "SNMPv2-MIB.h"
#include "SNMP-FRAMEWORK-MIB.h"

int main(int argc, char **argv)
{
    int msock;
    int debuglevel = CONFD_DEBUG;
    struct sockaddr_in addr;
    int usid, thandle;
    u_int32_t incntr;
    int32_t ucntr;
    unsigned char *rbuf;
    int rsize;

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4565);

    confd_init("show_snmp", stderr, debuglevel);
    if ((msock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open socket\n");

    if (maapi_connect(msock, (struct sockaddr*)&addr,
                      sizeof (struct sockaddr_in)) < 0)
        confd_fatal("Failed to confd_connect() to confd: %s\n",
                    confd_lasterr());

    usid = atoi(getenv("CONFD_MAAPI_USID"));
    thandle = atoi(getenv("CONFD_MAAPI_THANDLE"));

    if ((maapi_attach2(msock, SNMPv2_MIB__ns, usid, thandle)) != CONFD_OK)
        confd_fatal("Failed to attach: %s\n", confd_lasterr());


    printf("Chassis: %d\n", 1506199);


    /* Mimic output form Cisco show snmp */

    /* Input */

    if (maapi_get_u_int32_elem(msock, thandle, &incntr,
                               "/SNMPv2-MIB/snmp/snmpInPkts") != CONFD_OK)
        confd_fatal("failed to read SNMP input counter");
    printf("%d SNMP packets input\n", incntr);

    if (maapi_get_u_int32_elem(msock, thandle, &incntr,
                               "/SNMPv2-MIB/snmp/snmpInBadVersions") !=
        CONFD_OK)
        confd_fatal("failed to read SNMP input counter");
    printf("    %d Bad SNMP version errors\n", incntr);

    if (maapi_get_u_int32_elem(msock, thandle, &incntr,
                               "/SNMPv2-MIB/snmp/snmpInBadCommunityNames") !=
        CONFD_OK)
        confd_fatal("failed to read SNMP input counter");
    printf("    %d Unknown community name\n", incntr);

    if (maapi_get_u_int32_elem(msock, thandle, &incntr,
                               "/SNMPv2-MIB/snmp/snmpInBadCommunityUses") !=
        CONFD_OK)
        confd_fatal("failed to read SNMP input counter");
    printf("    %d Illegal operation for community name supplied\n", incntr);

    if (maapi_get_u_int32_elem(msock, thandle, &incntr,
                               "/SNMPv2-MIB/snmp/snmpInASNParseErrs") !=
        CONFD_OK)
        confd_fatal("failed to read SNMP input counter");
    printf("    %d Encoding errors\n", incntr);

    if (maapi_get_u_int32_elem(msock, thandle, &incntr,
                               "/SNMPv2-MIB/snmp/snmpSilentDrops") !=
        CONFD_OK)
        confd_fatal("failed to read SNMP input counter");
    printf("    %d Number of dropped packets\n", incntr);

    if (maapi_get_u_int32_elem(msock, thandle, &incntr,
                               "/SNMPv2-MIB/snmp/snmpProxyDrops") !=
        CONFD_OK)
        confd_fatal("failed to read SNMP input counter");
    printf("    %d Number of dropped proxy packets\n", incntr);


    /* Output is deprecated and not included in the YANG data model. We
     *  keep the counters internally in the SNMP agent and can add
     *  them to the SNMPv2-MIB on request.
     */

    printf("\n");

    /* various */

    if (maapi_get_enum_value_elem(msock, thandle, &ucntr,
                                 "/SNMPv2-MIB/snmp/snmpEnableAuthenTraps") !=
        CONFD_OK)
        confd_fatal("failed to read SNMP enabled flag");
    if (ucntr == 1)
        printf("SNMP Authentication traps: enabled\n");
    else
        printf("SNMP Authentication traps: disabled\n");

    if (maapi_set_namespace(msock, thandle, SNMP_FRAMEWORK_MIB__ns) !=
        CONFD_OK)
        confd_fatal("failed to set namespace");

    if (maapi_get_binary_elem(msock, thandle, &rbuf, &rsize,
                              "/SNMP-FRAMEWORK-MIB/snmpEngine/snmpEngineID") !=
        CONFD_OK)
      confd_fatal("failed to read SNMP engine id");

    printf("SNMP Engine ID: %02x:%02x:%02x:%02x:%02x:%02x\n",
           rbuf[0], rbuf[1], rbuf[2], rbuf[3], rbuf[4], rbuf[5]);

    if (maapi_get_int32_elem(msock, thandle, &ucntr,
                             "/SNMP-FRAMEWORK-MIB/snmpEngine/snmpEngineBoots")
        != CONFD_OK)
        confd_fatal("failed to read SNMP input counter");
    printf("SNMP Engine boots: %d\n", ucntr);

    if (maapi_get_int32_elem(msock, thandle, &ucntr,
                             "/SNMP-FRAMEWORK-MIB/snmpEngine/snmpEngineTime")
        != CONFD_OK)
        confd_fatal("failed to read SNMP input counter");
    printf("SNMP Engine time: %d\n", ucntr);

    exit(0);
}

