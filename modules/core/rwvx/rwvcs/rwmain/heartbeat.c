
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 *
 */

#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <reaper_client.h>
#include <rwsched.h>
#include <rwtrace.h>
#include <rwvcs_rwzk.h>
#include <rwvx.h>

#include "rwmain.h"

// Maximum message size
#define HEARTBEAT_MSG_SIZE 1024

// Statistic history length
#define HEARTBEAT_HISTORY_SIZE 20

// Number of buckes to keep in the missed histogram
#define HEARTBEAT_HISTOGRAM_SIZE 100

// Time duration for each histogram bucket
#define HEARTBEAT_HISTOGRAM_DELTA 10

struct max_timer_cls;

struct stats {
  bool active;
  double recv_time;
  double poll_time;
  double send_time;
  uint32_t missed;
};

struct subscriber_cls {
  struct rwmain * rwmain;
  char * instance_name;
  mqd_t mqd;
  uint32_t missed;
  rwsched_CFRunLoopTimerRef timer;
  struct max_timer_cls * max_timer_cls;

  // Statistics
  double init_duration;
  size_t stat_id;
  struct stats stats[HEARTBEAT_HISTORY_SIZE];

  struct {
    uint32_t histogram[HEARTBEAT_HISTOGRAM_SIZE];
    size_t index;
    int started_at;
  } missed_histogram;
};

struct max_timer_cls {
  struct subscriber_cls * sub_cls;
  rwsched_CFRunLoopTimerRef timer;
};

struct publisher_cls {
  struct rwmain * rwmain;
  mqd_t mqd;
  rwsched_CFRunLoopTimerRef timer;
  rwsched_CFRunLoopTimerRef subscriber_timeout;

  // Statistics
  size_t stat_id;
  struct stats stats[HEARTBEAT_HISTORY_SIZE];
};

struct rwproc_heartbeat {
  uint16_t frequency;
  uint32_t tolerance;
  bool enabled;
  struct subscriber_cls ** subs;
  struct publisher_cls ** pubs;
};

/*
 * Get the current time
 */
static inline double current_time() {
  int r;
  struct timespec ts;

  r = clock_gettime(CLOCK_REALTIME, &ts);
  if (r)
    return -1;

  return (double)ts.tv_sec + (double)ts.tv_nsec / 1E9;
}

static void kill_process(struct subscriber_cls * cls)
{
  int r;
  rw_status_t status;
  rw_component_info ci;
  int wait_status;
  char * path;

  status = rwvcs_rwzk_lookup_component(cls->rwmain->rwvx->rwvcs, cls->instance_name, &ci);
  RW_ASSERT(status == RW_STATUS_SUCCESS || status == RW_STATUS_NOTFOUND);

  if (status != RW_STATUS_SUCCESS)
    return;

  r = kill(ci.proc_info->pid, SIGTERM);
  if (r != -1) {
    for (size_t i = 0; i < 1000; ++i) {
      r = kill(ci.proc_info->pid, 0);
      if (r == -1)
        break;
      usleep(1000);
    }

    if (r != -1)
      kill(ci.proc_info->pid, SIGKILL);
  }

  status = rwvcs_rwzk_update_state(cls->rwmain->rwvx->rwvcs, cls->instance_name, RW_BASE_STATE_TYPE_CRASHED);
  if (status != RW_STATUS_SUCCESS && status != RW_STATUS_NOTFOUND) {
    rwmain_trace_crit(
        cls->rwmain,
        "Failed to update %s state to CRASHED",
        cls->instance_name);
  }

  r = waitpid(ci.proc_info->pid, &wait_status, WNOHANG);
  if (r != ci.proc_info->pid) {
    rwmain_trace_crit(
        cls->rwmain,
        "Failed to wait for pid %u, instance-name %s",
        ci.proc_info->pid,
        cls->instance_name);
  }

  r = asprintf(&path, "/R/%s/%lu", ci.component_name, ci.instance_id);
  RW_ASSERT(r != -1);

  status = rwdts_member_deregister_path(cls->rwmain->dts, path);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  free(path);
  protobuf_free_stack(ci);
}

/* Fired after a maximum delay waiting for the subscriber to read enough
 * messages from the queue so that the publisher has room to continue to send
 * heartbeats.
 *
 * Timer added by the publisher when the queue is filled and removed by the
 * publisher as soon as there is room on the queue.
 */
