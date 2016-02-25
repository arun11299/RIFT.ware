#include <stdlib.h>
#include "confd_global.h"

#include "quagga.h"     /* generated from yang */

int validatePrefixCombination(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval);
int validateLifeTime(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval);
static int get_route_elem(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath);
static int get_next_route(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, long next);
static int get_zebra_memory_elem(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath);

extern action_cb_t zebra_route_action;

static struct confd_valpoint_cb valpoints[] = {
        { .valpoint = "valAddrPrefixPair", .validate = validateAddrPrefixPair},
        { .valpoint = "valPrefixCombination", .validate = validatePrefixCombination},
        { .valpoint = "valLifeTime", .validate = validateLifeTime},
        { .valpoint = ""}
};


#define ZEBRA_COMMAND(data, command, prompt, args...) cli_command( zebra, data, prompt, 1, CLI_ERR_STR, command , ##args)
#define ZEBRA_BASE_COMMAND(data, command, args...) ZEBRA_COMMAND(data, command, ".*\\(config\\)# " , ##args)
#define ZEBRA_IF_COMMAND(data, command, args...) ZEBRA_COMMAND(data, command, ".*\\(config-if\\)# " , ##args)

static int zebra_connect()
{
        //initialize CLI
        if ( cliOpen( &g_clients[zebra], getCliHost( zebra), getCliPort( zebra), getCliTimeout( zebra)) != CLI_SUCC) {
                ERROR_LOG( "cannot connect zebra %s:%d", getCliHost( zebra), getCliPort( zebra));
                return CONFD_ERR;
        }

        cli_command_raw( zebra, NULL, ".*Password: ", 0, NULL, NULL, NULL);

        if ( cli_command_raw( zebra, getCliPassword( zebra), ".*> ", 0, NULL, NULL, NULL) < 0){
                ERROR_LOG( "zebra password set error to %s:%d", getCliHost( zebra), getCliPort( zebra));
                cliClose( &g_clients[zebra]);
                return CONFD_ERR;
        }

        cli_command_raw( zebra, "enable", "Password: ", 0, NULL, NULL, NULL);
        cli_command_raw( zebra, getCliPassword( zebra), ".*# ", 0, NULL, NULL, NULL);
        return CONFD_OK;
}

int zebra_d_init(struct confd_daemon_ctx *dctx) {
        struct confd_data_cbs dcb;

  /* Register validation points */
        register_valpoint_cb_arr( dctx, valpoints);

        /* action callback */
        zconfd_register_action(dctx, zebra_route_action, "kernel-routes");

        extern int quagga_not_running;
        if(quagga_not_running) return CONFD_OK;

        /* Register data callback */
        memset( &dcb, 0, sizeof(dcb));
        strcpy( dcb.callpoint, "zebraGetRoutes");
        dcb.get_elem = get_route_elem;
        dcb.get_next = get_next_route;

        if ( confd_register_data_cb( dctx, &dcb) != CONFD_OK) {
                ERROR_LOG( "Failed to register callpoint '%s'", dcb.callpoint);
                return CONFD_ERR;
        }

        /* Register data callback */
        memset( &dcb, 0, sizeof(dcb));
        strcpy( dcb.callpoint, "zebraGetMemory");
        dcb.get_elem = get_zebra_memory_elem;

        if ( confd_register_data_cb( dctx, &dcb) != CONFD_OK) {
                ERROR_LOG( "Failed to register callpoint '%s'", dcb.callpoint);
                return CONFD_ERR;
        }

        //initialize CLI
        if ( zebra_connect() == CONFD_ERR) {
                ERROR_LOG( "cannot connect to zebra %s:%d", getCliHost( zebra), getCliPort( zebra));
                return CONFD_ERR;
        }

  return CONFD_OK;
}

typedef enum { LEAF_MANDATORY, LEAF_OPTIONAL, LEAF_DYNAMIC } leaf_type_t;

/**
 * Description of element handling.
 */
typedef struct {
        const char *path;       ///< path to the element (relative to subscription base path subscr_descr_t::base_path)
        int tag;                        ///< XML tag to match
        leaf_type_t type;       ///< element type
        int path_len;           ///< depth of the element (relative to sw root, typically /system)
        get_func_t *handler;///< element's get-function
        int reset;                      ///< true if the element should be deleted before changing its value
} leaf_node_t;

typedef struct leaf_list {
        leaf_node_t node;
        struct leaf_list *next;
} leaf_list_t;

/** This is to indicate if the subscription needs in any way special handling. */
typedef enum { TYPE_INTERFACE, COMMON } subscr_type_t;

/**
 * Group of element handlers with common root that is suitable to be handled with one subscription.
 */
