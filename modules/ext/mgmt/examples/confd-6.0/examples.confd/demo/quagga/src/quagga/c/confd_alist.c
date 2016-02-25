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
#include "quagga.h"       /* generated from yang */
#include "confd_global.h"

#define ALIST_ID_LEN 10

#define ALIST_COMMAND(data, command, args...) cli_command( all, data, ".*\\(config\\)# ", 1, CLI_ERR_STR, command , ##args)

static get_func_priv_cb_t get_access_list_generic;
static subscr_cb_t update_access_list_generic;

typedef int add_line_t(cb_data_t *data, char *alist_id);

typedef struct access_list_pars {
  add_line_t* add_line;
  char* name;
} access_list_pars_t;

static add_line_t add_access_list_line;
static add_line_t add_access_list_extended_line;
static add_line_t add_access_list_word_line;
static add_line_t add_access_list6_line;

static access_list_pars_t access_list_pars =
  {add_access_list_line, "access-list" };

static access_list_pars_t access_list_extended_pars =
  {add_access_list_extended_line, "access-list-extended" };

static access_list_pars_t access_list_word_pars =
  {add_access_list_word_line, "access-list-word" };

static access_list_pars_t access_list6_pars =
  {add_access_list6_line, "access-list" };

static int validateAccessListRef(struct confd_trans_ctx *tctx,
                           confd_hkeypath_t *keypath,
                           confd_value_t *newval);

static struct confd_valpoint_cb valpoints[] = {
        { .valpoint = "valAccessListRef", .validate = validateAccessListRef},
        { .valpoint = "" }
};

int confd_alist_setup(struct confd_daemon_ctx *dctx)
{
        DEBUG_LOG( "confd_alist_setup ");
        /* Register validation points */
        register_valpoint_cb_arr( dctx, valpoints);

        extern int quagga_not_running;
        if(quagga_not_running) return CONFD_OK;

        zconfd_subscribe_cdb(dctx, ACCESS_PRIO, update_access_list_generic, "ip/access-list", NULL);
        zconfd_subscribe_cdb(dctx, ACCESS_PRIO, update_access_list_generic, "ip/access-list-extended", NULL);
        zconfd_subscribe_cdb(dctx, ACCESS_PRIO, update_access_list_generic, "ip/access-list-word", NULL);
        zconfd_subscribe_cdb(dctx, ACCESS_PRIO, update_access_list_generic, "ipv6/access-list", NULL);

        cb_data_t data;
        zconfd_init_cb_data_ctx(&data, dctx, NULL, MOP_CREATED, NULL, NULL, NULL);

        cli_configure_command(1, all);
        CHECK_OK( get_dynamic_priv(&data, "ip/access-list", get_access_list_generic,
                                    1, &access_list_pars ));

        CHECK_OK( get_dynamic_priv(&data, "ip/access-list-extended", get_access_list_generic,
                                    1, &access_list_extended_pars ));

        CHECK_OK( get_dynamic_priv(&data,  "ip/access-list-word", get_access_list_generic,
                                    1, &access_list_word_pars ));

        CHECK_OK( get_dynamic_priv(&data,  "ipv6/access-list", get_access_list_generic,
                                    1, &access_list6_pars));
        cli_configure_command(0, all);

        DEBUG_LOG( "confd_alist_read_config end " );
        return CONFD_OK;
}

static int get_access_list_generic(cb_data_t *data, int set, void* priv)
{
        int i, n, ret = CONFD_OK;
        access_list_pars_t* pars = (access_list_pars_t*)priv;
        char alist_id[ALIST_ID_LEN];

        CHECK_CONFD(get_data_key(data, "id", 0, alist_id, ALIST_ID_LEN), "read id failed for %s", pars->name);

        if ( set == 0) {
                /* if delete only, exit */
                ALIST_COMMAND(data, "no%s access-list %s", pars->add_line == add_access_list6_line ? " ipv6" : "", alist_id);
                return ret;
        }

        n = cdb_num_instances(data->datasock, "line");
        CHECK_CONFD(n, "Failed to get number of instances '%s'", "line");

        for (i = 0; i < n && ret == CONFD_OK; i++) {
                DEBUG_LOG( "get_access_list line %d of %d", i, n);
                CHECK_CONFD(cdb_pushd(data->datasock, "line[%d]", i),   "Failed to pushd to 'line[%d]'",  i);
                ret = pars->add_line(data, alist_id);
                cdb_popd(data->datasock);
        }
        return ret;
}

static int update_list(cb_data_t *data, access_list_pars_t* pars )
{
        char path[BUFSIZ];
        char pattern[BUFSIZ];
        int ret = CONFD_OK;
        char* p,*pp;

        // update line - strip the rest after key{X} because the line has to be update
        // as a whole
        confd_pp_kpath(path, KPATH_MAX, data->kp);
        DEBUG_LOG( "update_list full_path=%s", path );

        sprintf(pattern, "/%s{", pars->name );
        p = strstr(path, pattern );
        if(!p){
          return CONFD_ERR;
        }

        pp = strchr(p, '}');
        if(!pp){
          return CONFD_ERR;
        }
        pp++;
        *pp = '\0';

        DEBUG_LOG( "update_list push_path=%s", path );

        if (data->op != MOP_CREATED) {
                ret = get_access_list_generic(data, 0, pars);
        }
        CHECK_OK(ret);
        if (data->op != MOP_DELETED) {
                CHECK_CONFD( cdb_pushd(data->datasock, path), "Failed to pushd to %s", path);
                ret = get_access_list_generic(data, 1, pars);
                cdb_popd(data->datasock);
        }
        return ret;
}

static enum cdb_iter_ret update_access_list_generic(cb_data_t *data)
{
        int ind = data->kp_start;
        char path[KPATH_MAX];
        access_list_pars_t* pars = NULL;

