
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwuagent_confd_upgrade.cpp
 *
 * Management agent confd upgrade support.
 */

#include "rwuagent.hpp"


using namespace rw_uagent;
using namespace rw_yang;


void rw_management_agent_confd_log_cb(void *user_data,
                                      rw_confd_log_level_e level,
                                      const char* fn,
                                      const char *log_msg)
{
  auto *inst = static_cast<Instance*>(user_data);

  switch (level) {
    case RW_CONFD_LOG_LEVEL_DEBUG:
      RW_MA_DOMMGR_LOG(inst, ConfdUpgradeDebug, fn, log_msg);
      break;
    case RW_CONFD_LOG_LEVEL_INFO:
      RW_MA_DOMMGR_LOG(inst, ConfdUpgradeNotice, fn, log_msg);
      break;
    case RW_CONFD_LOG_LEVEL_ERROR:
      RW_MA_DOMMGR_LOG(inst, ConfdUpgradeError, fn, log_msg);
      break;
    default:
      RW_ASSERT_NOT_REACHED();
  }
}


static void start_confd_upgrade_commit(void *ctxt)
{
  auto *inst = static_cast<Instance*>(ctxt);

  std::unique_ptr<rw_yang::ConfdUpgradeMgr> 
    u_mgr(inst->upgrade_ctxt_.upgrade_mgr_);

  if (!u_mgr->start_upgrade()) {
    inst->update_dyn_state(RW_MGMT_SCHEMA_APPLICATION_STATE_ERROR,
                           "confd upgrade failed");
    return;
  }

  if (!u_mgr->commit()) {
    inst->update_dyn_state(RW_MGMT_SCHEMA_APPLICATION_STATE_ERROR,
        "confd upgrade commit failed");
    return;
  }

  ++inst->upgrade_ctxt_.schema_version_;

  if (inst->perform_dynamic_schema_update() !=
               RW_STATUS_SUCCESS) {
    inst->update_dyn_state(RW_MGMT_SCHEMA_APPLICATION_STATE_ERROR,
                "Dynamic schema update failed");
    return;
  }

  // ATTN: does not look to be a good place to update this state
  inst->update_dyn_state(RW_MGMT_SCHEMA_APPLICATION_STATE_READY);

  std::string log_str;
  RW_MA_INST_LOG(inst, InstanceCritInfo, 
            (log_str="Upgrade commit complete. Schema version updated to = " +
                 std::to_string(inst->upgrade_ctxt_.schema_version_)).c_str());
  return;
}

static void start_confd_upgrade(void* ctxt)
{
  auto *inst = static_cast<Instance*>(ctxt);
  RW_MA_INST_LOG(inst, InstanceCritInfo, "Starting confd upgrade procedure.");

  // This will be released in start_confd_upgrade_commit
  auto *u_mgr = new rw_yang::ConfdUpgradeMgr(inst->upgrade_ctxt_.schema_version_, 
                                inst->confd_addr_,
                                inst->confd_addr_size_);

  inst->upgrade_ctxt_.upgrade_mgr_ = u_mgr;

  u_mgr->set_log_cb(rw_management_agent_confd_log_cb, inst);

  rwsched_dispatch_async_f(inst->rwsched_tasklet(),
                           inst->upgrade_ctxt_.serial_upgrade_q_, 
                           inst, start_confd_upgrade_commit);
}


void Instance::start_upgrade(size_t n_modules, char** module_name)
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "start upgrade");

  if (n_modules == 0) {
    return;
  }
  /* ATTN:
   * MAAPI api takes enum confd_proto
   * Did not see an identifier matching domain socket
   * enum confd_proto {
      CONFD_PROTO_UNKNOWN = 0,
      CONFD_PROTO_TCP = 1,
      CONFD_PROTO_SSH = 2,
      CONFD_PROTO_SYSTEM = 3, // ConfD initiated transactions
      CONFD_PROTO_CONSOLE = 4,
      CONFD_PROTO_SSL = 5,
      CONFD_PROTO_HTTP = 6,
      CONFD_PROTO_HTTPS = 7,
      CONFD_PROTO_UDP = 8 // SNMP sessions
     };
   *
   */

  if (confd_unix_socket_.size() > 0) {
    RW_MA_INST_LOG(this, InstanceError, "Upgrade not supported for UNIX Domain sockets");
    update_dyn_state(RW_MGMT_SCHEMA_APPLICATION_STATE_ERROR,
                     "UNIX domain socket not supported");
    return;
  }

  if (!upgrade_ctxt_.ready_for_upgrade_) {
    RW_MA_INST_LOG(this, InstanceError, "Upgrade will not be done due to consistency failure");
    update_dyn_state(RW_MGMT_SCHEMA_APPLICATION_STATE_ERROR,
                    "Consistency Failure");
    return;
  }

  rwsched_dispatch_async_f(rwsched_tasklet(),
                           rwsched_dispatch_get_main_queue(rwsched()),
                           this, start_confd_upgrade);

  return;
}
