
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <confd.h>
#include <confd_maapi.h>
#include <confd_cdb.h>
#include <confd_logsyms.h>

#undef _POSIX_C_SOURCE

#include "logs.h"
#include "main.h"

#include "includes.h"
#include "zconfd_api.h"

#define CDB_CLOSE(sock) do { \
        if ((sock) >= 0) {   \
            cdb_close(sock); \
            (sock) = -1;     \
        }                    \
    } while(0)

struct subscription {
    struct subscription *next;
    int spoint;
    int kp_depth;
    subscr_cb_t *cb_root, *cb_tree;
    void *priv;
};

struct zconfd_data {
    struct lib_globals *zg;
    int kp_off;
    char dname[64];
    int debuglevel;
    FILE *estream;
    struct confd_daemon_ctx *dctx;
    struct sockaddr_in addr;
    int connected;
    int ctlsock, workersock, maapisock;
    int subsock, readsock, oldsock, notifsock;
    int in_phase0;
    int restarting;
    int num_subs;
    struct subscription *subs;
    void *ctl_read, *worker_read, *sub_read, *notif_read;
    init_cb_t *init;
    setup_cb_t *setup;
};

struct iter_state {
    struct confd_trans_ctx *tctx;
    struct zconfd_data *zd;
};

struct sub_iter_state {
    struct zconfd_data *zd;
    struct subscription *p;
};

#define MK_PATH(elem) "/" #elem
#define XMK_PATH(ROOT) MK_PATH(ROOT)
#define ROOT_PATH XMK_PATH(ROOT)
#define ROOT_OFFSET 1
struct confd_cs_node *zconfd_root;
int zconfd_root_length;

void init_root_path(struct zconfd_data *zd)
{
    zconfd_root_length = zd->kp_off = ROOT_OFFSET;
    zconfd_root = confd_find_cs_root(NAMESPACE);
    while (zconfd_root->tag != NSPREF(ROOT))
      zconfd_root = zconfd_root->next;
}

int zconfd_init_kpath(confd_hkeypath_t *kp, int depth, int dir)
{
    kp->len = ROOT_OFFSET;
    CONFD_SET_NOEXISTS(&kp->v[depth + 1][0]);
    CONFD_SET_XMLTAG(&kp->v[depth][0], NSPREF(ROOT), NAMESPACE);
    CONFD_SET_NOEXISTS(&kp->v[depth][1]);
    return ROOT_OFFSET;
}

static int zconfd_start(struct zconfd_data *zd);
static int zconfd_restart(struct zconfd_data *zd, char *fmt, ...);
static int do_setup(struct zconfd_data *zd);

static int startup_thread(void *thread_data);
static int ctlsock_read(void *thread_data, int sock);
static int workersock_read(void *thread_data, int sock);
static int subsock_read(void *thread_data, int sock);
static int notifsock_read(void *thread_data, int sock);

static int get_ctlsock(struct zconfd_data *zd);
static int get_workersock(struct zconfd_data *zd);
static int get_maapisock(struct zconfd_data *zd);
static int get_readsock(struct zconfd_data *zd);
static int get_subsock(struct zconfd_data *zd);
static int get_notifsock(struct zconfd_data *zd);

static int init_validation(struct confd_trans_ctx *tctx);
static int stop_validation(struct confd_trans_ctx *tctx);
static int init_data(struct confd_trans_ctx *tctx);

static int start_read_session(int sock);
static int start_read_old_session(int sock);
static void call_subscriber(struct zconfd_data *zd, int spoint);
static void clear_subscriptions(struct zconfd_data *zd);

static void get_rootpath(struct zconfd_data *zd, confd_hkeypath_t *kp,
                         char *path, int sz);

static void zconfd_init_cb_data_zcd(cb_data_t *data, struct zconfd_data *zd,
                                    confd_hkeypath_t *kp, enum cdb_iter_op op,
                                    confd_value_t *oldv, confd_value_t *newv, void *priv);

