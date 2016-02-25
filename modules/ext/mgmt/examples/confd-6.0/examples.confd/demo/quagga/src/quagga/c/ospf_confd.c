#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <confd.h>
#include <confd_cdb.h>
#include <confd_maapi.h>

#undef _POSIX_C_SOURCE

#include "zconfd_api.h"
#include "zconfd_subs.h"
#include "confd_global.h"

#include "quagga.h"     /* generated from yang */

#define BASE_PROMPT ".*\\(config\\)# "
#define ROUTER_PROMPT ".*\\(config-router\\)# "
#define IF_PROMPT ".*\\(config-if\\)# "

#define OSPF_COMMAND(data, command, prompt, args...) cli_command(ospfd, data, prompt, 1, CLI_ERR_STR, command , ##args)
#define OSPF_BASE_COMMAND(data, command, args...) OSPF_COMMAND(data, command, BASE_PROMPT , ##args)
#define OSPF_ROUTER_COMMAND(data, command, args...) OSPF_COMMAND(data, command, ROUTER_PROMPT , ##args)
#define OSPF_IF_COMMAND(data, command, args...) OSPF_COMMAND(data, command, IF_PROMPT , ##args)

#define PARTS_ROUTER_HANDLER(elem_path, common_part, set_part, reset) PARTS_HANDLER(ospfd, ROUTER_PROMPT, elem_path, common_part, set_part, reset)
#define STD_ROUTER_HANDLER(elem_path, yes_command, no_command, reset) STD_HANDLER(ospfd, ROUTER_PROMPT, elem_path, yes_command, no_command, reset)
#define BOOL_ROUTER_HANDLER(elem_path, command, reset) BOOL_HANDLER(ospfd, ROUTER_PROMPT, elem_path, command, reset)

#define DELETE_ROUTER_HANDLER(elem_path, command) {elem_path, get_gen_inst, 0, gen_priv(ROUTER_PROMPT, command)}

#define PARTS_IF_HANDLER(elem_path, common_part, set_part, reset) PARTS_HANDLER(ospfd, IF_PROMPT, elem_path, common_part, set_part, reset)
#define STD_IF_HANDLER(elem_path, yes_command, no_command, reset) STD_HANDLER(ospfd, IF_PROMPT, elem_path, yes_command, no_command, reset)
#define BOOL_IF_HANDLER(elem_path, command, reset) BOOL_HANDLER(ospfd, IF_PROMPT, elem_path, command, reset)

#define AREA_PREFIX "area ${key: area(-4)} "

#define PARTS_AREA_HANDLER(elem_path, common_part, set_part, reset) PARTS_ROUTER_HANDLER(elem_path, AREA_PREFIX common_part, set_part, reset)
#define ASTD_AREA_HANDLER(elem_path, yes_command, no_no_command, reset) STD_ROUTER_HANDLER(elem_path, AREA_PREFIX yes_command, "no " AREA_PREFIX no_no_command, reset)

#define OSPF_PRIO 6
#define OSPF_EARLY_PRIO 5

static int validateOspfNetworkIp(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval);
static int validateDistanceOspf(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval);
static int validateAreaType(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval);
static int validateRangeValuesOspf(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval);
static int validateRangeSubstitute(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval);
static int validateAutoCostReferenceBandwidth(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval);
static int validateAreaShortcut(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval);

static int get_ospf_memory_elem(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath);

typedef struct {
    const char *prompt;
    const char *command;
} gen_priv_t;

gen_priv_t *gen_priv(const char *prompt, const char *command)
{
    gen_priv_t *priv = malloc(sizeof(gen_priv_t));
    priv->prompt = prompt;
    priv->command = command;
    return priv;
}

static struct confd_valpoint_cb valpoints[] = {
    { .valpoint = "valOspfNetworkIp", .validate = validateOspfNetworkIp},
    { .valpoint = "valDistanceOspf", .validate = validateDistanceOspf},
    { .valpoint = "valAreaType", .validate = validateAreaType},
    { .valpoint = "valRangeValuesOspf", .validate = validateRangeValuesOspf},
    { .valpoint = "valRangeSubstitute", .validate = validateRangeSubstitute},
    { .valpoint = "valAutoCostReferenceBandwidth", .validate = validateAutoCostReferenceBandwidth},
    { .valpoint = "valShortcut", .validate = validateAreaShortcut},
    { .valpoint = ""}
};

