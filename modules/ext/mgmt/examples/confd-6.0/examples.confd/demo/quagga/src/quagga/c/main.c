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
#include <sys/time.h>
#include <sys/types.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/poll.h>
#include <sys/select.h>

#include <confd.h>
#include <confd_maapi.h>
#include <confd_cdb.h>

#include "main.h"
#include "logs.h"

int quagga_not_running = 0;

struct zconfd_data;

#define MAX_THREAD 10
#define EMPTY_SLOT (-1)
static int s_threads_sock[MAX_THREAD]; // value -1 has the meaning unused
static thread_read_func_t* s_threads_func[MAX_THREAD];
static void* s_threads_data[MAX_THREAD];

struct tTimerItem {
        thread_timer_func_t* timer_func;
        void* timer_data;
        time_t timeout;
};

static struct tTimerItem s_timers[MAX_THREAD];

static void thread_init(){
  int thread_ind;
  for(thread_ind = 0; thread_ind < MAX_THREAD; thread_ind++ ){
    s_threads_sock[thread_ind] = EMPTY_SLOT;
    s_threads_func[thread_ind] = NULL;
  }
}

static void timer_init(){
        memset( s_timers, 0, sizeof( s_timers));
}

void* thread_add_read(thread_read_func_t* func, void* zd, int sock){
  int thread_ind;
  for(thread_ind = 0; thread_ind < MAX_THREAD; thread_ind++ ){
    if(s_threads_sock[thread_ind] == EMPTY_SLOT)
      break;
  }

  if(thread_ind == MAX_THREAD ){
    ERROR_LOG( "No free slot for thread found! Exit...");
    exit(1);
  }

  s_threads_sock[thread_ind] = sock;
  s_threads_func[thread_ind] = func;
  s_threads_data[thread_ind] = zd;

  return (void *)thread_ind;
}

void* thread_add_timer(thread_timer_func_t* func, void *thread_data, int timer ){
        time_t now;
        time(&now);

        int timer_ind;
        for ( timer_ind = 0; timer_ind < MAX_THREAD; timer_ind++ ){
                if( s_timers[timer_ind].timer_func == NULL)
                        break;
        }

        if ( timer_ind == MAX_THREAD ){
                ERROR_LOG( "No free slot for timer found! Exit...");
                exit(1);
        }

        s_timers[timer_ind].timer_func = func;
        s_timers[timer_ind].timer_data = thread_data;
        s_timers[timer_ind].timeout = now + timer;

        return NULL;
}

void thread_cancel(void* thread)
{
    int thread_ind = (int)thread;

    s_threads_sock[thread_ind] = EMPTY_SLOT;
}

static void simulate_read_threads(){

        struct timeval to;
        fd_set set_r;
        int thread_ind;
        int r;

        // prepare read set
        FD_ZERO(&set_r);

        for(thread_ind = 0; thread_ind < MAX_THREAD; thread_ind++ ){
                if(s_threads_sock[thread_ind] != EMPTY_SLOT){
                        FD_SET(s_threads_sock[thread_ind], &set_r);
                }
        }


        // timeout 1 sec.
        to.tv_sec = 1;
        to.tv_usec = 0;

        r = select(FD_SETSIZE, &set_r, NULL, NULL, &to );
        if(r < 0){
                ERROR_LOG( "select failed: %s Exit...", strerror(errno));
                exit(1);
        }

        // call hooks with descriptors
        for(thread_ind = 0; thread_ind < MAX_THREAD; thread_ind++ ){
                if( s_threads_sock[thread_ind] != EMPTY_SLOT ){
                        if( FD_ISSET(s_threads_sock[thread_ind], &set_r ) ){

                                // copy parameters
                                int sock = s_threads_sock[thread_ind];
                                void* data = s_threads_data[thread_ind];
                                thread_read_func_t* func = s_threads_func[thread_ind];

                                // free slot
                                s_threads_sock[thread_ind] = EMPTY_SLOT;
                                s_threads_data[thread_ind] = NULL;
                                s_threads_func[thread_ind] = NULL;

                                /*      printf("call read func for socket %d\n", sock); */

                                // call function
                                func(data,sock);
                        }
                }
        }
}

static void simulate_timer_thread() {
  time_t now;
        time(&now);

        // check timeout
        int timer_ind;
        for ( timer_ind = 0; timer_ind < MAX_THREAD; timer_ind++)
                if (( s_timers[timer_ind].timer_func != NULL) && ( now > s_timers[timer_ind].timeout)){
                        //printf("timer timeout %d at %lu for %lu\n", timer_ind, now, s_timers[timer_ind].timeout);
                        s_timers[timer_ind].timer_func( s_timers[timer_ind].timer_data);        //call timer function
                        s_timers[timer_ind].timer_func = NULL;  //remove from list
                }
}

int main(int argc, char **argv) {
  // init thread structures
  thread_init();
        timer_init();

  // initialization of the framework
        framework_init();

  for(;;){
    simulate_read_threads();
    simulate_timer_thread();
  }
}
