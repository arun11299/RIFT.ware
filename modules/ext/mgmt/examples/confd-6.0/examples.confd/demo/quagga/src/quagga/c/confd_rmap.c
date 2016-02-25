#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <confd.h>
#include <confd_cdb.h>
#include <confd_maapi.h>

#undef _POSIX_C_SOURCE

#include "zconfd_api.h"
#include "quagga.h"     /* generated from yang */
#include "confd_global.h"

#define ALIST_ID_LEN 10

static get_func_t get_route_map;
static subscr_cb_t update_route_map_line;
static subscr_cb_t update_route_map_line_inst;
static subscr_cb_t update_route_map_inst;

#define RM_COMMAND(data, command, prompt, args...) cli_command( all_routers, data, prompt, 1, CLI_ERR_STR, command , ##args)
#define ROUTEMAP_COMMAND(data, command, args...) RM_COMMAND(data, command, ".*\\(config-route-map\\)# " , ##args)
#define BASE_COMMAND(data, command, args...) RM_COMMAND(data, command, ".*\\(config\\)# " , ##args)

/* some of the commands are specific to rip */
#define ROUTEMAP_RIPD_COMMAND(data, command, args...) cli_command( ripd, data, ".*\\(config-route-map\\)# ", 1, CLI_ERR_STR, command , ##args)

static int validateRmapOnMatchGotoRef(struct confd_trans_ctx *tctx,
                           confd_hkeypath_t *keypath,
                           confd_value_t *newval);

static struct confd_valpoint_cb valpoints[] = {
        { .valpoint = "valRmapOnMatchGotoRef", .validate = validateRmapOnMatchGotoRef},
        { .valpoint = "" }
};


int confd_rmap_setup(struct confd_daemon_ctx *dctx)
{
        DEBUG_LOG( "confd_rmap_setup ");
        /* Register validation points */
        register_valpoint_cb_arr( dctx, valpoints);

        extern int quagga_not_running;
        if(quagga_not_running) return CONFD_OK;

        zconfd_subscribe_cdb_root(dctx, RMAP_PRIO, update_route_map_inst, "route-map", NULL);
        zconfd_subscribe_cdb_pair(dctx, RMAP_PRIO, update_route_map_line_inst, update_route_map_line, "route-map/line", NULL);

        cb_data_t data;
        zconfd_init_cb_data_ctx(&data, dctx, NULL, MOP_CREATED, NULL, NULL, NULL);
        CHECK_OK( get_dynamic(&data, "route-map", get_route_map));
        return CONFD_OK;

}

#define TAG_LEN 255

struct route_map_pars {
  char* map;
  unsigned perm;
  unsigned pref;
};

static get_func_t get_route_map_call;
static get_func_t get_route_map_goto;
static get_func_t get_route_map_match_if;
static get_func_t get_route_map_match_addr;
static get_func_t get_route_map_match_addr_plist;
static get_func_t get_route_map_match_hop;
static get_func_t get_route_map_match_hop_plist;
static get_func_t get_route_map_match_metric;
static get_func_t get_route_map_match_tag;
static get_func_t get_route_map_set_hop;
static get_func_t get_route_map_set_metric;
static get_func_t get_route_map_set_tag;

static int get_route_map_line_inst(cb_data_t *data, int set, void *priv)
{
        char *tag = priv;
        if (set == 0) {
                ERROR_LOG("%s: set == 0 not supported!", __FUNCTION__);
                return CONFD_ERR;
        }
        ROUTEMAP_COMMAND(data, "route-map %s ${ex: action} ${ex: number}", tag);
        CHECK_OK(test_single(data, "call", get_route_map_call));
        CHECK_OK(test_single(data, "on-match-goto", get_route_map_goto));
        CHECK_OK(test_single(data, "match-interface", get_route_map_match_if));
        CHECK_OK(test_single(data, "match-ip-address", get_route_map_match_addr));
        CHECK_OK(test_single(data, "match-ip-address-prefix-list", get_route_map_match_addr_plist));
        CHECK_OK(test_single(data, "match-ip-next-hop", get_route_map_match_hop));
        CHECK_OK(test_single(data, "match-ip-next-hop-prefix-list", get_route_map_match_hop_plist));
        CHECK_OK(test_single(data, "match-metric", get_route_map_match_metric));
        CHECK_OK(test_single(data, "match-tag", get_route_map_match_tag));
        CHECK_OK(test_single(data, "set-ip-next-hop", get_route_map_set_hop));
        CHECK_OK(test_single(data, "set-metric", get_route_map_set_metric));
        CHECK_OK(test_single(data, "set-tag", get_route_map_set_tag));
        BASE_COMMAND(data, "exit");
        return CONFD_OK;
}