void zconfd_init(char *module,
                 enum confd_debug_level debuglevel,
                 init_cb_t *init, setup_cb_t *setup)
{
    struct zconfd_data *zd;
    char *port;

    zd = malloc(sizeof(struct zconfd_data));
    memset(zd, 0, sizeof(struct zconfd_data));
    snprintf(zd->dname, sizeof(zd->dname), "%s", module);
    zd->debuglevel = debuglevel;
    zd->estream = stderr;
    zd->init = init;
    zd->setup = setup;
/*     if (debuglevel >= CONFD_TRACE) */
/*      set_zlog_stdout(zg); */
    confd_init(zd->dname, zd->estream, zd->debuglevel);
    zd->addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    zd->addr.sin_family = AF_INET;
    if ((port = getenv("CONFD_IPC_PORT")) != NULL)
        zd->addr.sin_port = htons(atoi(port));
    else
        zd->addr.sin_port = htons(CONFD_PORT);

    if (zconfd_start(zd) != 0)
        ERROR_LOG( "Failed to connect to ConfD, retrying");
}

static int zconfd_start(struct zconfd_data *zd)
{
    struct confd_trans_validate_cbs vcb;
    struct confd_trans_cbs tcb;
    struct cdb_phase cdb_phase;
    int spoint;

    if (zd->connected) {
      //dpe:
        thread_cancel(zd->ctl_read);
        thread_cancel(zd->worker_read);
        thread_cancel(zd->notif_read);
        close(zd->ctlsock);
        close(zd->workersock);
        maapi_close(zd->maapisock);
        close(zd->notifsock);
        CDB_CLOSE(zd->readsock);
        CDB_CLOSE(zd->oldsock);
        if (!zd->subsock >= 0)
            thread_cancel(zd->sub_read);
        CDB_CLOSE(zd->subsock);
        zd->connected = 0;
        confd_release_daemon(zd->dctx);
    }

    if ((zd->dctx =
         confd_init_daemon(zd->dname)) == NULL)
        ERROR_LOG( "Failed to initialize ConfD");

    do {
        if ((zd->ctlsock = get_ctlsock(zd)) >= 0) {
            if ((zd->workersock = get_workersock(zd)) >= 0) {
                if ((zd->maapisock = get_maapisock(zd)) >= 0) {
                    if ((zd->readsock = get_readsock(zd)) >= 0) {
                        if ((zd->oldsock = get_readsock(zd)) >= 0) {
                            if ((zd->subsock = get_subsock(zd)) >= 0) {
                                if ((zd->notifsock = get_notifsock(zd)) >= 0) {
                                    zd->connected = 1;
                                    break;
                                }
                                CDB_CLOSE(zd->subsock);
                            }
                            CDB_CLOSE(zd->oldsock);
                        }
                        CDB_CLOSE(zd->readsock);
                    }
                    maapi_close(zd->maapisock);
                }
                close(zd->workersock);
            }
            close(zd->ctlsock);
        }
    } while (0);

    if (zd->connected &&
        confd_load_schemas((struct sockaddr *)&zd->addr,
                           sizeof(struct sockaddr_in)) == CONFD_OK) {
        init_root_path(zd);
    } else {
        zd->connected = 0;
    }

    if (!zd->connected) {
      // dpe: TODO replace in our main loop
        confd_release_daemon(zd->dctx);
        thread_add_timer(startup_thread, zd, 5);
        return -1;
    }

    // dpe: TODO replace machanizm by our own loop
    zd->dctx->d_opaque = zd;
    zd->ctl_read = thread_add_read(ctlsock_read, zd, zd->ctlsock);
    zd->worker_read = thread_add_read(workersock_read, zd, zd->workersock);
    zd->sub_read = thread_add_read(subsock_read, zd, zd->subsock);
    zd->notif_read = thread_add_read(notifsock_read, zd, zd->notifsock);

    memset(&vcb, 0, sizeof(vcb));
    vcb.init = init_validation;
    vcb.stop = stop_validation;
    confd_register_trans_validate_cb(zd->dctx, &vcb);

    memset(&tcb, 0, sizeof(tcb));
    tcb.init = init_data;
    tcb.finish = NULL;
    confd_register_trans_cb(zd->dctx, &tcb);

    /* Check for phase 0 */
    /* - final version should probably always be started in phase 0 */
    if ((cdb_get_phase(zd->readsock, &cdb_phase) == CONFD_OK) &&
        (cdb_phase.phase == 0)) {
        zd->in_phase0 = 1;
        thread_cancel(zd->sub_read);
        CDB_CLOSE(zd->readsock);
        CDB_CLOSE(zd->oldsock);
        CDB_CLOSE(zd->subsock);
    } else {
        zd->in_phase0 = 0;
    }
    zd->restarting = 0;

    if (zd->init(zd->dctx) != CONFD_OK)
        /* Not much point in restarting - maybe config change can fix it */
        ERROR_LOG( "Error in initialization");

    if (!zd->in_phase0)
        (void)do_setup(zd);

    if (confd_register_done(zd->dctx) != CONFD_OK)
        return zconfd_restart(zd, "Failed to complete registration \n");

    return 0;
}

