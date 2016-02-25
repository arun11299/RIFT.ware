#include <stdlib.h>
#include "confd_global.h"

#include "zconfd_subs.h"
#include "quagga.h"     /* generated from yang */

int cli_connect(enum cDaemons daemon)
{
  //initialize CLI
  if (cliOpen(&g_clients[daemon], getCliHost(daemon), getCliPort(daemon), getCliTimeout(daemon)) != CLI_SUCC) {
    ERROR_LOG("cannot connect to daemon %s:%d", getCliHost(daemon), getCliPort(daemon));
    return CONFD_ERR;
  }

  cli_command_raw(daemon, NULL, ".*Password: ", 0, NULL, NULL, NULL);

  if (cli_command_raw(daemon, getCliPassword(daemon), ".*> ", 0, NULL, NULL, NULL) < 0){
    ERROR_LOG("daemon password set error to %s:%d", getCliHost(daemon), getCliPort(daemon));
    cliClose(&g_clients[daemon]);
    return CONFD_ERR;
  }

  cli_command_raw(daemon, "enable", ".*# ", 0, NULL, NULL, NULL);

  return CONFD_OK;
}


typedef enum { NODE_MANDATORY, NODE_OPTIONAL, NODE_DYNAMIC } leaf_type_t;

/**
 * Description of element handling.
 */
typedef struct {
  struct confd_cs_node *cs_node;        ///< element's cs_node
  const char *element_path;             ///< path from subscription root to the element
  get_func_t *handler;                  ///< element's get-function
  int length;                           ///< length of the path from description root
  int reset;                            ///< true if the element should be deleted before changing its value
  void *priv;                           ///< user callback data
} leaf_node_t;

#define NODE_TYPE(sub) CS_NODE_TYPE((sub)->cs_node)
#define CS_NODE_TYPE(node) ((node)->info.maxOccurs == 1 ? \
                            ((node)->info.minOccurs == 1 ? NODE_MANDATORY : NODE_OPTIONAL) : \
                            NODE_DYNAMIC)

typedef struct leaf_list {
  leaf_node_t node;
  struct leaf_list *next;
} leaf_list_t;

/**
 * Group of element handlers within one subtree that is suitable to be handled with one subscription.
 */
typedef struct {
  subscription_init_t *subscr_init;     ///< Callback called whenever subscription begins/ends to be processed; ignored if NULL
  const char *base_path;                ///< Path to the subscribed subtree
  struct confd_cs_node *cs_node;        ///< Node corresponding to the subscription root
  int depth;                            ///< Distance (1 for each static, 2 for each dynamic node on the path) from root to the subtree
  leaf_list_t *list;                    ///< List of element handling data
} subscr_desc_t;

typedef int (get_handler_t)(cb_data_t *data, const char *elem_path);

/**
 * Structure used while performing initial `get' sequence.
 */
typedef struct {
  subscr_desc_t *description;
  leaf_node_t *element;
  const char *level_path;
  const struct confd_cs_node *level_node;
  get_handler_t *tail_handler;
  confd_hkeypath_t *rev_kp;
} get_desc_t;

/**
 * Description of private fields for single-element handlers.
 */
typedef struct {
  get_func_t *handler;
  struct confd_cs_node *node;
  int depth;
  void *priv;
} single_priv_t;

static subscr_cb_t update_single_element;
static subscr_cb_t update_elements;

static get_handler_t descend_path;
static get_handler_t get_elements;

handler_priv_t *make_std_priv(enum cDaemons daemon, const char *prompt, const char *yes_str, const char *no_str)
{
  handler_priv_t *p = malloc(sizeof(handler_priv_t));
  p->daemon = daemon;
  p->prompt = prompt;
  p->yes_command = yes_str;
  p->no_command = no_str;
  return p;
}

/**
 * Create list of leaf_list instances according to given element descriptions.
 */