        confd_pp_kpath(path, KPATH_MAX, data->kp);
        DUMP_UPDATE( __FUNCTION__, data);

        switch( CONFD_GET_XMLTAG(&data->kp->v[ind][0] )){
        case NSPREF(ip):{
                switch(CONFD_GET_XMLTAG(&data->kp->v[ind-1][0])){
                case NSPREF(access_list):
                        pars = &access_list_pars;
                        break;

                case NSPREF(access_list_extended):
                        pars = &access_list_extended_pars;
                        break;

                case NSPREF(access_list_word):
                        pars = &access_list_word_pars;
                        break;

                default:
                        ERROR_LOG( "update_access_list_generic ip not found full_path=%s", path );
                        break;

                }
                break;
        }
        case NSPREF(ipv6):{
                switch(CONFD_GET_XMLTAG(&data->kp->v[ind-1][0])){
                case NSPREF(access_list):
                        pars = &access_list6_pars;
                        break;

                default:
                        ERROR_LOG( "update_access_list_generic ipv6 not found full_path=%s", path );
                        break;
                }
                break;
        }
        default:

                ERROR_LOG( "update_access_list_generic not found full_path=%s", path );
        }

        ind-=2;

        DEBUG_LOG( "update_access_list_generic id=%d", ind );

        confd_hkeypath_t flat_kp, *t_kp;
        t_kp = data->kp;
        /* delete and recreate whole list if anything is changed there */
        if(ind > 0){
                DEBUG_LOG( "update_access_list_generic on list lebel full_path=%s", path );
                data->op = MOP_MODIFIED;
                zconfd_flatten_data_path(&flat_kp, data, ind);
        }

        CHECK_OK( update_list(data, pars));

        return ITER_CONTINUE;
}

static int add_access_list_line(cb_data_t *data, char *alist_id) {
        ALIST_COMMAND(data, "access-list %s ${ex: action} ${ex: address} ${ex: mask}", alist_id);
        return CONFD_OK;
}


static int add_access_list_extended_line(cb_data_t *data, char *alist_id) {
        ALIST_COMMAND(data, "access-list %s ${ex: action} ip ${ex: source} ${ex: source-mask} ${ex: destination} ${ex: destination-mask}", alist_id);
        return CONFD_OK;
}


static int add_access_list_word_line(cb_data_t *data, char *alist_id) {
        ALIST_COMMAND(data, "access-list %s ${ex: action} ${ex:match/address}/${ex: match/prefix-length}${bool: exact-match, ' exact-match', ''}", alist_id);
        return CONFD_OK;
}


/**************** IPv6 access-lists ****************/
static int add_access_list6_line(cb_data_t *data, char *alist_id) {
        ALIST_COMMAND(data, "ipv6 access-list %s ${ex: action} ${ex:match/address}/${ex: match/prefix-length}${bool: exact-match, ' exact-match', ''}", alist_id);
        return CONFD_OK;
}

static int validateAccessListRef(struct confd_trans_ctx *tctx,
                                 confd_hkeypath_t *keypath,
                                 confd_value_t *newval) {

        int th = tctx->thandle;
        int maapisock, kp_start;
        char aname[ BUFSIZ];
        int anameNum = -1;
        char* test;

        confd_pp_value( aname, sizeof( aname), &keypath->v[0][0]);
        if ( isNumeric( aname))
                anameNum = atoi( aname);

        CHECK_OK( zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL ));

        if ((( anameNum >= 1) && ( anameNum <= 99)) || (( anameNum >= 1300) && ( anameNum <= 1999))) {
                test = "access-list";
                if (( CONFD_GET_XMLTAG(&(keypath->v[1][0])) != NSPREF( access_list)) ||
                        ( CONFD_GET_XMLTAG(&(keypath->v[2][0])) != NSPREF( ip))) {      //check access-list
                                if ( maapi_exists( maapisock, th, "ip/%s{%s}", test, aname) == 1)
                                        VALIDATE_FAIL("access list name %s already exists in ip/%s, use another name", aname, test);
                }
        }

        if ((( anameNum >= 100) && ( anameNum <= 199)) || (( anameNum >= 2000) && ( anameNum <= 2699))) {
                test = "access-list-extended";
                if (( CONFD_GET_XMLTAG(&(keypath->v[1][0])) != NSPREF( access_list_extended)) ||
                        ( CONFD_GET_XMLTAG(&(keypath->v[2][0])) != NSPREF( ip))) {      //check access-list-extended
                                if ( maapi_exists( maapisock, th, "ip/%s{%s}", test, aname) == 1)
                                        VALIDATE_FAIL("access list name %s already exists in ip/%s, use another name", aname, test);
                }
        }

        test = "access-list-word";
        if (( CONFD_GET_XMLTAG(&(keypath->v[1][0])) != NSPREF( access_list_word)) ||
                ( CONFD_GET_XMLTAG(&(keypath->v[2][0])) != NSPREF( ip))) {      //check access-list-word
                if ( maapi_exists( maapisock, th, "ip/%s{%s}", test, aname) == 1)
                        VALIDATE_FAIL("access list name %s already exists in ip/%s, use another name", aname, test);
        }

        /* ipv6 */
        test = "access-list";
        if (( CONFD_GET_XMLTAG(&(keypath->v[1][0])) != NSPREF( access_list)) ||
                        ( CONFD_GET_XMLTAG(&(keypath->v[2][0])) != NSPREF( ipv6))) {    //check ipv6 access-list
                if ( maapi_exists( maapisock, th, "ipv6/%s{%s}", test, aname) == 1)
                        VALIDATE_FAIL("access list name %s already exists in ipv6/%s, use another name", aname, test);
        }

        return CONFD_OK;
}