static void on_subscriber_timeout(rwsched_CFRunLoopTimerRef timer, void * ctx)
{
  struct publisher_cls * cls;
  char * instance_name;
  rw_component_info self;
  rw_status_t status;


  cls = (struct publisher_cls *)ctx;

  rwmain_trace_crit(
      cls->rwmain,
      "Timeout waiting for subscriver to flush the message queue.  Exiting.");

  rwsched_tasklet_CFRunLoopTimerRelease(cls->rwmain->rwvx->rwsched_tasklet, cls->timer);

  instance_name = to_instance_name(cls->rwmain->component_name, cls->rwmain->instance_id);

  status = rwvcs_rwzk_update_state(cls->rwmain->rwvx->rwvcs, instance_name, RW_BASE_STATE_TYPE_CRASHED);
  if (status != RW_STATUS_SUCCESS)
    rwmain_trace_crit(cls->rwmain, "Failed to update component state");

  status = rwvcs_rwzk_lookup_component(
      cls->rwmain->rwvx->rwvcs,
      instance_name,
      &self);
  if (status != RW_STATUS_SUCCESS) {
    RW_ASSERT(0);
    exit(1);
  }

  status = rwmain_stop_instance(cls->rwmain, &self);
  if (status != RW_STATUS_SUCCESS) {
    RW_ASSERT(0);
    exit(1);
  }

  free(instance_name);
}

/*
 * Fired after a maximum delay waiting for the publisher to send the first
 * heartbeat.  Removes the subscriber timer and cleans out all allocated
 * heartbeat resources.
 *
 * Note this ideally this will never fire as the first message received by the
 * subscriber will trigger the remove of this timer.
 */
static void on_subscriber_delay_timeout(rwsched_CFRunLoopTimerRef timer, void * ctx)
{
  int r;
  struct max_timer_cls * cls;
  char * path;
  bool found;

  cls = (struct max_timer_cls *)ctx;

  rwmain_trace_crit(
      cls->sub_cls->rwmain,
      "Timeout waiting for first heartbeat from %s, assuming dead",
      cls->sub_cls->instance_name);

  rwsched_tasklet_CFRunLoopTimerRelease(
      cls->sub_cls->rwmain->rwvx->rwsched_tasklet,
      cls->sub_cls->timer);
  kill_process(cls->sub_cls);

  r = asprintf(&path, "/%s", cls->sub_cls->instance_name);
  if (r != -1) {
    mq_unlink(path);
    free(path);
  }

  found = false;
  for (size_t i = 0; cls->sub_cls->rwmain->rwproc_heartbeat->subs[i]; ++i) {
    if (cls->sub_cls->rwmain->rwproc_heartbeat->subs[i] == cls->sub_cls)
      found = true;

    if (found)
      cls->sub_cls->rwmain->rwproc_heartbeat->subs[i] = cls->sub_cls->rwmain->rwproc_heartbeat->subs[i+1];
  }

  // We could realloc cls->rwmain->rwproc_heartbeat->subs here to fit the new
  // size, but why bother?  We're hanging out to a tiny bit of memory and if
  // something is added we'll just have to realloc then.  Better to skip the
  // risk of memory errors.

  free(cls->sub_cls->instance_name);
  mq_close(cls->sub_cls->mqd);
  free(cls->sub_cls);
  free(cls);
}