int ospf_init(struct confd_daemon_ctx *dctx)
{
    register_valpoint_cb_arr(dctx, valpoints);

    extern int quagga_not_running;
    if(quagga_not_running) return CONFD_OK;

    CHECK_OK(cli_connect(ospfd));

    return CONFD_OK;
}

void subscr_router_init(int start, cb_data_t *data)
{
    if (start)
        OSPF_ROUTER_COMMAND(data, "router ospf");
    else
        OSPF_BASE_COMMAND(data, "exit");
}

void subscr_if_init(int start, cb_data_t *data)
{
    if (start)
        OSPF_IF_COMMAND(data, "interface ${key: name(-2)}");
    else
        OSPF_BASE_COMMAND(data, "exit");
}

static get_func_t get_passive_by_default;
static get_func_t get_passive_except;

static get_func_t get_area_type;
static get_func_t get_filter_list;
static get_func_t get_area_inst;

static get_func_t get_gen_inst;

int ospf_setup(struct confd_daemon_ctx *dctx) {
    extern int quagga_not_running;
    if(quagga_not_running) return CONFD_OK;

    struct confd_data_cbs dcb;

    const element_desc_t router_elems[] = {
        PARTS_ROUTER_HANDLER("auto-cost-reference-bandwidth",
                             "auto-cost reference-bandwidth",
                             " ${arg: auto-cost-reference-bandwidth}",
                             0),
        //TODO: "no auto-cost reference-bandwidth" generates warning message with "%" that is considered as an error
        BOOL_ROUTER_HANDLER("compatible-rfc1583", "compatible rfc1583", 0),
        PARTS_ROUTER_HANDLER("default-information-originate",
                             "default-information originate",
                             "${bool: default-information-originate/always, ' always', ''}${ex: default-information-originate/metric, ' metric %s'}" \
                             "${ex: default-information-originate/metric-type, ' metric-type %s'}${ex: default-information-originate/route-map, ' route-map %s'}",
                             0),
        PARTS_ROUTER_HANDLER("default-metric", "default-metric", " ${arg: default-metric}", 0),
        STD_ROUTER_HANDLER("distance", "distance ${arg: distance}", "no distance 1", 0),
        PARTS_ROUTER_HANDLER("distance-ospf",
                             "distance ospf",
                             "${ex: distance-ospf/external, ' external %s'}${ex: distance-ospf/inter-area, ' inter-area %s'}${ex: distance-ospf/intra-area, ' intra-area %s'}",
                             1),
        STD_ROUTER_HANDLER("distribute-list",
                           "distribute-list ${alist: alist} out ${key: source-type}",
                           "no distribute-list alist out ${key: source-type}",
                           0),
        PARTS_ROUTER_HANDLER("log-adjacency-changes", "log-adjacency-changes", "${bool: log-adjacency-changes/detail, ' detail', ''}", 1),
        PARTS_ROUTER_HANDLER("max-metric-router-lsa-startup", "max-metric router-lsa on-startup", " ${arg: max-metric-router-lsa-startup}", 0),
        PARTS_ROUTER_HANDLER("max-metric-router-lsa-shutdown", "max-metric router-lsa on-shutdown", " ${arg: max-metric-router-lsa-shutdown}", 0),
        PARTS_ROUTER_HANDLER("neighbor", "neighbor ${key: ip}", "${ex: poll-interval, ' poll-interval %s'}${ex: priority, ' priority %s'}", 1),
        PARTS_ROUTER_HANDLER("network-ip", "network ${key: address}/${key: prefix-length} area ${key: area}", "", 0),
        PARTS_ROUTER_HANDLER("ospf-abr-type", "ospf abr-type", " ${arg: ospf-abr-type}", 0),
        PARTS_ROUTER_HANDLER("redistribute",
                             "redistribute ${key: type}",
                             "${ex: metric, ' metric %s'}${ex: metric-type, ' metric-type %s'}${ex: route-map, ' route-map %s'}",
                             0),
        PARTS_ROUTER_HANDLER("refresh-timer", "refresh timer", " ${arg: refresh-timer}", 0),
        PARTS_ROUTER_HANDLER("router-id", "router-id", " ${arg: router-id}", 0),
        PARTS_ROUTER_HANDLER("timers-spf", "timers throttle spf", " ${ex: timers-spf/delay} ${ex: timers-spf/init-hold} ${ex: timers-spf/max-hold}", 0),
        {"passive-interfaces/except", get_passive_except, 0, NULL},
        {"passive-interfaces/passive-by-default", get_passive_by_default, 0, NULL},
        /* the following is mostly only to make zconfd_sub happy */
        {"area", get_area_inst, 0, NULL},
        DELETE_ROUTER_HANDLER("area/virtual-link", "no area ${key: area(2)} virtual-link ${key: addr}"),
        {NULL}};

    const element_desc_t area_elems[] = {
        PARTS_AREA_HANDLER("authentication", "authentication", "${bool: authentication/message-digest, ' message-digest', ''}", 0),
        {"filter-list-prefix", get_filter_list, 0, NULL},
        ASTD_AREA_HANDLER("default-cost", "default-cost ${arg: default-cost}", "default-cost 1", 0),
        PARTS_AREA_HANDLER("nssa", "nssa", " ${ex: nssa/translator-role}${bool: nssa/summary, '', ' no-summary'}", 0),
        PARTS_AREA_HANDLER("stub", "stub", "${bool: stub/summary, '', ' no-summary'}", 0),
        PARTS_AREA_HANDLER("range", "range ${key: addr}/${key: prefix-length}",
                           "${bool: advertise, '', ' not-advertise'}${ex: cost, ' cost %s'}${ex: substitute/addr, ' substitute %s/'}${ex: substitute/prefix-length}", 0),
        PARTS_AREA_HANDLER("shortcut", "shortcut ${arg: shortcut}", "", 0),
        PARTS_AREA_HANDLER("virtual-link/authentication",
                           "virtual-link ${key: addr} authentication",
                           "${ex: authentication/type, ' %s', ''}${ex: authentication/key, ' authentication-key %s', ''}",
                           0),
        PARTS_AREA_HANDLER("virtual-link/message-digest-key", "virtual-link ${key: addr(2)} message-digest-key ${key: key-id}", " md5 ${ex: md5}", 1),
        PARTS_AREA_HANDLER("virtual-link/dead-interval", "virtual-link ${key: addr} dead-interval", " ${arg: dead-interval}", 0),
        PARTS_AREA_HANDLER("virtual-link/hello-interval", "virtual-link ${key: addr} hello-interval", " ${arg: hello-interval}", 0),
        PARTS_AREA_HANDLER("virtual-link/retransmit-interval", "virtual-link ${key: addr} retransmit-interval", " ${arg: retransmit-interval}", 0),
        PARTS_AREA_HANDLER("virtual-link/transmit-delay", "virtual-link ${key: addr} transmit-delay", " ${arg: transmit-delay}", 0),
        {NULL}};

    const element_desc_t if_elems[] = {
        PARTS_IF_HANDLER("authentication", "ip ospf authentication", "${ex: authentication/type, ' %s'}", 0),
        STD_IF_HANDLER("authentication-ip",
                       "ip ospf authentication${ex: type, ' %s'} ${key: if-address}",
                       "no ip ospf authentication ${key: if-address}",
                       0),
        PARTS_IF_HANDLER("authentication-key", "ip ospf authentication-key", " ${arg: authentication-key}", 0),
        STD_IF_HANDLER("authentication-key-ip",
                       "ip ospf authentication-key ${ex: authentication-key} ${key: if-address}",
                       "no ip ospf authentication-key ${key: if-address}",
                       0),
        PARTS_IF_HANDLER("cost", "ip ospf cost", " ${arg: cost}", 0),
        STD_IF_HANDLER("cost-ip",
                       "ip ospf cost ${ex: cost} ${key: if-address}",
                       "no ip ospf cost ${key: if-address}",
                       0),
        PARTS_IF_HANDLER("dead-interval",
                         "ip ospf dead-interval",
                         "${ex: dead-interval/dead-interval, ' %s'}${ex: dead-interval/dead-interval-minimal-hello-multiplier, ' minimal hello-multiplier %s'}",
                         0),
        STD_IF_HANDLER("dead-interval-ip",
                       "ip ospf dead-interval${ex: dead-interval, ' %s'}${ex: dead-interval-minimal-hello-multiplier, ' minimal hello-multiplier %s'} ${key: if-address}",
                       "no ip ospf dead-interval ${key: if-address}",
                       0),
        PARTS_IF_HANDLER("hello-interval", "ip ospf hello-interval", " ${arg: hello-interval}", 0),
        STD_IF_HANDLER("hello-interval-ip",
                       "ip ospf hello-interval ${ex: hello-interval} ${key: if-address}",
                       "no ip ospf hello-interval ${key: if-address}",
                       0),
        PARTS_IF_HANDLER("message-digest-key", "ip ospf message-digest-key ${key: key-id}", " md5 ${ex: md5}", 0),
        STD_IF_HANDLER("message-digest-key-ip",
                       "ip ospf message-digest-key ${key: key-id} md5 ${ex: md5} ${key: if-address}",
                       "no ip ospf message-digest-key ${key: key-id} ${key: if-address}",
                       0),
        BOOL_IF_HANDLER("mtu-ignore", "ip ospf mtu-ignore", 0),
        PARTS_IF_HANDLER("mtu-ignore-ip", "ip ospf mtu-ignore ${key: if-address}", "", 0),
        PARTS_IF_HANDLER("network", "ip ospf network ${arg: network}", "", 0),
        PARTS_IF_HANDLER("priority", "ip ospf priority", " ${arg: priority}", 0),
        STD_IF_HANDLER("priority-ip",
                       "ip ospf priority ${ex: priority} ${key: if-address}",
                       "no ip ospf priority ${key: if-address}",
                       0),
        PARTS_IF_HANDLER("retransmit-interval", "ip ospf retransmit-interval", " ${arg: retransmit-interval}", 0),
        STD_IF_HANDLER("retransmit-interval-ip",
                       "ip ospf retransmit-interval ${ex: retransmit-interval} ${key: if-address}",
                       "no ip ospf retransmit-interval ${key: if-address}",
                       0),
        PARTS_IF_HANDLER("transmit-delay", "ip ospf transmit-delay", " ${arg: transmit-delay}", 0),
        STD_IF_HANDLER("transmit-delay-ip",
                       "ip ospf transmit-delay ${ex: transmit-delay} ${key: if-address}",
                       "no ip ospf transmit-delay ${key: if-address}",
                       0),
        {NULL}};

    /* Register data callback */
    memset( &dcb, 0, sizeof(dcb));
    strcpy( dcb.callpoint, "ospfGetMemory");
    dcb.get_elem = get_ospf_memory_elem;

    if ( confd_register_data_cb( dctx, &dcb) != CONFD_OK) {
        ERROR_LOG( "Failed to register callpoint '%s'", dcb.callpoint);
        return CONFD_ERR;
    }

    CHECK_OK(make_subscription(dctx, ospfd, OSPF_PRIO, subscr_router_init, "router/ospf", router_elems));
    CHECK_OK(make_subscription(dctx, ospfd, OSPF_PRIO, subscr_router_init, "router/ospf/area", area_elems));
    CHECK_OK(make_subscription(dctx, ospfd, OSPF_PRIO, subscr_if_init, "interface/ip/ospf", if_elems));

    CHECK_OK(subscribe_single_element(dctx, OSPF_PRIO, "interface", get_gen_inst, gen_priv(BASE_PROMPT, "no interface ${key: name}")));
    CHECK_OK(subscribe_single_element(dctx, OSPF_PRIO, "router/ospf", get_gen_inst, gen_priv(BASE_PROMPT, "no router ospf")));
    CHECK_OK(subscribe_single_element(dctx, OSPF_EARLY_PRIO, "router/ospf/area/nssa", get_area_type, (void*)NSPREF(nssa)));
    CHECK_OK(subscribe_single_element(dctx, OSPF_EARLY_PRIO, "router/ospf/area/stub", get_area_type, (void*)NSPREF(stub)));
    return CONFD_OK;
}

