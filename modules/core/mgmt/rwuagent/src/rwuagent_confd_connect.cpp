
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwuagent_confd_connect.cpp
 *
 * Management agent instance, confd server support.
 */

#include <rwvcs.h>
#include "rwuagent.hpp"


using namespace rw_uagent;
using namespace rw_yang;


static void
rw_uagent_confd_connection_retry_callback(void *user_context)
{
  Instance *instance = static_cast <Instance *> (user_context);
  instance->try_confd_connection();
  return;
}

void Instance::try_confd_connection()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "try confd connection",
           RWMEMLOG_ARG_PRINTF_INTPTR("try=%" PRIdPTR, (intptr_t)confd_connection_attempts_));
  std::string log_str;

  if (setup_confd_connection() == RW_STATUS_SUCCESS) {
    RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "setup conn failed");
    log_str = "RW.uagent - Connection succeded at try ";
    log_str += std::to_string(confd_connection_attempts_);
    RW_MA_INST_LOG(this, InstanceCritInfo, log_str.c_str());
    return;
  }

  confd_connection_attempts_++;

  log_str = "RW.uagent - Connection attempt at try ";
  log_str += std::to_string(confd_connection_attempts_);
  RW_MA_INST_LOG (this, InstanceNotice, log_str.c_str());

  rwsched_dispatch_after_f(
      rwsched_tasklet(),
      dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * RWUAGENT_RETRY_CONFD_CONNECTION_TIMEOUT_SEC),
      rwsched_dispatch_get_main_queue(rwsched()),
      this,
      rw_uagent_confd_connection_retry_callback);
}

rw_status_t Instance::setup_confd_connection()
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "setup confd connection");
  RW_ASSERT(confd_daemon_);

  if (!startup_handler_->is_instance_ready()) {
    RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "instance not ready");
    return RW_STATUS_FAILURE;
  }

  if (startup_handler_->initialize_conn() != RW_STATUS_SUCCESS) {
    RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "connection failed");
    RW_MA_INST_LOG(this, InstanceNotice, "RW.uAgent - startup_handler - maapi connection failed");
    return RW_STATUS_FAILURE;
  }

  rw_status_t ret = confd_daemon_->setup();
  if (ret != RW_STATUS_SUCCESS) {
    RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "daemon setup failed");
    RW_MA_INST_LOG(this, InstanceNotice, "RW.uAgent - confd_daemon_- setup failed");
    return RW_STATUS_FAILURE;
  }

  RW_MA_INST_LOG(this, InstanceNotice, "RW.uAgent - Moving on to subscription phase");
  startup_handler_->proceed_to_next_state();

  return RW_STATUS_SUCCESS;
}

rw_status_t Instance::setup_confd_subscription()
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "setup confd subscription");
  rw_status_t ret;

  RW_ASSERT(confd_addr_);
  
  if (confd_load_schemas(confd_addr_, confd_addr_size_) != CONFD_OK) {
    RW_MA_INST_LOG (this, InstanceNotice,
                    "RW.uAgent - load of  Confdb schema failed. Will retry");
    return RW_STATUS_FAILURE;
  }
  
  RW_MA_INST_LOG(this, InstanceDebug, "RW.uAgent - confd load schema done");
  
  // annotate the YANG model with tailf hash tags
  annotate_yang_model_confd ();
  
  ret = confd_config_->setup();
  if (ret != RW_STATUS_SUCCESS) {
    RW_MA_INST_LOG (this, InstanceNotice, "RW.uAgent - confd_config_ setup failed");
    return RW_STATUS_FAILURE;
  }

  uint32_t res = rw_yang::ConfdUpgradeMgr().get_max_version_linked();
  upgrade_ctxt_.schema_version_ = res;
 
  RW_MA_INST_LOG(this, InstanceInfo, "RW.uAgent - Version consistency check completed");

  RW_MA_INST_LOG(this, InstanceNotice, "RW.uAgent - Moving on to confd reload phase");
  startup_handler_->proceed_to_next_state();

  return ret;
}

void Instance::start_confd_reload()
{
  RW_MA_INST_LOG(this, InstanceCritInfo, "RW.uAgent - Starting with confd config reload");
  
  rwsched_dispatch_async_f(
                        rwsched_tasklet(),
                        confd_config_->reload_q_,
                        confd_config_,
                        NbReqConfdConfig::reload_configuration
       );
  return;
}

void Instance::annotate_yang_model_confd()
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "annotate yang model");

  namespace_map_t ns_map;
  struct confd_nsinfo *listp;

  uint32_t ns_count = confd_get_nslist(&listp);

  RW_ASSERT (ns_count); // for now

  for (uint32_t i = 0; i < ns_count; i++) {
    ns_map[listp[i].uri] = listp[i].hash;
  }

  yang_model()->annotate_nodes(ns_map, confd_str2hash,
                               YANGMODEL_ANNOTATION_CONFD_NS,
                               YANGMODEL_ANNOTATION_CONFD_NAME );
}

