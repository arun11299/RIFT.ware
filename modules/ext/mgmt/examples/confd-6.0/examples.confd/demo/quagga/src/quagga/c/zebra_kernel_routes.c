#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>

#include "confd.h"
#include "zconfd_api.h"
#include "confd_global.h"
#include "quagga.h"     /* generated from yang */

#define BUFLEN BUFSIZ
#define TOKCHARS " \n\t\r"
#define ROUTE_CMD "/sbin/route"
#define ELEMS_PER_ROUTE 9

#define ROUTE_FLAG(f) NSPREF(_bm_routeFlagsType_ ## f)

#define ACTION_ERROR(uinfo, fmt,...) return (ERROR_LOG(fmt , ##__VA_ARGS__), confd_action_seterr(uinfo, fmt , ##__VA_ARGS__), CONFD_ERR)

typedef struct {
  char buf[BUFLEN+1];
  char *ptr, *next, *end;
  int fd;
} reader_t;

enum { ROUTE_GATEWAY = 1 << 0, ROUTE_IFACE = 1 << 1, ROUTE_MASK = 1 << 2, ROUTE_METRIC = 1 << 3, ROUTE_FLAGS = 1 << 4 };

typedef struct {
  char destination[KPATH_MAX], gateway[KPATH_MAX], iface[KPATH_MAX];
  struct in_addr mask;
  u_int32_t flags;
  int32_t type;
  u_int16_t metric;
  int fields_present;
} route_t;

typedef struct route_list {
  route_t route;
  struct route_list *next;
} route_list_t;

void free_route_list(route_list_t *routes)
{
  route_list_t *n;
  for (;routes != NULL; routes = n) {
    n = routes->next;
    free(routes);
  }
}

static void init_reader(reader_t *r, int fd)
{
  r->fd = fd;
  // int n = read(fd, r->buf, BUFLEN)
  r->buf[BUFLEN] = 0;
  r->ptr = r->end = r->buf;
  r->next = NULL;
}

static char *get_next_line(reader_t *r)
{
  if (r->fd <= 0)
    return NULL;
  char* const sentinel = r->buf + BUFLEN;
  do {
    if (r->next == NULL) {
      if (sentinel == r->end) {
        int len = sentinel - r->ptr;
        if (len > 0)
          memmove(r->buf, r->ptr, len + 1); /* need to move the last NUL! */
        r->ptr = r->buf;
        r->end = r->buf + len;
      }
      int n = read(r->fd, r->end, sentinel - r->end);
      if (n <= 0) {
        if (r->end == r->ptr)
          return NULL;
        r->fd = -1;
        *r->end = 0;
        return r->ptr;
      }
      *(r->end = r->end + n) = 0;
    } else
      r->ptr = r->next + 1;
    r->next = strchr(r->ptr, '\n');
  } while (r->next == NULL);
  *r->next = 0;
  return r->ptr;
}

int read_routes(int fd, route_list_t **routes)
{
  reader_t reader;
  char *line;
  route_list_t **last = routes;
  route_t *route;
  int count = 0;
  init_reader(&reader, fd);
  if (get_next_line(&reader) == NULL || get_next_line(&reader) == NULL)
    return -1;
  while ((line = get_next_line(&reader)) != NULL) {
    const char *toks[8];
    int i;
    const char *flag;
    if ((toks[0] = strtok(line, TOKCHARS)) == NULL)
      return -1;
    for (i = 1; i < 8; i++)
      if ((toks[i] = strtok(NULL, TOKCHARS)) == NULL)
        return -1;
    *last = malloc(sizeof(route_list_t));
    route = &(*last)->route;
    last = &(*last)->next;
    strcpy(route->destination, toks[0]);
    strcpy(route->gateway, toks[1]);
    route->mask.s_addr = inet_addr(toks[2]);
    route->type = route->mask.s_addr == 0xffffffff ?
        NSPREF(_routeTargetType_host) : NSPREF(_routeTargetType_net);
    route->flags = 0;
    for (flag = toks[3]; *flag != 0; flag++)
      switch (*flag) {
      case 'U': route->flags |= ROUTE_FLAG(U); break;
      case 'H': route->flags |= ROUTE_FLAG(H); break;
      case 'G': route->flags |= ROUTE_FLAG(G); break;
      case 'R': route->flags |= ROUTE_FLAG(R); break;
      case 'D': route->flags |= ROUTE_FLAG(D); break;
      case 'M': route->flags |= ROUTE_FLAG(M); break;
      case 'A': route->flags |= ROUTE_FLAG(A); break;
      case 'C': route->flags |= ROUTE_FLAG(C); break;
      default: return -1;
      }
    route->metric = atoi(toks[4]);
    strcpy(route->iface, toks[7]);
    count++;
  }
  *last = NULL;
  return count;
}

