/*********************************************************************
 * ConfD Actions intro example
 * Implements a couple of actions
 *
 * (C) 2007 Tail-f Systems
 * Permission to use this code as a starting point hereby granted
 *
 * See the README file for more information
 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/poll.h>

#include <confd.h>
#include <confd_maapi.h>
#include <confd_cdb.h>
#include <confd_logsyms.h>

/********************************************************************/

static int ctlsock, workersock;
static struct confd_daemon_ctx *dctx;

static int init_action(struct confd_user_info *uinfo);
static int do_command(struct confd_user_info *uinfo,
                      char *path, int argc, char **argv);
static int do_show(struct confd_user_info *uinfo,
                   char *path, int argc, char **argv);
static int do_modename(struct confd_user_info *uinfo,
                       char *path, int argc, char **argv);
static int do_unhide(struct confd_user_info *uinfo,
                     char *path, int argc, char **argv);

static void main_loop(int do_phase0);

extern void fail(char *fmt, ...);

/********************************************************************/

int main(int argc, char **argv)
{
    struct sockaddr_in addr;
    int debuglevel = CONFD_TRACE;
    struct confd_action_cbs acb;

    /* Init library */
    confd_init("cli_actions_daemon",stderr, debuglevel);

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(CONFD_PORT);

    if ((dctx = confd_init_daemon("cli_actions_daemon")) == NULL)
        fail("Failed to initialize ConfD\n");

    if ((ctlsock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open ctlsocket\n");

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

    /* register the action handler callback */
    memset(&acb, 0, sizeof(acb));
    strcpy(acb.actionpoint, "cli-point");
    acb.init = init_action;
    acb.command = do_command;

    if (confd_register_action_cbs(dctx, &acb) != CONFD_OK)
        fail("Couldn't register action callbacks");

    /* register the show action handler callback */
    memset(&acb, 0, sizeof(acb));
    strcpy(acb.actionpoint, "show-point");
    acb.init = init_action;
    acb.command = do_show;

    if (confd_register_action_cbs(dctx, &acb) != CONFD_OK)
        fail("Couldn't register action callbacks");

    /* register the modename action handler callback */
    memset(&acb, 0, sizeof(acb));
    strcpy(acb.actionpoint, "mode_name");
    acb.init = init_action;
    acb.command = do_modename;

    if (confd_register_action_cbs(dctx, &acb) != CONFD_OK)
        fail("Couldn't register modename action callbacks");

    if (confd_register_done(dctx) != CONFD_OK)
        fail("Couldn't complete callback registration");

    main_loop(0);

    close(ctlsock);
    close(workersock);
    confd_release_daemon(dctx);
    return 0;
}

/* Main loop - receive and act on events from ConfD */
static void main_loop(int do_phase0)
{
    struct pollfd set[3];
    int ret;

    while (1) {

        set[0].fd = ctlsock;
        set[0].events = POLLIN;
        set[0].revents = 0;

        set[1].fd = workersock;
        set[1].events = POLLIN;
        set[1].revents = 0;

        if (poll(set, 2, -1) < 0) {
            fail("Poll failed");
        }

        /* Check for I/O */

        if (set[0].revents & POLLIN) { /* ctlsock */
            if ((ret = confd_fd_ready(dctx, ctlsock)) == CONFD_EOF) {
                fail("Control socket closed");
            } else if (ret == CONFD_ERR && confd_errno != CONFD_ERR_EXTERNAL) {
                fail("Error on control socket request: %s (%d): %s",
                     confd_strerror(confd_errno), confd_errno, confd_lasterr());
            }
        }

        if (set[1].revents & POLLIN) { /* workersock */
            if ((ret = confd_fd_ready(dctx, workersock)) == CONFD_EOF) {
                fail("Worker socket closed");
            } else if (ret == CONFD_ERR && confd_errno != CONFD_ERR_EXTERNAL) {
                fail("Error on worker socket request: %s (%d): %s",
                     confd_strerror(confd_errno), confd_errno, confd_lasterr());
            }
        }

    }
}

/********************************************************************/

static int init_action(struct confd_user_info *uinfo)
{
    int ret = CONFD_OK;

    printf("init_action called\n");
    confd_action_set_fd(uinfo, workersock);
    return ret;
}

/* This is the action callback function. In this example, we have a
   single function for all three actions. */
static int do_command(struct confd_user_info *uinfo,
                      char *path, int argc, char **argv)
{
    struct sockaddr_in addr;
    int i, res;
    int sock;
    char buf[BUFSIZ];
    char *yesno[] = {"yes","no"};

    printf("do_command called\n");

    printf("path: %s\n", path);
    for (i = 0; i < argc; i++) {
        printf("param %2d: %s\n", i, argv[i]);
    }

    /* Setup MAAPI socket for reading and writing to the CLI */
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4565);

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open socket\n");

    if (maapi_connect(sock, (struct sockaddr*)&addr,
                      sizeof (struct sockaddr_in)) < 0)
        confd_fatal("Failed to confd_connect() to confd \n");

    maapi_cli_write(sock, uinfo->usid, "Running CLI command\n", 20);
    maapi_cli_printf(sock, uinfo->usid, "You supplied %d arguments\n", argc-1);

    res = maapi_cli_prompt_oneof(sock, uinfo->usid,
                                 "Do you want to proceed: ",
                                 yesno, 2, buf, BUFSIZ);

    if (res == 0 && strncmp(buf, "yes", 3) == 0) {
        maapi_cli_printf(sock, uinfo->usid, "Proceeding\n");

        return CONFD_OK;
    }
    else {
        confd_action_seterr(uinfo, "not proceeding");
        return CONFD_ERR;
    }
}

static int do_show(struct confd_user_info *uinfo,
                   char *path, int argc, char **argv)
{
    struct sockaddr_in addr;
    int i;
    int sock;

    printf("do_show called\n");

    printf("path: %s\n", path);
    for (i = 0; i < argc; i++) {
        printf("param %2d: %s\n", i, argv[i]);
    }

    /* Setup MAAPI socket for reading and writing to the CLI */
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;
    addr.sin_port = htons(4565);

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0 )
        confd_fatal("Failed to open socket\n");

    if (maapi_connect(sock, (struct sockaddr*)&addr,
                      sizeof (struct sockaddr_in)) < 0)
        confd_fatal("Failed to confd_connect() to confd \n");

    if (argc >= 3)
        maapi_cli_printf(sock, uinfo->usid,
                         "Running show command with path %s\n",
                         argv[2]);

    return CONFD_OK;
}

static int do_modename(struct confd_user_info *uinfo,
                       char *path, int argc, char **argv)
{
    int i;
    char *mode = "config-priv";

    printf("do_modename called\n");

    printf("path: %s\n", path);
    for (i = 0; i < argc; i++) {
        printf("param %2d: %s\n", i, argv[i]);
    }

    if (confd_action_reply_command(uinfo, &mode, 1) < 0)
        confd_fatal("Failed to reply to confd\n");

    return CONFD_OK;
}

void fail(char *fmt, ...)
{
    va_list ap;
    char buf[BUFSIZ];

    va_start(ap, fmt);
    snprintf(buf, sizeof(buf), "%s, exiting", fmt);
    vsyslog(LOG_ERR, buf, ap);
    va_end(ap);
    exit(1);
}

/********************************************************************/
