#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define _XOPEN_SOURCE
#include <unistd.h>

#include <confd.h>
#include <confd_cdb.h>
#include <confd_maapi.h>

#undef _POSIX_C_SOURCE

#include "zconfd_api.h"
#include "cli_str.h"
#include "cliLib.h"
#include "quagga.h"     /* generated from yang */
#include "confd_global.h"
#include "main.h"

struct tcliClient g_clients[ cDaemons_end];

static subscr_cb_t update_password;
static get_func_t get_password_setting;

const char* CLI_ERR_STR[] = { "^%.*"};

static void confd_clients_refresh() {
  //prevention against VTY connect timeout
  char s[ BUFSIZ];
  enum cDaemons d;
        TRex *x = NULL;

        if ( strMatchRegCompile( &x, ".*# ") == CLI_ERR)
                return;

        for ( d = 0; d < cDaemons_end; d++)
                if ( cliOpened( &g_clients[ d])) {
                        //DEBUG_LOG( "Refresh client %d, write <CR>", d);
                        cliWriteStr( &g_clients[ d], "\n");
                        do {
                                if ( cliRecvStrReg( x, &g_clients[ d], s, sizeof( s)) < 0)
                                        break;
                                //DEBUG_LOG( "Refresh client %d, received '%s'", d, s);
                        } while ( strMatchRegCompiled( x, s) != 1);
                }

        trex_free(x);
}

static int cli_refresh_thread( void *thread_data) {
  confd_clients_refresh();
  thread_add_timer( cli_refresh_thread, NULL, 60);
  return 0;
}

const char* getCliHost( enum cDaemons d) {
  return "127.0.0.1";
}

unsigned short int getCliPort( enum cDaemons d) {
        unsigned short int tcpPort[ cDaemons_end] = { 2601, 2602, 2603, 2604, 2605, 2606, 2608};
        if ( d < cDaemons_end)
                return tcpPort[ d];
        ERROR_LOG( "Error: unknown daemon %d", d);
        return 0;
}

const char* getCliName( enum cDaemons d) {
        static const char dmdNames[][ cDaemons_end] = { "zebra", "ripd", "ripngd", "ospfd", "bgpd", "ospf6d", "isisd"};
        if ( d < cDaemons_end)
                return dmdNames[ d];
        ERROR_LOG( "Error: unknown daemon %d", d);
        return "error";
}
int getCliTimeout( enum cDaemons d) {
  return 1000;  //1s
}

const char* getCliPassword( enum cDaemons d) {
  return "zebra";
}

static int init(struct confd_daemon_ctx *dctx)
{
  /* CHECK_OK(access_confd_init(dctx, zg)); */
  /* ...                                         */

  // Here we want to verify connectivity with the Quagga routers that
  // we want to manage. In the real world, we'd shut down if no contact
  // is made, but in this demo we'd like to run as much as possible
  // even without Quagga(!). Not all users are going to have it installed

  extern int quagga_not_running;
  quagga_not_running = 1; // Assume not running, until contact established
  if ( cliOpen( &g_clients[zebra], getCliHost( zebra), getCliPort( zebra), getCliTimeout( zebra)) == CLI_SUCC) {
    if ( cliOpen( &g_clients[ripd], getCliHost( ripd), getCliPort( ripd), getCliTimeout( ripd)) == CLI_SUCC) {
      if ( cliOpen( &g_clients[ospfd], getCliHost( ospfd), getCliPort( ospfd), getCliTimeout( ospfd)) == CLI_SUCC) {

        quagga_not_running = 0; // Contact established, let's run for real!

        cliClose( &g_clients[ospfd]);
      } else {
        ERROR_LOG( "Could not establish contact with Quagga ospfd");
      }
      cliClose( &g_clients[ripd]);
    } else {
      ERROR_LOG( "Could not establish contact with Quagga ripd");
    }
    cliClose( &g_clients[zebra]);
  } else {
    ERROR_LOG( "Could not establish contact with Quagga zebra");
  }
  if(quagga_not_running) {
    ERROR_LOG( "==============================================================");
    ERROR_LOG( "NOTE: Cannot establish contact with Quagga, all needed daemons");
    ERROR_LOG( "(zebra, ripd, ospfd) are probably not running.");
    ERROR_LOG( "Now going into DRY RUN MODE -- ");
    ERROR_LOG( "will run without actually interacting with Quagga!");
    ERROR_LOG( "==============================================================");
  }

  CHECK_OK( zebra_d_init(dctx));
  CHECK_OK( rip_init(dctx));
  CHECK_OK(ospf_init(dctx));

  return CONFD_OK;
}

