/*********************************************************************
 * ConfD Web UI extended example
 * Implements a validation
 *
 * (C) 2012 Tail-f Systems
 * Permission to use this code as a starting point hereby granted
 *
 * See the README file for more information
 ********************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>
#include <confd.h>
#include "config.h"

int debuglevel = CONFD_DEBUG;

static int ctlsock;
static int workersock;
static struct confd_daemon_ctx *dctx;

struct confd_trans_validate_cbs vcb;
struct confd_valpoint_cb valp;

static void OK(int rval) {
     if (rval != CONFD_OK) {
          fprintf(stderr, "validation.c: error not CONFD_OK: %d : %s \n",
                  confd_errno, confd_lasterr());
          abort();
     }
}

static int init_validation(struct confd_trans_ctx *tctx) {
     confd_trans_set_fd(tctx, workersock);
     return CONFD_OK;
}

static int stop_validation(struct confd_trans_ctx *tctx) {
     return CONFD_OK;
}

static int validate(struct confd_trans_ctx *tctx,
                    confd_hkeypath_t *keypath,
                    confd_value_t *newval) {
  unsigned char *description = CONFD_GET_BUFPTR(newval);
  //int len = CONFD_GET_BUFSIZE(newval);

  if (strcmp("foo", (char *)description) == 0) {
    confd_trans_seterr(tctx, "foo is a dangerous value");
    return CONFD_VALIDATION_WARN;
  } else
    if (strcmp("bar", (char *)description) == 0) {
      confd_trans_seterr(tctx, "this a forbidden perfume!");
      return CONFD_ERR;
    }

  return CONFD_OK;
}

int main(int argc, char **argv) {
     int c;
     struct sockaddr_in addr;

     while ((c = getopt(argc, argv, "tdps")) != -1) {
          switch(c) {
          case 't':
               debuglevel = CONFD_TRACE;
               break;
          case 'd':
               debuglevel = CONFD_DEBUG;
               break;
          case 'p':
               debuglevel = CONFD_PROTO_TRACE;
               break;
          case 's':
               debuglevel = CONFD_SILENT;
               break;
          }
     }

     confd_init("validation", stderr, debuglevel);

     if ((dctx = confd_init_daemon("mydaemon")) == NULL)
          confd_fatal("Failed to initialize confd\n");

     if ((ctlsock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
          confd_fatal("Failed to open ctlsocket\n");

     addr.sin_addr.s_addr = inet_addr("127.0.0.1");
     addr.sin_family = AF_INET;
     addr.sin_port = htons(CONFD_PORT);

     if (confd_connect(dctx, ctlsock, CONTROL_SOCKET,
                       (struct sockaddr*)&addr,
                       sizeof (struct sockaddr_in)) < 0)
          confd_fatal("Failed to confd_connect() to confd \n");

     if ((workersock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
          confd_fatal("Failed to open workersocket\n");

     if (confd_connect(dctx, workersock, WORKER_SOCKET,(struct sockaddr*)&addr,
                       sizeof (struct sockaddr_in)) < 0)
          confd_fatal("Failed to confd_connect() to confd \n");

     vcb.init = init_validation;
     vcb.stop = stop_validation;
     confd_register_trans_validate_cb(dctx, &vcb);

     valp.validate = validate;
     strcpy(valp.valpoint, "validDescription");
     OK(confd_register_valpoint_cb(dctx, &valp));
     OK(confd_register_done(dctx));

     while (1) {
          struct pollfd set[2];
          int ret;

          set[0].fd = ctlsock;
          set[0].events = POLLIN;
          set[0].revents = 0;

          set[1].fd = workersock;
          set[1].events = POLLIN;
          set[1].revents = 0;

          if (poll(&set[0], 2, -1) < 0) {
               perror("Poll failed:");
               continue;
          }

          if (set[0].revents & POLLIN) {
               if ((ret = confd_fd_ready(dctx, ctlsock)) == CONFD_EOF) {
                    confd_fatal("Control socket closed\n");
               } else if (ret == CONFD_ERR &&
                          confd_errno != CONFD_ERR_EXTERNAL) {
                    confd_fatal(
                         "Error on control socket request: %s (%d): %s\n",
                         confd_strerror(confd_errno), confd_errno,
                         confd_lasterr());
               }
          }
          if (set[1].revents & POLLIN) {
               if ((ret = confd_fd_ready(dctx, workersock)) == CONFD_EOF) {
                    confd_fatal("Worker socket closed\n");
               } else if (ret == CONFD_ERR &&
                          confd_errno != CONFD_ERR_EXTERNAL) {
                    confd_fatal("Error on worker socket request: %s (%d): %s\n",
                                confd_strerror(confd_errno), confd_errno,
                                confd_lasterr());
               }
          }
     }
}
