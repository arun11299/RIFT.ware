/* -*- c-file-style: "stroustrup"; tab-width: 4; c-file-offsets: ((case-label . +)); -*- */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


#include <confd.h>
#include <confd_cdb.h>
#include <confd_maapi.h>

#undef _POSIX_C_SOURCE

#include "zconfd_api.h"
#include "confd_global.h"

#include "quagga.h"     /* generated from yang */


#define STR_MAX (255)

static int get_conf_router(cb_data_t *data, int set);
static int get_conf_interfaces(cb_data_t *data, int set);

static int get_conf_interfaces_instance(cb_data_t *data, char* ifname,  int set);

static get_func_t get_conf_router_inst;

static subscr_cb_t update_conf_router;
static subscr_cb_t update_conf_if_ip;
static subscr_cb_t update_conf_router_inst;
static subscr_cb_t update_conf_if_ip_inst;
static subscr_cb_t update_conf_router_zebra_redistribute_rip_inst;
int router_zebra_redistribute_rip( cb_data_t *data);

static int update_single_router(cb_data_t *data, get_func_t *get_func);

static int update_single_if_ip(cb_data_t *data, get_func_t *get_func, char *ifname);

static int update_dynamic_flatten(cb_data_t *data, get_func_t *get_func, int kp_ix);

static get_func_t get_version;
static get_func_t get_network_ip;
static get_func_t get_network_ifname;
static get_func_t get_redistribute;
static get_func_t get_default_information_originate;
static get_func_t get_default_metric;
static get_func_t get_neighbor;
static get_func_t get_passive_interface_default;
static get_func_t get_passive_interface;
static get_func_t get_route;
static get_func_t get_distribute_list;
static get_func_t get_distribute_list_prefix;
static get_func_t get_timers_basic;
static get_func_t get_offset_list;
static get_func_t get_distance_default;
static get_func_t get_distance;
static get_func_t get_route_map;

static get_func_t get_authentication_mode;
static get_func_t get_authentication_key_chain;
static get_func_t get_authentication_string;
static get_func_t get_receive_version;
static get_func_t get_send_version;
static get_func_t get_split_horizon;

static int get_rip_memory_elem(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath);


static int ripOnlineTest(struct confd_trans_ctx *tctx,
                           confd_hkeypath_t *keypath,
                           confd_value_t *newval);

static int validateKeyAddrPrefix(struct confd_trans_ctx *tctx,
                           confd_hkeypath_t *keypath,
                           confd_value_t *newval);

static int validateAlistIfname(struct confd_trans_ctx *tctx,
                           confd_hkeypath_t *keypath,
                           confd_value_t *newval);

static int validateIfname(struct confd_trans_ctx *tctx,
                           confd_hkeypath_t *keypath,
                           confd_value_t *newval);

static int validateRipRouteMap(struct confd_trans_ctx *tctx,
                           confd_hkeypath_t *keypath,
                           confd_value_t *newval);

static int validateAuthKeyString(struct confd_trans_ctx *tctx,
                           confd_hkeypath_t *keypath,
                           confd_value_t *newval);

static int validateAuthString(struct confd_trans_ctx *tctx,
                           confd_hkeypath_t *keypath,
                           confd_value_t *newval);

static struct confd_valpoint_cb valpoints[] = {
                { .valpoint = "ripOnlineTest", .validate = ripOnlineTest},
                { .valpoint = "valKeyAddrPrefix", .validate = validateKeyAddrPrefix},
                { .valpoint = "valAlistIfname", .validate = validateAlistIfname},
                { .valpoint = "valRipRouteMap", .validate = validateRipRouteMap},
                { .valpoint = "valIfname", .validate = validateIfname},
                { .valpoint = "valAuthKeyString", .validate = validateAuthKeyString},
                { .valpoint = "valAuthString", .validate = validateAuthString},
                { .valpoint = "" }
};

#define ROUTER_COMMAND(data, command, args...) cli_command( ripd, data, ".*\\(config-router\\)# ", 1, CLI_ERR_STR, command , ##args)
#define IF_COMMAND(data, command, args...) cli_command( ripd, data, ".*\\(config-if\\)# ", 1, CLI_ERR_STR, command , ##args)


int connectCli() {
        //initialize CLI
        if ( cliOpen( &g_clients[ripd], getCliHost( ripd), getCliPort( ripd), getCliTimeout( ripd)) != CLI_SUCC) {
                ERROR_LOG( "cannot connect ripd %s:%d", getCliHost( ripd), getCliPort( ripd));
                return CONFD_ERR;
        }

        cli_command_raw( ripd, NULL, ".*Password: ", 0, NULL, NULL, NULL);

        if ( cli_command_raw( ripd, getCliPassword( ripd), ".*> ", 0, NULL, NULL, NULL) < 0){
                ERROR_LOG( "ripd password set error to %s:%d", getCliHost( ripd), getCliPort( ripd));
                cliClose( &g_clients[ripd]);
                return CONFD_ERR;
        }

        cli_command_raw( ripd, "enable", ".*# ", 0, NULL, NULL, NULL);
        return CONFD_OK;
}