static int setup( struct confd_daemon_ctx *dctx) {
  return confd_global_setup( dctx);
}

int confd_global_setup(struct confd_daemon_ctx *dctx)
{
  CHECK_OK( confd_log_setup( dctx));
  CHECK_OK( confd_alist_setup( dctx));
  CHECK_OK( confd_plist_setup( dctx));
  CHECK_OK( confd_keychain_setup( dctx));
  CHECK_OK( confd_rmap_setup( dctx));

  /* daemon-specific setup */
  CHECK_OK(zebra_setup(dctx));
  CHECK_OK(rip_setup(dctx));
  CHECK_OK(ospf_setup(dctx));

  extern int quagga_not_running;
  if(quagga_not_running) return CONFD_OK;

  cb_data_t data;
  zconfd_init_cb_data_ctx(&data, dctx, NULL, MOP_CREATED, NULL, NULL, NULL);
  get_password_setting(&data, 1);
  zconfd_subscribe_cdb(dctx, PASSWD_PRIO, update_password, "password", NULL);
  return CONFD_OK;
}

void framework_init() {
  //clear all daemons
  enum cDaemons d;
  for ( d = 0; d < cDaemons_end; d++)
        cliClear( &g_clients[ d]);
  thread_add_timer( cli_refresh_thread, NULL, 60);
  zconfd_init( "cmng", CONFD_TRACE, init, setup);
}

int test_single(cb_data_t *data, const char *name, get_func_t *get_func)
{
  return cdb_exists(data->datasock, name) == 1 ? get_func(data, 1) : CONFD_OK;
}

int get_dynamic_priv(cb_data_t *data, const char *name, get_func_priv_cb_t *get_func, int set, void* priv)
{
  int i, n;
  int ret = CONFD_OK;
  char path[ BUFSIZ];
  path[ 0] = '\0';

  cdb_getcwd(data->datasock, sizeof( path), path);

  n = cdb_num_instances(data->datasock, name);

  DEBUG_LOG( "get_dynamic_priv name %s inst=%d %s", name, n, path);

  CHECK_CONFD( n, "Call failed cdb_num_instances %s '%s'", path, name);

  for (i = 0; i < n && ret == CONFD_OK; i++) {
        CHECK_CONFD(cdb_pushd(data->datasock, "%s[%d]", name, i), "Failed to pushd to '%s[%d]'", name, i);
        ret = (*get_func)(data,  set, priv);
        if (cdb_popd(data->datasock) != CONFD_OK)
          ret = CONFD_ERR;
  }
  return ret;
}

int update_dynamic(cb_data_t *data, get_func_t *get_func)
{
        char path[BUFSIZ];
        int ret = CONFD_OK;

        confd_pp_kpath(path, sizeof(path), data->kp);
        DEBUG_LOG( "path to push to: %s", path);
        if (data->op == MOP_DELETED || data->op == MOP_MODIFIED || data->op == MOP_VALUE_SET) {
                /* do not cd/pushd - the path need not exist! */
                ret = (*get_func)(data, 0);
        }
        CHECK_OK(ret);
        if (data->op == MOP_MODIFIED || data->op == MOP_VALUE_SET || data->op == MOP_CREATED) {
                CHECK_CONFD(cdb_pushd(data->datasock, path),
                                        "router/rip: failed to pushd to %s", path);
                ret = (*get_func)(data, 1);
                cdb_popd(data->datasock);
        }
        return ret;
}