static int do_setup(struct zconfd_data *zd)
{
        if (start_read_session(zd->readsock) != 0)
                return zconfd_restart(zd, "Failed to start read session");
        clear_subscriptions(zd);
        /* XXX should iterate over vr's if HAVE_VR - but
        ignore subscribe requests after first iteration... */
        //    vr = ipi_vr_get_privileged(zd->zg);
        cdb_cd(zd->readsock, ROOT_PATH);
        if ( zd->setup(zd->dctx) != CONFD_OK)
                /* Not much point in restarting - maybe config change can fix it */
                ERROR_LOG( "Error in initial setup");
        /* XXX iterate end */

        extern int quagga_not_running;
        if (!quagga_not_running) {
            if (cdb_subscribe_done(zd->subsock) != CONFD_OK)
                zconfd_restart(zd, "Failed to complete subscriptions");
        }

        cdb_end_session(zd->readsock);

        return 0;
}

static int zconfd_restart(struct zconfd_data *zd, char *fmt, ...)
{
    va_list ap;
    char buf[BUFSIZ];

    if (!zd->restarting) {
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);
#ifdef DO_AUTO_RESTART  /* we don't want this for examples/demo usage */
        ERROR_LOG( "%s - restarting", buf);
        thread_add_timer(startup_thread, zd, 0);
        zd->restarting = 1;
#else
        ERROR_LOG( "%s - exiting", buf);
        exit(0);
#endif
    }
    return CONFD_ERR;
}


static int startup_thread(void *thread_data )
{
  struct zconfd_data *zd = thread_data;

    return zconfd_start(zd);
}

static int ctlsock_read(void *thread_data, int sock)
{
    struct zconfd_data *zd = thread_data;
    int ret;

    zd->ctl_read = thread_add_read(ctlsock_read, zd, zd->ctlsock);
    if ((ret = confd_fd_ready(zd->dctx, sock)) == CONFD_EOF) {
        return zconfd_restart(zd, "ConfD control socket closed");
    } else if (ret == CONFD_ERR && confd_errno != CONFD_ERR_EXTERNAL) {
        return zconfd_restart(zd, "Error on ConfD control socket request");
    }
    return 0;
}

static int workersock_read(void *thread_data, int sock)
{
    struct zconfd_data *zd = thread_data;
    int ret;

    zd->worker_read = thread_add_read(workersock_read, zd, zd->workersock);
    if ((ret = confd_fd_ready(zd->dctx, sock)) == CONFD_EOF) {
        return zconfd_restart(zd, "ConfD worker socket closed");
    } else if (ret == CONFD_ERR && confd_errno != CONFD_ERR_EXTERNAL) {
        return zconfd_restart(zd, "Error on ConfD worker socket request");
    }
    return 0;
}

static int subsock_read(void *thread_data, int sock)
{
        struct zconfd_data *zd = thread_data;

        zd->sub_read = thread_add_read(subsock_read, zd, zd->subsock);
        {
                int spoint[zd->num_subs];
                int num_spoints, i;

                subscriptions_command(1);

                if (cdb_read_subscription_socket(zd->subsock, spoint,
                        &num_spoints) != CONFD_OK)
                        return zconfd_restart(zd, "Failed to read ConfD subscription");
                if (num_spoints > 0) {
                        if (start_read_session(zd->readsock) != 0)
                                return zconfd_restart(zd, "Failed to start read session");
                        if (start_read_old_session(zd->oldsock) != 0)
                                return zconfd_restart(zd, "Failed to start read session");
                        for (i = 0; i < num_spoints; i++)
                                call_subscriber(zd, spoint[i]);
                        cdb_end_session(zd->oldsock);
                        cdb_end_session(zd->readsock);
                        if (cdb_sync_subscription_socket(zd->subsock, CDB_DONE_PRIORITY) !=
                                CONFD_OK)
                                return zconfd_restart(zd,"Failed to sync subscription socket");
                }
                subscriptions_command(0);
        }
        return 0;
}