int get_gen_inst(cb_data_t *data, int set)
{
    if (! set) {
        gen_priv_t *priv = data->priv;
        OSPF_COMMAND(data, priv->command, priv->prompt);
    }
    return CONFD_OK;
}

int test_delete(cb_data_t *data, const char *elem, const char *cmd_part)
{
    if (cdb_exists(data->datasock, elem) == 1)
        return OSPF_ROUTER_COMMAND(data, "no " AREA_PREFIX "%s", cmd_part);
    return CONFD_OK;
}

int delete_vlink(cb_data_t *data, int ignored)
{
    return OSPF_ROUTER_COMMAND(data, "no " AREA_PREFIX "virtual-link ${ex: addr}");
}

int delete_range(cb_data_t *data, int ignored)
{
    return OSPF_ROUTER_COMMAND(data, "no " AREA_PREFIX "range ${ex: addr}/${ex: prefix-length}");
}

int delete_filter_list(cb_data_t *data, int ignored)
{
    return get_filter_list(data, 0);
}

static int get_area_inst(cb_data_t *data, int set)
{
    if (! set) {
        /* all subelements of the area must be deleted so as it is removed from ospf */
        CHECK_OK(test_delete(data, "default-cost", "default-cost ${ex: default-cost}"));
        CHECK_OK(test_delete(data, "nssa", "nssa"));
        CHECK_OK(test_delete(data, "stub", "stub"));
        CHECK_OK(get_dynamic(data, "virtual-link", delete_vlink));
        CHECK_OK(test_delete(data, "authentication", "authentication"));
        CHECK_OK(get_dynamic(data, "filter-list-prefix", delete_filter_list));
        CHECK_OK(get_dynamic(data, "range", delete_range));
        CHECK_OK(test_delete(data, "shortcut", "shortcut enable"));
    }
    return CONFD_OK;
}