int rip_init(struct confd_daemon_ctx *dctx) {
        struct confd_data_cbs dcb;

   /* Register validation points */
        register_valpoint_cb_arr( dctx, valpoints);

        extern int quagga_not_running;
        if(quagga_not_running) return CONFD_OK;

        /* Register data callback */
        memset( &dcb, 0, sizeof(dcb));
        strcpy( dcb.callpoint, "ripGetMemory");
        dcb.get_elem = get_rip_memory_elem;

        if ( confd_register_data_cb( dctx, &dcb) != CONFD_OK) {
                ERROR_LOG( "Failed to register callpoint '%s'", dcb.callpoint);
                return CONFD_ERR;
        }

        //initialize CLI
        if ( connectCli() == CONFD_ERR) {
                ERROR_LOG( "cannot connect ripd %s:%d", getCliHost( ripd), getCliPort( ripd));
                return CONFD_ERR;
        }

    return CONFD_OK;
}

void dump_data(cb_data_t *cb_data)
{
        confd_hkeypath_t *kp = cb_data->kp;
        confd_value_t *oldv = cb_data->oldv;
        confd_value_t *newv = cb_data->newv;
#ifdef DO_DEBUG_LOG
        enum cdb_iter_op op = cb_data->op;
#endif
        char path[BUFSIZ], nvalue[BUFSIZ], ovalue[BUFSIZ];
        path[0] = nvalue[0] = ovalue[0] = 0;
        confd_pp_kpath(path, BUFSIZ, kp);
        if (newv != NULL)
                confd_pp_value(nvalue, BUFSIZ, newv);
        if (oldv != NULL)
                confd_pp_value(ovalue, BUFSIZ, oldv);
        DEBUG_LOG("startup %s, path %s - %i, val. %s, old val. %s", (op == MOP_MODIFIED ? "modified" :
                                                                                                                                  (op == MOP_DELETED ? "deleted" :
                                                                                                                                   (op == MOP_CREATED ? "created" :
                                                                                                                                        (op == MOP_VALUE_SET ? "set" : "unknown")))),
                          path, kp->len, nvalue, ovalue);
}

int rip_setup( struct confd_daemon_ctx *dctx) {
        extern int quagga_not_running;
        if(quagga_not_running) return CONFD_OK;

        zconfd_subscribe_cdb_pair(dctx, PROTO_PRIO, update_conf_router_inst, update_conf_router, "router/rip", NULL);
        zconfd_subscribe_cdb_tree(dctx, PROTO_PRIO, update_conf_if_ip, "interface/ip/rip", NULL);
        zconfd_subscribe_cdb_root(dctx, PROTO_PRIO, update_conf_if_ip_inst, "interface", NULL);
        zconfd_subscribe_cdb_root(dctx, PROTO_PRIO, update_conf_router_zebra_redistribute_rip_inst, "router/zebra/redistribute-rip", NULL);

        cb_data_t data;
        zconfd_init_cb_data_ctx(&data, dctx, NULL, MOP_CREATED, NULL, NULL, NULL);
        CHECK_OK(get_conf_router(&data, 1));
        CHECK_OK(get_conf_interfaces(&data, 1));
        CHECK_OK( router_zebra_redistribute_rip( &data));

        DEBUG_LOG("***** setup finished");

        return CONFD_OK;
}

static int get_conf_interfaces(cb_data_t *data,  int set){
        int i, n;
        char ifname[STR_MAX];
        int ret = CONFD_OK;
        char* name;

        DEBUG_LOG("get_conf_interfaces");

        name = "interface";
        n = cdb_num_instances(data->datasock, name);
        CHECK_CONFD(n, "Failed to get number of instances '%s'", name );

        for (i = 0; i < n && ret == CONFD_OK; i++) {
                //<elem name="name" type="xs:string" key="true"/>
                CHECK_CONFD( cdb_get_str(data->datasock, ifname, sizeof( ifname), "interface[%d]/name", i), "router/rip: Failed to get interface[%d]/%s", i, "name");

                CHECK_CONFD(cdb_pushd(data->datasock, "interface[%d]", i), "router/rip: Failed to pushd to 'interface[%d]/ip/rip'", i);
                ret = get_conf_interfaces_instance(data, ifname,  set);
                CHECK_CONFD( cdb_popd(data->datasock), "router/rip: Failed to popd %d", i);
        }

        return ret;
}

static int get_conf_router(cb_data_t *data,  int set)
{
        char *path;
        int ret;

        if (!set){
                return get_conf_router_inst(data, 0);
        }

        /* set */
        path = "router/rip";
        if (cdb_exists(data->datasock, path) != 1)
                return CONFD_OK;
        CHECK_CONFD(cdb_pushd(data->datasock, path), "ripd: Failed to pushd to %s", path);
        ret = get_conf_router_inst(data, 1);
        CHECK_CONFD(cdb_popd(data->datasock),"ripd: Failed to popd %s", path);

        return ret;
}