static void check_mq_heartbeat(rwsched_CFRunLoopTimerRef timer, void * ctx)
{
  struct subscriber_cls * cls;
  char buf[HEARTBEAT_MSG_SIZE+1];
  ssize_t read;
  bool got_beat = false;
  size_t sent_id;
  double now = 0.0;

  cls = (struct subscriber_cls *)ctx;

  if (likely(!cls->stats[cls->stat_id].active)) {
    cls->stats[cls->stat_id].active = true;
    cls->stats[cls->stat_id].poll_time = current_time();
  }

  // Make sure to flush the queue each time.
  while (true) {
    read = mq_receive(cls->mqd, buf, HEARTBEAT_MSG_SIZE+1, NULL);
    if (read == -1) {
      RW_ASSERT(errno == EAGAIN);
      break;
    }

    RW_ASSERT(read == sizeof(size_t));
    memcpy(&sent_id, buf, sizeof(size_t));

    if (unlikely(sent_id != cls->stat_id)) {
      RW_ASSERT(0);
      // Not sure how this could happen, but we better check.  This should be
      // clear in the stats as we'll have some set which have a poll time but
      // no recv time and no missed beats.  Then we've skipped to the one with
      // zero for a poll time.

      cls->stats[cls->stat_id].recv_time = 0;
      cls->stats[cls->stat_id].active = false;
      cls->stat_id = sent_id;
      cls->stats[cls->stat_id].poll_time = 0;
    }

    cls->stats[cls->stat_id].recv_time = current_time();
    cls->stats[cls->stat_id].missed = cls->missed;
    cls->stats[cls->stat_id].active = false;

    cls->missed = 0;
    if (++cls->stat_id >= HEARTBEAT_HISTORY_SIZE)
      cls->stat_id = 0;

    cls->stats[cls->stat_id].recv_time = 0;
    cls->stats[cls->stat_id].missed = 0;
    cls->stats[cls->stat_id].poll_time = 0;
    cls->stats[cls->stat_id].active = false;

    got_beat = true;
  }

  now = current_time();

  if (unlikely(got_beat && cls->max_timer_cls != NULL)) {
    cls->init_duration = now - cls->init_duration;

    rwsched_tasklet_CFRunLoopTimerRelease(
        cls->rwmain->rwvx->rwsched_tasklet,
        cls->max_timer_cls->timer);
    free(cls->max_timer_cls);
    cls->max_timer_cls = NULL;
  }

  if (unlikely(now - cls->missed_histogram.started_at > HEARTBEAT_HISTOGRAM_DELTA)) {
    if (++cls->missed_histogram.index >= HEARTBEAT_HISTOGRAM_SIZE - 1)
      cls->missed_histogram.index = 0;
    cls->missed_histogram.started_at = now;
    cls->missed_histogram.histogram[cls->missed_histogram.index] = 0;
  }

  if (unlikely(!got_beat && !cls->max_timer_cls)) {
    cls->missed++;
    cls->missed_histogram.histogram[cls->missed_histogram.index]++;

    if (cls->missed >= cls->rwmain->rwproc_heartbeat->tolerance) {
      bool found;

      rwmain_trace_crit(
          cls->rwmain,
          "%u missed heartbeats from %s, assuming dead",
          cls->missed,
          cls->instance_name);

      rwsched_tasklet_CFRunLoopTimerRelease(cls->rwmain->rwvx->rwsched_tasklet, timer);
      kill_process(cls);

      found = false;
      for (size_t i = 0; cls->rwmain->rwproc_heartbeat->subs[i]; ++i) {
        if (cls->rwmain->rwproc_heartbeat->subs[i] == cls)
          found = true;

        if (found)
          cls->rwmain->rwproc_heartbeat->subs[i] = cls->rwmain->rwproc_heartbeat->subs[i+1];
      }

      free(cls->instance_name);
      mq_close(cls->mqd);
      free(cls);
      exit(-2);
    }
  }
}

static void publish_mq_heartbeat(rwsched_CFRunLoopTimerRef timer, void * ctx)
{
  int r;
  struct publisher_cls  * cls;
  struct mq_attr attr;


  cls = (struct publisher_cls *)ctx;

  r = mq_getattr(cls->mqd, &attr);
  if (unlikely(r != 0)) {
    int e = errno;

    rwmain_trace_crit(
        cls->rwmain,
        "Failed to get mq attributes: %s",
        strerror(e));
    RW_ASSERT(0);
  }

  if (unlikely(attr.mq_maxmsg == attr.mq_curmsgs)) {
    if (unlikely(!cls->subscriber_timeout && !getenv("RIFT_PROC_HEARTBEAT_NO_REVERSE"))) {
      rwsched_CFRunLoopTimerContext cf_context = { 0, NULL, NULL, NULL, NULL };

      cf_context.info = cls;

      cls->subscriber_timeout = rwsched_tasklet_CFRunLoopTimerCreate(
          cls->rwmain->rwvx->rwsched_tasklet,
          kCFAllocatorDefault,
          CFAbsoluteTimeGetCurrent() + RWVCS_HEARTBEAT_DELAY,
          0,
          0,
          0,
          on_subscriber_timeout,
          &cf_context);

      rwsched_tasklet_CFRunLoopAddTimer(
          cls->rwmain->rwvx->rwsched_tasklet,
          rwsched_tasklet_CFRunLoopGetCurrent(cls->rwmain->rwvx->rwsched_tasklet),
          cls->subscriber_timeout,
          cls->rwmain->rwvx->rwsched->main_cfrunloop_mode);
    }
    return;
  }

  if (unlikely(cls->subscriber_timeout != NULL)) {
    rwsched_tasklet_CFRunLoopTimerRelease(cls->rwmain->rwvx->rwsched_tasklet, cls->subscriber_timeout);
    cls->subscriber_timeout = NULL;
  }

  r = mq_send(cls->mqd, (char *)&cls->stat_id, sizeof(cls->stat_id), 0);
  if (unlikely(r && errno != EAGAIN)) {
    int e = errno;

    rwmain_trace_crit(
        cls->rwmain,
        "Failed to send heartbeat: %s",
        strerror(e));
    RW_ASSERT(0);
  }

  cls->stats[cls->stat_id].send_time = current_time();
  cls->stat_id++;
  if (cls->stat_id >= HEARTBEAT_HISTORY_SIZE)
    cls->stat_id = 0;
}

