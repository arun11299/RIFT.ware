#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <confd.h>
#include <confd_maapi.h>

#include "servers.h"

static int debuglevel = CONFD_TRACE;

static struct confd_daemon_ctx *dctx;
static int ctlsock;
static int workersock;
static struct confd_trans_cbs tcb;
static struct confd_data_cbs srv_hook;
static struct confd_data_cbs index_hook;

static int msock;
static struct sockaddr_in confd_addr;

static void _OK(int rval, int line)
{
    if (rval != CONFD_OK) {
        fprintf(stderr, "%s:%d: error not CONFD_OK: %d : %s \n",
                __FILE__, line, confd_errno, confd_lasterr());
        abort();
    }
}

#define OK(rval) _OK(rval, __LINE__)

static int do_maapi_connect(void)
{
    int sock;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0 ) {
        confd_fatal("Failed to open socket\n");
    }

    if (maapi_connect(sock, (struct sockaddr*)&confd_addr,
                      sizeof (struct sockaddr_in)) < 0) {
        confd_fatal("Failed to maapi_connect() to confd \n");
    }
    return sock;
}

static int init_trans(struct confd_trans_ctx *tctx)
{
    OK(maapi_attach(msock, srv__ns, tctx));
    confd_trans_set_fd(tctx, workersock);
    return CONFD_OK;
}

static int finish_trans(struct confd_trans_ctx *tctx)
{
    OK(maapi_detach(msock, tctx));
    return CONFD_OK;
}

int bit_map_set(unsigned char *bit_map, unsigned char **ret_bit_map,
                int len, int bit)
{
    /* the 'bit' number starts from 1, not 0 */
    int byte_pos = (bit - 1) / 8;
    int new_len = (byte_pos + 1) > len ? byte_pos + 1 : len;
    unsigned char * new_bit_map;

    new_bit_map = malloc(new_len * sizeof(unsigned char));
    /* caller should free this */
    memcpy(new_bit_map, bit_map, len);

    if (new_len > len) {
       int i;
       for (i = len; i < new_len; i++) {
           new_bit_map[i] = 0;
       }
    }

    new_bit_map[byte_pos] |= (1 << ((bit - 1) % 8));

    *ret_bit_map = new_bit_map;

    return new_len;
}

int lowest_free_bit(unsigned char *bit_map, int len)
{
    int byte_pos;
    int bit_pos = 0;

    for (byte_pos = 0; (byte_pos < len) && (bit_map[byte_pos] == 0xff);
            byte_pos++);

    if (byte_pos != len) {
        for (bit_pos = 0; ((bit_map[byte_pos] & (1 << bit_pos)) != 0);
                bit_pos++);
    }

    return 8 * byte_pos + bit_pos + 1;
}

int bit_map_clear(unsigned char *bit_map, unsigned char **ret_bit_map,
                  int len, int bit)
{
    int byte_pos = (bit - 1) / 8;
    int new_len;
    unsigned char rev_mask = 1 << ((bit - 1) % 8);
    unsigned char *new_bit_map;
    new_bit_map = malloc(len * sizeof(unsigned char));
    memcpy(new_bit_map, bit_map, len);
    new_bit_map[byte_pos] &= (~rev_mask);
    for (new_len = len; (new_bit_map[new_len - 1] == 0) && (new_len > 0);
            new_len--);
    *ret_bit_map = new_bit_map;
    return new_len;
}