static int get_except_if(cb_data_t *data, int set)
{
    OSPF_ROUTER_COMMAND(data, "%spassive-interface ${ex: ifname}", (int)data->priv ? "no " : "");
    return CONFD_OK;
}

int get_passive_by_default(cb_data_t *data, int set)
{
    OSPF_ROUTER_COMMAND(data, "${bool: passive-by-default, '', 'no '}passive-interface default");
    CHECK_OK(cdb_get_bool(data->datasock, (int*) &data->priv, "passive-by-default"));
    CHECK_OK(get_dynamic(data, "except", get_except_if));
    return CONFD_OK;
}

int get_passive_except(cb_data_t *data, int set)
{
    if (set)
        OSPF_ROUTER_COMMAND(data, "${bool: ../passive-by-default, 'no ', ''}passive-interface ${key: ifname}");
    else {
        /* in this case the cdb path was not set */
        char ifname[KPATH_MAX], path[KPATH_MAX];
        confd_hkeypath_t kp, *t_kp;
        int passive_def;
        if (get_data_key(data, "ifname", 0, ifname, KPATH_MAX) == CONFD_ERR)
            return CONFD_ERR;
        t_kp = zconfd_flatten_data_path(&kp, data, 2);
        CHECK_OK(confd_pp_kpath(path, KPATH_MAX, &kp));
        CHECK_OK(cdb_pushd(data->newsock, path)); /* need to use the new data socket! */
        CHECK_OK(cdb_get_bool(data->newsock, &passive_def, "passive-by-default"));
        OSPF_ROUTER_COMMAND(data, "%spassive-interface %s", passive_def ? "" : "no ", ifname);
        CHECK_OK(cdb_popd(data->newsock));
        zconfd_recover_data_path(data, t_kp);
    }
    return CONFD_OK;
}