struct rwproc_heartbeat * rwproc_heartbeat_alloc(uint32_t frequency, uint32_t tolerance)
{
  struct rwproc_heartbeat * hb;

  hb = (struct rwproc_heartbeat *)malloc(sizeof(struct rwproc_heartbeat));
  if (!hb) {
    RW_ASSERT(0);
    goto err;
  }

  hb->frequency = frequency;
  hb->tolerance = tolerance;
  hb->enabled = true;

  hb->subs = (struct subscriber_cls **)malloc(sizeof(struct subscriber_cls *));
  if (!hb->subs) {
    RW_ASSERT(0);
    goto err;
  }
  hb->subs[0] = NULL;

  hb->pubs = (struct publisher_cls **)malloc(sizeof(struct publisher_cls *));
  if (!hb->pubs) {
    RW_ASSERT(0);
    goto err;
  }
  hb->pubs[0] = NULL;

  goto done;

err:
  if (hb->subs)
    free(hb->subs);

  if (hb->pubs)
    free(hb->pubs);

  free(hb);
  hb = NULL;

done:
    return hb;

}

void rwproc_heartbeat_free(struct rwproc_heartbeat * rwproc_heartbeat)
{
  int r;
  char * path;

  for (size_t i = 0; rwproc_heartbeat->subs[i]; ++i) {
    struct subscriber_cls * cls = rwproc_heartbeat->subs[i];

    if (cls->max_timer_cls) {
     rwsched_tasklet_CFRunLoopTimerRelease(
          cls->rwmain->rwvx->rwsched_tasklet,
          cls->max_timer_cls->timer);
     free(cls->max_timer_cls);
    }

    if (cls->timer)
      rwsched_tasklet_CFRunLoopTimerRelease(
          cls->rwmain->rwvx->rwsched_tasklet,
          cls->timer);

    r = asprintf(&path, "/%s", cls->instance_name);
    if (r != -1) {
      mq_unlink(path);
      free(path);
    }

    free(cls->instance_name);
    mq_close(cls->mqd);
    free(cls->max_timer_cls);
    free(cls);
  }
  free(rwproc_heartbeat->subs);

  for (size_t i = 0; rwproc_heartbeat->pubs[i]; ++i) {
    struct publisher_cls * cls = rwproc_heartbeat->pubs[i];

    if (cls->timer)
      rwsched_tasklet_CFRunLoopTimerRelease(
          cls->rwmain->rwvx->rwsched_tasklet,
          cls->timer);

    mq_close(cls->mqd);
    free(cls);
  }
  free(rwproc_heartbeat->pubs);

  free(rwproc_heartbeat);
}