void wait_process(int pid)
{
  int status;
  waitpid(pid, &status, 0);
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    ERROR_LOG("Waiting for child process failure: %i", status);
}

int open_route_pipe(int *pid, int *fd)
{
  int pipefd[2];
  char *argv[2];
  if (pipe(pipefd) != 0) {
    ERROR_LOG("Failed to create pipe for running command");
    return -1;
  }

  switch ((*pid = fork())) {
  case 0:                       /* child */
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
    dup2(pipefd[1], STDERR_FILENO);
    if (pipefd[1] != STDOUT_FILENO && pipefd[1] != STDERR_FILENO)
      close(pipefd[1]);
    argv[0] = ROUTE_CMD;
    argv[1] = NULL;
    execve(ROUTE_CMD, argv, NULL);
    perror(NULL);
    exit(127);
  case -1:                      /* failure */
    close(pipefd[1]);
    close(pipefd[0]);
    ERROR_LOG("Failed to fork");
    return -1;
  }
  close(pipefd[1]);
  *fd = pipefd[0];
  return 0;
}

static confd_tag_value_t *process_route_params(route_t *route, confd_tag_value_t *params)
{
  confd_value_t *value;
  route->fields_present = 0;
  for (; (value = CONFD_GET_TAG_VALUE(params))->type != C_XMLEND; params++) {
    switch (CONFD_GET_TAG_TAG(params)) {
    case NSPREF(destination):
      strncpy(route->destination, CONFD_GET_CBUFPTR(value), CONFD_GET_BUFSIZE(value));
      route->destination[CONFD_GET_BUFSIZE(value)] = 0;
      break;
    case NSPREF(type):
      route->type = CONFD_GET_ENUM_VALUE(value);
      break;
    case NSPREF(gateway):
      strncpy(route->gateway, CONFD_GET_CBUFPTR(value), CONFD_GET_BUFSIZE(value));
      route->gateway[CONFD_GET_BUFSIZE(value)] = 0;
      route->fields_present |= ROUTE_GATEWAY;
      break;
    case NSPREF(mask):
      route->mask = CONFD_GET_IPV4(value);
      route->fields_present |= ROUTE_MASK;
      break;
    case NSPREF(flags):
      /* ehh.. should not happen... */
      route->flags = CONFD_GET_BIT32(value);
      route->fields_present |= ROUTE_FLAGS;
      break;
    case NSPREF(metric):
      route->metric = CONFD_GET_UINT16(value);
      route->fields_present |= ROUTE_METRIC;
      break;
    case NSPREF(iface):
      strncpy(route->iface, CONFD_GET_CBUFPTR(value), CONFD_GET_BUFSIZE(value));
      route->iface[CONFD_GET_BUFSIZE(value)] = 0;
      route->fields_present |= ROUTE_IFACE;
      break;
    }
  }
  return params;
}

int act_route(char *action, struct confd_user_info *uinfo, route_t *route)
{
  char *argv[20], **arg = argv;
  int pid;
  *(arg++) = ROUTE_CMD;
  *(arg++) = action;
  *(arg++) = route->type == NSPREF(_routeTargetType_host) ? "-host" : "-net";
  *(arg++) = route->destination;
  if (route->fields_present & ROUTE_MASK) {
    *(arg++) = "netmask";
    *(arg++) = inet_ntoa(route->mask);
  }
  if (route->fields_present & ROUTE_GATEWAY) {
    *(arg++) = "gw";
    *(arg++) = route->gateway;
  }
  if (route->fields_present & ROUTE_IFACE) {
    *(arg++) = "dev";
    *(arg++) = route->iface;
  }
  *arg = NULL;
  switch (pid = fork()) {
  case 0:
    execve("/sbin/route", argv, NULL);
    perror(NULL);
    exit(127);
  case -1:
    ACTION_ERROR(uinfo, "Unable to spawn process");
  }
  wait_process(pid);
  confd_action_reply_values(uinfo, NULL, 0);
  return CONFD_OK;
}