typedef struct {
        subscr_type_t subscr_type;
        const char *subscr_path;        ///< Subscription path, passed to cdb
        const char *base_path;          ///< Path where to cd before handling individual elements
        leaf_list_t *list;                      ///< List of element handling data
} subscr_desc_t;

static subscr_cb_t update_interface;
static subscr_cb_t update_elements;
static get_func_t get_interface;
static int get_elements(cb_data_t *data);

static get_func_t get_table;
static get_func_t get_router_id;

static get_func_t get_forwarding;
static get_func_t get_forwarding6;

static get_func_t get_bandwidth;
static get_func_t get_link_detect;
static get_func_t get_multicast;
static get_func_t get_if_up;

static get_func_t get_address;

static get_func_t get_address6;
static get_func_t get_adv_interval_option;
static get_func_t get_home_agent_config_flag;
static get_func_t get_home_agent_lifetime;
static get_func_t get_home_agent_preference;
static get_func_t get_managed_config_flag;
static get_func_t get_other_config_flag;
static get_func_t get_prefix;
static get_func_t get_ra_interval;
static get_func_t get_ra_lifetime;
static get_func_t get_reachable_time_msec;
static get_func_t get_suppress_ra;

static get_func_t get_route_gw;
static get_func_t get_route_if;
static get_func_t get_route_null;

static get_func_t get_route6_gw;
static get_func_t get_route6_if;
static get_func_t get_route6_gw_if;

leaf_list_t *make_list_item_opt(const char *path, int path_len, int tag, leaf_type_t type, get_func_t *handler, int reset)
{
        leaf_list_t *rv = malloc(sizeof(leaf_list_t));
        rv->node.path = path;
        rv->node.path_len = path_len;
        rv->node.tag = tag;
        rv->node.type = type;
        rv->node.handler = handler;
        rv->node.reset = reset;
        rv->next = NULL;
        return rv;
}

leaf_list_t *make_list_item(const char *path, int path_len, int tag, leaf_type_t type, get_func_t *handler)
{
        return make_list_item_opt(path, path_len, tag, type, handler, 0);
}

subscr_desc_t *make_sub_description(subscr_type_t type, const char *subscr_path, const char *base_path, leaf_list_t *list)
{
        subscr_desc_t *rv = malloc(sizeof(subscr_desc_t));
        rv->subscr_type = type;
        rv->subscr_path = subscr_path;
        rv->base_path = base_path;
        rv->list = list;
        return rv;
}

leaf_list_t *chain_list(leaf_list_t *list[])
{
        leaf_list_t **ptr;
        for (ptr = list; *ptr != NULL; ptr++)
                (*ptr)->next = *(ptr+1);
        return list[0];
}

static int make_subscription(struct confd_daemon_ctx *dctx, subscr_desc_t *desc)
{
        cb_data_t data;
        struct confd_cs_node *cs_node = zconfd_locate_cs_node(zconfd_root, desc->subscr_path);
        if (cs_node == NULL)
                return CONFD_ERR;
        zconfd_init_cb_data_ctx(&data, dctx, NULL, MOP_CREATED, NULL, NULL, desc);

        /* this is only guessing, but hopefully works in our case... */
        if (cs_node->children == NULL)
                zconfd_subscribe_cdb_root(dctx, PROTO_PRIO, update_elements, desc->subscr_path, desc);
        else if (desc->list->next != NULL)
                zconfd_subscribe_cdb_tree(dctx, PROTO_PRIO, update_elements, desc->subscr_path, desc);
        else
                zconfd_subscribe_cdb(dctx, PROTO_PRIO, update_elements, desc->subscr_path, desc);
        if (desc->subscr_type == TYPE_INTERFACE)
                CHECK_OK(get_dynamic(&data, "interface", get_interface));
        else
                CHECK_OK(get_elements(&data));
        return CONFD_OK;
}

