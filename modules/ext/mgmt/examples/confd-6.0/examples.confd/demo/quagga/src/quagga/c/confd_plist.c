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

#define NAME_LEN 100

typedef struct {
        const char *ip_version;
        char plist_name[NAME_LEN];
} prefix_list_pars_t;

#define PL_COMMAND(data, command, prompt, args...) cli_command( all_routers, data, prompt, 1, CLI_ERR_STR, command , ##args)
#define EXIT_COMMAND(data, command, args...) PL_COMMAND(data, command, ".*# " , ##args)
#define BASE_COMMAND(data, command, args...) PL_COMMAND(data, command, ".*\\(config\\)# " , ##args)

#define IP_VERSION(data) confd_hash2str(CONFD_GET_XMLTAG(&(data)->kp->v[(data)->kp_start][0]))

static subscr_cb_t update_prefix_list_inst;
static subscr_cb_t update_prefix_list_line;

static get_func_priv_cb_t get_prefix_list;
static get_func_priv_cb_t get_prefix_list_line;

static int validatePrefixListRef(struct confd_trans_ctx *tctx,
                           confd_hkeypath_t *keypath,
                           confd_value_t *newval);
static int validatePlistPrefix(struct confd_trans_ctx *tctx,
                                 confd_hkeypath_t *keypath,
                                 confd_value_t *newval);
static int validateUniqItems(struct confd_trans_ctx *tctx,
                                 confd_hkeypath_t *keypath,
                                 confd_value_t *newval);

static struct confd_valpoint_cb valpoints[] = {
        { .valpoint = "valPrefixListRef", .validate = validatePrefixListRef},
        { .valpoint = "valPlistPrefix", .validate = validatePlistPrefix},
        { .valpoint = "valUniqItems", .validate = validateUniqItems},
        { .valpoint = "" }
};

int confd_plist_setup(struct confd_daemon_ctx *dctx) {
        DEBUG_LOG( "confd_plist_setup");
        /* Register validation points */
        register_valpoint_cb_arr( dctx, valpoints);

        extern int quagga_not_running;
        if(quagga_not_running) return CONFD_OK;

        zconfd_subscribe_cdb_root(dctx, PLIST_PRIO, update_prefix_list_inst, "ip/prefix-list", NULL);
        zconfd_subscribe_cdb_root(dctx, PLIST_PRIO, update_prefix_list_inst, "ipv6/prefix-list", NULL);
        zconfd_subscribe_cdb(dctx, PLIST_PRIO, update_prefix_list_line, "ip/prefix-list/line", NULL);
        zconfd_subscribe_cdb(dctx, PLIST_PRIO, update_prefix_list_line, "ipv6/prefix-list/line", NULL);

        /* read configuration */
        cb_data_t data;
        zconfd_init_cb_data_ctx(&data, dctx, NULL, MOP_CREATED, NULL, NULL, NULL);
        cli_configure_command(1, all_routers);
        CHECK_OK( get_dynamic_priv(&data,  "ip/prefix-list", get_prefix_list, 1, "ip"));
        CHECK_OK( get_dynamic_priv(&data,  "ipv6/prefix-list", get_prefix_list, 1, "ipv6"));
        cli_configure_command(0, all_routers);

        return CONFD_OK;
}

static int get_prefix_list(cb_data_t *data, int set, void *priv)
{
        prefix_list_pars_t pars;
    if (set == 0) {
                ERROR_LOG("%s: can't unset!", __FUNCTION__);
                return CONFD_ERR;
        }

        CHECK_OK(cdb_get_str(data->datasock, pars.plist_name, NAME_LEN, "name"));
        pars.ip_version = priv;
        CHECK_OK(get_dynamic_priv(data, "line", get_prefix_list_line, 1, &pars));
        return CONFD_OK;
}

static enum cdb_iter_ret update_prefix_list_inst(cb_data_t *data)
{
        if (data->op == MOP_DELETED)
                BASE_COMMAND(data, "no %s prefix-list ${key: name}", IP_VERSION(data));
        /* other cases handled elsewhere */
        return ITER_CONTINUE;
}