static int get_conf_router_inst(cb_data_t *data, int set)
{
        DEBUG_LOG( "get_conf_router_inst  set=%d", set);
        if (!set) {
                ERROR_LOG("Can't unset!");
                return CONFD_ERR;
        }

        cli_configure_command(1, ripd);
        cli_command_raw( ripd, "router rip", ".*\\(config-router\\)# ", 1, CLI_ERR_STR, NULL, NULL);

        CHECK_OK( test_single(data, "version", get_version));
        CHECK_OK( get_dynamic(data, "network-ip", get_network_ip));
        CHECK_OK( get_dynamic(data, "network-ifname", get_network_ifname));
        CHECK_OK( get_dynamic(data, "neighbor", get_neighbor));

        /* passive-interfaces need to be handled specially.. */
        cdb_pushd(data->datasock, "passive-interfaces");
        CHECK_OK(get_passive_interface_default(data, 1));
        cdb_popd(data->datasock);

        CHECK_OK( get_dynamic(data, "redistribute", get_redistribute));
        CHECK_OK( get_default_information_originate(data, 1));
        CHECK_OK( get_dynamic(data, "route", get_route));
        CHECK_OK( get_dynamic(data, "distribute-list", get_distribute_list));
        CHECK_OK( get_dynamic(data, "distribute-list-prefix", get_distribute_list_prefix));
        CHECK_OK( test_single(data, "timers-basic", get_timers_basic));
        CHECK_OK( get_default_metric(data, 1));
        CHECK_OK( get_dynamic(data, "offset-list", get_offset_list));
        CHECK_OK( test_single(data, "distance-default", get_distance_default));
        CHECK_OK( get_dynamic(data, "distance", get_distance));
        CHECK_OK( get_dynamic(data, "route-map", get_route_map));

        cli_command_raw( ripd, "exit", ".*\\(config\\)# ", 1, CLI_ERR_STR, NULL, NULL);
        cli_configure_command(0, ripd);

        return CONFD_OK;
}

static int get_conf_interfaces_instance(cb_data_t *data, char* ifname, int set)
{
        char cmd[BUFSIZ];
        DEBUG_LOG( "get_conf_interfaces_instance ifname=%s", ifname);

        if (! set) {
                ERROR_LOG("delete interface ... unhandled!");
                return CONFD_OK;
        }
        sprintf(cmd, "interface %s", ifname);
        cli_configure_command(1, ripd);
        cli_command_raw( ripd, cmd, ".*\\(config-if\\)# ", 1, CLI_ERR_STR, NULL, NULL);

        CHECK_CONFD(cdb_pushd(data->datasock, "ip/rip"), "router/rip: Failed to pushd to '%s'", "ip/rip");


        CHECK_OK( test_single(data, "authentication-mode", get_authentication_mode));
        CHECK_OK( test_single(data, "authentication-key-chain", get_authentication_key_chain));
        CHECK_OK( test_single(data, "authentication-string", get_authentication_string));
        CHECK_OK( test_single(data, "receive-version", get_receive_version));
        CHECK_OK( test_single(data, "send-version", get_send_version));
        CHECK_OK( get_split_horizon(data, set));

        CHECK_CONFD( cdb_popd(data->datasock), "router/rip: Failed to popd %s", "ip/rip");

        cli_command_raw( ripd, "exit", ".*\\(config\\)# ", 1, CLI_ERR_STR, NULL, NULL);
        cli_configure_command(0, ripd);

        DEBUG_LOG( "get_conf_interfaces_instance ifname=%s", ifname);
        return CONFD_OK;
}

static enum cdb_iter_ret update_conf_router(cb_data_t *data)
{
        int kp_ix = data->kp_start;
        enum cdb_iter_ret ret = ITER_CONTINUE;

        DUMP_UPDATE( __FUNCTION__, data);
        kp_ix -= 2;

        DEBUG_LOG("update_conf_router before switch kp_ix=%d", kp_ix );
        cli_command_raw( ripd, "router rip", ".*\\(config-router\\)# ", 1, CLI_ERR_STR, NULL, NULL);
        switch (CONFD_GET_XMLTAG(&data->kp->v[kp_ix][0])) {
                case NSPREF(version):
                        ret = update_single_router(data, get_version);
                        break;
                case NSPREF(network_ip):
                        if (kp_ix == 1) /* => elem 0 is key(s) */
                        ret = update_dynamic(data, get_network_ip);
                        break;
                case NSPREF(network_ifname):
                        if (kp_ix == 1) /* => elem 0 is key(s) */
                                ret = update_dynamic(data, get_network_ifname);
                        break;
                case NSPREF(neighbor):
                        if (kp_ix == 1) /* => elem 0 is key(s) */
                                ret = update_dynamic(data, get_neighbor);
                        break;
                case NSPREF(passive_interfaces):
                        if (kp_ix == 1 && data->kp->v[0][0].type == C_XMLTAG && CONFD_GET_XMLTAG(&data->kp->v[0][0]) == NSPREF(passive_by_default))
                                ret = update_dynamic_flatten(data, get_passive_interface_default, kp_ix + 1); /* !! this is a hack - not a dynamic node, in fact.. */
                        else {
                                assert(kp_ix == 2 && CONFD_GET_XMLTAG(&data->kp->v[1][0]) == NSPREF(except_interface));
                                ret = update_dynamic(data, get_passive_interface);
                        }
                        break;
                case NSPREF(redistribute):
                        ret = update_dynamic_flatten(data, get_redistribute, kp_ix);
                        break;
                case NSPREF(default_information_originate):
                        ret = update_single_router(data, get_default_information_originate);
                        break;
                case NSPREF(route):
                        if (kp_ix == 1) /* => elem 0 is key(s) */
                                ret = update_dynamic(data, get_route);
                        break;
                case NSPREF(distribute_list):
                        ret = update_dynamic_flatten(data, get_distribute_list, kp_ix);
                        break;
                case NSPREF(distribute_list_prefix):
                        ret = update_dynamic_flatten(data, get_distribute_list_prefix, kp_ix);
                        break;
                case NSPREF(timers_basic):
                        ret = update_single_router(data, get_timers_basic);
                        break;
                case NSPREF(default_metric):
                        ret = update_single_router(data, get_default_metric);
                        break;
                case NSPREF(offset_list):
                        ret = update_dynamic_flatten(data, get_offset_list, kp_ix);
                        break;
                case NSPREF(distance_default):
                        ret = update_single_router(data, get_distance_default);
                        break;
                case NSPREF(distance):
                        ret = update_dynamic_flatten(data, get_distance, kp_ix);
                        break;
                case NSPREF(route_map):
                        ret = update_dynamic(data, get_route_map);
                        break;
                default:
                    ERROR_LOG("Unknown element: %s", confd_hash2str(CONFD_GET_XMLTAG(&data->kp->v[kp_ix][0])));
        }