int get_routes(struct confd_user_info *uinfo)
{
  int pid, fd;
  route_list_t *routes = NULL, *r;
  int count;
  confd_tag_value_t *reply, *start;
  if (open_route_pipe(&pid, &fd) == -1)
    ACTION_ERROR(uinfo, "Cannot start a system command");
  if ((count = read_routes(fd, &routes)) == -1)
    ACTION_ERROR(uinfo, "Cannot parse command output");
  wait_process(pid);
  close(fd);
#ifdef DEBUG
  for (r = routes; r != NULL; r = r->next)
    DEBUG_LOG("read route dest. %s, gw %s, mask %s, flags %i, metric %i, iface %s",
              r->route.destination, r->route.gateway, inet_ntoa(r->route.mask), r->route.flags, r->route.metric, r->route.iface);
#endif
  start = malloc(count * ELEMS_PER_ROUTE * sizeof(confd_tag_value_t));
  for (reply = start, r = routes; r != NULL; r = r->next) {
    CONFD_SET_TAG_XMLBEGIN(reply++, NSPREF(routes), NAMESPACE);
    CONFD_SET_TAG_ENUM_VALUE(reply++, NSPREF(type), r->route.type);
    CONFD_SET_TAG_STR(reply++, NSPREF(destination), r->route.destination);
    CONFD_SET_TAG_STR(reply++, NSPREF(gateway), r->route.gateway);
    CONFD_SET_TAG_IPV4(reply++, NSPREF(mask), r->route.mask);
    CONFD_SET_TAG_BIT32(reply++, NSPREF(flags), r->route.flags);
    CONFD_SET_TAG_UINT16(reply++, NSPREF(metric), r->route.metric);
    CONFD_SET_TAG_STR(reply++, NSPREF(iface), r->route.iface);
    CONFD_SET_TAG_XMLEND(reply++, NSPREF(routes), NAMESPACE);
  }
  confd_action_reply_values(uinfo, start, count * ELEMS_PER_ROUTE);
  free_route_list(routes);
  free(start);
  return CONFD_OK;
}

int zebra_route_action(struct confd_user_info *uinfo, struct xml_tag *name,
                 confd_hkeypath_t *kp, confd_tag_value_t *params, int nparams)
{
  confd_tag_value_t *const parend = params + nparams;
  route_t route;
  int to_add = 0, to_del = 0;
  int32_t operation = 0;

  for (; params < parend; params++) {
    confd_value_t *value = CONFD_GET_TAG_VALUE(params);
    switch (value->type) {
    case C_XMLBEGIN:
      /* route definition should begin here.. */
      *(CONFD_GET_TAG_TAG(params) == NSPREF(route_to_add) ? &to_add : &to_del) = 1;
      params = process_route_params(&route, params+1);
      break;
    case C_XMLEND:
      /* what?? should not happen.. */
      ERROR_LOG("XMLEND met..?");
      break;
    default:
      operation = CONFD_GET_ENUM_VALUE(value);
      break;
    }
  }

  switch (operation) {
  case NSPREF(_operationType2_add):
    if (to_del || !to_add)
      ACTION_ERROR(uinfo, "Bad arguments for \"add\" operation: %s",
                          !to_add ? "to-add routes needed" : "cannot have to-delete routes");
    return act_route("add", uinfo, &route);
  case NSPREF(_operationType2_delete):
    if (!to_del || to_add)
      ACTION_ERROR(uinfo, "Bad arguments for \"delete\" operation: %s",
                          !to_del ? "to-delete routes needed" : "cannot have to-add routes");
    return act_route("del", uinfo, &route);
  case NSPREF(_operationType2_show):
    if (to_del || to_add)
      ACTION_ERROR(uinfo, "Bad arguments for \"show\" operation: cannot have %s routes",
                   to_add ? "to-add" : "to-delete");
    return get_routes(uinfo);
  }
  return CONFD_OK;
}