int zebra_setup(struct confd_daemon_ctx *dctx)
{
        extern int quagga_not_running;
        if(quagga_not_running) return CONFD_OK;

        leaf_list_t
                *nd_lst[] = {
                        make_list_item("adv-interval-option", 4, NSPREF(adv_interval_option), LEAF_OPTIONAL, get_adv_interval_option),
                        make_list_item("home-agent-config-flag", 4, NSPREF(home_agent_config_flag), LEAF_OPTIONAL, get_home_agent_config_flag),
                        make_list_item("home-agent-lifetime", 4, NSPREF(home_agent_lifetime), LEAF_OPTIONAL, get_home_agent_lifetime),
                        make_list_item("home-agent-preference", 4, NSPREF(home_agent_preference), LEAF_OPTIONAL, get_home_agent_preference),
                        make_list_item("managed-config-flag", 4, NSPREF(managed_config_flag), LEAF_OPTIONAL, get_managed_config_flag),
                        make_list_item("other-config-flag", 4, NSPREF(other_config_flag), LEAF_OPTIONAL, get_other_config_flag),
                        make_list_item("prefix", 4, NSPREF(prefix), LEAF_DYNAMIC, get_prefix),
                        make_list_item("ra-interval", 4, NSPREF(ra_interval), LEAF_OPTIONAL, get_ra_interval),
                        make_list_item("ra-lifetime", 4, NSPREF(ra_lifetime), LEAF_OPTIONAL, get_ra_lifetime),
                        make_list_item("reachable-time-msec", 4, NSPREF(reachable_time_msec), LEAF_OPTIONAL, get_reachable_time_msec),
                        make_list_item("suppress-ra", 4, NSPREF(suppress_ra), LEAF_OPTIONAL, get_suppress_ra),
                        NULL};

        subscr_desc_t
                *addr_d = make_sub_description(TYPE_INTERFACE, "interface/ip/address", "ip", make_list_item("address", 3, NSPREF(address), LEAF_DYNAMIC, get_address)),
                *addr6_d = make_sub_description(TYPE_INTERFACE, "interface/ipv6/address", "ipv6", make_list_item("address", 3, NSPREF(address), LEAF_DYNAMIC, get_address6)),
                *nd_d = make_sub_description(TYPE_INTERFACE, "interface/ipv6/nd", "ipv6/nd", chain_list(nd_lst));

        cli_configure_command(1, zebra);

        zconfd_subscribe_cdb_root(dctx, PROTO_PRIO, update_interface, "interface", NULL);
        CHECK_OK(make_subscription(dctx, make_sub_description(COMMON, "router-id", NULL,
                                                                                                                                 make_list_item("router-id", 0, NSPREF(router_id), LEAF_OPTIONAL, get_router_id))));
        CHECK_OK(make_subscription(dctx, make_sub_description(COMMON, "table", NULL,
                                                                                                                                 make_list_item("table", 0, NSPREF(table), LEAF_OPTIONAL, get_table))));
        CHECK_OK(make_subscription(dctx, make_sub_description(TYPE_INTERFACE, "interface/bandwidth", NULL,
                                                                                                                                 make_list_item("bandwidth", 2, NSPREF(bandwidth), LEAF_OPTIONAL, get_bandwidth))));
        CHECK_OK(make_subscription(dctx, make_sub_description(TYPE_INTERFACE, "interface/link-detect", NULL,
                                                                                                                                 make_list_item("link-detect", 2, NSPREF(link_detect), LEAF_MANDATORY, get_link_detect))));
        CHECK_OK(make_subscription(dctx, make_sub_description(TYPE_INTERFACE, "interface/multicast", NULL,
                                                                                                                                 make_list_item("multicast", 2, NSPREF(multicast), LEAF_MANDATORY, get_multicast))));
        CHECK_OK(make_subscription(dctx, make_sub_description(TYPE_INTERFACE, "interface/up", NULL,
                                                                                                                                 make_list_item("up", 2, NSPREF(up), LEAF_MANDATORY, get_if_up))));
        CHECK_OK(make_subscription(dctx, addr_d));
        CHECK_OK(make_subscription(dctx, addr6_d));
        CHECK_OK(make_subscription(dctx, nd_d));
        CHECK_OK(make_subscription(dctx, make_sub_description(COMMON, "ip/forwarding", "ip",
                                                                                                                                 make_list_item_opt("forwarding", 1, NSPREF(forwarding), LEAF_MANDATORY, get_forwarding, 1))));
        CHECK_OK(make_subscription(dctx, make_sub_description(COMMON, "ipv6/forwarding", "ipv6",
                                                                                                                                 make_list_item_opt("forwarding", 1, NSPREF(forwarding), LEAF_MANDATORY, get_forwarding6, 1))));
        CHECK_OK(make_subscription(dctx, make_sub_description(COMMON, "ip/route-gw", "ip",
                                                                                                                                 make_list_item_opt("route-gw", 1, NSPREF(route_gw), LEAF_DYNAMIC, get_route_gw, 1))));
        CHECK_OK(make_subscription(dctx, make_sub_description(COMMON, "ip/route-if", "ip",
                                                                                                                                 make_list_item_opt("route-if", 1, NSPREF(route_if), LEAF_DYNAMIC, get_route_if, 1))));
        CHECK_OK(make_subscription(dctx, make_sub_description(COMMON, "ip/route-null", "ip",
                                                                                                                                 make_list_item_opt("route-null", 1, NSPREF(route_null), LEAF_DYNAMIC, get_route_null, 1))));
        CHECK_OK(make_subscription(dctx, make_sub_description(COMMON, "ipv6/route-gw", "ipv6",
                                                                                                                                 make_list_item_opt("route-gw", 1, NSPREF(route_gw), LEAF_DYNAMIC, get_route6_gw, 1))));
        CHECK_OK(make_subscription(dctx, make_sub_description(COMMON, "ipv6/route-if", "ipv6",
                                                                                                                                 make_list_item_opt("route-if", 1, NSPREF(route_if), LEAF_DYNAMIC, get_route6_if, 1))));
        CHECK_OK(make_subscription(dctx, make_sub_description(COMMON, "ipv6/route-gw-if", "ipv6",
                                                                                                                                 make_list_item_opt("route-gw-if", 1, NSPREF(route_gw_if), LEAF_DYNAMIC, get_route6_gw_if, 1))));
        cli_configure_command(0, zebra);

        return CONFD_OK;
}

