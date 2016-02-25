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

#include <confd_lib.h>
#include <confd_events.h>

static int notsock;
static int debuglevel;


char *vb_type(struct confd_snmp_varbind *vb) {
    switch (vb->vartype) {
    case CONFD_SNMP_NULL: return "NULL";
    case CONFD_SNMP_INTEGER: return "INTEGER";
    case CONFD_SNMP_Interger32: return "Integer32";
    case CONFD_SNMP_OCTET_STRING: return "OCTET STRING";
    case CONFD_SNMP_OBJECT_IDENTIFIER: return "OBJECT IDENTIFIER";
    case CONFD_SNMP_IpAddress: return "IpAddress";
    case CONFD_SNMP_Counter32: return "Counter32";
    case CONFD_SNMP_TimeTicks: return "TimeTicks";
    case CONFD_SNMP_Opaque: return "Opaque";
    case CONFD_SNMP_Counter64: return "Counter64";
    case CONFD_SNMP_Unsigned32: return "Unsigned32";
    }
    return "";
}

char *pdutype(struct confd_snmpa_notification *snmp) {
    switch (snmp->pdu_type) {
    case CONFD_SNMPA_PDU_V1TRAP: return("V1TRAP");
    case CONFD_SNMPA_PDU_V2TRAP: return("V2TRAP");
    case CONFD_SNMPA_PDU_INFORM: return("INFORM");
    case CONFD_SNMPA_PDU_GET_RESPONSE: return("GET_RESPONSE");
    case CONFD_SNMPA_PDU_GET_REQUEST: return("GET_REQUEST");
    case CONFD_SNMPA_PDU_GET_NEXT_REQUEST: return("GET_NEXT_REQUEST");
    case CONFD_SNMPA_PDU_REPORT: return("REPORT");
    case CONFD_SNMPA_PDU_GET_BULK_REQUEST: return("GET_BULK_REQUEST");
    case CONFD_SNMPA_PDU_SET_REQUEST: return("SET_REQUEST");
    default: return "";
    }
}


int main(int argc, char **argv)
{
    int c;

    struct sockaddr_in addr;

    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);

    while ((c = getopt(argc, argv, "tdpsr")) != -1) {
        switch(c) {
        case 'r':
            exit(0);
        case 't':
            debuglevel = CONFD_TRACE;
            break;
        case 'd':
            debuglevel = CONFD_DEBUG;
            break;
        case 'p':
            debuglevel = CONFD_PROTO_TRACE;
            break;
        case 's':
            debuglevel = CONFD_SILENT;
            break;
        }
    }

    confd_init("Foobar", stderr, debuglevel);
    if ((notsock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open notsocket\n");

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CONFD_PORT);


    int dflag = CONFD_NOTIF_SNMPA;
    if (confd_load_schemas((struct sockaddr*)&addr,
                           sizeof (struct sockaddr_in)) != CONFD_OK)
        confd_fatal("Failed to load schemas from confd\n");
    if (confd_notifications_connect(
            notsock,
            (struct sockaddr*)&addr,
            sizeof (struct sockaddr_in), dflag) < 0 ) {
        confd_fatal("Failed to confd_notifications_connect() to confd \n");
    }
    printf("Waiting for SNMP audit events...\n");

    while (1) {
        struct pollfd set[1];
        struct confd_notification n;
        set[0].fd = notsock;
        set[0].events = POLLIN;
        set[0].revents = 0;

        if (poll(&set[0], 1, -1) < 0) {
            perror("Poll failed:");
            continue;
        }

        if (set[0].revents & POLLIN) {
            if (confd_read_notification(notsock, &n) != CONFD_OK)
                exit(1);
            switch(n.type) {
            case CONFD_NOTIF_SNMPA: {
                int i,j;

                char buf[BUFSIZ];
                buf[0] = 0;
                char *ptr = &buf[0];
                struct confd_snmpa_notification *snmp = &n.n.snmpa;
                ptr += sprintf(ptr, "%s ", pdutype(snmp));
                ptr += sprintf(ptr,"Id = %d ", snmp->request_id);
                struct confd_ip *ip = &(snmp->ip);
                ptr += sprintf(ptr, " %s:%d ",
                               inet_ntoa(ip->ip.v4),
                               snmp->port);
                if ((snmp->error_status !=0 || snmp->error_index != 0)) {
                    ptr += sprintf(ptr, "ErrIx = %d ", snmp->error_index);
                }
                else if (snmp->pdu_type == CONFD_SNMPA_PDU_V1TRAP) {
                    ptr += sprintf(ptr,"Generic=%d Specific=%d",
                                   snmp->v1_trap->generic_trap,
                                   snmp->v1_trap->specific_trap);
                    struct confd_snmp_oid *enterp = &snmp->v1_trap->enterprise;
                    ptr += sprintf(ptr, " Enterprise=");
                    for(i=0; i < enterp->len; i++) {
                        ptr += sprintf(ptr,".%d", enterp->oid[i]);
                    }
                }
                for (i=0; i < snmp->num_variables; i++) {
                    struct confd_snmp_varbind *vb = &snmp->vb[i];
                    ptr += sprintf(ptr,"\n   ");
                    switch (vb->type) {
                    case CONFD_SNMP_VARIABLE:
                        ptr += sprintf(ptr, " %s ", vb_type(vb));
                        ptr += sprintf(ptr,"%s=", vb->var.name);
                        break;
                    case CONFD_SNMP_OID:
                        ptr += sprintf(ptr, " %s ", vb_type(vb));
                        for (j=0; j < vb->var.oid.len; j++) {
                            ptr += sprintf(ptr,"%d", vb->var.oid.oid[j]);
                            if (j != vb->var.oid.len-1)
                                ptr += sprintf(ptr,".");
                        }
                        break;
                    case CONFD_SNMP_COL_ROW:
                        ptr += sprintf(ptr, " %s ", vb_type(vb));
                        ptr += sprintf(ptr, "%s", vb->var.cr.column);
                        for(j=0; j<vb->var.cr.rowindex.len; j++) {
                            ptr += sprintf(ptr,".%d",
                                           vb->var.cr.rowindex.oid[j]);
                        }
                        break;
                    }
                    if (vb->val.type == C_BUF) {
                        char buf2[BUFSIZ];
                        confd_pp_value(buf2, BUFSIZ, &vb->val);
                        ptr += sprintf(ptr, "=%s", buf2);
                    }
                }
                printf("%s\n\n", buf);
                confd_free_notification(&n);
            }
                break;

            default:
                printf("odd type %d\n", n.type);
                exit(2);
            }
        }
    }
}