rw_status_t rwproc_heartbeat_subscribe(
    struct rwmain * rwmain,
    const char * instance_name)
{
  int r;
  mqd_t mqd = -1;
  struct mq_attr mq_attrs;
  mode_t mask;
  char * path = NULL;
  char * full_path = NULL;
  rw_status_t status;
  rwsched_CFRunLoopTimerRef cftimer;
  rwsched_CFRunLoopTimerContext cf_context = { 0, NULL, NULL, NULL, NULL };
  struct subscriber_cls * cls = NULL;
  struct max_timer_cls * max_timer_cls = NULL;
  size_t subs_end;

  mq_attrs.mq_flags = O_NONBLOCK;
  mq_attrs.mq_maxmsg = 10;
  mq_attrs.mq_msgsize = HEARTBEAT_MSG_SIZE;

  r = asprintf(&path, "/%s", instance_name);
  RW_ASSERT(r != -1);

  r = asprintf(&full_path, "/dev/mqueue/%s", instance_name);
  RW_ASSERT(r != -1);

  cls = (struct subscriber_cls *)malloc(sizeof(struct subscriber_cls));
  RW_ASSERT(cls);
  if (!cls) {
    status = RW_STATUS_FAILURE;
    goto err;
  }
  bzero(cls, sizeof(struct subscriber_cls));

  max_timer_cls = (struct max_timer_cls *)malloc(sizeof(struct max_timer_cls));
  RW_ASSERT(max_timer_cls);
  if (!max_timer_cls) {
    status = RW_STATUS_FAILURE;
    goto err;
  }
  bzero(max_timer_cls, sizeof(struct max_timer_cls));

  // The process may not be running as the same user.
  mask = umask(0);
  mqd = mq_open(
      path,
      O_RDONLY|O_CREAT|O_NONBLOCK,
      S_IRWXU|S_IRWXG|S_IRWXO,
      &mq_attrs);
  umask(mask);
  if (mqd == -1) {
    int e = errno;

    rwmain_trace_crit(
        rwmain,
        "Failed to create mq %s: %s",
        path,
        strerror(e));
    RW_ASSERT(0);
    status = RW_STATUS_FAILURE;
    goto err;
  }

  r = reaper_client_add_path(rwmain->rwvx->rwvcs->reaper_sock, full_path);
  if (r) {
    rwmain_trace_crit(rwmain, "Failed to add %s to reaper", path);
    RW_ASSERT(0);
    status = RW_STATUS_FAILURE;
    goto err;
  }

  cls->rwmain = rwmain;
  cls->instance_name = strdup(instance_name);
  RW_ASSERT(cls->instance_name);
  cls->mqd = mqd;
  cls->max_timer_cls = max_timer_cls;
  cls->init_duration = current_time();
  cls->stat_id = 0;
  cf_context.info = cls;

  cftimer = rwsched_tasklet_CFRunLoopTimerCreate(
      rwmain->rwvx->rwsched_tasklet,
      kCFAllocatorDefault,
      CFAbsoluteTimeGetCurrent() + 1.0,
      1.0 / (double)rwmain->rwproc_heartbeat->frequency,
      0,
      0,
      check_mq_heartbeat,
      &cf_context);
  cls->timer = cftimer;

  rwsched_tasklet_CFRunLoopAddTimer(
      rwmain->rwvx->rwsched_tasklet,
      rwsched_tasklet_CFRunLoopGetCurrent(rwmain->rwvx->rwsched_tasklet),
      cftimer,
      rwmain->rwvx->rwsched->main_cfrunloop_mode);


  max_timer_cls->sub_cls = cls;
  cf_context.info = max_timer_cls;

  cftimer = rwsched_tasklet_CFRunLoopTimerCreate(
      rwmain->rwvx->rwsched_tasklet,
      kCFAllocatorDefault,
      CFAbsoluteTimeGetCurrent() + RWVCS_HEARTBEAT_DELAY,
      0,
      0,
      0,
      on_subscriber_delay_timeout,
      &cf_context);
  max_timer_cls->timer = cftimer;

  rwsched_tasklet_CFRunLoopAddTimer(
      rwmain->rwvx->rwsched_tasklet,
      rwsched_tasklet_CFRunLoopGetCurrent(rwmain->rwvx->rwsched_tasklet),
      cftimer,
      rwmain->rwvx->rwsched->main_cfrunloop_mode);

  for (subs_end = 0; rwmain->rwproc_heartbeat->subs[subs_end]; ++subs_end) {;}

  rwmain->rwproc_heartbeat->subs = (struct subscriber_cls **)realloc(
      rwmain->rwproc_heartbeat->subs,
      (subs_end + 2) * sizeof(struct subscriber_cls *));
  RW_ASSERT(rwmain->rwproc_heartbeat->subs);
  rwmain->rwproc_heartbeat->subs[subs_end] = cls;
  rwmain->rwproc_heartbeat->subs[subs_end + 1] = NULL;

  status = RW_STATUS_SUCCESS;
  goto done;

err:
  if (cls) {
    if (cls->instance_name)
      free(cls->instance_name);
    free(cls);
  }

  if (max_timer_cls)
    free(max_timer_cls);

  if (mqd != -1) {
    mq_close(mqd);
    mq_unlink(path);
  }

done:
  if (path)
    free(path);

  if (full_path)
    free(full_path);

  return status;
}