static leaf_list_t *process_elements(subscr_desc_t *desc, const element_desc_t elems[], struct confd_cs_node *desc_root)
{
  const element_desc_t *elem;
  leaf_list_t *list = NULL, *nl;
  for (elem = elems; elem->element_path != NULL && elem->element_path[0] != 0; elem++) {
    struct confd_cs_node *child = zconfd_locate_cs_node(desc_root, elem->element_path);
    if (child == NULL) {
      ERROR_LOG("Cannot find element %s in context %s", elem->element_path, desc->base_path);
      while (list != NULL) {
        nl = list->next;
        free(list);
        list = nl;
      }
      return NULL;
    }
    nl = malloc(sizeof(leaf_list_t));
    nl->next = list;
    nl->node.cs_node = child;
    nl->node.handler = elem->handler;
    nl->node.element_path = elem->element_path;
    nl->node.length = zconfd_node_depth(desc_root, child);
    if (CS_NODE_TYPE(child) == NODE_DYNAMIC)
      /* length is 1 too much because of keys */
      nl->node.length--;
    nl->node.reset = elem->reset;
    nl->node.priv = elem->priv;
    list = nl;
  }
  return list;
}

static subscr_desc_t *make_sub_description(subscription_init_t *init, const char *base_path, const element_desc_t elems[])
{
  subscr_desc_t *desc = malloc(sizeof(subscr_desc_t));
  desc->cs_node = zconfd_locate_cs_node(zconfd_root, base_path);
  if (desc->cs_node == NULL)
    return NULL;
  desc->depth = zconfd_node_depth(zconfd_root, desc->cs_node) + zconfd_root_length - 1;
  desc->subscr_init = init;
  desc->base_path = base_path;
  desc->list = process_elements(desc, elems, desc->cs_node);
  return desc;
}

int subscribe_single_element(struct confd_daemon_ctx *dctx, int priority, const char *path, get_func_t *handler, void *priv)
{
  struct confd_cs_node *node = zconfd_locate_cs_node(zconfd_root, path);
  single_priv_t *spriv;
  if (node == NULL)
    return CONFD_ERR;
  spriv = malloc(sizeof(single_priv_t));
  spriv->handler = handler;
  spriv->priv = priv;
  spriv->node = node;
  spriv->depth = zconfd_node_depth(zconfd_root, node) + zconfd_root_length - 1;
  if (CS_NODE_TYPE(node) == NODE_DYNAMIC)
    spriv->depth--;
  zconfd_subscribe_cdb(dctx, priority, update_single_element, path, spriv);
  return CONFD_OK;
}

int make_subscription(struct confd_daemon_ctx *dctx, enum cDaemons daemon, int priority, subscription_init_t *init, const char *base_path, const element_desc_t elems[])
{
  cb_data_t data;
  subscr_desc_t *desc = make_sub_description(init, base_path, elems);
  get_desc_t g_desc;
  confd_hkeypath_t kp;
  if (desc == NULL)
    return CONFD_ERR;
  zconfd_subscribe_cdb_tree(dctx, priority, update_elements, desc->base_path, desc);
  g_desc.description = desc;
  g_desc.level_path = base_path;
  g_desc.level_node = zconfd_root;
  g_desc.tail_handler = get_elements;
  zconfd_init_cb_data_ctx(&data, dctx, NULL, MOP_CREATED, NULL, NULL, &g_desc);
  g_desc.rev_kp = &kp;
  kp.len = zconfd_init_kpath(&kp, 0, -1);
  cli_configure_command(1, daemon);
  CHECK_OK(descend_path(&data, NULL));
  cli_configure_command(0, daemon);
  return CONFD_OK;
}

/**
 * Call the handler for one element. Creates keypath, calls init function and the handler.
 */
static int do_get_element(cb_data_t *data, const char *elem_path_ignored)
{
  confd_hkeypath_t kp;
  confd_value_t *v, *rv;
  get_desc_t *g_desc = data->priv;
  subscr_desc_t *desc = g_desc->description;
  int i;
  if (desc->subscr_init != NULL) {
    kp.len = g_desc->rev_kp->len - 1;
    if (NODE_TYPE(g_desc->element) == NODE_DYNAMIC)
      kp.len++;
    for (i = kp.len; i > 0; i--) {
      for (v = kp.v[i - 1], rv = g_desc->rev_kp->v[kp.len - i]; rv->type != C_NOEXISTS; v++, rv++)
        *v = *rv;
      CONFD_SET_NOEXISTS(v);
    }
    data->kp = &kp;
    data->kp_start = kp.len - zconfd_root_length - 1;
    DUMP_UPDATE(__FUNCTION__, data);
    desc->subscr_init(1, data);
  }
  data->priv = g_desc->element->priv;
  g_desc->element->handler(data, 1);
  data->priv = g_desc;
  if (desc->subscr_init != NULL) {
    data->kp = &kp;
    desc->subscr_init(0, data);
    data->kp = NULL;
  }

  return CONFD_OK;
}