        cli_command_raw( ripd, "exit", ".*\\(config\\)# ", 1, CLI_ERR_STR, NULL, NULL);

        if (ret != CONFD_OK) {
                ERROR_LOG( "ripd: Failed to update config");
                return ret;
        }

        DEBUG_LOG( "update_conf_router end op=%d", data->op);
        return ITER_CONTINUE;
}

static enum cdb_iter_ret update_conf_if_ip(cb_data_t *data)
{
        int kp_ix = data->kp_start;
        int ret = CONFD_OK;
        char ifname[STR_MAX];

#ifdef DEBUG
        DUMP_UPDATE( __FUNCTION__, data);
#endif

        confd_pp_value(ifname, STR_MAX, &data->kp->v[kp_ix - 1][0]);
        IF_COMMAND(data, "interface %s", ifname);

        kp_ix -= 4;

        switch (CONFD_GET_XMLTAG(&data->kp->v[kp_ix][0])) {
                case NSPREF(authentication_mode):
                        if (kp_ix >= 1)
                                data->op = MOP_VALUE_SET;
                        ret = update_single_if_ip(data,
                                                                          get_authentication_mode, ifname);
                        break;
                case NSPREF(authentication_key_chain):
                        ret = update_single_if_ip(data,
                                                                          get_authentication_key_chain, ifname);
                        break;
                case NSPREF(authentication_string):
                        ret = update_single_if_ip(data,
                                                                          get_authentication_string, ifname);
                        break;
                case NSPREF(receive_version):
                        ret = update_single_if_ip(data,
                                                                          get_receive_version, ifname);
                        break;
                case NSPREF(send_version):
                        ret = update_single_if_ip(data,
                                                                          get_send_version, ifname);
                        break;
                case NSPREF(split_horizon):
                        ret = update_single_if_ip(data,
                                                                          get_split_horizon, ifname);
                        break;
        }
        cli_command_raw( ripd, "exit", ".*\\(config\\)# ", 1, CLI_ERR_STR, NULL, NULL);

        if (ret != CONFD_OK) {
                ERROR_LOG( "ripd: Failed to update config");
                return ret;
        }

        DEBUG_LOG( "update_conf_if_ip end op=%d", data->op);
        return ITER_CONTINUE;
}



static enum confd_iter_ret update_conf_router_inst(cb_data_t *data)
{
#ifdef DEBUG
        DEBUG_LOG("%s", __FUNCTION__);
        dump_data(data);
#endif
        switch (data->op) {
                case MOP_DELETED:
                        cli_command( ripd, data, ".*\\(config\\)# ", 1, CLI_ERR_STR, "no router rip");
                        break;
                case MOP_CREATED:
                        return ITER_RECURSE;
                default:
                        return ITER_RECURSE;
        }
        /* other cases handled by specific commands */
        return ITER_CONTINUE;
}

static enum confd_iter_ret update_conf_if_ip_inst(cb_data_t *data)
{
#ifdef DEBUG
        DEBUG_LOG("%s", __FUNCTION__);
        dump_data(data);
#endif
        if (data->op == MOP_DELETED) {
                char ifname[STR_MAX];
                assert(CONFD_GET_XMLTAG(&data->kp->v[1][0]) == NSPREF(interface));
                confd_pp_value(ifname, STR_MAX, &data->kp->v[0][0]);
                cli_command( ripd, data, ".*\\(config\\)# ", 1, CLI_ERR_STR, "no interface %s", ifname);
        }
        /* more specific callbacks will handle the other cases */
        return ITER_CONTINUE;
}

static int update_single(cb_data_t *data, char *path, get_func_t *get_func)
{
    int ret = CONFD_OK;

    if ((data->op == MOP_DELETED || data->op == MOP_VALUE_SET) && cdb_exists(data->datasock, path) == 1) {
                CHECK_CONFD(cdb_pushd(data->datasock, path),
                                        "router/rip: failed to pushd to %s", path);
                ret = (*get_func)(data, 0);
                cdb_popd(data->datasock);
    }
    CHECK_OK(ret);
    if (data->op == MOP_VALUE_SET || data->op == MOP_CREATED) {
                CHECK_CONFD(cdb_pushd(data->datasock, path),
                                        "router/rip: failed to pushd to %s", path);
                ret = (*get_func)(data, 1);
                cdb_popd(data->datasock);
    }
    return ret;
}