rw_status_t rwproc_heartbeat_publish(
    struct rwmain * rwmain,
    const char * instance_name)
{
  int r;
  mqd_t mqd;
  char * path = NULL;
  rw_status_t status;
  rwsched_CFRunLoopTimerRef cftimer;
  rwsched_CFRunLoopTimerContext cf_context = { 0, NULL, NULL, NULL, NULL };
  struct publisher_cls * cls;
  size_t pubs_end;

  r = asprintf(&path, "/%s", instance_name);
  RW_ASSERT(r != -1);

  cls = (struct publisher_cls *)malloc(sizeof(struct publisher_cls));
  RW_ASSERT(cls);
  if (!cls) {
    status = RW_STATUS_FAILURE;
    goto done;
  }
  bzero(cls, sizeof(struct publisher_cls));

  mqd = mq_open(path, O_WRONLY|O_NONBLOCK, NULL);
  if (mqd == -1) {
    int e = errno;

    rwmain_trace_crit(
        rwmain,
        "Failed to open mq %s: %s",
        path,
        strerror(e));
    RW_ASSERT(0);
    status = RW_STATUS_FAILURE;
    goto done;
  }

  // There will only ever be two ends to this queue.  It was created by the
  // subscriber and now we have the other end.  We want to make sure the queue
  // does not persist after both ends are closed so we can unlink it now.  This
  // will remove the queue name and once both file descriptors to the queue are
  // closed, will destroy the queue.
  r = mq_unlink(path);
  if (r == -1) {
    int e = errno;
    rwmain_trace_crit(
        rwmain,
        "Failed to unlink mq %s: %s",
        path,
        strerror(e));
    RW_ASSERT(0);
  }

  cls->rwmain = rwmain;
  cls->mqd = mqd;
  cls->stat_id = 0;
  cf_context.info = cls;

  cftimer = rwsched_tasklet_CFRunLoopTimerCreate(
      rwmain->rwvx->rwsched_tasklet,
      kCFAllocatorDefault,
      CFAbsoluteTimeGetCurrent() + 0.1,
      1.0 / (double)rwmain->rwproc_heartbeat->frequency,
      0,
      0,
      publish_mq_heartbeat,
      &cf_context);
  cls->timer = cftimer;

  rwsched_tasklet_CFRunLoopAddTimer(
      rwmain->rwvx->rwsched_tasklet,
      rwsched_tasklet_CFRunLoopGetCurrent(rwmain->rwvx->rwsched_tasklet),
      cftimer,
      rwmain->rwvx->rwsched->main_cfrunloop_mode);

  for (pubs_end = 0; rwmain->rwproc_heartbeat->pubs[pubs_end]; ++pubs_end) {;}

  rwmain->rwproc_heartbeat->pubs = (struct publisher_cls **)realloc(
      rwmain->rwproc_heartbeat->pubs,
      (pubs_end + 2) * sizeof(struct publisher_cls *));
  RW_ASSERT(rwmain->rwproc_heartbeat->pubs);
  rwmain->rwproc_heartbeat->pubs[pubs_end] = cls;
  rwmain->rwproc_heartbeat->pubs[pubs_end + 1] = NULL;

  status = RW_STATUS_SUCCESS;

done:
  if (path)
    free(path);

  if (status != RW_STATUS_SUCCESS && cls)
    free(cls);

  return status;
}