int get_dynamic(cb_data_t *data, const char *name, get_func_t *get_func)
{
        int i, n;
        int ret = CONFD_OK;

        n = cdb_num_instances(data->datasock, name);
        CHECK_CONFD(n, "Failed to get number of instances '%s'", name);
        DEBUG_LOG( "get_dynamic name=%s n=%d", name, n);

        for (i = 0; i < n && ret == CONFD_OK; i++) {
                CHECK_CONFD(cdb_pushd(data->datasock, "%s[%d]", name, i),
                        "router/rip: Failed to pushd to '%s[%d]'", name, i);
                ret = (*get_func)(data, 1);
                CHECK_CONFD( cdb_popd(data->datasock), "router/rip: Failed to popd %i", i);
        }
        return ret;
}

int get_hostname_setting(int rsock, int set) {
  char buff[BUFSIZ];

  //<elem name="hostname" type=""confd:inetAddressDNS" default="ripd"/>
  char* name = "hostname";
  CHECK_CONFD( cdb_get_str( rsock, buff, sizeof( buff), name), "Failed to get '%s'", name);

  //TODO: hostname is saved in cli variable (type struct cli*)

  /*
        if ( set)
        CHECK_API( host_hostname_set( cli, buff),       "Failed to call api host_hostname_set %s", buff);
        else
        CHECK_API( host_hostname_unset( cli), "Failed to call api host_hostname_unset %s", buff);

  */
  return CONFD_OK;
}

int get_password_setting(cb_data_t *data, int set) {
  char buff[BUFSIZ];

  //<elem name="password" type="confd:inetAddressDNS" default="zebra"/>
  char* name = "password";
  if ( cdb_exists(data->datasock, name) == 1) {
        CHECK_CONFD( cdb_get_str(data->datasock, buff, sizeof( buff), name), "Failed to get '%s'", name);
        DEBUG_LOG( "get_password_setting new password is %s", buff);
  } else {
        strcpy( buff, "zebra");
  }

  return CONFD_OK;
}

static enum cdb_iter_ret update_password(cb_data_t *data) {
  DUMP_UPDATE( __FUNCTION__, data);
  CHECK_OK( update_dynamic( data, get_password_setting));
  return ITER_CONTINUE;
}

const char* getMaapiIterOp( enum maapi_iter_op op) {
  switch ( op) {
  case MOP_CREATED: return "CREATE";
  case MOP_DELETED: return "DELETE";
  case MOP_MODIFIED: return "MODIFY";
  case MOP_VALUE_SET: return "SET";
  default: return "UNKNOWN!";
  }

}

void dumpUpdate( const char* file, int line, const char* func, int kp_start, confd_hkeypath_t *kp, enum maapi_iter_op op,  const char* msg, const char* data) {
  char path[ BUFSIZ];
  confd_pp_kpath( path, sizeof( path), kp);
  DEBUG_LOG("upd: %s:%d %s() - %s %s[%d] %s%s%s", file, line, func, getMaapiIterOp( op), path, kp_start, msg == NULL ? "" : msg, msg == NULL ? "" : "=", data == NULL ? "" : data);
}

int get_alist( int rsock, char *rval, int n, const char* name ) {
  confd_value_t v;
  rval[ 0] = '\0';

  if ( cdb_exists( rsock, "%s/access-list", name) == 1) {
        CHECK_CONFD( cdb_get( rsock, &v, "%s/access-list", name), "Failed to get '%s/access-list'", name);
        confd_pp_value( rval, n, &v);
        confd_free_value( &v);
  }

  if ( cdb_exists( rsock, "%s/access-list-extended", name) == 1) {
        CHECK_CONFD( cdb_get( rsock, &v, "%s/access-list-extended", name), "Failed to get '%s/access-list-extended'", name);
        confd_pp_value( rval, n, &v);
        confd_free_value( &v);
  }

  if ( cdb_exists( rsock, "%s/access-list-word", name) == 1)
        CHECK_CONFD(cdb_get_str( rsock, rval, n, "%s/access-list-word", name), "Failed to get '%s/access-list-word'", name);

  return CONFD_OK;
}

int get_alist6( int rsock, char *rval, int n, const char* name ) {
  rval[ 0] = '\0';

  if ( cdb_exists( rsock, name) == 1)
        CHECK_CONFD(cdb_get_str( rsock, rval, n, "%s/access-list", name), "Failed to get '%s/access-list'", name);

  return CONFD_OK;
}