static enum cdb_iter_ret update_prefix_list_line(cb_data_t *data)
{
        char path[KPATH_MAX];
        prefix_list_pars_t pars;
        confd_hkeypath_t flat_kp, *t_kp;
        int set;
#ifdef DEBUG
        confd_pp_kpath(path, KPATH_MAX, data->kp);
        DEBUG_LOG("%s: at %s, operation %i", __FUNCTION__, path, data->op);
#endif
        pars.ip_version = IP_VERSION(data);
        CHECK_OK(confd_pp_value(pars.plist_name, NAME_LEN, &data->kp->v[data->kp_start - 2][0]));
        t_kp = zconfd_flatten_data_path(&flat_kp, data, data->kp_start - 4);
        confd_pp_kpath(path, KPATH_MAX, data->kp);
        CHECK_OK(cdb_pushd(data->datasock, path));
        set = data->op != MOP_DELETED || data->kp_start > 4;
        CHECK_OK(get_prefix_list_line(data, set, &pars));
        CHECK_OK(cdb_popd(data->datasock));
        return ITER_CONTINUE;
}

static int get_prefix_list_line(cb_data_t *data, int set, void *priv)
{
        prefix_list_pars_t *pars = priv;
        if (set)
                BASE_COMMAND(data, "%s prefix-list %s seq ${key: number} ${ex: action}" \
                                         " ${ex: network/address}/${ex: network/prefix-length}" \
                                         "${ex: le, ' le %%s'}${ex: ge, ' ge %%s'}", pars->ip_version, pars->plist_name);
        else {
                /* need to set the plist to some fixed values.. */
                BASE_COMMAND(data, "%s prefix-list %s seq ${key: number} deny any", pars->ip_version, pars->plist_name);
                BASE_COMMAND(data, "no %s prefix-list %s seq ${key: number} deny any", pars->ip_version, pars->plist_name);
        }
        return CONFD_OK;
}

static int validatePrefixListRef(struct confd_trans_ctx *tctx,
                                                                 confd_hkeypath_t *keypath,
                                                                 confd_value_t *newval) {
        int th = tctx->thandle;
        int maapisock, kp_start;
        char pname[ BUFSIZ];
        char* findSubtree = "ipv6";

        confd_pp_value( pname, sizeof(pname), &keypath->v[0][0]);

        if ( CONFD_GET_XMLTAG(&(keypath->v[2][0])) == NSPREF(ipv6))     //we are in ipv6 subtree - check ip subtree
                findSubtree = "ip";

        //VALIDATE_FAIL( "Testing prefix list '%s' in %s, tag %d", pname, findSubtree, CONFD_GET_XMLTAG(&(keypath->v[2][0])));

        CHECK_OK( zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));

        if ( maapi_exists( maapisock, th, "%s/prefix-list{%s}", findSubtree, pname) == 1)
                VALIDATE_FAIL("Prefix list %s already exists in %s, use another name", pname, findSubtree);

        return CONFD_OK;
}

//////////////////////////////
// system/vr/ip{}/prefix-list
// checking ge and le, prefix-len in prefix-list
static int validatePlistPrefix(struct confd_trans_ctx *tctx,
                                                                 confd_hkeypath_t *keypath,
                                                                 confd_value_t *newval) {

        int maapisock, kp_start;
        uint8_t plen,   le,     ge;
        char path[KPATH_MAX];
        le = 0;
        ge = 0;

        CHECK_OK( zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));

        confd_pp_kpath(path, sizeof( path), keypath);

        CHECK_OK( maapi_pushd( maapisock, tctx->thandle, path));

        CHECK_OK( maapi_get_u_int8_elem( maapisock, tctx->thandle, &plen, "prefix-length"));

        if ( maapi_exists( maapisock, tctx->thandle, "../le") == 1)
                CHECK_OK( maapi_get_u_int8_elem( maapisock, tctx->thandle, &le, "../le"));

        if ( maapi_exists( maapisock, tctx->thandle, "../ge") == 1)
                CHECK_OK( maapi_get_u_int8_elem( maapisock, tctx->thandle, &ge, "../ge"));

        if ((ge != 0) && (!( plen < ge)))
                VALIDATE_FAIL( "prefix-len(%d) must be less than ge(%d)", plen, ge);

        if ((le != 0) && (!( plen < le)))
                VALIDATE_FAIL( "prefix-len(%d) must be less than le(%d)", plen, le);

        if ((ge != 0) && (le != 0) && (!( ge <= le)))
                VALIDATE_FAIL( "ge(%d) must be less or equal to le(%d)", ge, le);

        CHECK_OK( maapi_popd( maapisock, tctx->thandle));
        return CONFD_OK;
}