static int notifsock_read(void *thread_data, int sock)
{
    struct zconfd_data *zd = thread_data;
    struct confd_notification notif;

    zd->notif_read = thread_add_read(notifsock_read, zd, zd->notifsock);
    if (confd_read_notification(zd->notifsock, &notif) != CONFD_OK)
        return zconfd_restart(zd, "Failed to read ConfD notification");
    if (zd->in_phase0 && notif.type == CONFD_NOTIF_SYSLOG &&
               notif.n.syslog.logno == CONFD_PHASE1_STARTED) {
        zd->in_phase0 = 0;
        if ((zd->readsock = get_readsock(zd)) < 0 ||
            (zd->oldsock = get_readsock(zd)) < 0 ||
            (zd->subsock = get_subsock(zd)) < 0)
            return zconfd_restart(zd, "Failed to connect to ConfD");
        zd->sub_read = thread_add_read(subsock_read, zd, zd->subsock);
        return do_setup(zd);
    }

    return 0;
}

static enum cdb_iter_ret subscr_iter(confd_hkeypath_t *kp,
                                     enum cdb_iter_op op,
                                     confd_value_t *oldv,
                                     confd_value_t *newv,
                                     void *state)
{
    struct sub_iter_state *st = state;
    struct subscription *p = st->p;
    struct zconfd_data *zd = st->zd;
    cb_data_t cb_data;
    subscr_cb_t *cb;
    enum cdb_iter_ret rv;
    int do_root;

    zconfd_init_cb_data_zcd(&cb_data, zd, kp, op, oldv, newv, p->priv);
    cdb_cd(zd->readsock, ROOT_PATH);
    cdb_cd(zd->oldsock, ROOT_PATH);
    do_root = cb_data.kp_start == p->kp_depth - 1;
    cb = do_root ? p->cb_root : p->cb_tree;
    rv = cb == NULL ? (do_root ? ITER_RECURSE : ITER_CONTINUE) : cb(&cb_data);
    if (rv == CONFD_ERR) {
      ERROR_LOG("callback failed - stop");
      rv = ITER_STOP;
    }
    return rv;
}

static int get_ctlsock(struct zconfd_data *zd)
{
    int sock;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        zconfd_fail( "Failed to create control socket");
    }
    if (confd_connect(zd->dctx, sock, CONTROL_SOCKET,
                      (struct sockaddr *)&zd->addr,
                      sizeof(struct sockaddr_in)) != CONFD_OK) {
        close(sock);
        return -1;
    }
    return sock;
}

static int get_workersock(struct zconfd_data *zd)
{
    int sock;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        zconfd_fail( "Failed to create worker socket");
    }
    if (confd_connect(zd->dctx, sock, WORKER_SOCKET,
                      (struct sockaddr *)&zd->addr,
                      sizeof(struct sockaddr_in)) != CONFD_OK) {
        ERROR_LOG( "Failed to connect worker socket");
        close(sock);
        return -1;
    }
    return sock;
}

static int get_maapisock(struct zconfd_data *zd)
{
    int sock;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        zconfd_fail("Failed to create maapi socket");
    }
    if (maapi_connect(sock,
                      (struct sockaddr *)&zd->addr,
                      sizeof(struct sockaddr_in)) != CONFD_OK) {
        ERROR_LOG( "Failed to connect maapi socket");
        close(sock);
        return -1;
    }
    return sock;
}

static int get_readsock(struct zconfd_data *zd)
{
    int sock;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        zconfd_fail("Failed to create read socket");
    }
    if (cdb_connect(sock, CDB_READ_SOCKET,
                    (struct sockaddr *)&zd->addr,
                    sizeof(struct sockaddr_in)) != CONFD_OK) {
        ERROR_LOG( "Failed to connect read socket");
        close(sock);
        return -1;
    }
    return sock;
}

