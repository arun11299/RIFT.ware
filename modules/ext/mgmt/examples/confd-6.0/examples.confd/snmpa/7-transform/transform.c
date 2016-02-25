#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>

#include <confd_lib.h>
#include <confd_dp.h>
#include <confd_maapi.h>

#include "dlist.h"

/* high-level data model definitions */
#include "ospf.h"
#include "interface.h"

/* low-level data model definitions */
#include "OSPF-MIB.h"
#include "IP-MIB.h"
#include "IF-MIB.h"
#include "INET-ADDRESS-MIB.h"
#include "SNMPv2-MIB.h"

static int debuglevel = CONFD_TRACE;

static struct confd_daemon_ctx *dctx;
static int ctlsock;
static int workersock;
static struct confd_trans_cbs tcb;
static struct confd_data_cbs  if_transform;
static struct confd_data_cbs  if_hook;
static struct confd_data_cbs  ip_transform;
static struct confd_data_cbs  ospf_transform;
static struct confd_data_cbs  ospf_area_hook;

static int msock;
static struct sockaddr_in confd_addr;


/* Per-transaction data that we keep */
/* We may need multiple cursors for ospf_get_next, since the
   SNMP manager may run multiple parallel getnext walks */
struct saved_cursor {
    struct maapi_cursor mc;
    struct saved_cursor *next;
};
struct trans_data {
    struct in_addr *ipaddr_list;
    int ipaddr_list_len;
    struct saved_cursor *sc;
};

struct delayed_ospf_iface {
    int inuse;
    confd_value_t ipAddr;
    confd_value_t ifIndex;
    confd_value_t areaId;
    confd_value_t ifType;
    confd_value_t adminStat;
    confd_value_t priority;
};

static struct delayed_ospf_iface delayed_ospf_if;


static void _OK(int rval, int line)
{
    if (rval != CONFD_OK) {
        fprintf(stderr, "%s:%d: error not CONFD_OK: %d : %s \n",
                __FILE__, line, confd_errno, confd_lasterr());
        abort();
    }
}
#define is_xml_tag(_vp)  ((_vp)->type == C_XMLTAG)      \
// safe version of CONFD_GET_XMLTAG( macro
#define xml_tag(_vp) (is_xml_tag(_vp) ? CONFD_GET_XMLTAG(_vp) :\
                      (unsigned int)-1)

#define OK(rval) _OK(rval, __LINE__)

/*
 * Utility functions.
 */

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

static int start_maapi_system_session(int sock)
{
    struct confd_ip ip;

    ip.af = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &ip.ip.v4);

    OK(maapi_start_user_session(sock, NULL, "system", NULL, 0,
                                &ip, CONFD_PROTO_TCP));
    return maapi_start_trans(sock, CONFD_RUNNING, CONFD_READ);
}

/* An ipv4 address is represented as a struct in_addr when the
 * type is inet:ipv4-address, and as a 4-byte binary when the type is
 * INET-ADDRESS-MIB:InetAddress.
 * This function converts between these representations.
 * Return: <= 0 on error
 */
static int inet_addressv4_convert(struct in_addr *in,
                                  unsigned char *bin,
                                  int from_bin)
{
    char buf[BUFSIZ];
    int ints[4];
    int i;

    if (from_bin) {
        sprintf(buf, "%d.%d.%d.%d", bin[0], bin[1], bin[2], bin[3]);
        return inet_pton(AF_INET, buf, in);
    } else {
        sscanf(inet_ntoa(*in), "%d.%d.%d.%d",
               &ints[0], &ints[1], &ints[2], &ints[3]);
        for (i = 0; i < 4; i++) {
            bin[i] = (unsigned char)ints[i];
        }
        return 1;
    }
}


/*
 * Interface map
 */

struct ifmap_entry {
    int32_t ifIndex;
    unsigned char *ifName;
};

/*
 * A double-linked list of ifmap_entry.
 * This is a global variable which we keep in sync with ConfD by
 * using hooks.
 */
static Dlist *ifmap;

#if 0
static void ifmap_print(void)
{
    Dlist *p;
    struct ifmap_entry *tmp;

    printf("\nIFMAP\n");
    dl_traverse(p, ifmap) {
        tmp = (struct ifmap_entry *)dl_val(p);
        printf("%d: %s\n", tmp->ifIndex, tmp->ifName);
    }
    printf("END\n");
}
#endif

static void ifmap_add(struct ifmap_entry *new)
{
    Dlist *p;

    dl_rtraverse(p, ifmap) {
        if (((struct ifmap_entry *)dl_val(p))->ifIndex < new->ifIndex) {
            dl_insert_a(p, new);
            return;
        }
    }
    dl_prepend(ifmap, new);
}

static void ifmap_del(unsigned char *ifName)
{
    Dlist *p;
    struct ifmap_entry *tmp;
    unsigned int len;

    len = strlen((char *)ifName);

    dl_traverse(p, ifmap) {
        tmp = (struct ifmap_entry *)dl_val(p);
        if (strncmp((char *)tmp->ifName, (char *)ifName, len) == 0 &&
            strlen((char *)tmp->ifName) == len) {
            free(tmp->ifName);
            free(tmp);
            dl_delete_node(p);
            return;
        }
    }
}

static unsigned char *ifmap_get_ifname(int32_t ifIndex)
{
    Dlist *p;
    struct ifmap_entry *tmp;
    dl_traverse(p, ifmap) {
        tmp = (struct ifmap_entry *)dl_val(p);
        if (tmp->ifIndex == ifIndex) {
            return tmp->ifName;
        }
    }
    return NULL;
}

static void ifmap_build_map(void)
{
    struct maapi_cursor mc;
    int n;
    struct ifmap_entry *p;
    Dlist *tmp;
    int thandle;

    /* Free the old map */
    while (!dl_empty(ifmap)) {
        tmp = dl_first(ifmap);
        p = dl_val(tmp);
        free(p->ifName);
        free(p);
        dl_delete_node(tmp);
    }

    thandle = start_maapi_system_session(msock);

    OK(maapi_set_namespace(msock, thandle, if__ns));
    /* Loop over all interfaces */
    OK(maapi_init_cursor(msock, thandle, &mc, "/interface"));
    OK(maapi_get_next(&mc));
    while (mc.n != 0) {
        /* Allocate and fill in a new ifmap */
        p = (struct ifmap_entry *)malloc(sizeof(struct ifmap_entry));
        OK(maapi_get_int32_elem(msock, thandle, &p->ifIndex,
                                "/interface{%x}/ifIndex", &mc.keys[0]));
        OK(maapi_get_buf_elem(msock, thandle, &p->ifName, &n,
                              "/interface{%x}/name", &mc.keys[0]));
        ifmap_add(p);
        OK(maapi_get_next(&mc));
    }
    maapi_destroy_cursor(&mc);
    maapi_finish_trans(msock, thandle);
    maapi_end_user_session(msock);

}