static int server_hook_create(struct confd_trans_ctx *tctx,
                              confd_hkeypath_t *kp)
{
    int32_t idx;
    confd_value_t v;
    unsigned char *ifmap, *new_ifmap = NULL;
    int ifmap_len, new_ifmap_len;

    if (strcmp(tctx->uinfo->context, "snmp") == 0) {
        /*
         * This is a creation done via the SNMP agent. The index will
         * be added to the bitmap in the transaction hook on the index
         * leaf.
         */
        return CONFD_OK;
    } else {
        /*
         * We need to allocate a new index, and store it in the hidden leaf
         * 'index' in the server entry.
        */
        OK(maapi_get_elem(msock, tctx->thandle, &v, "/servers/ifmap"));
        ifmap = CONFD_GET_BINARY_PTR(&v);
        ifmap_len = CONFD_GET_BINARY_SIZE(&v);

        idx = lowest_free_bit(ifmap, ifmap_len);


        /* Also store the value in the bitmap already here, to prevent
         * that we reuse it if more instances are created before the
         * transaction is committed. */

        new_ifmap_len = bit_map_set(ifmap, &new_ifmap, ifmap_len, idx);
        confd_free_value(&v);
        CONFD_SET_BINARY(&v, new_ifmap, new_ifmap_len);
        OK(maapi_set_elem(msock, tctx->thandle, &v, "/servers/ifmap"));
        free(new_ifmap);

        CONFD_SET_INT32(&v, idx);
        OK(maapi_set_elem(msock, tctx->thandle, &v, "%h/index", kp));
        return CONFD_OK;
    }
}

static int server_hook_remove(struct confd_trans_ctx *tctx,
                              confd_hkeypath_t *kp)
{
    int32_t idx;
    confd_value_t v;
    unsigned char *ifmap, *new_ifmap = NULL;
    int ifmap_len, new_ifmap_len;

    /*
      We need to free the index, by zeroing it in the ifmap bitmap
    */

    OK(maapi_get_elem(msock, tctx->thandle, &v, "%h/index", kp));
    idx = CONFD_GET_INT32(&v);
    OK(maapi_get_elem(msock, tctx->thandle, &v, "/servers/ifmap"));
    ifmap = CONFD_GET_BINARY_PTR(&v);
    ifmap_len = CONFD_GET_BINARY_SIZE(&v);

    new_ifmap_len = bit_map_clear(ifmap, &new_ifmap, ifmap_len, idx);


    CONFD_SET_BINARY(&v, new_ifmap, ifmap_len);
    OK(maapi_set_elem(msock, tctx->thandle, &v, "/servers/ifmap"));
    free(new_ifmap);

    return CONFD_OK;
}

static int index_hook_set_elem(struct confd_trans_ctx *tctx,
                               confd_hkeypath_t *kp,
                               confd_value_t *newval)
{
    /* The set hooks above takes care of setting a value for
     * the index element when a new instance is created, and
     * for clearing bits in the bitmap when instances are removed.
     *
     * We also need to keep track of explicitly set indices,
     * via the SNMP agent.
     * Those can be handled in this transaction hook.
     * (We don't use a set hook because we would invoke that set
     * hook when calling maapi_set_elem in the create set hook, and
     * therefore need more than one worker socket and threads.) */
    if ((CONFD_GET_XMLTAG(&kp->v[0][0]) == srv_index) &&
            (strcmp(tctx->uinfo->context, "snmp") == 0)) {
        int32_t idx;
        confd_value_t v;
        unsigned char *ifmap, *new_ifmap = NULL;
        int ifmap_len, new_ifmap_len;

        OK(maapi_get_elem(msock, tctx->thandle, &v, "/servers/ifmap"));

        ifmap = CONFD_GET_BINARY_PTR(&v);
        ifmap_len = CONFD_GET_BINARY_SIZE(&v);

        idx = CONFD_GET_INT32(newval);

        new_ifmap_len = bit_map_set(ifmap, &new_ifmap, ifmap_len, idx);

        confd_free_value(&v);

        CONFD_SET_BINARY(&v, new_ifmap, new_ifmap_len);
        OK(maapi_set_elem(msock, tctx->thandle, &v, "/servers/ifmap"));
        free(new_ifmap);
    }
    return CONFD_OK;
}