confd_value_t *get_path_key(const cb_data_t *data, const char *name, int index)
{
    struct confd_cs_node *cs;
    u_int32_t *key;
    confd_value_t *keyval;
    cs = confd_find_cs_node(data->kp, index < 0 ? data->kp->len - data->kp_start - index - 1 : data->kp->len - index);
    if (cs->info.keys == NULL)
        return NULL;
    for (key = cs->info.keys; *key != 0 && strcmp(name, confd_hash2str(*key)) != 0; key++);
    if (*key == 0)
        return NULL;

    keyval = data->kp->v[index < 0 ? data->kp_start + index + 1 : index] + (key - cs->info.keys);
    return keyval->type == C_XMLTAG || keyval->type == C_NOEXISTS ? NULL : keyval;
}

int get_data_key(cb_data_t *data, const char *name, int index, char *value, int size) {
  if (data->kp == NULL) {
    confd_value_t v;
    if (cdb_exists(data->datasock, name) == 1 && cdb_get(data->datasock, &v, name) == CONFD_OK) {
      int rv = confd_pp_value(value, size, &v);
      confd_free_value(&v);
      return rv;
    } else
      return CONFD_ERR;
  } else {
    confd_value_t *v = get_path_key(data, name, index);
    return v == NULL ? CONFD_ERR : confd_pp_value(value, size, v);
  }
}

#define MASK(addr, len) (addr & (0xffffffff << (8 * sizeof(addr) - (len))))

void mk4subnet( const struct in_addr *addr, int prefixlen, struct in_addr *net) {
  net->s_addr = htonl( MASK( ntohl(addr->s_addr), prefixlen));
}

void mk6subnet( const struct in6_addr *addr, int prefixlen, struct in6_addr *net) {
  int i, rem;

  for (i = 0, rem = prefixlen; i < 16; i++, rem -= 8) {
        if (rem >= 8)
          net->s6_addr[i] = addr->s6_addr[i];
        else if (rem > 0)
          net->s6_addr[i] = MASK(addr->s6_addr[i], rem);
        else
          net->s6_addr[i] = 0;
  }
}

int register_valpoint_cb_arr(struct confd_daemon_ctx *dx,  const struct confd_valpoint_cb *vcb_arr) {
  const struct confd_valpoint_cb *valp;

  /* Register validation points */
  for ( valp = vcb_arr; valp->valpoint[0] != '\0'; valp++) {
        CHECK_CONFD( confd_register_valpoint_cb( dx, valp), "Failed to register validation point \"%s\"", valp->valpoint);
  }

  return CONFD_OK;
}

void subscriptions_command(int start)
{
  cli_configure_command(start, all);
}

void cli_configure_command(int start, enum cDaemons daemon)
{
  if (start)
    cli_command_raw(daemon, "configure terminal", ".*\\(config\\)# ", 0, NULL, NULL, NULL);
  else
    cli_command_raw(daemon, "exit", ".*# ", 0, NULL, NULL, NULL);
}

int cli_command_raw( enum cDaemons d, const char* cmd, const char* regStrPrompt, int regErrC, const char **regErrStr, get_clifunc_str_t* read_f, void* par) {
        int ret = CLI_SUCC;
        enum cDaemons di;
        DEBUG_LOG( "%s: %s '%s'", __FUNCTION__, d < cDaemons_end ? getCliName( d) : ( d == all ? "all" : "all_routers"), cmd == NULL ? "<CR>" : cmd);

        if ( d < cDaemons_end) { //just one daemon
                return cliWriteCommand( &g_clients[ d], cmd, regStrPrompt, regErrC, regErrStr, read_f, par);
        }

        //all the daemons
        for ( di = 0; di < cDaemons_end; di++) {
                if (( d == all_routers) && (di == zebra))       //skip zebra for d=all_routers
                        continue;
                if ( cliOpened( &g_clients[ di])) {
                        if ( cliWriteCommand( &g_clients[ di], cmd, regStrPrompt, regErrC, regErrStr, read_f, par) == CLI_ERR)
                                ret = CLI_ERR;
                }
        }
        return ret;
}