int get_interface(cb_data_t *data, int set)
{
        ZEBRA_IF_COMMAND(data, "interface ${ex: name}");
        CHECK_OK(get_elements(data));
        return ZEBRA_BASE_COMMAND(data, "exit");
}

int get_elements(cb_data_t *data)
{
        leaf_list_t *ptr;
        subscr_desc_t *desc = data->priv;

        if (desc->base_path != NULL)
                CHECK_OK(cdb_pushd(data->datasock, desc->base_path));
        for (ptr = desc->list; ptr != NULL; ptr = ptr->next) {
                leaf_node_t *sub = &ptr->node;
                switch (sub->type) {
                case LEAF_OPTIONAL:
                        CHECK_OK(test_single(data, sub->path, sub->handler));
                        break;
                case LEAF_DYNAMIC:
                        CHECK_OK(get_dynamic(data, sub->path, sub->handler));
                        break;
                case LEAF_MANDATORY:
                        CHECK_OK(sub->handler(data, 1));
                        break;
                }
        }
        if (desc->base_path != NULL)
                cdb_popd(data->datasock);
        return CONFD_OK;
}

enum cdb_iter_ret update_interface(cb_data_t *data)
{
        if (data->op == MOP_DELETED)
                ZEBRA_BASE_COMMAND(data, "no interface ${key: name}");
        /* other cases handled elsewhere */
        return ITER_CONTINUE;
}

enum cdb_iter_ret update_elements(cb_data_t *data)
{
        leaf_list_t *ptr;
        subscr_desc_t *desc = data->priv;
        int ret = CONFD_OK;
        DUMP_UPDATE(__FUNCTION__, data);
        for (ptr = desc->list; ptr != NULL; ptr = ptr->next) {
                leaf_node_t *sub = &ptr->node;
                int set = data->op != MOP_DELETED;
                if (sub->path_len > data->kp_start)
                        continue;
                confd_value_t *v = data->kp->v[data->kp_start - sub->path_len];
                if (v->type != C_XMLTAG)
                        continue;
                if (v->val.xmltag.tag == sub->tag) {
                        int flatten;
                        confd_hkeypath_t flat_kp, *t_kp;
                        if (sub->type == LEAF_DYNAMIC)
                                if (sub->path_len + 1 > data->kp_start)
                                        return ITER_RECURSE;
                                else {
                                        flatten = data->kp_start - sub->path_len - 1;
                                        set = set || flatten > 0;
                                }
                        else {
                                flatten = data->kp_start - sub->path_len + 1;
                                set = set || flatten > 1;
                        }
                        if (desc->subscr_type == TYPE_INTERFACE)
                                CHECK_OK(ZEBRA_IF_COMMAND(data, "interface ${key: name(-2)}"));
                        t_kp = zconfd_flatten_data_path(&flat_kp, data, flatten);
                        if (set) {
                                char path[KPATH_MAX];
                                confd_pp_kpath(path, KPATH_MAX, data->kp);
                                CHECK_OK(cdb_pushd(data->datasock, path));
                        }
                        if (data->op != MOP_CREATED && set && sub->reset)
                                ret = sub->handler(data, 0);
                        if (ret != CONFD_ERR)
                                ret = sub->handler(data, set);
                        if (set)
                                cdb_popd(data->datasock);
                        if (desc->subscr_type == TYPE_INTERFACE)
                                ZEBRA_BASE_COMMAND(data, "exit");
                        zconfd_recover_data_path(data, t_kp);
                        CHECK_OK(ret);
                        return ITER_CONTINUE;
                }
        }
        char path[KPATH_MAX];
        confd_pp_kpath(path, KPATH_MAX, data->kp);
        ERROR_LOG("Could not find subscription for %s with %s - %s", path, desc->subscr_path, desc->base_path);
        return CONFD_ERR;
}

