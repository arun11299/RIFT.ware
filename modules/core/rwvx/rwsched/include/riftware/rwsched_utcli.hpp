/* 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 * */

/**
 * @file rwutcli_rwsched.hpp
 * @author Tom Seidenberg
 * @date 2014/06/05
 * @brief Unit test CLI that runs in RW.Sched environment
 */

#include "rwutcli.hpp"
#include "rwsched/cfrunloop.h"
#include "rwsched/cfsocket.h"

/**
 * Class creating a simple CLI for unittests
 */
class RwUtCliSched 
: public RwUtCli
{
 public:
  RwUtCliSched(rw_yang::YangModel& model, bool debug);
  ~RwUtCliSched();

 public:
  void rwsched_interactive(
    rwsched_instance_ptr_t rwsched_instance,
    rwsched_tasklet_ptr_t rwsched_tasklet_info);

 private:
  static void rwcli_io_callback(
    rwsched_CFSocketRef s,
    CFSocketCallBackType type,
    CFDataRef address,
    const void *data,
    void *info);

 private:

  rwsched_CFSocketRef cfsocket;
  rwsched_CFRunLoopSourceRef cfsource;
  rwsched_instance_ptr_t sched;
};

