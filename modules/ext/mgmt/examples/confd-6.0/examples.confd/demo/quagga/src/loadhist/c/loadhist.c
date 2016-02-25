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
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#include <confd.h>
#include <confd_cdb.h>
#include "gen/loadhist.h"

#define INTERVAL    2
#define MAX_SAMPLES (86400/interval)

#ifdef __APPLE_CC__
#define LOAD_AVG "w | head -n1 | cut -d':' -f4 | sed 's/,/./g"
#define LOAD_STAT "top -l 1 | awk '/([0-9]+)\(0\) pageins/ {print $7}' \
 | sed 's/(0)//'"
static int current_load = 0;
#else
#define LOAD_AVG "/proc/loadavg"
#define LOAD_STAT "/proc/stat"
#endif


#define OK(rval) do {                                                   \
    if ((rval) != CONFD_OK)                                             \
      confd_fatal("get_load: error not CONFD_OK: %d : %s\n",            \
                  confd_errno, confd_lasterr());                        \
    } while (0);


static volatile int do_get_load = 0;

static void catch_alarm(int sig)
{
    do_get_load++;
}


static void get_load(struct sockaddr_in *addr, int max_samples)
{
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    struct confd_datetime dt;
    int sock;
    confd_value_t timeval, cpu;
    static double last_up = 0.0, last_idle = 0.0;
    double up, idle, util;
    FILE *proc;
    char buf[BUFSIZ];
    char *p;
    int n;
    confd_value_t load;
    static int ctxt_old = 0;
    int ctxt_new, ctxt_per_sec;
    int r = 0;

    dt.year = tm->tm_year + 1900; dt.month = tm->tm_mon + 1;
    dt.day = tm->tm_mday; dt.hour = tm->tm_hour;
    dt.min = tm->tm_min; dt.sec = tm->tm_sec;
    dt.micro = 0; dt.timezone = CONFD_TIMEZONE_UNDEF;
    CONFD_SET_DATETIME(&timeval, dt);

    if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
      confd_fatal("Failed to create socket");
    OK(cdb_connect(sock, CDB_DATA_SOCKET, (struct sockaddr *)addr,
                   sizeof(struct sockaddr_in)));
    OK(cdb_start_session(sock, CDB_OPERATIONAL));
    OK(cdb_set_namespace(sock, loadhist__ns));

#ifdef __APPLE_CC__
    /* Fake load for OS X for now */

    r = (int)(20.0 * ((float)rand() / (RAND_MAX + 1.0)));
    current_load = 60 + r;

    CONFD_SET_INT32(&load, current_load);
    OK(cdb_create(sock, "/load/sample{%x}", &timeval));
    OK(cdb_set_elem(sock, &load, "/load/sample{%x}/load", &timeval));
    OK(cdb_set_elem(sock, &load, "/load/current"));


#else
    if ((proc = fopen("/proc/loadavg", "r")) != NULL) {
      if (fgets(buf, sizeof(buf), proc) != NULL) {
        if ((p = strtok(buf, " \t")) != NULL) {
          CONFD_SET_INT32(&load, (int)(atof(p)*100));
          OK(cdb_create(sock, "/load/sample{%x}", &timeval));
          OK(cdb_set_elem(sock, &load, "/load/sample{%x}/load",
                          &timeval));
          OK(cdb_set_elem(sock, &load, "/load/current"));
        }
      }
      fclose(proc);
    }
#endif

    n = cdb_num_instances(sock, "/load/sample");
    while (n-- > max_samples)
      OK(cdb_delete(sock, "/load/sample[0]")); /* delete oldest */

    if ((proc = fopen("/proc/stat", "r")) != NULL) {
      while (fgets(buf, sizeof(buf), proc) != NULL) {
        if ((p = strtok(buf, " \t")) != NULL) {
          if (strcmp(p, "ctxt") == 0) {
            p = strtok(NULL, "\n");
            ctxt_new = atol(p);
            if(ctxt_old) {
              ctxt_per_sec = (int)((ctxt_new - ctxt_old) / INTERVAL);
              if(ctxt_per_sec > 10000)
                ctxt_per_sec = 10000;
              CONFD_SET_INT32(&load, ctxt_per_sec);
              OK(cdb_set_elem(sock, &load, "/load/context_switches"));
            }
            ctxt_old = ctxt_new;
          }
        }
      }
      fclose(proc);
    }
    OK(cdb_close(sock));
}

int main(int argc, char **argv)
{
    int interval = 0;
    int max_samples = 0;
    struct sockaddr_in addr;
    time_t now;
    struct tm *tm;
    struct itimerval timer;
    char *port;

    if (argc > 1)
      interval = atoi(argv[1]);
    if (argc > 2)
      max_samples = atoi(argv[2]);
    if (interval == 0)
      interval = INTERVAL;
    if (max_samples == 0)
      max_samples = MAX_SAMPLES;

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_family = AF_INET;

    if ((port = getenv("CONFD_IPC_PORT")) != NULL)
      addr.sin_port = htons(atoi(port));
    else {
      addr.sin_port = htons(CONFD_PORT);
    }

    confd_init(argv[0], stderr, CONFD_TRACE);
    OK(confd_load_schemas((struct sockaddr*)&addr, sizeof(struct sockaddr_in)));
    signal(SIGALRM, catch_alarm);
    /* start at next multiple of interval */
    now = time(NULL);
    tm = localtime(&now);
    timer.it_value.tv_sec = interval - tm->tm_sec % interval;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = interval;
    timer.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    while (1) {
      pause();
      if (do_get_load) {
        do_get_load = 0;
        get_load(&addr, max_samples);
      }
    }
}