/*
   Initializes the SNMP fields ipAddressCreated and
   ipAddressLastChanged.  These objects should return 0 if the mgmt
   subsystem was reinitalized, according to the MIB.  This means they
   should be 0 everytime ConfD (re)starts.  Thus, at startup, we set
   them to 0.
*/
static void ifmap_init_snmp_data(void)
{
    struct maapi_cursor mc;
    int ret;
    confd_value_t v;
    int thandle;

    /* start a MAAPI session to read from running */
    thandle = start_maapi_system_session(msock);

    OK(maapi_set_namespace(msock, thandle, if__ns));

    /* Loop over all existing interfaces */
    OK(maapi_init_cursor(msock, thandle, &mc, "/interface"));
    OK(maapi_get_next(&mc));
    while (mc.n != 0) {
        ret = maapi_get_elem(msock, thandle, &v,
                             "/interface{%x}/address", &mc.keys[0]);
        if (ret == CONFD_ERR && confd_errno == CONFD_ERR_NOEXISTS) {
            /* no address exists, skip the stats data */
            OK(maapi_get_next(&mc));
            continue;
        }
        OK(ret);

        /* the address exists, create the stats objects if they don't exist */
        ret = maapi_get_elem(msock, thandle, &v,
                             "/interface{%x}/ipAddressCreated", &mc.keys[0]);
        if (ret == CONFD_ERR && confd_errno == CONFD_ERR_NOEXISTS) {
            CONFD_SET_UINT32(&v, 0);
            OK(maapi_set_elem(msock, thandle, &v,
                              "/interface{%x}/ipAddressCreated",
                              &mc.keys[0]));
        }
        ret = maapi_get_elem(msock, thandle, &v,
                             "/interface{%x}/ipAddressLastChanged",
                             &mc.keys[0]);
        if (ret == CONFD_ERR && confd_errno == CONFD_ERR_NOEXISTS) {
            CONFD_SET_UINT32(&v, 0);
            OK(maapi_set_elem(msock, thandle, &v,
                              "/interface{%x}/ipAddressLastChanged",
                              &mc.keys[0]));
        }
        OK(maapi_get_next(&mc));
    }

    maapi_destroy_cursor(&mc);
    maapi_finish_trans(msock, thandle);
    maapi_end_user_session(msock);

}

static void ifmap_init(void)
{
    ifmap = new_dlist();
    ifmap_build_map();
    ifmap_init_snmp_data();
}

/*
 * Transformation admin functions
 */

static struct maapi_cursor *alloc_cursor(int msock,
                                         struct confd_trans_ctx *tctx,
                                         const char *path)
{
    struct trans_data *td = (struct trans_data*)tctx->t_opaque;
    struct saved_cursor *sc = malloc(sizeof(struct saved_cursor));

    OK(maapi_init_cursor(msock, tctx->thandle, &sc->mc, path));
    sc->next = td->sc;
    td->sc = sc;
    return &sc->mc;
}

static void free_cursor(struct confd_trans_ctx *tctx, struct maapi_cursor *mc)
{
    struct trans_data *td = (struct trans_data*)tctx->t_opaque;
    struct saved_cursor **prev = &td->sc, *sc;

    maapi_destroy_cursor(mc);
    while (*prev != NULL) {
        sc = *prev;
        if (&sc->mc == mc) {
            *prev = sc->next;
            free(sc);
            return;
        }
        prev = &sc->next;
    }
}

static int init_transformation(struct confd_trans_ctx *tctx)
{
    OK(maapi_attach(msock, 0, tctx));
    confd_trans_set_fd(tctx, workersock);
    tctx->t_opaque = (struct trans_data*)malloc(sizeof(struct trans_data));
    memset(tctx->t_opaque, 0, sizeof(struct trans_data));
    memset(&delayed_ospf_if, 0, sizeof(struct delayed_ospf_iface));
    return CONFD_OK;
}

static int stop_transformation(struct confd_trans_ctx *tctx)
{
    struct trans_data *td;

    td = (struct trans_data *)tctx->t_opaque;
    if (td->ipaddr_list != NULL) {
        free(td->ipaddr_list);
    }
    while (td->sc != NULL) {
        free_cursor(tctx, &td->sc->mc);
    }
    free(td);
    OK(maapi_detach(msock, tctx));
    return CONFD_OK;
}

/*
 * Hooks to keep the interface map up to date
 */

/* Called when a (high-level) /interface is created */
static int if_create_hook(struct confd_trans_ctx *tctx,
                          confd_hkeypath_t *keypath)
{
    struct ifmap_entry *new;
    int32_t idx;
    confd_value_t v;
    int n;

    /*
      Invoke this only when running is modified.
    */
    if (tctx->dbname != CONFD_RUNNING) {
        return CONFD_OK;
    }

    /*
      We need to allocate a new ifIndex, and store it in the
      hidden leaf 'ifIndex' in the interface entry.
      Also add the ifIndex and ifName to the ifmap.
    */
    if (dl_empty(ifmap)) {
        idx = 1;
    } else {
        idx = ((struct ifmap_entry *)dl_val(dl_last(ifmap)))->ifIndex + 1;
    }

    CONFD_SET_INT32(&v, idx);
    OK(maapi_set_elem(msock, tctx->thandle, &v,
                      "%h/ifIndex", keypath));

    new = (struct ifmap_entry *)malloc(sizeof(struct ifmap_entry));
    new->ifIndex = idx;
    OK(maapi_get_buf_elem(msock, tctx->thandle, &new->ifName, &n,
                          "%h/name", keypath));
    ifmap_add(new);

    return CONFD_OK;
}

/* Called when a (high-level) /interface or interface address is deleted */
static int if_remove_hook(struct confd_trans_ctx *tctx,
                          confd_hkeypath_t *keypath)
{
    confd_value_t *last = &(keypath->v[0][0]);
    confd_value_t *last_prev = &(keypath->v[1][0]);

    if (tctx->dbname != CONFD_RUNNING) {
        return CONFD_OK;
    }

    if (last->type == C_BUF &&
        xml_tag(last_prev) == if_interface) {
        /* Get the ifName to be removed, and delete it from the map */
        ifmap_del(CONFD_GET_BUFPTR(&keypath->v[0][0]));
    } else if (xml_tag(last) == if_address) {
        /* When the address is set, we need to delete the timestamps */
        maapi_delete(msock, tctx->thandle,
                     "%*h/ipAddressCreated", keypath->len - 1, keypath);
        maapi_delete(msock, tctx->thandle,
                     "%*h/ipAddressLastChanged", keypath->len - 1, keypath);
    }

    return CONFD_OK;
}

/* Called when a (high-level) /interface is modified */
static int if_set_elem_hook(struct confd_trans_ctx *tctx,
                            confd_hkeypath_t *keypath,
                            confd_value_t *newval)
{
    confd_value_t sysUpTime;
    confd_value_t *last = &(keypath->v[0][0]);

    switch (xml_tag(last)) {
    case if_address:
        /* When the address is set, we need to record the timestamp */
        OK(maapi_get_elem(msock, tctx->thandle, &sysUpTime,
                          "/SNMPv2_MIB:SNMPv2-MIB/system/sysUpTime"));
        OK(maapi_set_elem(msock, tctx->thandle, &sysUpTime,
                          "%*h/ipAddressCreated", keypath->len - 1,
                          keypath));
        OK(maapi_set_elem(msock, tctx->thandle, &sysUpTime,
                          "%*h/ipAddressLastChanged", keypath->len - 1,
                          keypath));
        break;
    }
    return CONFD_OK;
}

/*
 * Interface Transformation code
 *
 * This code acts as a data provider for the SNMP data model view,
 * by transforming to and from the high-level data model.
 */

/*
 * Callpoint: if-snmp
 * Called for: ifTable, ifXTable
 */
static int if_get_next(struct confd_trans_ctx *tctx,
                       confd_hkeypath_t *keypath,
                       long next)
{
    confd_value_t v[2];
    confd_value_t *list = &(keypath->v[0][0]);

    switch (xml_tag(list)) {
    case IF_MIB_ifEntry:
    case IF_MIB_ifXEntry: {
        /* These two tables have the same index */
        Dlist *p;
        if (next == -1) {
            p = ifmap;
        } else {
            p = (Dlist *)next;
        }
        if (dl_next(p) == ifmap) {
            return confd_data_reply_next_key(tctx, NULL, -1, -1);
        } else {
            p = dl_next(p);
            CONFD_SET_INT32(&v[0], ((struct ifmap_entry *)dl_val(p))->ifIndex);
            return confd_data_reply_next_key(tctx, v, 1, (long)p);
        }
    }
    default:
        confd_data_reply_next_key(tctx, NULL, -1, -1);
        return CONFD_OK;
    }
}


