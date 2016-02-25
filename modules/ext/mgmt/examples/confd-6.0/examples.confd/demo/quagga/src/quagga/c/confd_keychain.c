#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <confd.h>
#include <confd_cdb.h>
#include <confd_maapi.h>

#include "zconfd_api.h"
#include "quagga.h"     /* generated from yang */
#include "confd_global.h"

#define NAME_LEN 50

#define K_COMMAND(data, command, prompt, args...) cli_command( ripd, data, prompt, 1, CLI_ERR_STR, command , ##args)
#define KEYCHAIN_COMMAND(data, command, args...) K_COMMAND(data, command, ".*\\(config-keychain\\)# " , ##args)
#define KEY_COMMAND(data, command, args...) K_COMMAND(data, command, ".*\\(config-keychain-key\\)# " , ##args)
#define BASE_COMMAND(data, command, args...) K_COMMAND(data, command, ".*\\(config\\)# " , ##args)

static get_func_t get_keychain;
static get_func_t get_key_inst;
static get_func_t get_keystring;
static get_func_t get_accept_lifetime;
static get_func_t get_send_lifetime;

static subscr_cb_t update_keychain_inst;
static subscr_cb_t update_key_inst;
static subscr_cb_t update_key;

int confd_keychain_setup(struct confd_daemon_ctx *dctx) {
        DEBUG_LOG( "confd_keychain_setup");

        extern int quagga_not_running;
        if(quagga_not_running) return CONFD_OK;

        zconfd_subscribe_cdb_root(dctx, KEYCHAIN_PRIO, update_keychain_inst, "key-chain", NULL);
        zconfd_subscribe_cdb_pair(dctx, KEYCHAIN_PRIO, update_key_inst, update_key, "key-chain/key", NULL);

        /* read configuration */
        cb_data_t data;
        zconfd_init_cb_data_ctx(&data, dctx, NULL, MOP_CREATED, NULL, NULL, NULL);
        CHECK_OK( get_dynamic( &data, "key-chain", get_keychain));

        return CONFD_OK;
}

static int get_key_inst(cb_data_t *data, int set)
{
        if (set == 0) {
                ERROR_LOG("Can't unget a key line");
                return CONFD_ERR;
        }

        KEY_COMMAND(data, "key ${ex: id}");
        CHECK_OK(test_single(data, "key-string", get_keystring));
        CHECK_OK(test_single(data, "accept-lifetime", get_accept_lifetime));
        CHECK_OK(test_single(data, "send-lifetime", get_send_lifetime));
        KEYCHAIN_COMMAND(data, "exit");
        return CONFD_OK;
}

static int get_keychain(cb_data_t *data, int set)
{
        if (set == 0) {
                ERROR_LOG("Should not get here - can't unget a keychain");
                return CONFD_ERR;
        }
        cli_configure_command(1, ripd);
        KEYCHAIN_COMMAND(data, "key chain ${ex: name}");
        CHECK_OK(get_dynamic(data, "key", get_key_inst));
        BASE_COMMAND(data, "exit");
        cli_configure_command(0, ripd);
        return CONFD_OK;
}

static enum cdb_iter_ret update_keychain_inst(cb_data_t *data)
{
#ifdef DEBUG
        char path[KPATH_MAX];
        confd_pp_kpath(path, KPATH_MAX, data->kp);
        DEBUG_LOG("%s %s", __FUNCTION__, path);
#endif

        switch(data->op){
        case MOP_CREATED:
                KEYCHAIN_COMMAND(data, "key chain ${key: name}");
                BASE_COMMAND(data, "exit");
                break;
        case MOP_DELETED:
                BASE_COMMAND(data, "no key chain ${key: name}");
                break;
        default:
                return ITER_RECURSE;
        }
        return ITER_CONTINUE;
}

static enum cdb_iter_ret update_key_inst(cb_data_t *data)
{
#ifdef DEBUG
        char path[KPATH_MAX];
        confd_pp_kpath(path, KPATH_MAX, data->kp);
        DEBUG_LOG("%s %s", __FUNCTION__, path);
#endif

        switch(data->op){
        case MOP_CREATED:
                KEYCHAIN_COMMAND(data, "key chain ${key: name(2)}");
                KEY_COMMAND(data, "key ${key: id}");
                KEYCHAIN_COMMAND(data, "exit");
                BASE_COMMAND(data, "exit");
                return ITER_RECURSE;
        case MOP_DELETED:
                KEYCHAIN_COMMAND(data, "key chain ${key: name(2)}");
                KEYCHAIN_COMMAND(data, "no key ${key: id}");
                BASE_COMMAND(data, "exit");
                break;
        default:
                return ITER_RECURSE;
        }
        return ITER_CONTINUE;
}

static enum cdb_iter_ret update_key(cb_data_t *data)
{
        char path[KPATH_MAX];
        confd_hkeypath_t flat_kp, *t_kp;
        int tag = CONFD_GET_XMLTAG(&data->kp->v[data->kp_start - 4][0]);
#ifdef DEBUG
        confd_pp_kpath(path, KPATH_MAX, data->kp);
        DEBUG_LOG("%s %s - %s", __FUNCTION__, path, confd_hash2str(tag));
#endif

        KEYCHAIN_COMMAND(data, "key chain ${key: name(-2)}");
        t_kp = zconfd_flatten_data_path(&flat_kp, data, data->kp_start - 3);

        confd_pp_kpath(path, KPATH_MAX, &flat_kp);
        CHECK_OK(cdb_pushd(data->datasock, path));
        if (data->op == MOP_DELETED && tag != NSPREF(key_string)) {
                /* need to reset whole key */
                KEYCHAIN_COMMAND(data, "no key ${key: id}");
                get_key_inst(data, 1);
        } else {
                KEY_COMMAND(data, "key ${key: id}");
                switch (tag) {
                case NSPREF(key_string):
                        get_keystring(data, data->op != MOP_DELETED);
                        break;
                case NSPREF(accept_lifetime):
                        get_accept_lifetime(data, 1);
                        break;
                case NSPREF(send_lifetime):
                        get_send_lifetime(data, 1);
                        break;
                }
                KEYCHAIN_COMMAND(data, "exit");
        }
        zconfd_recover_data_path(data, t_kp);
        cdb_popd(data->datasock);
        BASE_COMMAND(data, "exit");
        return ITER_CONTINUE;
}

static int get_keystring(cb_data_t *data, int set) {
  if (set)
    KEY_COMMAND(data, "key-string ${ex: key-string}");
  else
    KEY_COMMAND(data, "no key-string");
  return CONFD_OK;
}

static int get_accept_lifetime(cb_data_t *data, int set) {
  if (set)
    KEY_COMMAND(data, "accept-lifetime ${ex: accept-lifetime/start} ${ex: accept-lifetime/expire}");
  return CONFD_OK;
}

static int get_send_lifetime(cb_data_t *data, int set) {
  if (set)
    KEY_COMMAND(data, "send-lifetime ${ex: send-lifetime/start} ${ex: send-lifetime/expire}");
  return CONFD_OK;
}