static int get_route_map(cb_data_t *data, int set)
{
        char tag[TAG_LEN];
        if (set == 0) {
                ERROR_LOG("%s: set == 0 not supported!", __FUNCTION__);
                return CONFD_ERR;
        }
        cli_configure_command(1, all_routers);
        CHECK_CONFD(cdb_get_str(data->datasock, tag, TAG_LEN, "tag"), "Failed to get tag");
        CHECK_OK(get_dynamic_priv(data, "line", get_route_map_line_inst, 1, tag));
        cli_configure_command(0, all_routers);
        return CONFD_OK;
}

static enum cdb_iter_ret update_route_map_inst(cb_data_t *data)
{
        if (data->op == MOP_DELETED) {
                BASE_COMMAND(data, "no route-map ${key: tag}");
        }
        /* other cases are handled elsewhere */
        return ITER_CONTINUE;
}

static enum cdb_iter_ret update_route_map_line_inst(cb_data_t *data)
{
        char path[KPATH_MAX];
        switch (data->op) {
        case MOP_DELETED:
                /* We need to hack deny/permit somehow (as ripd needs action type for "no route-map" command) */
                ROUTEMAP_COMMAND(data, "route-map ${key: tag(2)} deny ${key: number}");
                BASE_COMMAND(data, "no route-map ${key: tag(2)} deny ${key: number}");
                break;
        case MOP_CREATED:
                CHECK_OK(confd_pp_kpath(path, KPATH_MAX, data->kp));
                CHECK_CONFD(cdb_pushd(data->datasock, path), "Failed to pushd to %s", path);
                ROUTEMAP_COMMAND(data, "route-map ${key: tag(2)} ${ex: action} ${key: number}");
                CHECK_OK(cdb_popd(data->datasock));
                BASE_COMMAND(data, "exit");
                return ITER_RECURSE;
        default:
                return ITER_RECURSE;
        }

        return ITER_CONTINUE;
}

static enum cdb_iter_ret update_route_map_line(cb_data_t *data)
{
        char path[KPATH_MAX];
        confd_hkeypath_t flat_kp, *t_kp;
        int tag = CONFD_GET_XMLTAG(&data->kp->v[data->kp_start - 4][0]);
#ifdef DEBUG
        confd_pp_kpath(path, KPATH_MAX, data->kp);
        DEBUG_LOG("%s %s - %s", __FUNCTION__, path, confd_hash2str(tag));
#endif
        t_kp = zconfd_flatten_data_path(&flat_kp, data, data->kp_start - 3);

        confd_pp_kpath(path, KPATH_MAX, &flat_kp);
        CHECK_OK(cdb_pushd(data->datasock, path));
        if (tag == NSPREF(action) && data->oldv != NULL) {
                /* we must reconfigure whole key */
                char tag[TAG_LEN];
                CHECK_CONFD(get_data_key(data, "tag", 2, tag, TAG_LEN), "Unable to find route-map tag");

                /* poison newv */
                data->newv = NULL;
                get_route_map_line_inst(data, 1, tag);
        } else {
                int set = data->op != MOP_DELETED;
                ROUTEMAP_COMMAND(data, "route-map ${key: tag(2)} ${ex: action} ${key: number}", data->kp_start - data->kp->len);

                switch (tag) {
                case NSPREF(call):
                        get_route_map_call(data, set);
                        break;
                case NSPREF(on_match_goto):
                        get_route_map_goto(data, set);
                        break;
                case NSPREF(match_interface):
                        get_route_map_match_if(data, set);
                        break;
                case NSPREF(match_ip_address):
                        get_route_map_match_addr(data, set);
                        break;
                case NSPREF(match_ip_address_prefix_list):
                        get_route_map_match_addr_plist(data, set);
                        break;
                case NSPREF(match_ip_next_hop):
                        get_route_map_match_hop(data, set);
                        break;
                case NSPREF(match_ip_next_hop_prefix_list):
                        get_route_map_match_hop_plist(data, set);
                        break;
                case NSPREF(match_metric):
                        get_route_map_match_metric(data, set);
                        break;
                case NSPREF(match_tag):
                        get_route_map_match_tag(data, set);
                        break;
                case NSPREF(set_ip_next_hop):
                        get_route_map_set_hop(data, set);
                        break;
                case NSPREF(set_metric):
                        get_route_map_set_metric(data, set);
                        break;
                case NSPREF(set_tag):
                        get_route_map_set_tag(data, set);
                        break;
                }
                BASE_COMMAND(data, "exit");
        }

        zconfd_recover_data_path(data, t_kp);
        cdb_popd(data->datasock);
        return ITER_CONTINUE;
}