/*
 * Callpoint: if-snmp
 * Called for: leafs in ifTable, ifXTable
 */
static int if_get_elem(struct confd_trans_ctx *tctx,
                       confd_hkeypath_t *keypath)
{
    confd_value_t v;
    unsigned char *p;
    char buf[BUFSIZ];

    confd_value_t *leaf = &(keypath->v[0][0]);
    confd_value_t *key = &(keypath->v[1][0]);

    switch (xml_tag(leaf)) {
    case IF_MIB_ifIndex:
        /* This is an existance test on ifXTable.
           `key` is the value of ifIndex in ifXTable.  Use this value to
           see
        */
        if (ifmap_get_ifname(CONFD_GET_INT32(key)) == NULL) {
            fprintf(stderr, "Could not map ifIndex %d\n",
                    CONFD_GET_INT32(key));
            return confd_data_reply_not_found(tctx);
        }
        return confd_data_reply_value(tctx, key);

    case IF_MIB_ifDescr:
        /* Just reply with a dummy value if the interface exists */
        if (ifmap_get_ifname(CONFD_GET_INT32(key)) == NULL) {
            return confd_data_reply_not_found(tctx);
        }
        CONFD_SET_STR(&v, (char *)"dummy interface");
        return confd_data_reply_value(tctx, &v);

    case IF_MIB_ifName:
        /* serve ifName from ifXTable.
           `key` is the value of ifIndex in ifXTable.  Use this value to
           find the corresponding ifName.
        */
        if ((p = ifmap_get_ifname(CONFD_GET_INT32(key))) == NULL) {
            /* The entry does not exist. */
            fprintf(stderr, "Could not map ifIndex %d\n",
                    CONFD_GET_INT32(key));
            return confd_data_reply_not_found(tctx);
        }
        CONFD_SET_STR(&v, (char *)p);
        return confd_data_reply_value(tctx, &v);

    default: {
        confd_pp_kpath(buf, BUFSIZ, keypath);
        fprintf(stderr,"Unexpected switch tag (%d) %s\n",
                xml_tag(leaf), buf);
        return confd_data_reply_not_found(tctx);
    }
    }
}

/*
 * IP Address Transformation code
 *
 * This code acts as a data provider for the SNMP data model view,
 * by transforming to and from the high-level data model.
 */

static int cmp_in_addr(const void *a, const void *b)
{
    const struct in_addr *in1 = a;
    const struct in_addr *in2 = b;
    if (in1->s_addr < in2->s_addr) return -1;
    if (in1->s_addr == in2->s_addr) return 0;
    return 1;
}

/*
 * Builds an array of ip-addresses, in SNMP lexicographical order.
 * This array is stored per transaction, and built once in
 * each transaction.
 * This technique is simple, and suitable for small tables.
 */
static void ipaddr_list_build(struct confd_trans_ctx *tctx)
{
    struct maapi_cursor mc;
    int i, len, ret;
    struct trans_data *td;

    td = (struct trans_data *)tctx->t_opaque;

    if (td->ipaddr_list != NULL) {
        free(td->ipaddr_list);
    }
    OK(maapi_set_namespace(msock, tctx->thandle, if__ns));
    len = maapi_num_instances(msock, tctx->thandle, "/interface");
    if (len < 0) {
        fprintf(stderr, "transform.c: could not get number of ifs: %d : %s \n",
                confd_errno, confd_lasterr());
        abort();
    }
    td->ipaddr_list = (struct in_addr*)malloc(len * sizeof(struct in_addr));
    i = 0;
    OK(maapi_init_cursor(msock, tctx->thandle, &mc, "/interface"));
    OK(maapi_get_next(&mc));
    while (mc.n != 0) {
        ret = maapi_get_ipv4_elem(msock, tctx->thandle, &td->ipaddr_list[i],
                                  "/interface{%x}/address", &mc.keys[0]);
        if (ret == CONFD_OK) {
            i++;
        }
        OK(maapi_get_next(&mc));
    }
    maapi_destroy_cursor(&mc);
    td->ipaddr_list_len = i;
    qsort(td->ipaddr_list, td->ipaddr_list_len,
          sizeof(struct in_addr), cmp_in_addr);
}

/*
 * Callpoint: ip-snmp
 * Called for: ipAddressTable
 */
static int ip_get_next(struct confd_trans_ctx *tctx,
                       confd_hkeypath_t *keypath,
                       long next)
{
    confd_value_t v[2];
    confd_value_t *list = &(keypath->v[0][0]);
    struct trans_data *td;

    td = (struct trans_data *)tctx->t_opaque;

    switch (xml_tag(list)) {
    case IP_MIB_ipAddressEntry:
        if (next == -1) {
            ipaddr_list_build(tctx);
            next = 0;
        }
        if (next >= td->ipaddr_list_len) {
            return confd_data_reply_next_key(tctx, NULL, -1, -1);
        } else {
            unsigned char bin[4];
            inet_addressv4_convert(&td->ipaddr_list[next], bin, 0);
            CONFD_SET_ENUM_VALUE(&v[0], INET_ADDRESS_MIB_ipv4);
            CONFD_SET_BINARY(&v[1], bin, 4);
            return confd_data_reply_next_key(tctx, v, 2, next+1);
        }

    default:
        confd_data_reply_next_key(tctx, NULL, -1, -1);
        return CONFD_OK;
    }
}

/*
 * Callpoint: ip-snmp
 * Called for: leafs in ipAddressTable
 */
static int ip_get_elem(struct confd_trans_ctx *tctx,
                       confd_hkeypath_t *keypath)
{
    confd_value_t v;
    int ret;
    struct maapi_cursor mc;
    struct in_addr in;
    unsigned char *bin;
    confd_value_t *leaf = &(keypath->v[0][0]);
    confd_value_t *key = &(keypath->v[1][0]);
    confd_value_t *key2 = &(keypath->v[1][1]);
    confd_value_t ip_address;

    /* `key` is the value of ipAddressAddrType in ipAddrTable.
       `key2` is the value of ipAddressAddr in ipAddrTable.
       We could have kept the address in the `ifmap` list.  But
       in order to illustrate another implementation technique, we
       instead find this object directly from the data store.
       We need to scan all interfaces, and find the one with this
       address, and return its hidden ifIndex value.
    */

    if (CONFD_GET_ENUM_VALUE(key) != INET_ADDRESS_MIB_ipv4 ||
        CONFD_GET_BINARY_SIZE(key2) != 4) {
        /* we support v4 only */
        fprintf(stderr, "get_elem ipAddressTable: non-v4 key\n");
        return confd_data_reply_not_found(tctx);
    }
    bin = CONFD_GET_BINARY_PTR(key2);
    if (inet_addressv4_convert(&in, bin, 1) <= 0) {
        /* invalid address */
        fprintf(stderr, "get_elem ipAddressTable: invalid address: "
                "%d.%d.%d.%d\n", bin[0], bin[1], bin[2], bin[3]);
        return confd_data_reply_not_found(tctx);
    }
    CONFD_SET_IPV4(&ip_address, in);

    OK(maapi_set_namespace(msock, tctx->thandle, if__ns));
    OK(maapi_init_cursor(msock, tctx->thandle, &mc, "/interface"));
    OK(maapi_get_next(&mc));
    while (mc.n != 0) {
        ret = maapi_get_elem(msock, tctx->thandle, &v,
                             "/interface{%x}/address", &mc.keys[0]);
        if (ret == CONFD_OK && confd_val_eq(&ip_address, &v)) {
            /* We got a match - return the right value */
            switch (CONFD_GET_XMLTAG(leaf)) {
            case IP_MIB_ipAddressAddrType:
                maapi_destroy_cursor(&mc);
                return confd_data_reply_value(tctx, key);
            case IP_MIB_ipAddressAddr:
                maapi_destroy_cursor(&mc);
                return confd_data_reply_value(tctx, key2);
            case IP_MIB_ipAddressIfIndex:
                OK(maapi_get_elem(msock, tctx->thandle, &v,
                                  "/interface{%x}/ifIndex",
                                  &mc.keys[0]));
                maapi_destroy_cursor(&mc);
                return confd_data_reply_value(tctx, &v);
            case IP_MIB_ipAddressCreated:
                ret = maapi_get_elem(msock, tctx->thandle, &v,
                                     "/interface{%x}/ipAddressCreated",
                                     &mc.keys[0]);
                maapi_destroy_cursor(&mc);
                if (ret == CONFD_ERR && confd_errno == CONFD_ERR_NOEXISTS) {
                    return confd_data_reply_not_found(tctx);
                }
                return confd_data_reply_value(tctx, &v);
            case IP_MIB_ipAddressLastChanged:
                ret = maapi_get_elem(msock, tctx->thandle, &v,
                                     "/interface{%x}/ipAddressLastChanged",
                                     &mc.keys[0]);
                maapi_destroy_cursor(&mc);
                if (ret == CONFD_ERR && confd_errno == CONFD_ERR_NOEXISTS) {
                    return confd_data_reply_not_found(tctx);
                }
                return confd_data_reply_value(tctx, &v);
            }
        }
        OK(maapi_get_next(&mc));
    }
    maapi_destroy_cursor(&mc);
    return confd_data_reply_not_found(tctx);
}