static int update_single_if_ip(cb_data_t *data, get_func_t *get_func, char *ifname)
{
        char path[BUFSIZ];
        sprintf(path, "interface{%s}/ip/rip", ifname);
        return update_single(data, path,  get_func);
}

static int update_single_router(cb_data_t *data, get_func_t *get_func)
{
        return update_single(data, "router/rip", get_func);
}

static int update_dynamic_flatten(cb_data_t *data, get_func_t *get_func, int kp_ix)
{
        confd_hkeypath_t flat_kp;
        int ret;
        if (kp_ix > 1) {
                confd_hkeypath_t *kp_b = zconfd_flatten_data_path(&flat_kp, data, kp_ix - 1);
                data->op = MOP_MODIFIED;
                ret = update_dynamic(data, get_func);
                zconfd_recover_data_path(data, kp_b);
        } else
                ret = update_dynamic(data, get_func);
        return ret;
}

static int get_version(cb_data_t *data, int set)
{
    char *name = "version";

    if (cdb_exists(data->datasock, name) != 1)
      return CONFD_OK;

    if (set) {
      ROUTER_COMMAND(data, "version ${ex: version}");
    } else {
      ROUTER_COMMAND(data, "no version");
    }
    return CONFD_OK;
}

//network-ip (dynamic)
static int get_network_ip(cb_data_t *data, int set)
{
        if (set) {
          ROUTER_COMMAND(data, "network ${key: address}/${key: prefix-length}");
        } else {
          ROUTER_COMMAND(data, "no network ${key: address}/${key: prefix-length}");
        }
        return CONFD_OK;
}

//network-ifname (dynamic)
static int get_network_ifname(cb_data_t *data, int set)
{
    if (set) {
          ROUTER_COMMAND(data, "network ${key: ifname}");
    } else {
          ROUTER_COMMAND(data, "no network ${key: ifname}");
    }
    return CONFD_OK;
}

//redistribute (dynamic)
static int get_redistribute(cb_data_t *data, int set)
{
    if (set) {
      ROUTER_COMMAND(data, "redistribute ${key: type}${ex: metric, ' metric %%s'}${ex: route-map, ' route-map %%s'}");
    } else { /* unset */
      ROUTER_COMMAND(data, "no redistribute ${key: type}");
    }
    return CONFD_OK;
}

//default-information-originate (default false)
static int get_default_information_originate(cb_data_t *data, int set)
{
    if (set)
      ROUTER_COMMAND(data, "${bool: default-information-originate, '', 'no '}default-information originate");
    else
      ROUTER_COMMAND(data, "no default-information originate");
    return CONFD_OK;
}

//default-metric
static int get_default_metric(cb_data_t *data, int set)
{
    if (set) {
      ROUTER_COMMAND(data, "default-metric ${ex: default-metric}");
    }
    return CONFD_OK;
}

/*      <elem name="neighbor" minOccurs="0" maxOccurs="unbounded">
      <elem name="ip" type="confd:inetAddressIPv4" key="true"/>
    </elem>     */
static int get_neighbor(cb_data_t *data, int set)
{
        if (set)
                ROUTER_COMMAND(data, "neighbor ${key: ip}");
        else
                ROUTER_COMMAND(data, "no neighbor ${key: ip}");
        return CONFD_OK;
}

/*    <elem name="except-interface" minOccurs="0" maxOccurs="unbounded"> */
/*       <elem name="ifname" keyref="../../../../../interface/name" key="true"/> */
/*    </elem> */
static int get_passive_interface(cb_data_t *data, int set)
{
        int passive_default;
        char *no_pref;

        CHECK_OK(cdb_get_bool(data->datasock, &passive_default, set ? "../passive-by-default" : "router/rip/passive-interfaces/passive-by-default"));

        no_pref = passive_default ^ set ? "" : "no ";
        ROUTER_COMMAND(data, "%spassive-interface ${key: ifname}", no_pref);
        return CONFD_OK;
}

/*     <elem name="passive-interfaces"> */
/*       <elem name="passive-by-default" type="xs:boolean" default="false"/> */
/*       <elem name="except-interface" minOccurs="0" maxOccurs="unbounded"> */
/*         <elem name="ifname" keyref="../../../../../interface/name" key="true"/> */
/*       </elem> */
/*     </elem> */
static int get_passive_interface_default(cb_data_t *data, int set)
{
        if (set) {
                int passive_default, n, i;
                const char *name = "except-interface";
                char *no_pref;

                CHECK_OK(cdb_get_bool(data->datasock, &passive_default, "passive-by-default"));
                ROUTER_COMMAND(data, "%spassive-interface default", passive_default ? "" : "no ");

                /* need to reconfigure all exceptions (if any) */
                n = cdb_num_instances(data->datasock, name);
                CHECK_CONFD(n, "Failed to get number of interface instances");
                no_pref = passive_default ? "no " : "";
                for (i = 0; i < n; i++) {
                        ROUTER_COMMAND(data, "%spassive-interface ${ex: %s[%d]/ifname}", no_pref, name, i);
                }
        }
        return CONFD_OK;
}

