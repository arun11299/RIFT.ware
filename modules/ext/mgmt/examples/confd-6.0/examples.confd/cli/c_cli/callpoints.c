/*********************************************************************
 * ConfD CLI example
 * Implements a simple time utc data provider
 *
 * (C) 2007 Tail-f Systems
 * Permission to use this code as a starting point hereby granted
 *
 * See the README file for more information
 ********************************************************************/

#include <sys/time.h>
#include <string.h>
#include <sys/poll.h>

#include <confd.h>
#include "data.h"

/********************************************************************/

static struct confd_daemon_ctx *dctx;
static int ctlsock;
static int workersock;
static struct confd_trans_cbs trans;
static struct confd_data_cbs data;

/********************************************************************/

static int s_init(struct confd_trans_ctx *tctx)
{
     confd_trans_set_fd(tctx, workersock);
     return CONFD_OK;
}

static int s_finish(struct confd_trans_ctx *tctx)
{
     return CONFD_OK;
}

static int get_elem(struct confd_trans_ctx *tctx,
                    confd_hkeypath_t *keypath)
{
     confd_value_t v;
     struct timeval now;

     gettimeofday(&now, NULL);

     switch (CONFD_GET_XMLTAG(&(keypath->v[0][0]))) {
     case data_seconds:
          CONFD_SET_UINT64(&v, now.tv_sec);
          break;
     case data_microseconds:
          CONFD_SET_UINT64(&v, now.tv_usec);
          break;
     default:
          return CONFD_ERR;
     }

     confd_data_reply_value(tctx, &v);
     return CONFD_OK;
}

/********************************************************************/

int main(int argc, char *argv[]) {
    struct sockaddr_in addr;

    int debuglevel = CONFD_TRACE;

    memset(&trans, 0, sizeof (struct confd_trans_cbs));
    trans.init = s_init;
    trans.finish = s_finish;

    memset(&data, 0, sizeof (struct confd_data_cbs));
    data.get_elem = get_elem;
    data.get_next = NULL;
    strcpy(data.callpoint, data__callpointid_clock);

    /* initialize confd library */
    confd_init("data_daemon", stderr, debuglevel);

    if ((dctx = confd_init_daemon("data_daemon")) == NULL)
        confd_fatal("Failed to initialize confdlib\n");

    if ((ctlsock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open ctlsocket\n");

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CONFD_PORT);

    if (confd_load_schemas((struct sockaddr*)&addr,
                           sizeof (struct sockaddr_in)) != CONFD_OK)
        confd_fatal("Failed to load schemas from confd\n");

    /* Create the first control socket, all requests to */
    /* create new transactions arrive here */
    if (confd_connect(dctx, ctlsock, CONTROL_SOCKET, (struct sockaddr*)&addr,
                      sizeof (struct sockaddr_in)) < 0)
        confd_fatal("Failed to confd_connect() to confd \n");

    /* Also establish a workersocket, this is the most simple */
    /* case where we have just one ctlsock and one workersock */
    if ((workersock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open workersocket\n");
    if (confd_connect(dctx, workersock, WORKER_SOCKET,(struct sockaddr*)&addr,
                      sizeof (struct sockaddr_in)) < 0)
        confd_fatal("Failed to confd_connect() to confd \n");

    confd_register_trans_cb(dctx, &trans);

    if (confd_register_data_cb(dctx, &data) == CONFD_ERR)
        confd_fatal("Failed to register data cb \n");

    if (confd_register_done(dctx) != CONFD_OK)
        confd_fatal("Failed to complete registration \n");

    while(1) {
        struct pollfd set[2];
        int ret;

        set[0].fd = ctlsock;
        set[0].events = POLLIN;
        set[0].revents = 0;

        set[1].fd = workersock;
        set[1].events = POLLIN;
        set[1].revents = 0;

        if (poll(set, sizeof(set)/sizeof(*set), -1) < 0) {
            perror("Poll failed:");
            continue;
        }

        /* Check for I/O */
        if (set[0].revents & POLLIN) {
            if ((ret = confd_fd_ready(dctx, ctlsock)) == CONFD_EOF) {
                confd_fatal("Control socket closed\n");
            } else if (ret == CONFD_ERR && confd_errno != CONFD_ERR_EXTERNAL) {
                confd_fatal("Error on control socket request: %s (%d): %s\n",
                     confd_strerror(confd_errno), confd_errno, confd_lasterr());
            }
        }
        if (set[1].revents & POLLIN) {
            if ((ret = confd_fd_ready(dctx, workersock)) == CONFD_EOF) {
                confd_fatal("Worker socket closed\n");
            } else if (ret == CONFD_ERR && confd_errno != CONFD_ERR_EXTERNAL) {
                confd_fatal("Error on worker socket request: %s (%d): %s\n",
                     confd_strerror(confd_errno), confd_errno, confd_lasterr());
            }
        }
    }
}