/**
 * Process all elements below given subscription path.
 */
static int get_elements(cb_data_t *data, const char *elem_path)
{
  get_desc_t ndesc, *g_desc = data->priv;
  subscr_desc_t *desc = g_desc->description;
  memcpy(&ndesc, g_desc, sizeof(get_desc_t));
  ndesc.tail_handler = do_get_element;
  data->priv = &ndesc;
  leaf_list_t *ptr;
  for (ptr = desc->list; ptr != NULL; ptr = ptr->next) {
    leaf_node_t *sub = &ptr->node;
    ndesc.level_path = sub->element_path;
    ndesc.element = sub;
    CHECK_OK(descend_path(data, elem_path));
  }
  data->priv = g_desc;
  return CONFD_OK;
}

/**
 * Descend one level down the subscription path, for non-dynamic element.
 */
static int get_single_element(cb_data_t *data, int set)
{
  const get_desc_t *desc = (get_desc_t*) data->priv;
  const struct confd_cs_node *node = desc->level_node;
  confd_hkeypath_t *rev_kp = desc->rev_kp;
  CONFD_SET_XMLTAG(&rev_kp->v[rev_kp->len][0], node->tag, node->ns);
  CONFD_SET_NOEXISTS(&rev_kp->v[rev_kp->len][1]);
  rev_kp->len++;
  CHECK_OK(descend_path(data, confd_hash2str(node->tag)));
  rev_kp->len--;
  return CONFD_OK;
}

/**
 * Descend one level down the subscription path, for dynamic element.
 */
static int get_dynamic_element(cb_data_t *data, int set)
{
  get_desc_t *desc = (get_desc_t*) data->priv;
  const struct confd_cs_node *node = desc->level_node;
  confd_hkeypath_t *rev_kp = desc->rev_kp;
  confd_value_t *vptr;
  u_int32_t *key;
  CONFD_SET_XMLTAG(&rev_kp->v[rev_kp->len][0], node->tag, node->ns);
  CONFD_SET_NOEXISTS(&rev_kp->v[rev_kp->len][1]);
  for (key = node->info.keys, vptr = rev_kp->v[rev_kp->len + 1]; *key != 0; key++, vptr++)
    CHECK_OK(cdb_get(data->datasock, vptr, confd_hash2str(*key)));
  CONFD_SET_NOEXISTS(vptr);
  rev_kp->len += 2;
  CHECK_OK(descend_path(data, NULL));
  rev_kp->len -= 2;
  for (vptr = rev_kp->v[rev_kp->len + 1]; vptr->type != C_NOEXISTS; vptr++)
    confd_free_value(vptr);
  return CONFD_OK;
}

/**
 * Descend down the get_desc_t::level_path, call
 * get_desc_t::tail_handler if at the end.
 */
static int descend_path(cb_data_t *data, const char *elem_path)
{
  get_desc_t *desc = data->priv;

  if (desc->level_path[0] != 0) {
    struct confd_cs_node *child;
    const char *child_name;
    get_desc_t ndesc = *desc;
    int len;
    const char *sp;
    if (elem_path != NULL)
      cdb_pushd(data->datasock, elem_path);
    sp = zconfd_extract_elem_name(desc->level_path, &len);
    for (child = desc->level_node->children; child != NULL; child = child->next) {
      child_name = confd_hash2str(child->tag);
      if (strlen(child_name) == len && strncmp(child_name, desc->level_path, len) == 0)
        break;
    }
    if (child == NULL) {
      /* should not happen */
      ERROR_LOG("Could not find element %s at %s", desc->level_path, confd_hash2str(desc->level_node->tag));
      return CONFD_ERR;
    }
    ndesc.level_path = sp == NULL ? "" : sp;
    ndesc.level_node = child;
    data->priv = &ndesc;
    switch (CS_NODE_TYPE(child)) {
    case NODE_DYNAMIC:
      CHECK_OK(get_dynamic(data, child_name, get_dynamic_element));
      break;
    case NODE_OPTIONAL:
      CHECK_OK(test_single(data, child_name, get_single_element));
      break;
    case NODE_MANDATORY:
      CHECK_OK(get_single_element(data, 1));
      break;
    }
    data->priv = desc;
    if (elem_path != NULL)
      cdb_popd(data->datasock);
  } else {
    CHECK_OK(desc->tail_handler(data, elem_path));
  }

  return CONFD_OK;
}

