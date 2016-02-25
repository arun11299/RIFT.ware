/*
 * Copyright 2006 Tail-F Systems AB
 */


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/poll.h>
#include <fcntl.h>

#include <confd.h>
#include <confd_cdb.h>
#include "gen/syslog-helper.h"

#ifdef __APPLE_CC__
#define SYSLOG_FILENAME "/var/log/system.log"
#else
#define SYSLOG_FILENAME "/var/log/messages"
#endif

#define OK(rval) do {                                                   \
    if ((rval) != CONFD_OK) {                                           \
            fprintf(stderr,"syslog-helper: error not CONFD_OK: %d : %s\n",\
                        confd_errno, confd_lasterr());                  \
            confd_fatal("syslog-helper: error not CONFD_OK: %d : %s\n", \
                        confd_errno, confd_lasterr());                  \
    } \
    } while (0);


static int get_line(char *buf, int bufsiz, int fd)
{
  static char filebuf[BUFSIZ+1];
  static int len = 0;
  char *p;

  p = strchr(filebuf, '\n');
  if(!p) {
    int r;

    r = read(fd, filebuf+len, BUFSIZ-len);
    if(r<0)
      return 0;
    len += r;
    p = strchr(filebuf, '\n');
    if(!p && len >= BUFSIZ)
      p = filebuf + BUFSIZ;
  }
  if(!p) {
    return 0;
  } else {
    *p = '\0';
    strcpy(buf, filebuf);
    strcpy(filebuf, p+1);
    len = strlen(filebuf);
    return 1;
  }
}

static void loop(struct sockaddr_in *addr)
{
    int sock, msgsfd;
    FILE *msgs;
    char buf[BUFSIZ];
    int line = 0;
    char *edate, *enode, *etext;

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
        confd_fatal("Failed to create socket");
    OK(cdb_connect(sock, CDB_DATA_SOCKET, (struct sockaddr *)addr,
                   sizeof(struct sockaddr_in)));

    if ((msgsfd = open(SYSLOG_FILENAME, O_RDONLY|O_NONBLOCK)) < 0) {
      perror("Can't open " SYSLOG_FILENAME);
      OK(cdb_start_session(sock, CDB_OPERATIONAL));
      OK(cdb_set_namespace(sock, syslog__ns));
      OK(cdb_create(sock, "/syslog/message{%d}", line));
      OK(cdb_set_elem2(sock, "", "/syslog/message{%d}/date", line));
      OK(cdb_set_elem2(sock, "", "/syslog/message{%d}/node", line));
      OK(cdb_set_elem2(sock, "Can't open " SYSLOG_FILENAME,
                       "/syslog/message{%d}/text", line));
      OK(cdb_end_session(sock));
      exit(1);
    }

    while (1) {
      struct pollfd set[1];
      int rc;

        set[0].fd = msgsfd;
        set[0].events = POLLIN;
        set[0].revents = 0;

        fprintf(stderr,"syslog-helper: Polling...\n");
        sleep(1);
        rc = poll(&set[0], 1, 0);
        if(!rc) {
            sleep(1);
            continue;
        }
        if(rc < 0) {
            perror("Poll failed:");
            exit(1);
        }
        fprintf(stderr,"syslog-helper: Poll trigger\n");

        if (set[0].revents & POLLIN) {
          fprintf(stderr,"syslog-helper: Poll trigger on msgsfd\n");
          OK(cdb_start_session(sock, CDB_OPERATIONAL));
          OK(cdb_set_namespace(sock, syslog__ns));
          while (get_line(buf, sizeof(buf), msgsfd) != 0) {
            if(line >= 200) {
              OK(cdb_create(sock, "/syslog/message{%d}", line));
              OK(cdb_set_elem2(sock, "", "/syslog/message{%d}/date", line));
              OK(cdb_set_elem2(sock, "", "/syslog/message{%d}/node", line));
              OK(cdb_set_elem2(sock, "No more messages displayed",
                               "/syslog/message{%d}/text", line));
              break;
            }
            fprintf(stderr,"syslog-helper: Read line %d '%s'\n", line, buf);

            buf[15] = '\0';
            edate = &buf[0];
            fprintf(stderr,"syslog-helper: edate='%s'\n", edate);
            if ((enode = strtok(&buf[16], " \t")) != NULL) {
              fprintf(stderr,"syslog-helper: enode='%s'\n", enode);
              if ((etext = strtok(NULL, "\n")) != NULL) {
                fprintf(stderr,"syslog-helper: etext='%s', line=%d\n", etext,
                        line);
                OK(cdb_create(sock, "/syslog/message{%d}", line));
                OK(cdb_set_elem2(sock, edate, "/syslog/message{%d}/date",
                                 line));
                OK(cdb_set_elem2(sock, enode, "/syslog/message{%d}/node",
                                 line));
                OK(cdb_set_elem2(sock, etext, "/syslog/message{%d}/text",
                                 line));
                ++line;
              }
            }
          }
          OK(cdb_end_session(sock));
          fprintf(stderr,"syslog-helper: Done reading.\n");
        }
    }
    fclose(msgs);
}

int main(int argc, char **argv)
{
    struct sockaddr_in addr;
    char *port;

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;

    if ((port = getenv("CONFD_IPC_PORT")) != NULL)
      addr.sin_port = htons(atoi(port));
    else {
      addr.sin_port = htons(CONFD_PORT);
    }

    confd_init(argv[0], stderr, CONFD_TRACE);
    OK(confd_load_schemas((struct sockaddr*)&addr, sizeof(struct sockaddr_in)));

    loop(&addr);

    return 0;
}
