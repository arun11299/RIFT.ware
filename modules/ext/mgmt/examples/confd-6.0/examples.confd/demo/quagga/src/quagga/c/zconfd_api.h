
#ifndef _ZCONFD_API_H
#define _ZCONFD_API_H 1


#include <confd.h>
#include <confd_cdb.h>
#include "logs.h"
#include "ns_prefix.h"

#define _CONCAT(a, b, c)   a ## b ## c
#define _NSCONCAT(ns, val) _CONCAT(ns, _, val)
#define NSPREF(val)        _NSCONCAT(NS_PREFIX, val)
#define NAMESPACE          NSPREF(_ns)

#define PASSWD_PRIO        2
#define ACCESS_PRIO        3
#define KEYCHAIN_PRIO      3
#define PLIST_PRIO         3
#define LOG_PRIO           3
#define RMAP_PRIO          4
#define PROTO_PRIO         5

#define PATH_STACK_LENGTH  5

typedef struct {
  int datasock;
  int newsock, oldsock;
  confd_hkeypath_t *kp;
  int kp_start;
  confd_value_t *oldv, *newv;
  enum cdb_iter_op op;
  void *priv;
} cb_data_t;

typedef int init_cb_t(struct confd_daemon_ctx *dctx );
typedef int setup_cb_t(struct confd_daemon_ctx *dctx);
typedef enum cdb_iter_ret subscr_cb_t(cb_data_t *cb_data);

extern void zconfd_init( char *module, enum confd_debug_level debuglevel,
                        init_cb_t *init, setup_cb_t *setup);

extern void zconfd_subscribe_cdb_pair(struct confd_daemon_ctx *dctx, int prio, subscr_cb_t *cb_root,
                                      subscr_cb_t *cb_tree, const char *path, void *priv);
extern void zconfd_subscribe_cdb_root(struct confd_daemon_ctx *dctx, int prio,
                                      subscr_cb_t *cb_root, const char *path, void *priv);
extern void zconfd_subscribe_cdb_tree(struct confd_daemon_ctx *dctx, int prio,
                                      subscr_cb_t *cb_tree, const char *path, void *priv);
extern void zconfd_subscribe_cdb(struct confd_daemon_ctx *dctx, int prio,
                                 subscr_cb_t *cb, const char *path, void *priv);

/**
 * Function called before subscriptions begin to be processed. To be
 * implemented by the user.
 */
extern void subscriptions_command(int start);

typedef int action_cb_t(struct confd_user_info *uinfo, struct xml_tag *name,
                  confd_hkeypath_t *kp, confd_tag_value_t *params, int nparams);

extern void zconfd_register_action(struct confd_daemon_ctx *dctx, action_cb_t *cb, char *actionpoint);

/**
 * Distance (number of transitions) from confd_cs_node node to root.
 */
extern int zconfd_node_depth(const struct confd_cs_node *root, const struct confd_cs_node *node);

/**
 * Find the cs_node that corresponds to given tagpath relative to given root node.
 *
 * @return the node if found, NULL othervise
 */
extern struct confd_cs_node *zconfd_locate_cs_node(struct confd_cs_node *node, const char *path);

/**
 * Identify next element name in given path, return pointer to next
 * element in the path (ignoring {} and []).
 *
 * @param path
 * @param len output value - length of the element name
 */
extern const char *zconfd_extract_elem_name(const char *path, int *len);

extern void zconfd_init_cb_data_ctx(cb_data_t *data, struct confd_daemon_ctx *dctx, confd_hkeypath_t *kp,
                                    enum cdb_iter_op op, confd_value_t *oldv, confd_value_t *newv,
                                    void *priv);

extern int zconfd_trans_get(struct confd_trans_ctx *tctx, confd_hkeypath_t *kp,
                            int *maapisockp, int *kp_startp, int *readsockp);

extern void *zconfd_malloc( size_t size);

/**
 * Shorten the path data->kp by kp_ix elements, use new_kp as a storage. Will do nothing if kp_ix <= 0.
 *
 * @param new_kp storage for the new keypath, must be allocated
 * @param data its field cb_data_t::kp will point to new_kp, start_kp will be updated
 * @param kp_ix how many elements to drop
 * @return old value of data->kp (to be stored and used in zconfd_recover_data_path later, if necessary)
 */
extern confd_hkeypath_t *zconfd_flatten_data_path(confd_hkeypath_t *new_kp, cb_data_t *data, int kp_ix);
/**
 * Return flattened keypath to previous state.
 * @see zconfd_flatten_data_path
 */
extern void zconfd_recover_data_path(cb_data_t *data, confd_hkeypath_t *kp);

extern void zconfd_fail( char *fmt, ...);

extern struct confd_cs_node *zconfd_root;
extern int zconfd_root_length;
/**
 * Fill the keypath part corresponding to zconfd_root.
 *
 * @param kp
 * @param depth where to start filling (inclusive)
 * @param dir path direction: 1 for the standard direction (i.e. from
 * depth up), -1 for reverse direction (from depth down; depth is
 * usually 0 in this case)
 *
 * @return length of the root path
 */
extern int zconfd_init_kpath(confd_hkeypath_t *kp, int depth, int dir);

#define CHECK_CONFD(cond, fmt, ...) do {                                \
        int _ret = (cond);                                              \
        if (_ret < CONFD_OK) {                                          \
                ERROR_LOG(fmt ": %s (%d) command:'%s'", ##__VA_ARGS__,  \
                          confd_lasterr(), confd_errno, #cond);         \
            return _ret;                                                \
        }else{                                                          \
                DEBUG_LOG("confd:'%s' - OK", #cond);    \
        }} while (0)

#define CHECK_OK(cond) do {                                     \
        int _ret = (cond);                                      \
        if (_ret < CONFD_OK) {                                  \
            ERROR_LOG("error: %d command:'%s'", _ret, #cond);   \
            return _ret;                                        \
        }else{                                                          \
                DEBUG_LOG("'%s' - OK", #cond);  \
        }} while (0)

#define CHECK_OK1(cond) do {                                    \
        int _ret = (cond);                                      \
        if (_ret < CONFD_OK) {                                  \
            ERROR_LOG("error: %d command:'%s'", _ret, #cond);   \
            return _ret;                                        \
        }                                                       \
    } while (0)

#define VALIDATE_FAIL(...) do {                                         \
        confd_trans_seterr(tctx, __VA_ARGS__);                          \
        return CONFD_ERR;                                               \
    } while (0)

#define VALIDATE_WARN(...) \
    (confd_trans_seterr(tctx, __VA_ARGS__), CONFD_VALIDATION_WARN)

#endif /* _ZCONFD_API_H */
