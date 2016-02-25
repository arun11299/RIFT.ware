
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

#include <sys/types.h>
#include <sys/wait.h>

#include <stdio.h>

#include <confd_lib.h>
#include <confd_dp.h>


#define OK(E) assert((E) == CONFD_OK)
#define ERR(E) assert((E) == CONFD_ERR)
#define XEOF(E) assert((E) == CONFD_EOF)

int NUMTESTS = 9;
int debuglevel = CONFD_SILENT;
char *notify_name = "";

/* In a real application, we could use the 'ref'
   to track multiple "ongoing" inform-requests, and
   e.g. have an array indexed by 'ref' here instead */
int num_inform_targets;
int do_poll;

struct confd_daemon_ctx *dctx;
int ctlsock, workersock;

int doruntest(struct confd_notification_ctx *nctx, int testno);

void pval(confd_value_t *v)
{
    char buf[BUFSIZ];
    confd_pp_value(buf, BUFSIZ, v);
    fprintf(stderr, "%s\n", buf);
}


static int cnct(struct confd_daemon_ctx *dx,
                enum confd_sock_type type, int *sock)
{

    struct sockaddr_in addr;

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4565);

    if ((*sock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open socket\n");

    if (confd_connect(dx, *sock, type, (struct sockaddr*)&addr,
                      sizeof (struct sockaddr_in)) < 0)
        confd_fatal("Failed to confd_connect() to confd \n");

    return CONFD_OK;
}


/**
 * Send a "coldstart"
 *
 */
int test1(struct confd_notification_ctx *nctx)
{
  OK(confd_notification_send_snmp(nctx, "coldStart", NULL, 0));
  return 1;
}


/**
 * Send a "notif1"
 * The "numberOfHosts" is not provided and will be looked up by the agent.
 */
int test2(struct confd_notification_ctx *nctx)
{
  OK(confd_notification_send_snmp(nctx, "notif1", NULL, 0));
  return 1;
}


/**
 * Send a "notif1"
 * The "numberOfHosts" is now provided here, and will be used.
 */
int test3(struct confd_notification_ctx *nctx)
{
  struct confd_snmp_varbind vb;
  vb.type = CONFD_SNMP_VARIABLE;
  strcpy(vb.var.name, "numberOfHosts");
  CONFD_SET_INT32(&vb.val, 32);
  OK(confd_notification_send_snmp(nctx, "notif1", &vb, 1));
  return 1;
}


/**
 * Send a "notif1"
 * The "numberOfHosts is not provided here and should be looked up
 * by the server.
 * Also an extra variable binding is sent in the event.
 * "extraVariable1" but is has no value here and should
 * be looked up by the agent.
 */
int test4(struct confd_notification_ctx *nctx)
{
  struct confd_snmp_varbind vb;
  vb.type = CONFD_SNMP_VARIABLE;
  strcpy(vb.var.name, "extraVariable1");
  CONFD_SET_NOEXISTS(&vb.val);
  OK(confd_notification_send_snmp(nctx, "notif1", &vb, 1));
  return 1;
}


/**
 * Send a "notif1"
 * The numberOfHosts is provided here.
 * Also the "extraVariable1" is provided here.
 */

int test5(struct confd_notification_ctx *nctx)
{
  struct confd_snmp_varbind vb[2];
  vb[0].type = CONFD_SNMP_VARIABLE;
  strcpy(vb[0].var.name, "numberOfHosts");
  CONFD_SET_INT32(&vb[0].val, 1);

  vb[1].type = CONFD_SNMP_VARIABLE;
  strcpy(vb[1].var.name, "extraVariable1");
  CONFD_SET_INT32(&vb[1].val, 55);
  OK(confd_notification_send_snmp(nctx, "notif1", &vb[0], 2));
  return 1;
}


/**
 * Send a "notif1"
 * An OID for "extraVariable2" is provided here.
 * It's string.
 */
int test6(struct confd_notification_ctx *nctx)
{
  struct confd_snmp_varbind vb;
  vb.type = CONFD_SNMP_OID;
  /* OID = [1,3,6,1,4,1,24961,3,1,1,3,0]
   * points to "extraVariable2"
   */
  vb.var.oid.oid[0] = 1;
  vb.var.oid.oid[1] = 3;
  vb.var.oid.oid[2] = 6;
  vb.var.oid.oid[3] = 1;
  vb.var.oid.oid[4] = 4;
  vb.var.oid.oid[5] = 1;
  vb.var.oid.oid[6] = 24961;
  vb.var.oid.oid[7] = 3;
  vb.var.oid.oid[8] = 1;
  vb.var.oid.oid[9] = 1;
  vb.var.oid.oid[10] = 3;
  vb.var.oid.oid[11] = 0;
  vb.var.oid.len= 12;
  CONFD_SET_STR(&vb.val, "test string");
  OK(confd_notification_send_snmp(nctx, "notif1", &vb, 1));
  return 1;
}


/**
 * Send a "notif1"
 * An extra variable binding for a column object
 * is provided.
 */
int test7(struct confd_notification_ctx *nctx)
{
  struct confd_snmp_varbind vb;
  vb.type = CONFD_SNMP_COL_ROW;
  /* RowIndex = 6."saturn" */
  strcpy(vb.var.cr.column, "hostNumberOfServers");
  vb.var.cr.rowindex.oid[0] = 6;
  vb.var.cr.rowindex.oid[1] = 's';
  vb.var.cr.rowindex.oid[2] = 'a';
  vb.var.cr.rowindex.oid[3] = 't';
  vb.var.cr.rowindex.oid[4] = 'u';
  vb.var.cr.rowindex.oid[5] = 'r';
  vb.var.cr.rowindex.oid[6] = 'n';
  vb.var.cr.rowindex.len = 7;
  CONFD_SET_INT32(&vb.val, 987);
  OK(confd_notification_send_snmp(nctx, "notif1", &vb, 1));
  return 1;
}

/**
 * Send a "notif1"
 * An extra variable binding for a column object
 * is provided, but the value is fetched by ConfD.
 */
int test8(struct confd_notification_ctx *nctx)
{
  struct confd_snmp_varbind vb;
  vb.type = CONFD_SNMP_COL_ROW;
  /* RowIndex = 6."saturn" */
  strcpy(vb.var.cr.column, "hostNumberOfServers");
  vb.var.cr.rowindex.oid[0] = 6;
  vb.var.cr.rowindex.oid[1] = 's';
  vb.var.cr.rowindex.oid[2] = 'a';
  vb.var.cr.rowindex.oid[3] = 't';
  vb.var.cr.rowindex.oid[4] = 'u';
  vb.var.cr.rowindex.oid[5] = 'r';
  vb.var.cr.rowindex.oid[6] = 'n';
  vb.var.cr.rowindex.len = 7;
  CONFD_SET_NOEXISTS(&vb.val); /* let ConfD get the value */
  OK(confd_notification_send_snmp(nctx, "notif1", &vb, 1));
  return 1;
}

/**
 * Track inform-request delivery
 */
int test9(struct confd_notification_ctx *nctx)
{
  struct confd_snmp_varbind vb;
  vb.type = CONFD_SNMP_VARIABLE;
  strcpy(vb.var.name, "numberOfHosts");
  CONFD_SET_INT32(&vb.val, 32);
  OK(confd_notification_send_snmp_inform(nctx, "notif1",
                                         &vb, 1, "MyInformCallback", 10));
  do_poll = 1;
  return 1;
}

int doruntest(struct confd_notification_ctx *nctx, int testno)
{
     switch(testno) {
     case 1:
         return test1(nctx);
     case 2:
         return test2(nctx);
     case 3:
         return test3(nctx);
     case 4:
         return test4(nctx);
     case 5:
         return test5(nctx);
     case 6:
         return test6(nctx);
     case 7:
         return test7(nctx);
     case 8:
         return test8(nctx);
     case 9:
         return test9(nctx);
     default:
         printf("Unknown testcase %d\n", testno);
         exit(1);

     }
     return 0;
 }


int runtest(struct confd_notification_ctx *nctx, int testno)
{
    printf("RUNNING test %d\n", testno);

    do_poll = 0;

    doruntest(nctx,testno);

    while (do_poll) {
        struct pollfd set[1];
        int ret;

        set[0].fd = ctlsock;
        set[0].events = POLLIN;
        set[0].revents = 0;

        if (poll(&set[0], 1, -1) < 0) {
            perror("Poll failed:");
            continue;
        }

        /* Check for I/O */
        if (set[0].revents & POLLIN) {
            if ((ret = confd_fd_ready(dctx, ctlsock)) == CONFD_EOF) {
                confd_fatal("Control socket closed\n");
            } else if (ret == CONFD_ERR && confd_errno != CONFD_ERR_EXTERNAL) {
                confd_fatal("Error on control socket request: %s (%d): %s\n",
                            confd_strerror(confd_errno),
                            confd_errno, confd_lasterr());
            }
        }
    }

    printf("FINISHED test %d\n", testno);
    return 1;
}


/* Callback for the inform targets */
void inform_targets_cb(struct confd_notification_ctx *nctx, int ref,
                       struct confd_snmp_target *targets, int num_targets)
{
  int i;
  char buf[255];
  if (num_targets == 0) {
      printf("No targets for inform-request (ref=%d) - exiting\n", ref);
      do_poll = 0;
  } else {
      printf("Targets for inform-request (ref=%d):\n", ref);
      for (i=0; i<num_targets; i++) {
          confd_pp_value(buf,255, &targets[i].address);
          printf(" %s port %d\n", buf, targets[i].port);
      }
      num_inform_targets = num_targets;
      printf("Waiting for results...\n");
      do_poll = 1;
  }
}


/* Callback for the inform result */
void inform_result_cb(struct confd_notification_ctx *nctx, int ref,
                      struct confd_snmp_target *target, int got_response)
{
  char buf[255];
  char *result;
  confd_pp_value(buf,255, &target->address);
  if (got_response)
      result = "Got";
  else
      result = "NO";
  printf("%s response to inform-request (ref=%d) from target %s port %d\n",
         result, ref, buf, target->port);
  if (--num_inform_targets == 0) {
      printf("All results received - exiting\n");
      do_poll = 0;
  } else {
      printf("Waiting for %d more results...\n", num_inform_targets);
  }
}



int main(int argc, char **argv)
{
    int c;
    int debuglevel = CONFD_SILENT;
    int num = -1;
    int loops = 0;
    struct confd_notification_ctx *nctx;
    struct confd_notification_snmp_inform_cbs cbs;

    while ((c = getopt(argc, argv, "n:tdpsL:N:")) != -1) {
        switch(c) {
        case 'n':
            num = atoi(optarg);
            break;
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
        case 'L':
            loops = atoi(optarg);
            break;
        case 'N':
            notify_name = optarg;
            break;
        }
    }

    confd_init("snmp_MYNAME", stderr, debuglevel);
    dctx = confd_init_daemon("snmp_MYNAME");
    OK(cnct(dctx, CONTROL_SOCKET, &ctlsock));
    OK(cnct(dctx, WORKER_SOCKET, &workersock));
    OK(confd_register_snmp_notification(dctx, workersock,
                                        notify_name, "", &nctx));

    /* callbacks are needed for inform-request delivery tracking (test 9) */
    memset(&cbs, 0, sizeof(cbs));
    cbs.targets = inform_targets_cb;
    cbs.result = inform_result_cb;
    strcpy(cbs.cb_id, "MyInformCallback");
    OK(confd_register_notification_snmp_inform_cb(dctx, &cbs));

    OK(confd_register_done(dctx));

    if (num != -1) {
        if (loops) {
            int i;
            for(i=0; i<loops; i++) {
                runtest(nctx, num);
            }
        }
        else {
            runtest(nctx, num);
        }
    }
    else {
        int i;
        for (i=1; i< NUMTESTS+1; i++)
            runtest(nctx, i);
    }

    exit(0);

}