int get_filter_list(cb_data_t *data, int set)
{
    if (set)
        OSPF_ROUTER_COMMAND(data, AREA_PREFIX "filter-list prefix ${ex: prefix-list} ${key: direction}");
    else {
        OSPF_ROUTER_COMMAND(data, "no " AREA_PREFIX "filter-list prefix ${ex: prefix-list} ${key: direction}");
    }
    return CONFD_OK;
}

int get_area_type(cb_data_t *data, int set)
{
    int tag = (int) data->priv;
    const char *type = tag == NSPREF(nssa) ? "nssa" : "stub";
    OSPF_ROUTER_COMMAND(data, "router ospf");
    if (set)
        if (tag == NSPREF(nssa))
            OSPF_ROUTER_COMMAND(data, AREA_PREFIX "nssa ${ex: nssa/translator-role}${bool: nssa/summary, '', ' no-summary'}");
        else
            OSPF_ROUTER_COMMAND(data, AREA_PREFIX "stub ${bool: stub/summary, '', ' no-summary'}");
    else
        OSPF_ROUTER_COMMAND(data, "no " AREA_PREFIX "%s", type);
    OSPF_BASE_COMMAND(data, "exit");
    return CONFD_OK;
}

/* Validations */

int validateOspfNetworkIp(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval)
{
    int maapisock, kp_start;
    struct maapi_cursor mc;
    int valid = 1;

    assert(keypath->v[0][0].type == C_IPV4 && keypath->v[0][2].type == C_IPV4);

    /* address/prefix pair must be valid */
    CHECK_OK(validateAddrPrefixPair(tctx, keypath, newval));
    CHECK_OK(zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));
    CHECK_OK1(maapi_init_cursor(maapisock, tctx->thandle, &mc, "router/ospf/network-ip"));
    while (maapi_get_next(&mc) == CONFD_OK && mc.n != 0) {
        if (memcmp(&keypath->v[0][2].val.ip, &mc.keys[2].val.ip, sizeof(struct in_addr)) == 0)
            continue;
        if (CONFD_GET_UINT8(&keypath->v[0][1]) == CONFD_GET_UINT8(&mc.keys[1]) &&
            memcmp(&keypath->v[0][0].val.ip, &mc.keys[0].val.ip, sizeof(struct in_addr)) == 0) {
            valid = 0;
            break;
        }
    }
    maapi_destroy_cursor(&mc);
    if (! valid)
        VALIDATE_FAIL("network-ip must be uniquely determined by address and prefix-length");
    return CONFD_OK;
}