static int get_subsock(struct zconfd_data *zd)
{
    int sock;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        zconfd_fail("Failed to create subscription socket");
    if (cdb_connect(sock, CDB_SUBSCRIPTION_SOCKET,
                    (struct sockaddr *)&zd->addr,
                    sizeof(struct sockaddr_in)) != CONFD_OK) {
        ERROR_LOG( "Failed to connect subscription socket");
        close(sock);
        return -1;
    }
    return sock;
}

static int get_notifsock(struct zconfd_data *zd)
{
    int sock;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        zconfd_fail("Failed to create notifications socket");
    if (confd_notifications_connect(sock,
                                    (struct sockaddr *)&zd->addr,
                                    sizeof(struct sockaddr_in),
                                    CONFD_NOTIF_SYSLOG)
        != CONFD_OK) {
        ERROR_LOG( "Failed to connect notifications socket");
        close(sock);
        return -1;
    }
    return sock;
}


static int init_validation(struct confd_trans_ctx *tctx)
{
    struct zconfd_data *zd = tctx->dx->d_opaque;

    confd_trans_set_fd(tctx, zd->workersock);
    maapi_attach(zd->maapisock, NAMESPACE, tctx);
    tctx->t_opaque = (void *)1;
    return CONFD_OK;
}

static int stop_validation(struct confd_trans_ctx *tctx)
{
    if ((int)(tctx->t_opaque) == 2) { /* read session was started */
        struct zconfd_data *zd = tctx->dx->d_opaque;

        cdb_end_session(zd->readsock);
        tctx->t_opaque = (void *)1;
    }
    return CONFD_OK;
}

static int init_data(struct confd_trans_ctx *tctx)
{
    struct zconfd_data *zd = tctx->dx->d_opaque;

    confd_trans_set_fd(tctx, zd->workersock);
    maapi_attach(zd->maapisock, NAMESPACE, tctx);
    tctx->t_opaque = (void *)0;
    return CONFD_OK;
}

static int start_read_any_session(int sock, enum cdb_db_type type)
{
    int started = 0;

    while (!started) {
        DEBUG_LOG("starting read session for sock %i, type %i", sock, type);
        if (cdb_start_session(sock, type) != CONFD_OK) {
            if (confd_errno != CONFD_ERR_LOCKED) {
                return -1;
            } else {
                sleep(1);
            }
        } else {
            started = 1;
        }
    }
    if (cdb_set_namespace(sock, NAMESPACE) != CONFD_OK)
        return -1;
    return 0;
}

static int start_read_session(int sock)
{
    return start_read_any_session(sock, CDB_RUNNING);
}

static int start_read_old_session(int sock)
{
    return start_read_any_session(sock, CDB_PRE_COMMIT_RUNNING);
}

static void call_subscriber(struct zconfd_data *zd, int spoint)
{
    struct subscription *p = zd->subs;
    struct sub_iter_state st;
    st.zd = zd;

    while (p != NULL) {
        if (p->spoint == spoint) {
            int rv;
            DEBUG_LOG("calling subscriber for spoint %i", spoint);
            st.p = p;
            if ((rv = cdb_diff_iterate(zd->subsock, spoint, subscr_iter, ITER_WANT_PREV, &st)) != CONFD_OK)
              ERROR_LOG("cdb_diff_iterate failed with rv %i for point %i", rv, spoint);
            return;
        }
        p = p->next;
    }
}

static void clear_subscriptions(struct zconfd_data *zd)
{
    struct subscription *p = zd->subs, *np;

    while (p != NULL) {
        np = p->next;
        free(p);
        p = np;
    }
    zd->subs = NULL;
    zd->num_subs = 0;
}

static void get_rootpath(struct zconfd_data *zd, confd_hkeypath_t *kp,
                         char *path, int sz)
{
    confd_hkeypath_t root_kp;
    int i;

    for (i = 0; i < zd->kp_off; i++) {
        root_kp.v[i][0] = kp->v[kp->len - zd->kp_off + i][0];
        CONFD_SET_NOEXISTS(&root_kp.v[i][1]);
    }
    root_kp.len = zd->kp_off;
    confd_pp_kpath(path, sz, &root_kp);
}