int get_bandwidth(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_IF_COMMAND(data, "bandwidth ${arg: bandwidth}");
        else
                ZEBRA_IF_COMMAND(data, "no bandwidth");
        return CONFD_OK;
}

int get_link_detect(cb_data_t *data, int set)
{
        ZEBRA_IF_COMMAND(data, "${bool: link-detect, '', 'no '}link-detect");
        return CONFD_OK;
}

int get_multicast(cb_data_t *data, int set)
{
        int multicast;
        cdb_get_enum_value(data->datasock, &multicast, "multicast");
        switch (multicast) {
        case NSPREF(_MulticastType_yes):
                ZEBRA_IF_COMMAND(data, "multicast");
                break;
        case NSPREF(_MulticastType_no):
                ZEBRA_IF_COMMAND(data, "no multicast");
                break;
        case NSPREF(_MulticastType_if_default):
                /* can't be done once the value has changed */
                DEBUG_LOG("interface multicast set to default...");
                break;
        }
        return CONFD_OK;
}

int get_table(cb_data_t *data, int set)
{
        ZEBRA_BASE_COMMAND(data, "table ${arg: table}");
        return CONFD_OK;
}

int get_router_id(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_BASE_COMMAND(data, "router-id ${arg: router-id}");
        else
                ZEBRA_BASE_COMMAND(data, "no router-id");
        return CONFD_OK;
}

int get_if_up(cb_data_t *data, int set)
{
        ZEBRA_IF_COMMAND(data, "${bool: up, 'no ', ''}shutdown");
        return CONFD_OK;
}

int get_address(cb_data_t *data, int set)
{
        ZEBRA_IF_COMMAND(data, "%sip address ${key: address}/${key: prefix-length}", set ? "" : "no ");
        return CONFD_OK;
}

int get_address6(cb_data_t *data, int set)
{
        ZEBRA_IF_COMMAND(data, "%sipv6 address ${key: address}/${key: prefix-length}", set ? "" : "no ");
        return CONFD_OK;
}

int get_adv_interval_option(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_IF_COMMAND(data, "${bool: adv-interval-option, '', 'no '}ipv6 nd adv-interval-option");
        else
                ZEBRA_IF_COMMAND(data, "no ipv6 nd adv-interval-option");
        return CONFD_OK;
}

int get_home_agent_config_flag(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_IF_COMMAND(data, "${bool: home-agent-config-flag, '', 'no '}ipv6 nd home-agent-config-flag");
        else
                ZEBRA_IF_COMMAND(data, "no ipv6 nd home-agent-config-flag");
        return CONFD_OK;
}

int get_home_agent_lifetime(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_IF_COMMAND(data, "ipv6 nd home-agent-lifetime ${arg: home-agent-lifetime}");
        else
                ZEBRA_IF_COMMAND(data, "no ipv6 nd home-agent-lifetime");
        return CONFD_OK;
}

int get_home_agent_preference(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_IF_COMMAND(data, "ipv6 nd home-agent-preference ${arg: home-agent-preference}");
        else
                ZEBRA_IF_COMMAND(data, "no ipv6 nd home-agent-preference");
        return CONFD_OK;
}

int get_managed_config_flag(cb_data_t *data, int set)
{
        ZEBRA_IF_COMMAND(data, "${bool: managed-config-flag, '', 'no '}ipv6 nd managed-config-flag");
        return CONFD_OK;
}

int get_other_config_flag(cb_data_t *data, int set)
{
        ZEBRA_IF_COMMAND(data, "${bool: other-config-flag, '', 'no '}ipv6 nd other-config-flag");
        return CONFD_OK;
}

int get_prefix(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_IF_COMMAND(data, "ipv6 nd prefix ${key: address}/${key: prefix-length} ${ex: lifetime/valid-lifetime} ${ex: lifetime/preferred-lifetime}"\
                                                 "${bool: off-link, ' off-link', ''}${bool: no-autoconfig, ' no-autoconfig', ''}${bool: router-address, ' router-address', ''}");
        else
                ZEBRA_IF_COMMAND(data, "no ipv6 nd prefix ${key: address}/${key: prefix-length}");
        return CONFD_OK;
}

int get_ra_interval(cb_data_t *data, int set)
{
        if (set) {
                int unit;
                cdb_get_enum_value(data->datasock, &unit, "ra-interval/unit");
                ZEBRA_IF_COMMAND(data, "ipv6 nd ra-interval %s${ex: ra-interval/value}", unit == NSPREF(_TimeUnitType_msec) ? "msec " : "");
        } else
                ZEBRA_IF_COMMAND(data, "no ipv6 nd ra-interval");
        return CONFD_OK;
}