/*      <elem name="route" minOccurs="0" maxOccurs="unbounded">
      <elem name="address" type="confd:inetAddressIPv4" key="true"/>
      <elem name="prefix-length" type="PrefixLengthIPv4" key="true"/>
    </elem> */
static int get_route(cb_data_t *data, int set)
{
        if (set)
                ROUTER_COMMAND(data, "route ${key: address}/${key: prefix-length}");
        else
                ROUTER_COMMAND(data, "no route ${key: address}/${key: prefix-length}");
        return CONFD_OK;
}

/*      <elem name="distribute-list" minOccurs="0" maxOccurs="unbounded">
      <elem name="alist" keyref="../../../../ip/access-list/id" key="true"/>
      <elem name="direction" type="DirectionType" key="true"/>
      <elem name="ifname" keyref="../../../../interface/name" minOccurs="0"/>
    </elem> */
static int get_distribute_list(cb_data_t *data, int set)
{
        if (set)
                ROUTER_COMMAND(data, "distribute-list ${key: alist} ${key: direction} ${key: ifname}");
        else
                ROUTER_COMMAND(data, "no distribute-list ${key: alist} ${key: direction} ${key: ifname}");
        return CONFD_OK;
}

/*      <elem name="distribute-list-prefix" minOccurs="0" maxOccurs="unbounded">
      <elem name="prefix" keyref="../../../../ip/prefix-list/name" key="true"/>
      <elem name="direction" type="DirectionType" key="true"/>
      <elem name="ifname" keyref="../../../../interface/name" minOccurs="0"/>
    </elem>     */
static int get_distribute_list_prefix(cb_data_t *data, int set)
{
        if (set)
                ROUTER_COMMAND(data, "distribute-list prefix ${key: prefix} ${key: direction} ${key: ifname}");
        else
                ROUTER_COMMAND(data, "no distribute-list prefix ${key: prefix} ${key: direction} ${key: ifname}");
        return CONFD_OK;
}

/*      <elem name="timers-basic" minOccurs="0">
      <elem name="update" type="TimerRangeType" default="30"/>
      <elem name="timeout" type="TimerRangeType" default="180"/>
      <elem name="garbage" type="TimerRangeType" default="120"/>
    </elem>     */
static int get_timers_basic(cb_data_t *data, int set)
{
        if (set && ( cdb_exists(data->datasock, "timers-basic") == 1)) {
                ROUTER_COMMAND(data, "timers basic ${ex: timers-basic/update} ${ex: timers-basic/timeout} ${ex: timers-basic/garbage}");
        } else
                ROUTER_COMMAND(data, "no timers basic");
        return CONFD_OK;
}


/*      <elem name="offset-list" minOccurs="0" maxOccurs="unbounded">
      <elem name="alist" keyref="../../../../ip/access-list/id" key="true"/>
      <elem name="direction" type="DirectionType" key="true"/>
      <elem name="metric" type="MetricRangeType"/>
      <elem name="ifname" keyref="../../../../interface/name" minOccurs="0"/>
    </elem> */
static int get_offset_list(cb_data_t *data, int set)
{
        if (set)
                ROUTER_COMMAND(data, "offset-list ${key: alist} ${key: direction} ${ex: metric} ${key: ifname}");
        else
                // metric required (but any meaningful value will do)
                ROUTER_COMMAND(data, "no offset-list ${key: alist} ${key: direction} 1 ${key: ifname}");
        return CONFD_OK;
}

/*      <elem name="distance-default" type="DistanceRangeType" minOccurs="0"/> */
static int get_distance_default(cb_data_t *data, int set)
{
        if ( cdb_exists(data->datasock, "distance-default") != 1)
                return CONFD_OK;

        if ( set)
                ROUTER_COMMAND(data, "distance ${ex: distance-default}");
        else
                ROUTER_COMMAND(data, "no distance 1");
        return CONFD_OK;
}

 /*     <elem name="distance" minOccurs="0" maxOccurs="unbounded">
      <elem name="source" type="confd:inetAddressIPv4" key="true"/>
      <elem name="prefix-length" type="PrefixLengthIPv4" key="true"/>
      <elem name="value" type="DistanceRangeType"/>
      <elem name="alist" type="AccessListRefType" minOccurs="0"/>
    </elem>     */
static int get_distance(cb_data_t *data, int set)
{
        if (set) {
                char alist[BUFSIZ];
                alist[0] = 0;
                if (cdb_exists(data->datasock, "alist") == 1)
                        CHECK_OK(get_alist(data->datasock, alist, BUFSIZ, "alist"));
                ROUTER_COMMAND(data, "distance ${ex: value} ${key: source}/${key: prefix-length} %s", alist);
        } else
                // metric required (but any meaningful value will do)
                ROUTER_COMMAND(data, "no distance 1 ${key: source}/${key: prefix-length}");
        return CONFD_OK;
}

static int get_route_map(cb_data_t *data, int set)
{
        ROUTER_COMMAND(data, "%sroute-map ${key: route-map} ${key: direction} ${key: ifname}", set ? "" : "no ");
        return CONFD_OK;
}

/**************** interface{*}/ip/rip functions ****************/