int cli_command( enum cDaemons d, cb_data_t *data, const char* regStrPrompt, int regErrC, const char **regErrStr, const char *format, ...) {
  char buf[BUFSIZ];
  int rv;
  va_list ap;
  va_start(ap, format);
  rv = cli_format(data, buf, BUFSIZ, format, ap);
  va_end(ap);

  if (rv < 0)
    return CONFD_ERR;

  return cli_command_raw( d, buf, regStrPrompt, regErrC, regErrStr, NULL, NULL) == CLI_SUCC ? CONFD_OK : CONFD_ERR;
}

int read_variable_ex(void *data_, const char *name, char *value, int size) {
  cb_data_t *data = data_;
  confd_value_t v;
  int rv;
  int ret;
  switch (ret = cdb_exists(data->datasock, name)) {
  case 1:
    CHECK_OK(cdb_get(data->datasock, &v, name));
    if (v.type == C_DATETIME) {
      struct confd_datetime *dt = &v.val.datetime;
      const char * const MONTHS[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
      if ((rv = snprintf(value, size, "%u:%u:%u %u %s %i", dt->hour, dt->min, dt->sec, dt->day, MONTHS[dt->month - 1], dt->year)) >= size)
        rv = -1;
    } else
      rv = confd_pp_value(value, size, &v);
    confd_free_value(&v);
    break;
  case 0:
    value[0] = 0;
    rv = 0;
    break;
  default:
    ERROR_LOG("cdb_exists(%i, \"%s\") failed: %i", data->datasock, name, ret);
    rv = -1;
  }
  return rv;
}

int read_variable_bool(void *data_, const char *name, int *value) {
  int ret, rv;
  cb_data_t *data = data_;
  // should not be key value (?)
  switch (ret = cdb_exists(data->datasock, name)) {
  case 1:
    CHECK_OK(cdb_get_bool(data->datasock, value, name));
    rv = 1;
    break;
  case 0:
    *value = 0;
    rv = 0;
    break;
  default:
    ERROR_LOG("cdb_exists(%i, \"%s\") failed: %i", data->datasock, name, ret);
    rv = -1;
  }
  return rv;
}

int read_variable_key(void *data_, const char *name, int index, char *value, int size) {
  int ret = get_data_key(data_, name, index, value, size);
  return ret == CONFD_ERR ? -1 : ret;
}

int read_variable_arg(void *data_, const char *name, char *value, int size) {
  cb_data_t *data = data_;
  return data->newv == NULL ? read_variable_ex(data_, name, value, size) : confd_pp_value(value, size, data->newv);
}

int read_variable_alist(void *data_, const char *name, char *value, int size) {
  cb_data_t *data = data_;
  return get_alist(data->datasock, value, size, name) == CONFD_OK ? strlen(value) : -1;
}

int isNumeric( const char * s) {
        while ( *s != '\0') {
                if (! isalnum( *s))
                        return 0;
                s++;
        }
        return 1;
}

/**
 * Validate IP4 address/prefix pair. The pair validates if all address
 * bits after the prefix are zero.
 *
 * @param tctx
 * @param keypath must contain address as a first key, prefix as a second
 * @param newval
 */
int validateAddrPrefixPair(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval)
{
  return validateAddrPrefixValues(tctx, &keypath->v[0][0], &keypath->v[0][1]);

}

/**
 * Validate that the subelements' values as given by the two paths form valid address/prefix pair.
 */
int validateAddrPrefixPaths(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, const char *addrPath, const char *prefPath)
{
  int maapisock, kp_start;
  confd_value_t addrVal, prefVal;
  char path[KPATH_MAX];
  int rv;
  confd_pp_kpath(path, KPATH_MAX, keypath);
  CHECK_OK(zconfd_trans_get(tctx, keypath, &maapisock, &kp_start, NULL));
  CHECK_CONFD(maapi_get_elem(maapisock, tctx->thandle, &addrVal, "%s/%s", path, addrPath), "Cannot get address value: %s", addrPath);
  CHECK_CONFD(maapi_get_elem(maapisock, tctx->thandle, &prefVal, "%s/%s", path, prefPath), "Cannot get prefix value: %s", prefPath);
  rv = validateAddrPrefixValues(tctx, &addrVal, &prefVal);
  confd_free_value(&addrVal);
  confd_free_value(&prefVal);
  return rv;
}

/**
 * Validate that the two values form correct address/prefix pair.
 */
int validateAddrPrefixValues(struct confd_trans_ctx *tctx, const confd_value_t *addrVal, const confd_value_t *prefVal)
{
        struct in_addr addr, subnet;
        struct in6_addr addr6, subnet6;
        void *aptr, *sptr;
        int8_t prefixLen;
        int af, len;
        char addrstr[INET6_ADDRSTRLEN];

        prefixLen = CONFD_GET_UINT8(prefVal);

        switch (addrVal->type) {
        case C_IPV4:
                addr = CONFD_GET_IPV4(addrVal);
                mk4subnet( &addr, prefixLen, &subnet);
                aptr = &addr;
                sptr = &subnet;
                af = AF_INET;
                len = sizeof(addr);
                break;
        case C_IPV6:
                addr6 = CONFD_GET_IPV6(addrVal);
                mk6subnet( &addr6, prefixLen, &subnet6);
                aptr = &addr6;
                sptr = &subnet6;
                af = AF_INET6;
                len = sizeof(addr6);
                break;
        default:
                VALIDATE_FAIL("Unknown key type: %i", addrVal->type);
                break;
        }

        if ( memcmp(aptr, sptr, len) != 0) {    //addr is not subnet
                inet_ntop(af, sptr, addrstr, sizeof(addrstr));
                VALIDATE_FAIL( "Invalid network address with prefix, correct address is %s", addrstr);
        }
        return CONFD_OK;
}

struct tmemoryStatus {
        u_int64_t total;
        u_int64_t used;
        u_int64_t free;
};

/*********************************************/
int memoryStrFunc( const char* str, int prompt, void* par) {
        struct tmemoryStatus* pm = (struct tmemoryStatus*)par;
        u_int64_t* pv;

        //DEBUG_LOG( "memoryStrFunc '%s' prompt(%d)", str, prompt);

        pv = NULL;

        if ( strstr( str, "Total heap allocated:") != NULL) {
                //DEBUG_LOG( "memoryStrFunc total");
                pv = &pm->total;
        }

        if ( strstr( str, "Used ordinary blocks:") != NULL) {
                //DEBUG_LOG( "memoryStrFunc used");
                pv = &pm->used;
        }

        if ( strstr( str, "Free ordinary blocks:") != NULL) {
                //DEBUG_LOG( "memoryStrFunc free");
                pv = &pm->free;
        }

        if ( pv == NULL)        //other lines - skip
                return 0;

        *pv = atoi( strpbrk( str, "0123456789"));       //find number in string

        //convert units
        if ( strstr( str, "TiB") != NULL)
                *pv <<= 40;

        if ( strstr( str, "GiB") != NULL)
                *pv <<= 30;

        if ( strstr( str, "MiB") != NULL)
                *pv <<= 20;

        if ( strstr( str, "KiB") != NULL)
                *pv <<= 10;

        return 0;
}

/*********************************************/
int get_memory_elem(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, enum cDaemons d) {
        struct tmemoryStatus m;
        confd_value_t v;

        memset( &m, 0, sizeof( m));
        if ( cli_command_raw( d, "show memory", ".*# ", 1, CLI_ERR_STR, memoryStrFunc, &m) ==   CLI_ERR) //fill m struct
                return CONFD_ERR;

        switch ( CONFD_GET_XMLTAG( &(keypath->v[0][0]))) {
                case NSPREF(total):     CONFD_SET_UINT32( &v, m.total); break;
                case NSPREF(used):      CONFD_SET_UINT32( &v, m.used); break;
                case NSPREF(free):      CONFD_SET_UINT32( &v, m.free); break;
        }
        confd_data_reply_value(tctx, &v);
        return CONFD_OK;
}