static void zconfd_init_cb_data_zcd(cb_data_t *data, struct zconfd_data *zd, confd_hkeypath_t *kp, enum cdb_iter_op op, confd_value_t *oldv, confd_value_t *newv, void *priv)
{
    data->newsock = data->datasock = zd->readsock;
    data->oldsock = zd->oldsock;
    data->kp = kp;
    data->oldv = oldv;
    data->newv = newv;
    data->op = op;
    data->priv = priv;
    data->kp_start = kp == NULL ? 0 : kp->len - ROOT_OFFSET - 1;
}

/**************** Exported functions ****************/

const char *zconfd_extract_elem_name(const char *path, int *len)
{
  const char *sp = strchr(path, '/'),
    *bp = strpbrk(path, "[{");

  if (bp == NULL && sp == NULL)
    *len = strlen(path);
  else
    *len = (sp == NULL ? bp : (bp == NULL || sp < bp ? sp : bp)) - path;
  return sp == NULL || *++sp == 0 ? NULL : sp;
}

struct confd_cs_node *zconfd_locate_cs_node(struct confd_cs_node *node, const char *path)
{
  const char *sp = path, *np;
  int len;
  struct confd_cs_node *child;
  while (sp != NULL && sp[0] != 0) {
    np = zconfd_extract_elem_name(sp, &len);
    for (child = node->children; child != NULL; child = child->next) {
      const char *child_name = confd_hash2str(child->tag);
      if (strlen(child_name) == len && strncmp(child_name, sp, len) == 0)
        break;
    }
    if (child == NULL) {
      ERROR_LOG("Cannot find %s in path %s", sp, path);
      return NULL;
    }
    node = child;
    sp = np;
  }
  return node;
}

int zconfd_node_depth(const struct confd_cs_node *root, const struct confd_cs_node *node)
{
  int depth;
  for (depth = 0; node != root; node = node->parent, depth++)
    if (node->info.flags & CS_NODE_IS_DYN)
      depth++;
  return depth;
}

void zconfd_subscribe_cdb_pair(struct confd_daemon_ctx *dctx,
                               int prio, subscr_cb_t *cb_root, subscr_cb_t *cb_tree, const char *path, void *priv)
{
    struct zconfd_data *zd = dctx->d_opaque;
    struct subscription *p;
    int ret;

    struct confd_cs_node *node = zconfd_locate_cs_node(zconfd_root, path);
    if (node == NULL)
        zconfd_fail("Unable to locate the subtree root for %s", path);
    p = malloc(sizeof(struct subscription));
    memset(p, 0, sizeof(struct subscription));
    if ((ret = cdb_subscribe(zd->subsock, prio, NAMESPACE, &p->spoint,
                             ROOT_PATH "/%s", path)) != CONFD_OK) {
        if (ret == CONFD_EOF) {
            /* Lost connection - will be handled in poll loop */
            ERROR_LOG( "Failed to do initial subscription - restarting");
        } else {
            zconfd_fail("Failed to subscribe to %s", path);
        }
    }
    p->cb_root = cb_root;
    p->cb_tree = cb_tree;
    p->kp_depth = zconfd_node_depth(zconfd_root, node);
    p->priv = priv;
    p->next = zd->subs;
    zd->subs = p;
    zd->num_subs++;
}

void zconfd_subscribe_cdb(struct confd_daemon_ctx *dctx,
                          int prio, subscr_cb_t *cb, const char *path, void *priv)
{
    zconfd_subscribe_cdb_pair(dctx, prio, cb, cb, path, priv);
}

void zconfd_subscribe_cdb_tree(struct confd_daemon_ctx *dctx,
                          int prio, subscr_cb_t *cb_tree, const char *path, void *priv)
{
    zconfd_subscribe_cdb_pair(dctx, prio, NULL, cb_tree, path, priv);
}

void zconfd_subscribe_cdb_root(struct confd_daemon_ctx *dctx,
                          int prio, subscr_cb_t *cb_root, const char *path, void *priv)
{
    zconfd_subscribe_cdb_pair(dctx, prio, cb_root, NULL, path, priv);
}