/*
 * Callpoint: ip-snmp
 * Called for: writeable leafs in ipAddressTable
 */
static int ip_set_elem(struct confd_trans_ctx *tctx,
                       confd_hkeypath_t *keypath,
                       confd_value_t *newval)
{
    confd_value_t *leaf = &(keypath->v[0][0]);
    confd_value_t *key2 = &(keypath->v[1][1]); /* ipAddressAddr */
    struct in_addr in;
    confd_value_t v, ip_address;
    struct maapi_cursor mc;
    int ret;

    /* Convert the ipAddressAddr to a struct in_addr */
    inet_addressv4_convert(&in, CONFD_GET_BINARY_PTR(key2), 1);
    CONFD_SET_IPV4(&ip_address, in);

    switch (CONFD_GET_XMLTAG(leaf)) {
    case IP_MIB_ipAddressIfIndex:
        /* The if index of an address is changed.  We must remove the
           address leaf from the old interface, and set the address of
           the new interface.
        */
        OK(maapi_init_cursor(msock, tctx->thandle, &mc, "/interface"));
        OK(maapi_get_next(&mc));
        while (mc.n != 0) {
            ret = maapi_get_elem(msock, tctx->thandle, &v,
                                 "/interface{%x}/address", &mc.keys[0]);
            if (ret == CONFD_OK && confd_val_eq(&ip_address, &v)) {
                /* this is the old interface - delete the address */
                maapi_delete(msock, tctx->thandle,
                             "/interface{%x}/address", &mc.keys[0]);
            }
            OK(maapi_get_elem(msock, tctx->thandle, &v,
                              "/interface{%x}/ifIndex", &mc.keys[0]));
            if (confd_val_eq(&v, newval)) {
                /* This is the new interface which should get the address */
                OK(maapi_set_elem(msock, tctx->thandle, &ip_address,
                                  "/if:interface{%x}/address",
                                  &mc.keys[0]));
            }
            OK(maapi_get_next(&mc));
        }
        maapi_destroy_cursor(&mc);
        break;
    }

    return CONFD_OK;
}

// works very well for snmp
static int inconsistent(struct confd_trans_ctx *tctx,
                        const char *str) {
    fprintf(stderr, "INCONSISTENT %s", str);
    confd_trans_seterr_extended(tctx,
                                CONFD_ERRCODE_INCONSISTENT_VALUE, 0, 0,
                                "%s", str);
    return CONFD_ERR;
}


/*
 * Callpoint: ip-snmp
 * Called for: a new row is created in ipAddressTable
 */
static int ip_create(struct confd_trans_ctx *tctx,
                     confd_hkeypath_t *keypath)
{
    confd_value_t *key = &(keypath->v[0][0]);
    confd_value_t *key2 = &(keypath->v[0][1]);

    /* We just make sure that the type is ipv4.  We don't have to
       anything else here; the work will be done in ip_set_elem. */

    if (CONFD_GET_ENUM_VALUE(key) != INET_ADDRESS_MIB_ipv4 ||
        CONFD_GET_BINARY_SIZE(key2) != 4) {
        /* we support v4 only */
        return inconsistent(tctx, "Only ipv4 is supported");
    }
    return CONFD_OK;
}

/*
 * Callpoint: ip-snmp
 * Called for: an existing row is deleted from ipAddressTable
 */
static int ip_dbremove(struct confd_trans_ctx *tctx,
                       confd_hkeypath_t *keypath)
{
    confd_value_t *key2 = &(keypath->v[0][1]);

    struct in_addr in;
    confd_value_t v, ip_address;
    struct maapi_cursor mc;
    int ret;

    /* Convert the ipAddressAddr to a struct in_addr */
    inet_addressv4_convert(&in, CONFD_GET_BINARY_PTR(key2), 1);
    CONFD_SET_IPV4(&ip_address, in);

    /* Loop over the interfaces and delete the address from the interface
       with this address. */
    OK(maapi_init_cursor(msock, tctx->thandle, &mc, "/interface"));
    OK(maapi_get_next(&mc));
    while (mc.n != 0) {
        ret = maapi_get_elem(msock, tctx->thandle, &v,
                             "/interface{%x}/address", &mc.keys[0]);
        if (ret == CONFD_OK && confd_val_eq(&ip_address, &v)) {
            /* found the interface - delete the address */
            maapi_delete(msock, tctx->thandle,
                         "/interface{%x}/address", &mc.keys[0]);
        }
        OK(maapi_get_next(&mc));
    }
    maapi_destroy_cursor(&mc);

    return CONFD_OK;
}


/*
 * Hooks to keep the OSPF interface map table up to date
 */

/* Called when a (high-level) /ospf/area or /ospf/area/interface is created */
static int ospf_area_create_hook(struct confd_trans_ctx *tctx,
                                 confd_hkeypath_t *keypath)
{
    confd_value_t *area_id;
    confd_value_t *iface_name;
    confd_value_t ip_address;
    confd_value_t address_less_if;
    static struct in_addr in_zero;
    int ret;
    confd_value_t *tag = &keypath->v[1][0];

    /* Invoke this only when running is modified. */
    if (tctx->dbname != CONFD_RUNNING) {
        return CONFD_OK;
    }

    /*
      We need to populate the hidden table /snmp-map/ospf-if
    */

    if (xml_tag(tag) != ospf_interface) {
        /* This is a /ospf/area being created.  Ignore. */
        return CONFD_OK;
    }

    /*
      The keypath we are interested in here will be like:
      /ospf/area{area-id}/interface{if-name}
      [4]  [3]    [2]        [1]     [0]
    */
    area_id = &keypath->v[2][0];
    iface_name = &keypath->v[0][0];

    /* Find the ip address of the interface */
    ret = maapi_get_elem(msock, tctx->thandle, &ip_address,
                         "/if:interface{%x}/address", iface_name);
    if (ret == CONFD_ERR && confd_errno == CONFD_ERR_NOEXISTS) {
        /* This is an addressless interface - find the ifIndex */
        OK(maapi_get_elem(msock, tctx->thandle, &address_less_if,
                          "/if:interface{%x}/ifIndex", iface_name));
        /* store zero in the address */
        CONFD_SET_IPV4(&ip_address, in_zero);
    } else {
        OK(ret);
        /* store 0 as the address_less, since we have an ip */
        CONFD_SET_INT32(&address_less_if, 0);
    }

    /* Create the hidden entry */
    maapi_create(msock, tctx->thandle, "/ospf:snmp-map");
    maapi_create(msock, tctx->thandle,
                 "/ospf:snmp-map/ospf-if{%x %x}",
                 &ip_address, &address_less_if);
    maapi_set_elem(msock, tctx->thandle, area_id,
                   "/ospf:snmp-map/ospf-if{%x %x}/area-id",
                   &ip_address, &address_less_if);
    maapi_set_elem(msock, tctx->thandle, iface_name,
                   "/ospf:snmp-map/ospf-if{%x %x}/if-name",
                   &ip_address, &address_less_if);
    return CONFD_OK;
}