int get_ra_lifetime(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_IF_COMMAND(data, "ipv6 nd ra-lifetime ${arg: ra-lifetime}");
        else
                ZEBRA_IF_COMMAND(data, "no ipv6 nd ra-lifetime");
        return CONFD_OK;
}

int get_reachable_time_msec(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_IF_COMMAND(data, "ipv6 nd reachable-time ${arg: reachable-time-msec}");
        else
                ZEBRA_IF_COMMAND(data, "no ipv6 nd reachable-time");
        return CONFD_OK;
}

int get_suppress_ra(cb_data_t *data, int set)
{
        ZEBRA_IF_COMMAND(data, "${bool: suppress-ra, '', 'no '}ipv6 nd suppress-ra");
        return CONFD_OK;
}

int get_route_gw(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_BASE_COMMAND(data, "ip route ${key: destination}/${key: dst-prefix-length} ${key: gw}" \
                                                   "${ex: operation, ' %%s'}${ex: distance, ' %%s'}");
        else
                ZEBRA_BASE_COMMAND(data, "no ip route ${key: destination}/${key: dst-prefix-length} ${key: gw}");
        return CONFD_OK;
}

int get_route_if(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_BASE_COMMAND(data, "ip route ${key: destination}/${key: dst-prefix-length} ${key: interface}" \
                                                   "${ex: operation, ' %%s'}${ex: distance, ' %%s'}");
        else
                ZEBRA_BASE_COMMAND(data, "no ip route ${key: destination}/${key: dst-prefix-length} ${key: interface}");
        return CONFD_OK;
}

int get_route_null(cb_data_t *data, int set)
{
        if (set)
                ZEBRA_BASE_COMMAND(data, "ip route ${key: destination}/${key: dst-prefix-length} null0 ${ex: distance, ' %%s'}");
        else
                ZEBRA_BASE_COMMAND(data, "no ip route ${key: destination}/${key: dst-prefix-length} null0");
        return CONFD_OK;
}

int get_distance(cb_data_t *data, char *dist, int size)
{
        CHECK_OK(get_data_key(data, "distance", 0, dist, size));
        if (strcmp(dist, "default") == 0)
                dist[0] = 0;
        return CONFD_OK;
}

int get_route6_gw(cb_data_t *data, int set)
{
        char distance[KPATH_MAX];
        CHECK_OK(get_distance(data, distance, KPATH_MAX));
        if (set)
                ZEBRA_BASE_COMMAND(data, "ipv6 route ${key: destination}/${key: dst-prefix-length} ${key: gw}"\
                                                   "${ex: operation, ' %%s'} %s", distance);
        else
                ZEBRA_BASE_COMMAND(data, "no ipv6 route ${key: destination}/${key: dst-prefix-length} ${key: gw} %s", distance);
        return CONFD_OK;
}

int get_route6_if(cb_data_t *data, int set)
{
        char distance[KPATH_MAX];
        CHECK_OK(get_distance(data, distance, KPATH_MAX));
        if (set)
                ZEBRA_BASE_COMMAND(data, "ipv6 route ${key: destination}/${key: dst-prefix-length} ${key: interface}"\
                                                   "${ex: operation, ' %%s'} %s", distance);
        else
                ZEBRA_BASE_COMMAND(data, "no ipv6 route ${key: destination}/${key: dst-prefix-length} ${key: interface} %s", distance);
        return CONFD_OK;
}

int get_route6_gw_if(cb_data_t *data, int set)
{
        char distance[KPATH_MAX];
        CHECK_OK(get_distance(data, distance, KPATH_MAX));
        if (set)
                ZEBRA_BASE_COMMAND(data, "ipv6 route ${key: destination}/${key: dst-prefix-length} ${key: gw} ${key: interface}"\
                                                   "${ex: operation, ' %%s'} %s", distance);
        else
                ZEBRA_BASE_COMMAND(data, "no ipv6 route ${key: destination}/${key: dst-prefix-length} ${key: gw} ${key: interface} %s", distance);
        return CONFD_OK;
}

int get_forwarding(cb_data_t *data, int set)
{
        ZEBRA_BASE_COMMAND(data, "${bool: forwarding, '', 'no '}ip forwarding");
        return CONFD_OK;
}

int get_forwarding6(cb_data_t *data, int set)
{
        ZEBRA_BASE_COMMAND(data, "${bool: forwarding, '', 'no '}ipv6 forwarding");
        return CONFD_OK;
}

/**** validations ****/

#define PRESENT(f) (f ? "present" : "missing")