static int zconfd_action_init(struct confd_user_info *uinfo)
{
  struct zconfd_data *zd = uinfo->actx.dx->d_opaque;
  confd_action_set_fd(uinfo, zd->workersock);
  return CONFD_OK;
}

void zconfd_register_action(struct confd_daemon_ctx *dctx, action_cb_t *cb, char *actionpoint)
{
  struct confd_action_cbs cbs;
  strncpy(cbs.actionpoint, actionpoint, MAX_CALLPOINT_LEN);
  cbs.init = zconfd_action_init;
  cbs.action = cb;
  confd_register_action_cbs(dctx, &cbs);
}

int zconfd_trans_get(struct confd_trans_ctx *tctx, confd_hkeypath_t *kp,
                      int *maapisockp, int *kp_startp, int *readsockp
                     /*struct lib_globals **zgp, struct ipi_vr **vrp*/)
{
    struct zconfd_data *zd = tctx->dx->d_opaque;
    char path[BUFSIZ];

    if (maapisockp != NULL || readsockp != NULL)
        get_rootpath(zd, kp, path, sizeof(path));
    if (maapisockp != NULL) {
        *maapisockp = zd->maapisock;
        if (maapi_cd(zd->maapisock, tctx->thandle, path) != CONFD_OK)
            return zconfd_restart(zd, "Failed MAAPI cd to root: %s", path);
    }
    if (kp_startp != NULL)
        *kp_startp = kp->len - 1 - zd->kp_off;
    if (readsockp != NULL) {
        if (zd->readsock == -1 || /* not available in phase 0 */
            (int)(tctx->t_opaque) == 0) { /* not a validation transaction */
            *readsockp = -1;
        } else {
            *readsockp = zd->readsock;
            if ((int)(tctx->t_opaque) == 1) {
                if (start_read_session(zd->readsock) != 0)
                    return zconfd_restart(zd, "Failed to start read session");
                tctx->t_opaque = (void *)2;
            }
            if (cdb_cd(zd->readsock, path) != CONFD_OK)
                return zconfd_restart(zd, "Failed CDB cd to root: %s", path);
        }
    }
    /*    if (zgp != NULL)
        *zgp = zd->zg;
        if (vrp != NULL)
        *vrp = get_vr(zd, kp);

        */
    return CONFD_OK;
}

void zconfd_fail(char *fmt, ...)
{
    va_list ap;
    char buf[BUFSIZ];

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    ERROR_LOG( "%s, exiting", buf);
    va_end(ap);
    exit(1);
}

void *zconfd_malloc(size_t size)
{
        void *ret;
        if ((ret = malloc(size)) == NULL){
                ERROR_LOG( "Failed to allocate %d bytes", size);
                exit(1);
        }
        return ret;
}

extern void confd_set_errno(int ecode);

confd_hkeypath_t *zconfd_flatten_data_path(confd_hkeypath_t *new_kp, cb_data_t *data, int kp_ix)
{
    confd_hkeypath_t *kp = data->kp;
    int i;
    if (kp_ix <= 0)
        /* nothing to do - I am not going to copy this needlessly */
        return kp;
    for (i = kp_ix; i < kp->len; i++) {
        int j;
        for (j = 0; kp->v[i][j].type != C_NOEXISTS; j++)
            new_kp->v[i - kp_ix][j] = kp->v[i][j];
        CONFD_SET_NOEXISTS(&new_kp->v[i - kp_ix][j]);
    }
    new_kp->len = kp->len - kp_ix;
    data->kp = new_kp;
    data->kp_start -= kp_ix;
    return kp;
}

void zconfd_recover_data_path(cb_data_t *data, confd_hkeypath_t *kp)
{
    data->kp_start += kp->len - data->kp->len;
    data->kp = kp;
}

void zconfd_init_cb_data_ctx(cb_data_t *data, struct confd_daemon_ctx *dctx, confd_hkeypath_t *kp, enum cdb_iter_op op, confd_value_t *oldv, confd_value_t *newv, void *priv)
{
  return zconfd_init_cb_data_zcd(data, (struct zconfd_data *)dctx->d_opaque, kp, op, oldv, newv, priv);
}