/* auxillary function called by ospf_area_remove_hook()  */
static int ospf_do_remove_if(struct confd_trans_ctx *tctx,
                             confd_value_t *area_id,
                             confd_value_t *iface_name)
{
    confd_value_t ip_address;
    confd_value_t address_less_if;
    static struct in_addr in_zero;
    int ret;

    /* Find the ip address of the interface */
    ret = maapi_get_elem(msock, tctx->thandle, &ip_address,
                         "/if:interface{%x}/address", iface_name);
    if (ret == CONFD_ERR && confd_errno == CONFD_ERR_NOEXISTS) {
        /* This is an addressless interface - find the ifIndex */
        OK(maapi_get_elem(msock, tctx->thandle, &address_less_if,
                          "/if:interface{%x}/ifIndex", iface_name));
        /* store zero in the address */
        CONFD_SET_IPV4(&ip_address, in_zero);
    } else {
        OK(ret);
        /* store 0 as the address_less, since we have an ip */
        CONFD_SET_INT32(&address_less_if, 0);
    }

    /* Delete the hidden entry */
    maapi_delete(msock, tctx->thandle,
                 "/ospf:snmp-map/ospf-if{%x %x}",
                 &ip_address, &address_less_if);

    return CONFD_OK;
}


/* Called when a (high-level) /ospf/area or /ospf/area/interface is deleted */
static int ospf_area_remove_hook(struct confd_trans_ctx *tctx,
                                 confd_hkeypath_t *keypath)
{
    confd_value_t *area_id;
    confd_value_t *iface_name;

    /* Invoke this only when running is modified. */
    if (tctx->dbname != CONFD_RUNNING) {
        return CONFD_OK;
    }

    if (xml_tag(&keypath->v[1][0]) == ospf_area) {
        struct maapi_cursor mc;
        int ret = CONFD_OK;

        /*
          The keypath we get here will be like:
          /ospf/area{area-id}
          [2]  [1]    [0]
        */

        area_id = &keypath->v[0][0];
        if (maapi_init_cursor(
                msock, tctx->thandle, &mc,
                "/ospf:ospf/area{%x}/interface", area_id) != CONFD_OK) {
            return CONFD_OK;
        }

        OK(maapi_get_next(&mc));
        while (ret == CONFD_OK && mc.n != 0) {
            if (ospf_do_remove_if(tctx, area_id, &mc.keys[0]) != CONFD_OK) {
                ret = CONFD_ERR;
            } else {
                OK(maapi_get_next(&mc));
            }
        }
        maapi_destroy_cursor(&mc);
        return ret;
    }
    else if (xml_tag(&keypath->v[1][0]) == ospf_interface) {

        /*
          The keypath we get here will be like:
          /ospf/area{area-id}/interface{if-name}
          [4]  [3]    [2]        [1]     [0]
        */

        area_id = &keypath->v[2][0];
        iface_name = &keypath->v[0][0];
        return ospf_do_remove_if(tctx, area_id, iface_name);
    }
    return CONFD_OK;
}


/* Called when a (high-level) /ospf/area or /ospf/area/interface is modified */
static int ospf_area_set_elem_hook(struct confd_trans_ctx *tctx,
                                   confd_hkeypath_t *keypath,
                                   confd_value_t *newval)
{
    return CONFD_OK;
}

/*
 * OSPF Transformation code
 *
 * This code acts as a data provider for the SNMP data model view,
 * by transforming to and from the high-level data model.
 */

/*
 * Callpoint: ospf-snmp
 * Called for: ospfAreaTable, ospfIfTable
 */


static int ospf_get_next(struct confd_trans_ctx *tctx,
                         confd_hkeypath_t *keypath,
                         long next)
{
    struct maapi_cursor *mc;
    confd_value_t *tag =  &keypath->v[0][0];

    if (next == -1) {
        if (CONFD_GET_XMLTAG(tag) == OSPF_MIB_ospfAreaEntry) {
            mc = alloc_cursor(msock, tctx, "/ospf:ospf/area");
        }
        else if (CONFD_GET_XMLTAG(tag) == OSPF_MIB_ospfIfEntry) {
            mc = alloc_cursor(msock, tctx, "/ospf:snmp-map/ospf-if");
        }
    } else {
        mc = (struct maapi_cursor *)next;
    }
    OK(maapi_get_next(mc));
    if (mc->n == 0) {
        free_cursor(tctx, mc);
        confd_data_reply_next_key(tctx, NULL, -1, -1);
        return CONFD_OK;
    }
    if (CONFD_GET_XMLTAG(tag) == OSPF_MIB_ospfAreaEntry) {
        confd_data_reply_next_key(tctx, &mc->keys[0], 1, (long)mc);
    }
    else if (CONFD_GET_XMLTAG(tag) == OSPF_MIB_ospfIfEntry) {
        confd_value_t arr[2];
        arr[0] = mc->keys[0];
        CONFD_SET_INT32(&arr[1], 0);
        confd_data_reply_next_key(tctx, &arr[0], 2, (long)mc);
    }
    return CONFD_OK;
}

static int directReply(struct confd_trans_ctx *tctx,  const char *path)
{
    confd_value_t v;
    if (maapi_get_elem(msock, tctx->thandle, &v, path) != CONFD_OK) {
        if (confd_errno == CONFD_ERR_NOEXISTS) {
            confd_data_reply_not_found(tctx);
            return CONFD_OK;
        }
        return CONFD_ERR;
    }
    confd_data_reply_value(tctx, &v);
    return CONFD_OK;
}

static int directReply1(struct confd_trans_ctx *tctx,
                        const char *path, confd_value_t *vp)
{
    confd_value_t v;
    if (maapi_get_elem(msock, tctx->thandle, &v, path, vp) != CONFD_OK) {
        if (confd_errno == CONFD_ERR_NOEXISTS) {
            confd_data_reply_not_found(tctx);
            return CONFD_OK;
        }
        return CONFD_ERR;
    }
    confd_data_reply_value(tctx, &v);
    return CONFD_OK;
}

static int directReply2(struct confd_trans_ctx *tctx,
                        const char *path,
                        confd_value_t *vp1, confd_value_t *vp2)
{
    confd_value_t v;
    if (maapi_get_elem(msock, tctx->thandle, &v, path, vp1,vp2) != CONFD_OK) {
        if (confd_errno == CONFD_ERR_NOEXISTS) {
            confd_data_reply_not_found(tctx);
            return CONFD_OK;
        }
        return CONFD_ERR;
    }
    confd_data_reply_value(tctx, &v);
    return CONFD_OK;
}


