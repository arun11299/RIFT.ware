#ifndef _ZCONFD_SUBS_H_
#define _ZCONFD_SUBS_H_

#include "confd_global.h"

extern int cli_connect(enum cDaemons daemon);

/**
 * List of "leaf" element names.
 */
typedef struct {
  const char *element_path;     ///< Element name
  get_func_t *handler;  ///< Element change handler
  /**
   * "do-reset" indicator. If true, the handler is called twice if the element is being
   * changed - once to delete it, once to re-create it.
   */
  int reset;
  void *priv;
} element_desc_t;

/**
 * Function type called whenever a subscription begins/ends to be
 * processed (the argument start is set to 1 or 0 respectively).
 */
typedef void subscription_init_t(int start, cb_data_t *data);

typedef struct {
  enum cDaemons daemon;
  const char *prompt;
  const char *yes_command;
  const char *no_command;
} handler_priv_t;

extern handler_priv_t *make_std_priv(enum cDaemons daemon, const char *prompt, const char *yes_cmd, const char *no_cmd);
extern get_func_t std_get_handler;
extern get_func_t bool_get_handler;

#define BOOL_HANDLER(daemon, prompt, elem_path, command, res) {.element_path = elem_path, .handler = bool_get_handler, .reset = res, .priv = make_std_priv(daemon, prompt, "${bool: " elem_path ", '', 'no '}" command, "")}

#define STD_HANDLER(daemon, prompt, elem_path, yes_command, no_command, res) {.element_path = elem_path, .handler = std_get_handler, .reset = res, .priv = make_std_priv(daemon, prompt, yes_command, no_command)}
#define PARTS_HANDLER(daemon, prompt, elem_path, common_command_part, set_command_part, res) STD_HANDLER(daemon, prompt, elem_path, common_command_part set_command_part, "no " common_command_part, res)

/**
 * Subscribe to given subtree with given set of handlers.
 */
int make_subscription(struct confd_daemon_ctx *dctx, enum cDaemons daemon, int priority, subscription_init_t *init, const char *base_path, const element_desc_t elems[]);

/**
 * Subscribe to given element with given handler.
 */
int subscribe_single_element(struct confd_daemon_ctx *dctx, int priority, const char *path, get_func_t *handler, void *priv);

#endif