enum cdb_iter_ret process_element(cb_data_t *data, leaf_node_t *sub, int tot_depth, subscription_init_t *init)
{
  int flatten;
  int ret = CONFD_OK;
  int set = data->op != MOP_DELETED;
  confd_hkeypath_t flat_kp;
  char path[KPATH_MAX];
  if (NODE_TYPE(sub) == NODE_DYNAMIC)
    if (tot_depth == data->kp->len - 1)
      return ITER_RECURSE;
    else {
      flatten = data->kp->len - tot_depth - 2;
      set = set || flatten > 0;
    }
  else {
    flatten = data->kp->len - tot_depth;
    set = set || flatten > 1;
  }
  if (flatten > 0) {
    zconfd_flatten_data_path(&flat_kp, data, flatten);
  }
  data->priv = sub->priv;
  if (init != NULL)
    init(1, data);
  confd_pp_kpath(path, KPATH_MAX, data->kp);
  if (! set || (data->op != MOP_CREATED && sub->reset)) {
    data->datasock = data->oldsock;
    CHECK_OK(cdb_pushd(data->datasock, path));
    ret = sub->handler(data, 0);
    cdb_popd(data->datasock);
    data->datasock = data->newsock;
  }
  if (set && ret != CONFD_ERR) {
    CHECK_OK(cdb_pushd(data->datasock, path));
    ret = sub->handler(data, 1);
    cdb_popd(data->datasock);
  }
  if (init != NULL)
    init(0, data);
  CHECK_OK(ret);
  return ITER_CONTINUE;
}

enum cdb_iter_ret update_single_element(cb_data_t *data)
{
  single_priv_t *spriv = data->priv;
  leaf_node_t sub;
  sub.priv = spriv->priv;
  sub.handler = spriv->handler;
  sub.reset = 0;
  sub.cs_node = spriv->node;
  return process_element(data, &sub, spriv->depth, NULL);
}

enum cdb_iter_ret update_elements(cb_data_t *data)
{
  leaf_list_t *ptr;
  subscr_desc_t *desc = data->priv;
  DUMP_UPDATE(__FUNCTION__, data);
  for (ptr = desc->list; ptr != NULL; ptr = ptr->next) {
    leaf_node_t *sub = &ptr->node;
    int tot_depth = desc->depth + sub->length;
    if (tot_depth >= data->kp->len)
      continue;
    confd_value_t *v = data->kp->v[data->kp->len - tot_depth - 1];
    if (v->type != C_XMLTAG)
      continue;
    if (v->val.xmltag.tag == sub->cs_node->tag)
      return process_element(data, sub, tot_depth, desc->subscr_init);
  }
  char path[KPATH_MAX];
  confd_pp_kpath(path, KPATH_MAX, data->kp);
  DEBUG_LOG("Could not find subscription for %s with %s (%i / %i) - trying to recur", path, desc->base_path, desc->depth, data->kp->len);
  return ITER_RECURSE;
}

int std_get_handler(cb_data_t *data, int set)
{
  handler_priv_t *p = data->priv;
  cli_command(p->daemon, data, p->prompt, 1, CLI_ERR_STR, "%s", set ? p->yes_command : p->no_command);
  return CONFD_OK;
}

int bool_get_handler(cb_data_t *data, int set)
{
  handler_priv_t *p = data->priv;
  cli_command(p->daemon, data, p->prompt, 1, CLI_ERR_STR, "%s", p->yes_command);
  return CONFD_OK;
}