static int get_route_map_call(cb_data_t *data, int set)
{
        if (set)
                ROUTEMAP_COMMAND(data, "call ${arg: call}");
        else
                ROUTEMAP_COMMAND(data, "no call");
        return CONFD_OK;
}

static int get_route_map_goto(cb_data_t *data, int set)
{
        if (set)
                ROUTEMAP_COMMAND(data, "on-match goto ${arg: on-match-goto}");
        else
                ROUTEMAP_COMMAND(data, "no on-match goto");
        return CONFD_OK;
}

static int get_route_map_match_if(cb_data_t *data, int set)
{
        if (set)
                ROUTEMAP_COMMAND(data, "match interface ${arg: match-interface}");
        else
                ROUTEMAP_COMMAND(data, "no match interface");
        return CONFD_OK;
}

static int get_route_map_match_addr(cb_data_t *data, int set)
{
        if (set) {
                char alist[KPATH_MAX];
                CHECK_OK(get_alist(data->datasock, alist, KPATH_MAX, "match-ip-address"));
                ROUTEMAP_COMMAND(data, "match ip address %s", alist);
        } else
                ROUTEMAP_COMMAND(data, "no match ip address");
        return CONFD_OK;
}

static int get_route_map_match_addr_plist(cb_data_t *data, int set)
{
        if (set)
                ROUTEMAP_COMMAND(data, "match ip address prefix-list ${arg: match-ip-address-prefix-list}");
        else
                ROUTEMAP_COMMAND(data, "no match ip address prefix-list");
        return CONFD_OK;
}

static int get_route_map_match_hop(cb_data_t *data, int set)
{
        if (set) {
                char alist[KPATH_MAX];
                CHECK_OK(get_alist(data->datasock, alist, KPATH_MAX, "match-ip-next-hop"));
                ROUTEMAP_COMMAND(data, "match ip next-hop %s", alist);
        } else
                ROUTEMAP_COMMAND(data, "no match ip next-hop");
        return CONFD_OK;
}

static int get_route_map_match_hop_plist(cb_data_t *data, int set)
{
        if (set)
                ROUTEMAP_COMMAND(data, "match ip next-hop prefix-list ${arg: match-ip-next-hop-prefix-list}");
        else
                ROUTEMAP_COMMAND(data, "no match ip next-hop prefix-list");
        return CONFD_OK;
}

static int get_route_map_match_metric(cb_data_t *data, int set)
{
        if (set)
                ROUTEMAP_RIPD_COMMAND(data, "match metric ${arg: match-metric}");
        else
                ROUTEMAP_RIPD_COMMAND(data, "no match metric");
        return CONFD_OK;
}

static int get_route_map_match_tag(cb_data_t *data, int set)
{
        if (set)
                ROUTEMAP_RIPD_COMMAND(data, "match tag ${arg: match-tag}");
        else
                ROUTEMAP_RIPD_COMMAND(data, "no match tag");
        return CONFD_OK;
}

static int get_route_map_set_hop(cb_data_t *data, int set)
{
        if (set)
                ROUTEMAP_RIPD_COMMAND(data, "set ip next-hop ${arg: set-ip-next-hop}");
        else
                ROUTEMAP_RIPD_COMMAND(data, "no set ip next-hop");
        return CONFD_OK;
}

static int get_route_map_set_metric(cb_data_t *data, int set)
{
        if (set) {
                int operation;
                const char *opname = "";
                CHECK_OK(cdb_get_enum_value(data->datasock, &operation, "set-metric/operation"));
                switch (operation) {
                case NSPREF(_MetricOperType_set): opname = ""; break;
                case NSPREF(_MetricOperType_sub): opname = "-"; break;
                case NSPREF(_MetricOperType_add): opname = "+"; break;
                }
                ROUTEMAP_COMMAND(data, "set metric %s${ex: set-metric/value}", opname);
        } else
                ROUTEMAP_COMMAND(data, "no set metric");
        return CONFD_OK;
}

static int get_route_map_set_tag(cb_data_t *data, int set) {
        if (set)
                ROUTEMAP_RIPD_COMMAND(data, "set tag ${arg: set-tag}");
        else
                ROUTEMAP_RIPD_COMMAND(data, "no set tag");
        return CONFD_OK;
}

static int validateRmapOnMatchGotoRef(struct confd_trans_ctx *tctx,
                                                                 confd_hkeypath_t *keypath,
                                                                 confd_value_t *newval) {
        int onMatchGoto = CONFD_GET_UINT32( newval);
        int line = CONFD_GET_UINT32( &keypath->v[1][0]);

        if ( onMatchGoto <= line)
                VALIDATE_FAIL("on-match-goto (%d) must be higher than line (%d)", onMatchGoto, line);

        return CONFD_OK;
}