int validatePrefixCombination(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval)
{
        char path[KPATH_MAX];
        int off_link, no_autoconfig, router_address, lifetime;
        int maapisock, kp_start;
        int th = tctx->thandle;
        CHECK_OK( zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));

        confd_pp_kpath(path, KPATH_MAX, keypath);
        CHECK_OK(maapi_get_bool_elem(maapisock, th, &off_link, "%s/%s", path, "off-link"));
        CHECK_OK(maapi_get_bool_elem(maapisock, th, &no_autoconfig, "%s/%s", path, "no-autoconfig"));
        CHECK_OK(maapi_get_bool_elem(maapisock, th, &router_address, "%s/%s", path, "router-address"));
        lifetime = maapi_exists(maapisock, th, "%s/%s", path, "lifetime") == 1;

        if (off_link && no_autoconfig && router_address && lifetime)
                return CONFD_OK;
        if ((no_autoconfig || off_link) && router_address)
                VALIDATE_FAIL("This combination of fields is invalid: lifetime %s, off-link %s, no-autoconfig %s, router-address %s",
                                          PRESENT(lifetime), PRESENT(off_link), PRESENT(no_autoconfig), PRESENT(router_address));
        return CONFD_OK;
}

int validateLifeTime(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval)
{
        int maapisock, kp_start;
        confd_value_t preferred, valid;
        int th = tctx->thandle;
        char path[KPATH_MAX];
        CHECK_OK( zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));
        confd_pp_kpath(path, KPATH_MAX, keypath);
        CHECK_OK(maapi_get_elem(maapisock, th, &preferred, "%s/%s", path, "preferred-lifetime"));
        CHECK_OK(maapi_get_elem(maapisock, th, &valid, "%s/%s", path, "valid-lifetime"));
        if (valid.type == C_UINT32) {
                if (preferred.type != C_UINT32)
                        VALIDATE_FAIL("Valid time (%d) must not be less then preferred time (infinite)", CONFD_GET_UINT32(&valid));
                else if (CONFD_GET_UINT32(&valid) < CONFD_GET_UINT32(&preferred))
                        VALIDATE_FAIL("Valid time (%d) must not be less then preferred time (%d)", CONFD_GET_UINT32(&valid), CONFD_GET_UINT32(&preferred));
        }
        return CONFD_OK;
}

struct tRouteEntry {
        int type;
        struct in_addr ipv4;
        unsigned char prefix;
        char* target;
        struct tRouteEntry *next;
};

int g_enableReloadRoutes = 1;
static struct tRouteEntry *g_routes = NULL;

/*********************************************/
void freeRoutesList( struct tRouteEntry** pEntry) {
        struct tRouteEntry *ae = *pEntry;
        while (ae) {
                struct tRouteEntry *next = ae->next;
                //free target item
                if (ae->target)
                        free(ae->target);
                ae->target = NULL;
                //free the whole structure
                free(ae);
                ae = next;
        }
        *pEntry = NULL;
}

/*********************************************/
struct tRouteEntry *findRoute(const struct in_addr* ip, unsigned char prefix, const char* target) {
        struct tRouteEntry *ae = g_routes;
        while (ae != NULL) {
                if (( ip->s_addr == ae->ipv4.s_addr) && ( prefix == ae->prefix) && ( strcmp( target, ae->target) == 0))
                        return ae;
                ae = ae->next;
        }
        return NULL;
}

/*********************************************/
int routeStrFunc( const char* str, int prompt, void* par) {
        struct tRouteEntry* pr;
        struct tRouteEntry* pr1;
        char* delim = " ,/";
        char buff[ BUFSIZ];
        char* ps;

        //DEBUG_LOG( "routeStrFunc '%s' prompt(%d)", str, prompt);

        if ( prompt)    //prompt after route table?
                return 0;

        if ( strchr( str, '/') == NULL) //process just lines wit '/' char
                return 0;


        //process one line and convert to r
        pr = (struct tRouteEntry*) malloc(sizeof( struct tRouteEntry));
        memset( pr, 0, sizeof( struct tRouteEntry));
        strncpy( buff, str, sizeof( buff));
        pr->type = str[ 0];
        //read flag
        ps = strtok( buff, delim);
        //read IP
        ps = strtok( NULL, delim);
        //DEBUG_LOG( "Token IP: %s", ps);
        inet_pton(AF_INET, ps, &pr->ipv4);
        ps = strtok( NULL, delim);
        //DEBUG_LOG( "Token prefix: %s", ps);
        pr->prefix = atoi( ps);

        ps = strtok( NULL, delim);
        //DEBUG_LOG( "Token type: %s", ps);
        if ( ps[ 0] == '[') {   //metric?
                ps = strtok( NULL, delim);
                //DEBUG_LOG( "Token type0: %s", ps);
                ps = strtok( NULL, delim);
                //DEBUG_LOG( "Token type00: %s", ps);
        }

        if ( strcmp( ps, "via") == 0) {
                ps = strtok( NULL, delim);
                //DEBUG_LOG( "Token gw: %s", ps);
                pr->target = strdup( ps);
        } else if ( strcmp( ps, "is") == 0) {
                ps = strtok( NULL, delim);
                //DEBUG_LOG( "Token type1: %s", ps);
                ps = strtok( NULL, delim);
                //DEBUG_LOG( "Token type2: %s", ps);
                ps = strtok( NULL, delim);
                //DEBUG_LOG( "Token itf: %s", ps);
                pr->target = strdup( ps);
        }

        //DEBUG_LOG( "routeStrFunc '%s', result: %c %s/%d %s", str, pr->type, inet_ntoa( pr->ipv4), pr->prefix, pr->target);

        switch ( pr->type) {    //convert route types to confd equivalents
                case 'K' : pr->type = NSPREF(_RouteType_kernel); break;
                case 'C' : pr->type = NSPREF(_RouteType_connected); break;
                case 'S' : pr->type = NSPREF(_RouteType_static); break;
                case 'R' : pr->type = NSPREF(_RouteType_rip); break;
                case 'O' : pr->type = NSPREF(_RouteType_ospf); break;
                case 'I' : pr->type = NSPREF(_RouteType_isis); break;
                case 'B' : pr->type = NSPREF(_RouteType_bgp); break;
                default:
                        ERROR_LOG( "routeStrFunc - unknown route type '%s'", str);
                        return -1;
        }

        //add r to list
        if ( g_routes == NULL) {        //define root
                g_routes = pr;
                return 0;
        }

        pr1 = g_routes; //find last item
        while ( pr1->next != NULL)
                pr1 = pr1->next;

        pr1->next       = pr;   //save new item

        return 0;
}