static int service_hook_create(struct confd_trans_ctx *tctx,
                               confd_hkeypath_t *kp)
{
    int32_t idx;
    confd_value_t v;
    unsigned char *ifmap, *new_ifmap = NULL;
    int ifmap_len, new_ifmap_len;

    if (strcmp(tctx->uinfo->context, "snmp") == 0) {
        /*
         * This is a creation done via the SNMP agent. The index will
         * be added to the bitmap in the transaction hook on the index
         * leaf.
         */
        return CONFD_OK;
    } else {
        /*
         * We need to allocate a new index, and store it in the hidden leaf
         * 'index' in the service entry.
         *
         * This is about the creation of:
         * .../server{server_key}/service{service_key}, so we'll find
         * our bitmap at:
        */
        OK(maapi_get_elem(msock, tctx->thandle, &v, "%*h/service-map",
                          kp->len - 2, kp));

        ifmap = CONFD_GET_BINARY_PTR(&v);
        ifmap_len = CONFD_GET_BINARY_SIZE(&v);

        idx = lowest_free_bit(ifmap, ifmap_len);


        /* Also store the value in the bitmap already here, to prevent
         * that we reuse it if more instances are created before the
         * transaction is committed. */

        new_ifmap_len = bit_map_set(ifmap, &new_ifmap, ifmap_len, idx);

        confd_free_value(&v);
        CONFD_SET_BINARY(&v, new_ifmap, new_ifmap_len);
        OK(maapi_set_elem(msock, tctx->thandle, &v, "%*h/service-map",
                          kp->len - 2, kp));
        free(new_ifmap);

        CONFD_SET_INT32(&v, idx);
        OK(maapi_set_elem(msock, tctx->thandle, &v, "%h/index", kp));
        return CONFD_OK;
    }
}

static int service_hook_remove(struct confd_trans_ctx *tctx,
                               confd_hkeypath_t *kp)
{
    int32_t idx;
    confd_value_t v;
    unsigned char *ifmap, *new_ifmap = NULL;
    int ifmap_len, new_ifmap_len;

    /*
      We need to free the index, by zeroing it in the ifmap bitmap
    */

    OK(maapi_get_elem(msock, tctx->thandle, &v, "%h/index", kp));
    idx = CONFD_GET_INT32(&v);
    OK(maapi_get_elem(msock, tctx->thandle, &v, "%*h/service-map",
                      kp->len - 2, kp));
    ifmap = CONFD_GET_BINARY_PTR(&v);
    ifmap_len = CONFD_GET_BINARY_SIZE(&v);

    new_ifmap_len = bit_map_clear(ifmap, &new_ifmap, ifmap_len, idx);

    CONFD_SET_BINARY(&v, new_ifmap, ifmap_len);
    OK(maapi_set_elem(msock, tctx->thandle, &v, "%*h/service-map",
                      kp->len - 2, kp));
    free(new_ifmap);

    return CONFD_OK;
}

static int service_index_hook_set_elem(
        struct confd_trans_ctx *tctx, confd_hkeypath_t *kp,
        confd_value_t *newval)
{
    /* The set hooks above takes care of setting a value for
     * the index element when a new instance is created, and
     * for clearing bits in the bitmap when instances are removed.
     *
     * We also need to keep track of explicitly set indices,
     * via the SNMP agent, since we skip those in the set hook.
     *
     * Those can be handled in this transaction hook.
     * (We don't use a set hook because we would invoke that set
     * hook when calling maapi_set_elem in the create set hook, and
     * therefore need more than one worker socket and threads.) */
    if ((CONFD_GET_XMLTAG(&kp->v[0][0]) == srv_index) &&
            (strcmp(tctx->uinfo->context, "snmp") == 0)) {
        int32_t idx;
        confd_value_t v;
        unsigned char *ifmap, *new_ifmap = NULL;
        int ifmap_len, new_ifmap_len;

        OK(maapi_get_elem(msock, tctx->thandle, &v, "%*h/service-map",
                          kp->len - 3, kp));

        ifmap = CONFD_GET_BINARY_PTR(&v);
        ifmap_len = CONFD_GET_BINARY_SIZE(&v);

        idx = CONFD_GET_INT32(newval);

        new_ifmap_len = bit_map_set(ifmap, &new_ifmap, ifmap_len, idx);

        confd_free_value(&v);

        CONFD_SET_BINARY(&v, new_ifmap, new_ifmap_len);
        OK(maapi_set_elem(msock, tctx->thandle, &v, "%*h/service-map",
                          kp->len - 3, kp));
        free(new_ifmap);
    }
    return CONFD_OK;
}