static int uniqItemsStr(char* str, int sock, int thandle, const char* path, confd_value_t* keys) {
        char key[ BUFSIZ];
        char tmp[ BUFSIZ];
        int h;
        str[ 0] = '\0';
        key[ 0] = '\0';

        confd_value_t v;
        if ( keys != NULL) {
                key[ 0] = '{';
                confd_pp_value( &key[ 1], BUFSIZ, &keys[ 0]);
                strncat( key, "}", sizeof( key));
        }
        CHECK_OK( maapi_get_enum_value_elem( sock, thandle, &h, "%s%s/action", path, key));
        strncpy( str, confd_hash2str(h), BUFSIZ);

        CHECK_OK( maapi_get_elem( sock, thandle, &v, "%s%s/network/address", path, key));
        confd_pp_value( tmp, sizeof( tmp), &v);
        confd_free_value( &v);
        strncat( tmp, "/", sizeof( tmp));
        strncat( str, " ", BUFSIZ);
        strncat( str, tmp, BUFSIZ);

        CHECK_OK( maapi_get_elem( sock, thandle, &v, "%s%s/network/prefix-length", path, key));
        confd_pp_value( tmp, sizeof( tmp), &v);
        confd_free_value( &v);
        strncat( str, tmp, BUFSIZ);

        if ( maapi_exists( sock, thandle, "%s%s/ge", path, key) == 1) {
                CHECK_OK( maapi_get_elem( sock, thandle, &v, "%s%s/ge", path, key));
                confd_pp_value( tmp, sizeof( tmp), &v);
                confd_free_value( &v);
                strncat( str, " ge ", BUFSIZ);
                strncat( str, tmp, BUFSIZ);
        }

        if ( maapi_exists( sock, thandle, "%s%s/le", path, key) == 1) {
                CHECK_OK( maapi_get_elem( sock, thandle, &v, "%s%s/le", path, key));
                confd_pp_value( tmp, sizeof( tmp), &v);
                confd_free_value( &v);
                strncat( str, " le ", BUFSIZ);
                strncat( str, tmp, BUFSIZ);
        }

        //DEBUG_LOG( "uniqItemsStr %s%s - '%s'", path, key, str);
        return CONFD_OK;
}

static int validateUniqItems(struct confd_trans_ctx *tctx,
                                                                                                                                         confd_hkeypath_t *keypath,
                                                                                                                                         confd_value_t *newval) {
        int maapisock, kp_start;
        struct maapi_cursor mc;
        char path[KPATH_MAX];
        char fndStr[ BUFSIZ];
        char curStr[ BUFSIZ];
        int eql = 0;
        int ret = CONFD_OK;
        confd_pp_kpath( path, KPATH_MAX, keypath);
        CHECK_OK( zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));
        CHECK_OK( uniqItemsStr( fndStr, maapisock, tctx->thandle, path, NULL));
        //delete last {*}
        char* lastP = strrchr( path, '{');
        if ( lastP != NULL)
                *lastP = '\0';  //trim the string

        CHECK_OK( maapi_init_cursor( maapisock, tctx->thandle, &mc, path));
        CHECK_OK( maapi_get_next( &mc));
        while ( mc.n != 0 && ret == CONFD_OK) {
                CHECK_OK( uniqItemsStr( curStr, maapisock, tctx->thandle, path, mc.keys));
                if ( strcmp( curStr, fndStr) == 0) {
                        eql++;
                        if ( eql > 1)
                                VALIDATE_FAIL( "denied duplicities in prefix-lists, path %s values '%s'", path, curStr);
                }
                ret = maapi_get_next( &mc);
        }
        maapi_destroy_cursor( &mc);
        return CONFD_OK;
}
