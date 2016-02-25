#ifndef _CONFD_GLOBAL_H_
#define _CONFD_GLOBAL_H_

#include <confd.h>
#include <confd_cdb.h>
#include <confd_maapi.h>
#include "zconfd_api.h"
#include "cliLib.h"

#define KPATH_MAX 255
#define IP4_LEN 20


enum cDaemons{ zebra, ripd, ripngd, ospfd, bgpd, ospf6d, isisd, cDaemons_end,
                all, all_routers}; //"all" and "all_routers" must be after cDaemons_end

extern struct tcliClient g_clients[ cDaemons_end];
const char* CLI_ERR_STR[ 1];

typedef int get_func_priv_t(int rsock, int set, void* priv);
typedef int get_func_priv_cb_t(cb_data_t *data, int set, void* priv);
typedef int get_func_t(cb_data_t *data, int set);

int confd_global_setup(struct confd_daemon_ctx *dctx);

int confd_alist_setup(struct confd_daemon_ctx *dctx);
int confd_rmap_setup(struct confd_daemon_ctx *dctx);
int confd_plist_setup(struct confd_daemon_ctx *dctx);
int confd_log_setup(struct confd_daemon_ctx *dctx);
int confd_keychain_setup(struct confd_daemon_ctx *dctx);
int ospf_init(struct confd_daemon_ctx *dctx);
int ospf_setup(struct confd_daemon_ctx *dctx);
int rip_init(struct confd_daemon_ctx *dctx);
int rip_setup( struct confd_daemon_ctx *dctx);
int zebra_d_init(struct confd_daemon_ctx *dctx);
int zebra_setup( struct confd_daemon_ctx *dctx);

int get_dynamic_priv(cb_data_t *data, const char *name, get_func_priv_cb_t *get_func, int set,void* priv);
int test_single(cb_data_t *data, const char *name, get_func_t *get_func);
int update_dynamic(cb_data_t *data, get_func_t *get_func);
int get_dynamic(cb_data_t *data, const char *name, get_func_t *get_func);

int get_data_key(cb_data_t *data, const char *name, int index, char *value, int size);

void dumpUpdate( const char* file, int line, const char* func, int kp_start, confd_hkeypath_t *kp, enum maapi_iter_op op, const char* msg, const char* data);
const char* getMaapiIterOp( enum maapi_iter_op op);
int get_alist( int rsock, char *rval, int n, const char* name);
int get_alist6( int rsock, char *rval, int n, const char* name);
void setPassword(  const char* pass);
void mk4subnet( const struct in_addr *addr, int prefixlen, struct in_addr *net);
void mk6subnet( const struct in6_addr *addr, int prefixlen, struct in6_addr *net);
int register_valpoint_cb_arr(struct confd_daemon_ctx *dx, const struct confd_valpoint_cb *vcb_arr);
int cli_command( enum cDaemons d, cb_data_t *data, const char* regStrPrompt, int regErrC, const char **regErrStr, const char *format, ...);
int cli_command_raw( enum cDaemons d, const char* cmd, const char* regStrPrompt, int regErrC, const char **regErrStr, get_clifunc_str_t* read_f, void* par);
void cli_configure_command(int start, enum cDaemons daemon);

const char* getCliHost( enum cDaemons d);
unsigned short int getCliPort( enum cDaemons d);
int getCliTimeout( enum cDaemons d);
const char* getCliPassword( enum cDaemons d);
int isNumeric( const char * s);

int validateAddrPrefixPair(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, confd_value_t *newval);
int validateAddrPrefixValues(struct confd_trans_ctx *tctx, const confd_value_t *addrVal, const confd_value_t *prefVal);
int validateAddrPrefixPaths(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, const char *addrPath, const char *prefPath);

int get_memory_elem(struct confd_trans_ctx *tctx, confd_hkeypath_t *keypath, enum cDaemons d);
#endif