int main(int argc, char **argv)
{
    int c;
    int num = -1;

    while ((c = getopt(argc, argv, "n:tdpsr")) != -1) {
        switch(c) {
        case 'n':
            num = atoi(optarg);
            break;
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

    confd_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    confd_addr.sin_family = AF_INET;
    confd_addr.sin_port = htons(CONFD_PORT);

    /* Initialize confdlib */

    confd_init((char *)"server-hook", stderr, debuglevel);

    /* Load data model schemas from to ConfD */

    if ((dctx = confd_init_daemon("server-hook")) == NULL)
        confd_fatal("Failed to initialize confd\n");

    if ((ctlsock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open ctlsocket\n");

    if (confd_load_schemas((struct sockaddr*)&confd_addr,
                           sizeof (struct sockaddr_in)) != CONFD_OK) {
        confd_fatal("Failed to load schemas from confd\n");
    }

    /* MAAPI connect to ConfD */

    msock = do_maapi_connect();

    /* Create the first control socket, all requests to */
    /* create new transactions arrive here */

    if (confd_connect(dctx, ctlsock, CONTROL_SOCKET,
                      (struct sockaddr*)&confd_addr,
                      sizeof (struct sockaddr_in)) < 0)
        confd_fatal("Failed to confd_connect() to confd \n");

    /* Also establish a workersocket, this is the most simple */
    /* case where we have just one ctlsock and one workersock */

    if ((workersock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open workersocket\n");
    if (confd_connect(dctx, workersock, WORKER_SOCKET,
                      (struct sockaddr*)&confd_addr,
                      sizeof (struct sockaddr_in)) < 0)
        confd_fatal("Failed to confd_connect() to confd \n");

    tcb.init = init_trans;
    tcb.finish = finish_trans;
    confd_register_trans_cb(dctx, &tcb);

    /* Register the Interface hook */

    srv_hook.create   = server_hook_create;
    srv_hook.remove   = server_hook_remove;

    strcpy(srv_hook.callpoint, "srv-hook");

    index_hook.set_elem = index_hook_set_elem;

    strcpy(index_hook.callpoint, "index-hook");

    if (confd_register_data_cb(dctx, &srv_hook) == CONFD_ERR)
        confd_fatal("Failed to register hook cb \n");

    if (confd_register_data_cb(dctx, &index_hook) == CONFD_ERR)
        confd_fatal("Failed to register hook cb \n");

    srv_hook.create = service_hook_create;
    srv_hook.remove = service_hook_remove;
    strcpy(srv_hook.callpoint, "service-hook");

    index_hook.set_elem = service_index_hook_set_elem;
    strcpy(index_hook.callpoint, "service-index-hook");

    if (confd_register_data_cb(dctx, &srv_hook) == CONFD_ERR)
        confd_fatal("Failed to register hook cb \n");

    if (confd_register_data_cb(dctx, &index_hook) == CONFD_ERR)
        confd_fatal("Failed to register hook cb \n");

    if (confd_register_done(dctx) != CONFD_OK)
        confd_fatal("Failed to complete registration \n");


    while (1) {
        struct pollfd set[2];
        int ret;

        set[0].fd = ctlsock;
        set[0].events = POLLIN;
        set[0].revents = 0;

        set[1].fd = workersock;
        set[1].events = POLLIN;
        set[1].revents = 0;


        if (poll(&set[0], 2, -1) < 0) {
            perror("Poll failed:");
            continue;
        }

        if (set[0].revents & POLLIN) {
            if ((ret = confd_fd_ready(dctx, ctlsock)) == CONFD_EOF) {
                confd_fatal("Control socket closed\n");
            } else if (ret == CONFD_ERR && confd_errno != CONFD_ERR_EXTERNAL) {
                confd_fatal("Error on control socket request: %s (%d): %s\n",
                            confd_strerror(confd_errno),
                            confd_errno, confd_lasterr());
            }
        }
        if (set[1].revents & POLLIN) {
            if ((ret = confd_fd_ready(dctx, workersock)) == CONFD_EOF) {
                confd_fatal("Worker socket closed\n");
            } else if (ret == CONFD_ERR && confd_errno != CONFD_ERR_EXTERNAL) {
                confd_fatal("Error on worker socket request: %s (%d): %s\n",
                            confd_strerror(confd_errno),
                            confd_errno, confd_lasterr());
            }
        }

    }
}