/*
 * Callpoint: ospf-snmp
 * Called for: leafs in ospfAreaTable, ospfIfTable
 *             ospfRouterId, ospfAdminStat
 */

static int ospf_get_elem(struct confd_trans_ctx *tctx,
                         confd_hkeypath_t *keypath)
{
    confd_value_t *tag =  &keypath->v[0][0];
    confd_value_t *ipAddr =  &keypath->v[1][0];
    confd_value_t *ifIndex =  &keypath->v[1][1];
    confd_value_t v, v1;
    int rval;
    int th = tctx->thandle;

    switch CONFD_GET_XMLTAG(tag)  {
        case OSPF_MIB_ospfRouterId:
            return directReply(tctx, "/ospf:ospf/router-id");
        case OSPF_MIB_ospfAdminStat:
            return directReply(tctx, "/ospf:ospf/admin-status");
        case OSPF_MIB_ospfAreaId:
            return directReply1(tctx, "/ospf:ospf/area{%x}/id", ipAddr);
        case OSPF_MIB_ospfImportAsExtern:
            return confd_data_reply_not_found(tctx);
        case OSPF_MIB_ospfAreaSummary:
            if ((maapi_exists(msock, th,
                              "/ospf:ospf/area{%x}/stub/summary",
                              ipAddr) == 1)) {
                CONFD_SET_ENUM_VALUE(&v, OSPF_MIB_sendAreaSummary);
            }
            else {
                CONFD_SET_ENUM_VALUE(&v, OSPF_MIB_noAreaSummary);
            }
            confd_data_reply_value(tctx, &v);
            return CONFD_OK;
        case OSPF_MIB_ospfAreaNssaTranslatorRole:
            return directReply1(tctx,
                                "/ospf:ospf/area{%x}/nssa/translator",
                                ipAddr);

        case OSPF_MIB_ospfIfIpAddress:
            return directReply2(tctx,
                                "/ospf:snmp-map/ospf-if{%x %x}/ospfIfIpAddress",
                                ipAddr, ifIndex);
        case OSPF_MIB_ospfAddressLessIf:
            return directReply2(
                tctx,
                "/ospf:snmp-map/ospf-if{%x %x}/ospfAddressLessIf",
                ipAddr, ifIndex);
        case OSPF_MIB_ospfIfAreaId:
            return directReply2(tctx,
                                "/ospf:snmp-map/ospf-if{%x %x}/area-id",
                                ipAddr, ifIndex);

        case OSPF_MIB_ospfIfType:

            if (maapi_get_elem(msock, th, &v,
                               "/ospf:snmp-map/ospf-if{%x %x}/area-id",
                               ipAddr, ifIndex) != CONFD_OK) {
                if (confd_errno == CONFD_ERR_NOEXISTS) {
                    confd_data_reply_not_found(tctx);
                    return CONFD_OK;
                }
                return CONFD_ERR;
            }

            if (maapi_get_elem(msock, th, &v1,
                               "/ospf:snmp-map/ospf-if{%x %x}/if-name",
                               ipAddr, ifIndex) != CONFD_OK) {
                if (confd_errno == CONFD_ERR_NOEXISTS) {
                    confd_data_reply_not_found(tctx);
                    return CONFD_OK;
                }
                return CONFD_ERR;
            }
            if (maapi_get_enum_value_elem(
                    msock, th, &rval,
                    "/ospf:ospf/area{%x}/interface{%x}/type",
                    &v, &v1) != CONFD_OK)
                return confd_data_reply_not_found(tctx);
            switch(rval) {
            case ospf_broadcast:
                CONFD_SET_ENUM_VALUE(&v, OSPF_MIB_broadcast);
                break;
            case ospf_nbma:
                CONFD_SET_ENUM_VALUE(&v, OSPF_MIB_nbma);
                break;
            case ospf_point_to_point:
                CONFD_SET_ENUM_VALUE(&v, OSPF_MIB_pointToPoint);
                break;
            case ospf_point_to_multipoint:
                CONFD_SET_ENUM_VALUE(&v, OSPF_MIB_pointToMultipoint);
                break;
            }
            return confd_data_reply_value(tctx, &v);

        case OSPF_MIB_ospfIfAdminStat:
            if (maapi_get_elem(msock, th, &v,
                               "/ospf:snmp-map/ospf-if{%x %x}/area-id",
                               ipAddr, ifIndex) != CONFD_OK) {
                if (confd_errno == CONFD_ERR_NOEXISTS) {
                    confd_data_reply_not_found(tctx);
                    return CONFD_OK;
                }
                return CONFD_ERR;
            }

            if (maapi_get_elem(msock, th, &v1,
                               "/ospf:snmp-map/ospf-if{%x %x}/if-name",
                               ipAddr, ifIndex) != CONFD_OK) {
                if (confd_errno == CONFD_ERR_NOEXISTS) {
                    confd_data_reply_not_found(tctx);
                    return CONFD_OK;
                }
                return CONFD_ERR;
            }

            if ((rval =
                 maapi_exists(msock, th,
                              "/ospf:ospf/area{%x}/interface{%x}/disable",
                              &v, &v1))  != CONFD_ERR) {
                if (rval == 1)
                    CONFD_SET_ENUM_VALUE(&v, OSPF_MIB_disabled);
                else
                    CONFD_SET_ENUM_VALUE(&v, OSPF_MIB_enabled);
                confd_data_reply_value(tctx, &v);
                return CONFD_OK;
            }
            else {
                return rval;
            }
        case OSPF_MIB_ospfIfRtrPriority:
            if (maapi_get_elem(msock, th, &v,
                               "/ospf:snmp-map/ospf-if{%x %x}/area-id",
                               ipAddr, ifIndex) != CONFD_OK) {
                if (confd_errno == CONFD_ERR_NOEXISTS) {
                    confd_data_reply_not_found(tctx);
                    return CONFD_OK;
                }
                return CONFD_ERR;
            }
            if (maapi_get_elem(msock, th, &v1,
                               "/ospf:snmp-map/ospf-if{%x %x}/if-name",
                               ipAddr, ifIndex) != CONFD_OK) {
                if (confd_errno == CONFD_ERR_NOEXISTS) {
                    confd_data_reply_not_found(tctx);
                    return CONFD_OK;
                }
                return CONFD_ERR;
            }

            return directReply2(
                tctx,"/ospf:ospf/area{%x}/interface{%x}/priority",
                &v, &v1);

        default:
            confd_data_reply_not_found(tctx);
            return CONFD_OK;
        }
}

static char *ip2ifname(int th, confd_value_t *ipAddr, confd_value_t *ifIndex) {
    static char buf[255];
    int ret;
    confd_value_t v;
    struct maapi_cursor mc;

    /* Loop over all existing interfaces */
    /* we could have chosen to add the ipAddr as well to the in memory */
    /* ifmap - instead we search for the ifName here instead using maapi*/

    OK(maapi_init_cursor(msock, th, &mc, "/if:interface"));
    OK(maapi_get_next(&mc));
    while (mc.n != 0) {
        ret = maapi_get_elem(msock, th, &v,
                             "/if:interface{%x}/address", &mc.keys[0]);
        if ((ret == CONFD_OK) && confd_val_eq(&v, ipAddr)) {
            strcpy(buf, (char*)CONFD_GET_BUFPTR(&mc.keys[0]));
            maapi_destroy_cursor(&mc);
            return buf;
        }
        OK(maapi_get_next(&mc));
    }
    maapi_destroy_cursor(&mc);
    // It could be that we're refering to an IP less interface
    // in which case the ifIndex /= 0
    if (CONFD_GET_INT32(ifIndex) == 0)
        return NULL;
    // search the ifmap
    return (char*)ifmap_get_ifname(CONFD_GET_INT32(ifIndex));
}


