/* 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 * */

/**
 * @file rwsched_utcli.cpp
 * @author Tom Seidenberg
 * @date 2014/06/05
 * @brief Unit test CLI that runs in RW.Sched environment
 */

#ifndef RW_YANGPBC_ENABLE_UTCLI
#define RW_YANGPBC_ENABLE_UTCLI
#endif

#include "rwsched_utcli.hpp"
#include <stdio.h>
#include "../src/rwsched_internal.h"

using namespace rw_yang;

/**
 * Create a RwUtCliSched object.
 */
RwUtCliSched::RwUtCliSched(YangModel& model, bool debug)
: RwUtCli(model, debug),
  cfsocket(),
  cfsource(),
  sched()
{
}

/**
 * Destroy RwUtCliSched object and all related objects.
 */
RwUtCliSched::~RwUtCliSched()
{
#if 0 // FIXME ???
  if (sched) {
    rwsched_tasklet_CFRunLoopSourceInvalidate( sched, cfsource );
    rwsched_tasklet_CFSocketInvalidate( sched, cfsocket );
    rwsched_tasklet_CFSocketReleaseRunLoopSource( sched, cfsource );
    rwsched_tasklet_CFSocketRelease( sched, cfsocket );
  }
#endif
}

/**
 * Create an interactive CLI bound to a scheduler queue.
 */
void RwUtCliSched::rwsched_interactive(
  rwsched_instance_ptr_t rwsched_instance,
  rwsched_tasklet_ptr_t rwsched_tasklet_info)
{
  // ATTN: This is a hack because of recursive main_function()
  if (editline_) {
    return;
  }

  RW_ASSERT(!editline_);
  RW_ASSERT(!sched || sched == rwsched_instance);

  editline_ =  new CliEditline( *this, stdin, stdout, stderr );
  editline_->nonblocking_mode();

  // Create a CFSocket for standard input
  // Example of using CFSocket to manage file descriptors on a runloop using CFStreamCreatePairWithSocket()
  // http://lists.apple.com/archives/macnetworkprog/2003/Jul/msg00075.html

  // Use the pollbits to determine which socket callback events to register
  CFOptionFlags cf_callback_flags = kCFSocketReadCallBack;

  // Create a CFSocket as a runloop source for the toyfd file descriptor
  CFSocketContext cf_context = { 0, nullptr, nullptr, nullptr, nullptr };
  cf_context.info = (void*)this;
  cfsocket = rwsched_tasklet_CFSocketCreateWithNative(
    rwsched_tasklet_info,
    kCFAllocatorSystemDefault,
    0,
    cf_callback_flags,
    rwcli_io_callback,
    &cf_context);
  RW_CF_TYPE_VALIDATE(cfsocket, rwsched_CFSocketRef);

  CFOptionFlags cf_option_flags = kCFSocketAutomaticallyReenableReadCallBack;
  rwsched_tasklet_CFSocketSetSocketFlags(rwsched_tasklet_info, cfsocket, cf_option_flags);
  cfsource = rwsched_tasklet_CFSocketCreateRunLoopSource(
    rwsched_tasklet_info,
    kCFAllocatorSystemDefault,
    cfsocket,
    0);
  RW_CF_TYPE_VALIDATE(cfsource, rwsched_CFRunLoopSourceRef);

  rwsched_CFRunLoopRef runloop = rwsched_tasklet_CFRunLoopGetCurrent(rwsched_tasklet_info);
  rwsched_tasklet_CFRunLoopAddSource(rwsched_tasklet_info, runloop, cfsource, kCFRunLoopDefaultMode);
}

/**
 * Callback from scheduler when IO is ready.
 */
void RwUtCliSched::rwcli_io_callback(
  rwsched_CFSocketRef s,
  CFSocketCallBackType type,
  CFDataRef address,
  const void *data,
  void *info)
{
  RW_CF_TYPE_VALIDATE(s, rwsched_CFSocketRef);
  UNUSED(type);
  UNUSED(address);
  UNUSED(data);

  RwUtCliSched *cli = (RwUtCliSched *)info;
  cli->editline_->read_ready();
}