/*********************************************/
static int loadRoutesCli() {
        freeRoutesList( &g_routes);
        return cli_command_raw( zebra, "show ip route", ".*# ", 1, CLI_ERR_STR, routeStrFunc, NULL);
}

/*********************************************/
int get_route_elem(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath) {
        struct tRouteEntry *ae;
        confd_value_t v;
        struct in_addr ip = CONFD_GET_IPV4( &keypath->v[1][0]);
        unsigned char prefix = CONFD_GET_UINT8( &keypath->v[1][1]);
        char target[ BUFSIZ];

        confd_pp_value( target, sizeof(target), &keypath->v[1][2]);

        ae = findRoute( &ip, prefix, target);
        if (ae == NULL) {
                confd_data_reply_not_found(tctx);
                return CONFD_OK;
        }

        switch ( CONFD_GET_XMLTAG( &(keypath->v[0][0]))) {
                case NSPREF(type):
                        CONFD_SET_ENUM_VALUE( &v, ae->type);
                        break;
                case NSPREF(network):
                        CONFD_SET_IPV4(&v, ae->ipv4);
                        break;
                case NSPREF(prefix):
                        CONFD_SET_UINT8( &v, ae->prefix);
                        break;
                case NSPREF(target):
                        if ( ae->target == NULL) {
                                confd_data_reply_not_found( tctx);
                                return CONFD_OK;
                        }
                        CONFD_SET_STR(&v, ae->target == NULL ? "<unknown>" : ae->target);
                        break;
                default:
                        ERROR_LOG( "unknown type %d", CONFD_GET_XMLTAG( &(keypath->v[0][0])));
                        return CONFD_ERR;
        }
        confd_data_reply_value(tctx, &v);
        return CONFD_OK;
}

/*********************************************/
int get_next_route(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, long next) {
        struct tRouteEntry *curr = g_routes;
        confd_value_t v[ 4];

        //DEBUG_LOG( "get_next_route");

        if ( g_enableReloadRoutes) {
                loadRoutesCli();
                curr = g_routes;
        }

        if ( next != -1)        //are we inside the list?
                curr = (struct tRouteEntry*) next;

        if ( curr == NULL) { //are we in the end?
                confd_data_reply_next_key(tctx, NULL, -1, -1);
                freeRoutesList( &g_routes);
                g_enableReloadRoutes = 1;
                //DEBUG_LOG( "get_next_route eend");
                return CONFD_OK;
        }

        g_enableReloadRoutes = 0;       //disable reloading because we use pointers for list traversing

        CONFD_SET_ENUM_VALUE( &v[ 0], curr->type);
        CONFD_SET_IPV4(&v[ 1], curr->ipv4);
        CONFD_SET_UINT8(&v[ 2], curr->prefix);
        CONFD_SET_STR(&v[ 3], curr->target);
        confd_data_reply_next_key(tctx, v, 4, (long)curr->next);

        //DEBUG_LOG( "get_next_route end");
        return CONFD_OK;
}

int get_zebra_memory_elem(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath) {
        return get_memory_elem( tctx, keypath, zebra);
}