/*
 * Callpoint: ospf-snmp
 * Called for: writeable leafs in ospfAreaTable, ospfIfTable
 *             ospfRouterId, ospfAdminStat
 */
static int ospf_set_elem(struct confd_trans_ctx *tctx,
                         confd_hkeypath_t *keypath,
                         confd_value_t *newval)
{
    confd_value_t *tag =  &keypath->v[0][0];
    confd_value_t *ipAddr =  &keypath->v[1][0];
    confd_value_t *ifIndex =  &keypath->v[1][1];
    confd_value_t v, v1;
    int th = tctx->thandle;
    char *ifName;

    switch CONFD_GET_XMLTAG(tag)  {
        case OSPF_MIB_ospfRouterId:
            return maapi_set_elem(msock, th, newval,
                                  "/ospf:ospf/router-id");
        case OSPF_MIB_ospfAdminStat:
            return maapi_set_elem(msock, th, newval,
                                  "/ospf:ospf/admin-status");
        case OSPF_MIB_ospfAreaId:
            return maapi_set_elem(msock, th, newval,
                                  "/ospf:ospf/area{%x}/id", ipAddr);
        case OSPF_MIB_ospfImportAsExtern:
            return CONFD_OK;
        case OSPF_MIB_ospfAreaSummary:
            return maapi_create(
                msock, tctx->thandle,
                "/ospf:ospf/area{%x}/stub/summary", ipAddr);
        case OSPF_MIB_ospfAreaNssaTranslatorRole:
            return maapi_set_elem(msock, th, newval,
                                  "/ospf:ospf/area{%x}/nssa/translator",
                                  ipAddr);
        case OSPF_MIB_ospfIfAreaId: {

            if (delayed_ospf_if.inuse &&
                confd_val_eq(ipAddr, &delayed_ospf_if.ipAddr) &&
                confd_val_eq(ifIndex, &delayed_ospf_if.ifIndex)) {


                /* Since we don't know the areaid when this object got created,
                   we couldn't then create /ospf/area{id}/interface{ifname}
                   thus we must do this now - delayed
                   this also the earliest point where we can create
                   the mapping.
                */
                ifName = ip2ifname(th, ipAddr, ifIndex);
                if (ifName == NULL)
                    return inconsistent(tctx, "No ip address found");

                maapi_create(msock, th, "/ospf:snmp-map/ospf-if{%x %x}",
                             ipAddr, ifIndex);
                OK(maapi_set_elem(msock,th, newval,
                                  "/ospf:snmp-map/ospf-if{%x %x}/area-id",
                                  ipAddr, ifIndex));
                CONFD_SET_STR(&v, ifName);
                OK(maapi_set_elem(msock,th, &v,
                                  "/ospf:snmp-map/ospf-if{%x %x}/if-name",
                                  ipAddr, ifIndex));
                /* That was the required mapping,
                   now we create the actual object */
                maapi_create(msock, th,
                             "/ospf:ospf/area{%x}/interface{%x}", newval, &v);
                delayed_ospf_if.inuse = 0;
                // and finally we fill in any accumulated values
                if (delayed_ospf_if.ifType.type != 0) {
                    // patch keypath and call ourselves
                    CONFD_SET_XMLTAG(&keypath->v[0][0], OSPF_MIB_ospfIfType,
                                     OSPF_MIB__ns);
                    ospf_set_elem(tctx, keypath, &delayed_ospf_if.ifType);
                }

                if (delayed_ospf_if.adminStat.type != 0) {
                    // patch keypath and call ourselves
                    CONFD_SET_XMLTAG(&keypath->v[0][0],
                                     OSPF_MIB_ospfIfAdminStat,
                                     OSPF_MIB__ns);
                    ospf_set_elem(tctx, keypath, &delayed_ospf_if.adminStat);
                }

                if (delayed_ospf_if.priority.type != 0) {
                    // patch keypath and call ourselves
                    CONFD_SET_XMLTAG(&keypath->v[0][0],
                                     OSPF_MIB_ospfIfRtrPriority,
                                     OSPF_MIB__ns);
                    ospf_set_elem(tctx, keypath, &delayed_ospf_if.priority);
                }
                // indicate we're completely done
                memset(&delayed_ospf_if, 0, sizeof(struct delayed_ospf_iface));
                return CONFD_OK;
            }
            else {
                // swap areId for an existing ospf interface
                confd_value_t prevAreaId;
                if (maapi_get_elem(msock, th, &prevAreaId,
                                   "/ospf:snmp-map/ospf-if{%x %x}/area-id",
                                   ipAddr, ifIndex) != CONFD_OK)
                    return inconsistent(tctx, "No such areaId");
                return maapi_set_elem(msock, th, newval,
                                      "/ospf:snmp-map/ospf-if{%x %x}/area-id",
                                      ipAddr, ifIndex);
            }

        }
        case OSPF_MIB_ospfIfType:

            if (delayed_ospf_if.inuse &&
                confd_val_eq(ipAddr, &delayed_ospf_if.ipAddr) &&
                confd_val_eq(ifIndex, &delayed_ospf_if.ifIndex)) {
                // remember this setting until we get the areaId
                delayed_ospf_if.ifType = *newval;
                return CONFD_OK;
            }

            if (maapi_get_elem(msock, tctx->thandle, &v,
                               "/ospf:snmp-map/ospf-if{%x %x}/area-id",
                               ipAddr, ifIndex) != CONFD_OK) {
                return CONFD_ERR;
            }

            if (maapi_get_elem(msock, tctx->thandle, &v1,
                               "/ospf:snmp-map/ospf-if{%x %x}/if-name",
                               ipAddr, ifIndex) != CONFD_OK) {
                return CONFD_ERR;
            }
            switch CONFD_GET_ENUM_VALUE(newval) {
                case OSPF_MIB_broadcast:
                    CONFD_SET_ENUM_VALUE(&v, ospf_broadcast);
                    break;
                case OSPF_MIB_nbma:
                    CONFD_SET_ENUM_VALUE(&v, ospf_nbma);
                    break;
                case OSPF_MIB_pointToPoint:
                    CONFD_SET_ENUM_VALUE(&v, ospf_point_to_point);
                    break;
                case OSPF_MIB_pointToMultipoint:
                    CONFD_SET_ENUM_VALUE(&v, ospf_point_to_multipoint);
                    break;

                }

            if (maapi_set_elem(msock, tctx->thandle, &v,
                               "/ospf:ospf/area{%x}/interface{%x}/type",
                               &v, &v1) != CONFD_OK)
                return CONFD_ERR;
            return CONFD_OK;

        case OSPF_MIB_ospfIfAdminStat:

            if (delayed_ospf_if.inuse &&
                confd_val_eq(ipAddr, &delayed_ospf_if.ipAddr) &&
                confd_val_eq(ifIndex, &delayed_ospf_if.ifIndex)) {
                // remember this setting until we get the areaId
                delayed_ospf_if.adminStat = *newval;
                return CONFD_OK;
            }

            if (maapi_get_elem(msock, tctx->thandle, &v,
                               "/ospf:snmp-map/ospf-if{%x 0}/area-id",
                               ipAddr) != CONFD_OK) {
                return CONFD_ERR;
            }

            if (maapi_get_elem(msock, tctx->thandle, &v1,
                               "/ospf:snmp-map/ospf-if{%x 0}/if-name",
                               ipAddr) != CONFD_OK) {
                return CONFD_ERR;
            }

            return maapi_create(
                msock, tctx->thandle,
                "/ospf:ospf/area{%x}/interface{%x}/disable", &v, &v1);

        case OSPF_MIB_ospfIfRtrPriority:
            if (delayed_ospf_if.inuse &&
                confd_val_eq(ipAddr, &delayed_ospf_if.ipAddr) &&
                confd_val_eq(ifIndex, &delayed_ospf_if.ifIndex)) {
                // remember this setting until we get the areaId
                delayed_ospf_if.priority = *newval;
                return CONFD_OK;
            }

            if (maapi_get_elem(msock, tctx->thandle, &v,
                               "/ospf:snmp-map/ospf-if{%x 0}/area-id",
                               ipAddr) != CONFD_OK) {
                return CONFD_ERR;
            }
            if (maapi_get_elem(msock, tctx->thandle, &v1,
                               "/ospf:snmp-map/ospf-if{%x 0}/if-name",
                               ipAddr) != CONFD_OK) {
                return CONFD_ERR;
            }

            return maapi_set_elem(
                msock, th, newval,
                "/ospf:ospf/area{%x}/interface{%x}/priority",&v, &v1);

        default:
            return CONFD_OK;
        }

}


