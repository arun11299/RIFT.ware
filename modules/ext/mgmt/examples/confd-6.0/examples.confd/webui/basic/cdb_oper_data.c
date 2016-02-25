/*********************************************************************
 * ConfD Web UI intro example
 * Implement live data for operational stored in CDB
 *
 * (C) 2005-2009 Tail-f Systems
 * Permission to use this code as a starting point hereby granted
 *
 * See the README file for more information
 ********************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <confd.h>
#include <confd_cdb.h>
#include "config.h"

#define OK(rval) do {                                                   \
        if ((rval) != CONFD_OK)                                         \
            confd_fatal("cdb_oper_data: error not CONFD_OK: %d : %s\n", \
                        confd_errno, confd_lasterr());                  \
    } while (0);

void gen_value(confd_value_t *value) {
    long rxCounter = 0;
    rxCounter += random() % 50000;
    if (rxCounter > 1000000)
        rxCounter = 100000;
    CONFD_SET_UINT64(value, rxCounter);
}

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in addr;
    confd_value_t eth0_value;
    confd_value_t eth1_value;
    confd_value_t vsid1_value;
    confd_value_t vsid2_value;

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CONFD_PORT);

    confd_init("cdb_oper_data", stderr, CONFD_DEBUG); // CONFD_TRACE);
    OK(confd_load_schemas((struct sockaddr*)&addr, sizeof(struct sockaddr_in)));

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        confd_fatal("Failed to create socket");

    OK(cdb_connect(sock, CDB_DATA_SOCKET, (struct sockaddr *)&addr,
                   sizeof(struct sockaddr_in)));
    OK(cdb_start_session2(
        sock, CDB_OPERATIONAL, CDB_LOCK_REQUEST | CDB_LOCK_WAIT
    ));
    OK(cdb_set_namespace(sock, config__ns));

    while (1) {

        gen_value(&eth0_value);
        gen_value(&eth1_value);
        gen_value(&vsid1_value);
        gen_value(&vsid2_value);

        OK(
            cdb_set_elem(
                sock,
                &eth0_value,
                "/chassis/slot{1}/interface{eth0}/rx"
            )
        );
        OK(
            cdb_set_elem(
                sock,
                &eth1_value,
                "/chassis/slot{1}/interface{eth1}/rx"
            )
        );
        OK(
            cdb_set_elem(
                sock,
                &vsid1_value,
                "/chassis/slot{2}/interface{vsid1}/rx"
            )
        );
        OK(
            cdb_set_elem(
                sock,
                &vsid2_value,
                "/chassis/slot{2}/interface{vsid2}/rx"

            )
        );
        sleep(2);
    }
}