//<elem name="authentication-mode" type="AuthenticationModeType" minOccurs="0"/>
static int get_authentication_mode(cb_data_t *data, int set)
{
        if (set)
                IF_COMMAND(data, "ip rip authentication mode ${ex: authentication-mode/mode}${ex: authentication-mode/auth-length, ' auth-length %%s'}");
        else
                IF_COMMAND(data, "no ip rip authentication mode");
        return CONFD_OK;
}

//<elem name="authentication-key-chain" keyref="../../../../key-chain/name" minOccurs="0"/>
static int get_authentication_key_chain(cb_data_t *data,  int set)
{
        if (set)
                IF_COMMAND(data, "ip rip authentication key-chain ${ex: authentication-key-chain}");
        else
                IF_COMMAND(data, "no ip rip authentication key-chain");
        return CONFD_OK;
}

//<elem name="authentication-string" type="xs:string" minOccurs="0"/>
static int get_authentication_string(cb_data_t *data, int set)
{
        if (set)
                IF_COMMAND(data, "ip rip authentication string ${ex: authentication-string}");
        else
                IF_COMMAND(data, "no ip rip authentication string");
        return CONFD_OK;
}

static int fetch_version(int rsock, const char *name, const char **version)
{
        int h;
        if ( cdb_exists(rsock, name) != 1)
                return 0;
        CHECK_CONFD(cdb_get_enum_value(rsock, &h, name),        "Failed to read '%s'", name);
        switch (h) {
        case NSPREF(_RipVersionSetType_1): *version = "1"; break;
        case NSPREF(_RipVersionSetType_2): *version = "2"; break;
        case NSPREF(_RipVersionSetType_10x2c2): *version = "1 2"; break;
        case NSPREF(_RipVersionSetType_20x2c1): *version = "2 1"; break;
        }
        return 1;
}

//<elem name="receive-version" type="RipVersionSetType" minOccurs="0"/>
static int get_receive_version(cb_data_t *data, int set)
{
        if (set) {
                const char *version = "";

                DEBUG_LOG("get_receive_version");
                if (fetch_version(data->datasock, "receive-version", &version))
                        IF_COMMAND(data, "ip rip receive version %s", version);
        } else
                IF_COMMAND(data, "no ip rip receive version");
        return CONFD_OK;
}

//<elem name="send-version" type="RipVersionCompSetType" minOccurs="0"/>
static int get_send_version(cb_data_t *data, int set)
{
        if (set) {
                const char *version = "";

                DEBUG_LOG("get_send_version");
                if (fetch_version(data->datasock, "send-version", &version))
                        IF_COMMAND(data, "ip rip send version %s", version);
        } else
                IF_COMMAND(data, "no ip rip send version");
        return CONFD_OK;
}

//<elem name="split-horizon" type="SplitHorizonType" minOccurs="0"/>
static int get_split_horizon(cb_data_t *data,  int set)
{
        if (set) {
                int h;
                if (cdb_exists(data->datasock, "split-horizon") != 1)
                        return 0;
                CHECK_CONFD(cdb_get_enum_value(data->datasock, &h, "split-horizon"), "Failed to read 'split-horizon'");
                switch (h) {
                        case NSPREF(_SplitHorizonType_split):
                                IF_COMMAND(data, "ip rip split-horizon");
                                break;
                        case NSPREF(_SplitHorizonType_poisoned_reverse):
                                IF_COMMAND(data, "ip rip split-horizon poisoned-reverse");
                                break;
                        case NSPREF(_SplitHorizonType_none):
                                IF_COMMAND(data, "no ip rip split-horizon");
                                break;
                }
        }
        return CONFD_OK;
}

static int validateKeyAddrPrefix(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval)
{
        CHECK_OK(validateAddrPrefixPair(tctx, keypath, newval));
        return ripOnlineTest( tctx, keypath, newval);
}

static int ripOnlineTest(struct confd_trans_ctx *tctx,
                                                                                                                                 confd_hkeypath_t *keypath,
                                                                                                                                 confd_value_t *newval) {
        extern int quagga_not_running;
        if(quagga_not_running) return CONFD_OK;

        if ( !cliOpened( &g_clients[ ripd])) {  //check the connection to quagga
                VALIDATE_FAIL( "Cannot connect to quagga rid daemon cli");
        }
        return CONFD_OK;
}

static int validateAlistIfname(struct confd_trans_ctx *tctx,
                                                                                                                                 confd_hkeypath_t *keypath,
                                                                                                                                 confd_value_t *newval) {
        int th = tctx->thandle;
        int maapisock, kp_start;
        char aname[ BUFSIZ];
        char ifname[ BUFSIZ];
        char* alists[] = { "access-list", "access-list-extended", "access-list-word"};
        char* alists6[] = { "access-list"};
        int anameNum = -1;
        int i, exists;

        confd_pp_value( aname, sizeof( aname), &keypath->v[0][0]);

        if ( aname[ 0] == '\0')         //empty aname is not enabled
                VALIDATE_FAIL("access list name must be defined");

        if ( isNumeric( aname))
                anameNum = atoi( aname);

        CHECK_OK( zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));

        exists = 0;
        for ( i = 0; i < sizeof( alists) / sizeof( alists[ 0]); i++) {
                if (( i == 0) && (( anameNum < 1) || ( anameNum > 99)) && (( anameNum < 1300) || ( anameNum > 1999)))
                        continue;
                if (( i == 1) && (( anameNum < 100) || ( anameNum > 199)) && (( anameNum < 2000) && ( anameNum > 2699)))
                        continue;
                if ( maapi_exists( maapisock, th, "ip/%s{%s}", alists[ i], aname) == 1) {
                        exists = 1;
                        break;
                }
        }

        if ( exists == 0) {     //check ipv6
                for ( i = 0; i < sizeof( alists6) / sizeof( alists6[ 0]); i++)
                        if ( maapi_exists( maapisock, th, "ipv6/%s{%s}", alists6[ i], aname) == 1) {
                                exists = 1;
                                break;
                        }
        }

        if ( exists == 0)
                VALIDATE_FAIL("access list name %s does not exist", aname);

        confd_pp_value( ifname, sizeof( ifname), &keypath->v[0][2]);

        if ( ifname[ 0] == '\0')                //empty ifname is enabled
                return CONFD_OK;

        if ( maapi_exists( maapisock, th, "interface{%s}", ifname) != 1)                //check interface
                VALIDATE_FAIL("interface name %s does not exist", ifname);

        return CONFD_OK;
}