/*
 * Callpoint: ospf-snmp
 * Called for: a new row is created in ospfAreaTable, ospfIfTable
 */
static int ospf_create(struct confd_trans_ctx *tctx,
                       confd_hkeypath_t *keypath)
{

    confd_value_t *tag =   &keypath->v[1][0];
    if (xml_tag(tag) == OSPF_MIB_ospfAreaEntry) {
        confd_value_t *areaId =  &keypath->v[0][0];
        return maapi_create(msock, tctx->thandle,
                            "/ospf:ospf/area{%x}", areaId);
    }
    else if (xml_tag(tag) == OSPF_MIB_ospfIfEntry) {

        confd_value_t *ipAddr =  &keypath->v[0][0];
        confd_value_t *ifIndex =  &keypath->v[0][1];

        /* This is complicated, we cannot yet create this object
           as /ospf/area{areaId}/interface{name} because we don't
           yet know the areaId. We must delay the creation until
           the snmp agent sets the areaId.

           Since the areaId is mandatory to set in the MIB, we know
           that that we will be invoked in the ospf_set_elem()
           callback in the same transaction, and thus we can use a
           single static variable to keep track of the data to create.
        */

        memset(&delayed_ospf_if, 0, sizeof(struct delayed_ospf_iface));
        delayed_ospf_if.inuse = 1;
        delayed_ospf_if.ipAddr = *ipAddr;
        delayed_ospf_if.ifIndex = *ifIndex;

        return CONFD_OK;
    }
    return CONFD_ERR;
}

/*
 * Callpoint: ospf-snmp
 * Called for: an existing  row is deleted from ospfAreaTable, ospfIfTable
 */
static int ospf_dbremove(struct confd_trans_ctx *tctx,
                         confd_hkeypath_t *keypath)
{

    confd_value_t *tag =   &keypath->v[1][0];
    int th = tctx->thandle;

    if (xml_tag(tag) == OSPF_MIB_ospfAreaEntry) {
        confd_value_t *areaId =  &keypath->v[0][0];
        return maapi_delete(msock, th, "/ospf:ospf/area{%x}", areaId);
    }
    else if (xml_tag(tag) == OSPF_MIB_ospfIfEntry) {
        // nested table remove, need to find the areaId
        confd_value_t *ipAddr =  &keypath->v[0][0];
        confd_value_t *ifIndex =  &keypath->v[0][1];
        char *ifName;
        confd_value_t areaId;

        if ((ifName = ip2ifname(th, ipAddr, ifIndex)) == NULL) {
            return CONFD_OK;
        }

        // Now we need to find the interface in the right area
        if (maapi_get_elem(msock, th, &areaId,
                           "/ospf:snmp-map/ospf-if{%x %x}/area-id",
                           ipAddr, ifIndex) != CONFD_OK)
            // already gone
            return CONFD_OK;
        maapi_delete(msock, th, "/ospf:snmp-map/ospf-if{%x %x}",
                     ipAddr, ifIndex);
        maapi_delete(msock, th, "/ospf:ospf/area{%x}/interface{%s}",
                     &areaId, ifName);

    }
    return CONFD_OK;
}



/*
 * main
 */

int main(int argc, char **argv)
{
    int c;

    while ((c = getopt(argc, argv, "tdpsr")) != -1) {
        switch(c) {
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

    confd_init((char *)"transform", stderr, debuglevel);

    /* Load data model schemas from to ConfD */

    if ((dctx = confd_init_daemon("transform")) == NULL)
        confd_fatal("Failed to initialize confd\n");

    if ((ctlsock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open ctlsocket\n");

    if (confd_load_schemas((struct sockaddr*)&confd_addr,
                           sizeof (struct sockaddr_in)) != CONFD_OK) {
        confd_fatal("Failed to load schemas from confd\n");
    }

    /* MAAPI connect to ConfD */

    msock = do_maapi_connect();

    /* Initialize our data structures */
    ifmap_init();

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

    tcb.init = init_transformation;
    tcb.finish = stop_transformation;
    confd_register_trans_cb(dctx, &tcb);

    /* Register the Interface hook */

    if_hook.create   = if_create_hook;
    if_hook.remove   = if_remove_hook;
    if_hook.set_elem = if_set_elem_hook;

    strcpy(if_hook.callpoint, "if-hook");

    if (confd_register_data_cb(dctx, &if_hook) == CONFD_ERR)
        confd_fatal("Failed to register hook cb \n");

    /* Register the Interface transformation */

    if_transform.get_elem        = if_get_elem;
    if_transform.get_next        = if_get_next;

    /* This callback is NULL because there are no writable objects in ifTable
       in this example. */
    if_transform.set_elem        = NULL;

    /* These two callbacks are NULL because entries in ifTable cannot be
       created or deleted over SNMP. */
    if_transform.create          = NULL;
    if_transform.remove          = NULL;

    if_transform.exists_optional = NULL;

    strcpy(if_transform.callpoint, "if-snmp");

    if (confd_register_data_cb(dctx, &if_transform) == CONFD_ERR)
        confd_fatal("Failed to register transform cb \n");

    /* Register the IP Address transformation */

    ip_transform.get_elem        = ip_get_elem;
    ip_transform.get_next        = ip_get_next;
    ip_transform.set_elem        = ip_set_elem;
    ip_transform.create          = ip_create;
    ip_transform.remove          = ip_dbremove;
    ip_transform.exists_optional = NULL;

    strcpy(ip_transform.callpoint, "ip-snmp");

    if (confd_register_data_cb(dctx, &ip_transform) == CONFD_ERR)
        confd_fatal("Failed to register transform cb \n");

    /* Register the OSPF interface hook */

    ospf_area_hook.create   = ospf_area_create_hook;
    ospf_area_hook.remove   = ospf_area_remove_hook;
    ospf_area_hook.set_elem = ospf_area_set_elem_hook;

    strcpy(ospf_area_hook.callpoint, "ospf-area-hook");

    if (confd_register_data_cb(dctx, &ospf_area_hook) == CONFD_ERR)
        confd_fatal("Failed to register hook cb \n");

    /* Register the OSPF transformation */

    ospf_transform.get_elem        = ospf_get_elem;
    ospf_transform.get_next        = ospf_get_next;
    ospf_transform.set_elem        = ospf_set_elem;
    ospf_transform.create          = ospf_create;
    ospf_transform.remove          = ospf_dbremove;
    ospf_transform.exists_optional = NULL;

    strcpy(ospf_transform.callpoint, "ospf-snmp");

    if (confd_register_data_cb(dctx, &ospf_transform) == CONFD_ERR)
        confd_fatal("Failed to register transform cb \n");


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