int validateDistanceOspf(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval)
{
    int maapisock, kp_start;
    int exter, intra, inter;
    char path[KPATH_MAX];

    CHECK_OK(zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));
    confd_pp_kpath(path, KPATH_MAX, keypath);
    exter = maapi_exists(maapisock, tctx->thandle, "%s/external", path) == 1;
    intra = maapi_exists(maapisock, tctx->thandle, "%s/intra-area", path) == 1;
    inter = maapi_exists(maapisock, tctx->thandle, "%s/inter-area", path) == 1;
    if (!exter && !intra && !inter)
        VALIDATE_FAIL("At least one of external, intra-area or inter-area must be set");

    return CONFD_OK;
}

int validateAreaType(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval)
{
    int maapisock, kp_start;
    int nssa, stub, virt, dcost, num = 0;
    char path[KPATH_MAX];

    CHECK_OK(zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));
    confd_pp_kpath(path, KPATH_MAX, keypath);
    nssa = maapi_exists( maapisock, tctx->thandle, "%s/nssa", path) == 1;
    stub = maapi_exists( maapisock, tctx->thandle, "%s/stub", path) == 1;
    virt = maapi_num_instances( maapisock, tctx->thandle, "%s/virtual-link", path);
    dcost = maapi_exists( maapisock, tctx->thandle, "%s/default-cost", path) == 1;

    if ( nssa) {
        //DEBUG_LOG( "inc nssa");
        num++;
    }
    if ( stub) {
        //DEBUG_LOG( "inc stub");
        num++;
    }

    if ( virt) {
        //DEBUG_LOG( "inc virt");
        num++;
    }

    if ( num > 1)
        VALIDATE_FAIL( "%s: at most one of values can be set - nssa or stub, in which case virtual-link must be undefined", path);

    if ( dcost && (!nssa && !stub))
        VALIDATE_FAIL( "%s: default cost can be set only for stub or nssa areas", path);

    return CONFD_OK;
}

int validateRangeValuesOspf(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval) {
    int maapisock, kp_start;
    char path[KPATH_MAX];
    int adv, cost, subst;

    CHECK_OK(validateAddrPrefixPair(tctx, keypath, newval));

    CHECK_OK(zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));
    confd_pp_kpath(path, KPATH_MAX, keypath);

    maapi_get_bool_elem( maapisock, tctx->thandle, &adv, "%s/advertise", path);
    cost = maapi_exists( maapisock, tctx->thandle, "%s/cost", path) == 1;
    subst = maapi_exists( maapisock, tctx->thandle, "%s/substitute", path) == 1;

    //DEBUG_LOG( "adv %d, cost %d, subst %d", adv, cost, subst);

    // if advertise is false, neither cost or substitute can be set
    if (( adv == 0) && (cost || subst))
        VALIDATE_FAIL( "%s: if advertise is false, neither cost or substitute can be set", path);

    //if cost is set, substitute must not be set
    if (cost && subst)
        VALIDATE_FAIL( "%s: if cost is set, substitute must not be set", path);

    return CONFD_OK;
}

int validateRangeSubstitute(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval)
{
    return validateAddrPrefixPaths(tctx, keypath, "addr", "prefix-length");
}

int validateAutoCostReferenceBandwidth(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval) {
    return CONFD_OK; //VALIDATE_WARN( "Reference bandwidth is changed to %d. Please ensure reference bandwidth is consistent across all routers.", CONFD_GET_UINT32( newval));
}

int get_ospf_memory_elem(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath) {
    return get_memory_elem( tctx, keypath, ospfd);
}

static int validateAreaShortcut(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval)
{
    if (CONFD_GET_IPV4(&keypath->v[1][0]).s_addr == 0)
        VALIDATE_FAIL("You cannot configure shortcut to backbone area");
    return CONFD_OK;
}