static int validateIfname(struct confd_trans_ctx *tctx, confd_hkeypath_t *kp, confd_value_t *val)
{
        int kp_start, maapisock;
        char ifname[KPATH_MAX];

        CHECK_OK(zconfd_trans_get(tctx, kp, &maapisock, &kp_start, NULL));
        confd_pp_value(ifname, KPATH_MAX, &kp->v[0][2]);
        if (ifname[0] != 0 && maapi_exists(maapisock, tctx->thandle, "interface{%s}", ifname) != 1)
                VALIDATE_FAIL("interface name %s does not exist", ifname);
        return CONFD_OK;
}

static int bufeq(confd_value_t *v1, confd_value_t *v2)
{
        assert(v1->type == C_BUF && v2->type == C_BUF);
        return v1->val.buf.size == v2->val.buf.size && strncmp((char*)v1->val.buf.ptr, (char*) v2->val.buf.ptr, v1->val.buf.size) == 0;
}

static int validateRipRouteMap(struct confd_trans_ctx *tctx, confd_hkeypath_t *kp, confd_value_t *val)
{
        int maapisock, kp_start;
        struct maapi_cursor mc;
        int ret = CONFD_OK;

        CHECK_OK(zconfd_trans_get(tctx, kp, &maapisock, &kp_start, NULL));
        CHECK_OK1(maapi_init_cursor(maapisock, tctx->thandle, &mc, "router/rip/route-map"));
        CHECK_OK1(maapi_get_next(&mc));
        while (mc.n != 0 && ret == CONFD_OK) {
                if (! bufeq(&kp->v[0][0], &mc.keys[0]) &&
                    CONFD_GET_ENUM_VALUE(&kp->v[0][1]) ==
                      CONFD_GET_ENUM_VALUE(&mc.keys[1]) &&
                    bufeq(&kp->v[0][2], &mc.keys[2]))
                        VALIDATE_FAIL("route-map conflict with route-map %.*s", mc.keys[0].val.buf.size, mc.keys[0].val.buf.ptr);
                ret = maapi_get_next(&mc);
        }
        maapi_destroy_cursor(&mc);
        return CONFD_OK;
}

static int validateAuthKeyString(struct confd_trans_ctx *tctx,
                                 confd_hkeypath_t *keypath,
                                 confd_value_t *newval)
{
        int th = tctx->thandle;
        int maapisock, kp_start;

        CHECK_OK(zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));
        if (maapi_exists(maapisock, th, "%h/authentication-key-chain", keypath) == 1 &&
            maapi_exists(maapisock, th, "%h/authentication-string", keypath) == 1)
                VALIDATE_FAIL("It is not possible to set both authentication-key-chain and authentication-string parameters");
        return CONFD_OK;
}

static int validateAuthString(struct confd_trans_ctx *tctx,
                              confd_hkeypath_t *keypath,
                              confd_value_t *newval)
{
        if (CONFD_GET_BUFSIZE(newval) < 6)
                return VALIDATE_WARN("The authentication-string is very short, this is a potential security hazard!");
        return CONFD_OK;
}

int get_rip_memory_elem(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath) {
        return get_memory_elem( tctx, keypath, ripd);
}

int router_zebra_redistribute_rip( cb_data_t *data) {
        cli_configure_command( 1, ripd);
        cli_command_raw( ripd, "router zebra", ".*\\(config-router\\)# ", 1, CLI_ERR_STR, NULL, NULL);

        CHECK_OK( ROUTER_COMMAND( data, "${bool: router/zebra/redistribute-rip, '', 'no '}redistribute rip"));

        cli_command_raw( ripd, "exit", ".*\\(config\\)# ", 1, CLI_ERR_STR, NULL, NULL); //exit from "router zebra" mode
        cli_configure_command( 0, ripd);
        return CONFD_OK;
}

static enum confd_iter_ret update_conf_router_zebra_redistribute_rip_inst( cb_data_t *data)
{
#ifdef DEBUG
        DEBUG_LOG("%s", __FUNCTION__);
        dump_data(data);
#endif
        assert(CONFD_GET_XMLTAG(&data->kp->v[0][0]) == NSPREF(redistribute_rip));
        router_zebra_redistribute_rip( data);
        return ITER_CONTINUE;
}