rw_status_t rwproc_heartbeat_reset(
    struct rwmain * rwmain,
    uint16_t frequency,
    uint32_t tolerance,
    bool enabled)
{
  struct rwproc_heartbeat * rwproc_heartbeat;

  rwproc_heartbeat = rwmain->rwproc_heartbeat;

  for (size_t i = 0; rwproc_heartbeat->subs[i]; ++i) {
    rwsched_CFRunLoopTimerRef cftimer;
    rwsched_CFRunLoopTimerContext cf_context = { 0, NULL, NULL, NULL, NULL };
    struct subscriber_cls * cls = rwproc_heartbeat->subs[i];

    if (cls->timer) {
      rwsched_tasklet_CFRunLoopTimerRelease(
          cls->rwmain->rwvx->rwsched_tasklet,
          cls->timer);
      cls->timer = NULL;
    }

    if (!enabled) {
      if (cls->max_timer_cls) {
        rwsched_tasklet_CFRunLoopTimerRelease(
            cls->rwmain->rwvx->rwsched_tasklet,
            cls->max_timer_cls->timer);
        cls->max_timer_cls = NULL;
      }
      continue;
    }

    cf_context.info = cls;

    cftimer = rwsched_tasklet_CFRunLoopTimerCreate(
        rwmain->rwvx->rwsched_tasklet,
        kCFAllocatorDefault,
        CFAbsoluteTimeGetCurrent(),
        1.0 / (double)frequency,
        0,
        0,
        check_mq_heartbeat,
        &cf_context);
    cls->timer = cftimer;

    rwsched_tasklet_CFRunLoopAddTimer(
        rwmain->rwvx->rwsched_tasklet,
        rwsched_tasklet_CFRunLoopGetCurrent(rwmain->rwvx->rwsched_tasklet),
        cftimer,
        rwmain->rwvx->rwsched->main_cfrunloop_mode);
  }

  for (size_t i = 0; rwproc_heartbeat->pubs[i]; ++i) {
    rwsched_CFRunLoopTimerRef cftimer;
    rwsched_CFRunLoopTimerContext cf_context = { 0, NULL, NULL, NULL, NULL };
    struct publisher_cls * cls = rwproc_heartbeat->pubs[i];

    if (cls->timer) {
      rwsched_tasklet_CFRunLoopTimerRelease(
          cls->rwmain->rwvx->rwsched_tasklet,
          cls->timer);
      cls->timer = NULL;
    }

    if (!enabled) {
      continue;
    }

    cf_context.info = cls;

    cftimer = rwsched_tasklet_CFRunLoopTimerCreate(
        rwmain->rwvx->rwsched_tasklet,
        kCFAllocatorDefault,
        CFAbsoluteTimeGetCurrent(),
        1.0 / (double)frequency,
        0,
        0,
        publish_mq_heartbeat,
        &cf_context);
    cls->timer = cftimer;

    rwsched_tasklet_CFRunLoopAddTimer(
        rwmain->rwvx->rwsched_tasklet,
        rwsched_tasklet_CFRunLoopGetCurrent(rwmain->rwvx->rwsched_tasklet),
        cftimer,
        rwmain->rwvx->rwsched->main_cfrunloop_mode);
  }

  rwproc_heartbeat->frequency = frequency;
  rwproc_heartbeat->tolerance = tolerance;
  rwproc_heartbeat->enabled = enabled;

  return RW_STATUS_SUCCESS;
}

void rwproc_heartbeat_settings(
    struct rwmain * rwmain,
    uint16_t * frequency,
    uint32_t * tolerance,
    bool * enabled)
{
  *frequency = rwmain->rwproc_heartbeat->frequency;
  *tolerance = rwmain->rwproc_heartbeat->tolerance;
  *enabled = rwmain->rwproc_heartbeat->enabled;
}

rw_status_t rwproc_heartbeat_stats(
    struct rwmain * rwmain,
    rwproc_heartbeat_stat *** stats,
    size_t * n_stats)
{
  rwproc_heartbeat_stat ** ret;
  struct rwproc_heartbeat * hb;
  size_t max_stats;
  size_t current_stat;


  *n_stats = 0;
  *stats = NULL;
  hb = rwmain->rwproc_heartbeat;

  for (max_stats = 0; hb->subs[max_stats]; ++max_stats) {;}
  for (size_t i = 0; hb->pubs[i]; ++max_stats, ++i) {;}

  if (max_stats == 0)
    return RW_STATUS_SUCCESS;

  ret = (rwproc_heartbeat_stat **)malloc(max_stats * sizeof(void *));
  if (!ret) {
    RW_ASSERT(0);
    goto err;
  }
  bzero(ret, max_stats * sizeof(void *));

  for (size_t i = 0; i < max_stats; ++i) {
    ret[i] = (rwproc_heartbeat_stat *)malloc(sizeof(rwproc_heartbeat_stat));
    if (!ret[i]) {
      RW_ASSERT(0);
      goto free_stats;
    }

    rwproc_heartbeat_stat__init(ret[i]);

    ret[i]->timing = (rwproc_heartbeat_timing **)malloc(HEARTBEAT_HISTORY_SIZE * sizeof(void *));
    if (!ret[i]->timing) {
      RW_ASSERT(0);
      goto free_stats;
    }

    for (size_t j = 0; j < HEARTBEAT_HISTORY_SIZE; ++j) {
      ret[i]->timing[j] = (rwproc_heartbeat_timing *)malloc(sizeof(rwproc_heartbeat_timing));
      if (!ret[i]->timing[j]) {
        RW_ASSERT(0);
        goto free_stats;
      }
      rwproc_heartbeat_timing__init(ret[i]->timing[j]);
    }
    ret[i]->n_timing = HEARTBEAT_HISTORY_SIZE;
  }

  for (current_stat = 0; hb->subs[current_stat]; ++current_stat) {
    struct subscriber_cls * cls = hb->subs[current_stat];
    rwproc_heartbeat_stat * stat = ret[current_stat];
    size_t index;
    size_t array_end;

    if (!cls->max_timer_cls) {
      stat->init_duration = cls->init_duration;
      stat->has_init_duration = true;
    }

    stat->instance_name = strdup(cls->instance_name);
    if (!stat->instance_name) {
      RW_ASSERT(0);
      goto free_stats;
    }

    for (size_t j = 0; j < HEARTBEAT_HISTORY_SIZE; ++j) {
      stat->timing[j]->id = j;
      stat->timing[j]->has_recv_time = true;
      stat->timing[j]->has_missed = true;
      stat->timing[j]->recv_time = cls->stats[j].recv_time;
      stat->timing[j]->missed = cls->stats[j].missed;
    }

    stat->missed_histogram = (rwproc_heartbeat_histogram *)malloc(sizeof(rwproc_heartbeat_histogram));
    if (!stat->missed_histogram) {
      RW_ASSERT(0);
      goto free_stats;
    }
    rwproc_heartbeat_histogram__init(stat->missed_histogram);
    stat->missed_histogram->has_interval_duration = true;
    stat->missed_histogram->interval_duration = HEARTBEAT_HISTOGRAM_DELTA;
    stat->missed_histogram->n_histogram = HEARTBEAT_HISTOGRAM_SIZE;
    stat->missed_histogram->histogram = (uint32_t *)malloc(HEARTBEAT_HISTOGRAM_SIZE  * sizeof(uint32_t));
    if (!stat->missed_histogram->histogram) {
      RW_ASSERT(0);
      goto free_stats;
    }

    index = cls->missed_histogram.index;
    array_end = HEARTBEAT_HISTOGRAM_SIZE - 1;
    for (size_t j = 0; j + index < array_end; ++j)
      stat->missed_histogram->histogram[j] = cls->missed_histogram.histogram[j + index + 1];
    for (size_t j = 0; j <= index; j++)
      stat->missed_histogram->histogram[array_end - index + j] = cls->missed_histogram.histogram[j];
  }

  for (size_t i = 0; hb->pubs[i]; ++i) {
    struct publisher_cls * cls = hb->pubs[i];
    rwproc_heartbeat_stat * stat = ret[current_stat];

    stat->instance_name = to_instance_name(rwmain->component_name, rwmain->instance_id);
    if (!stat->instance_name) {
      RW_ASSERT(0);
      goto free_stats;
    }

    for (size_t j = 0; j < HEARTBEAT_HISTORY_SIZE; ++j) {
      stat->timing[j]->id = j;
      stat->timing[j]->has_send_time = true;
      stat->timing[j]->send_time = cls->stats[j].send_time;
    }

    current_stat++;
  }

  *stats = ret;
  *n_stats = max_stats;

  return RW_STATUS_SUCCESS;


free_stats:
  for (size_t i = 0; ret[i]; ++i) {
    if (ret[i]->timing) {
      for (size_t j = 0; j < HEARTBEAT_HISTORY_SIZE && ret[i]->timing[j]; j++)
        protobuf_free(ret[i]->timing[j]);
    }

    free(ret[i]->timing);
    ret[i]->timing = NULL;
    ret[i]->n_timing = 0;
    protobuf_free(ret[i]);
  }

  free(ret);

err:
    return RW_STATUS_FAILURE;
}

