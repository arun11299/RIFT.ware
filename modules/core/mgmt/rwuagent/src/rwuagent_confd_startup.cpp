
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/*
* @file rwuagent_confd_startup.cpp
*
* Management agent startup phase handler
*/
#include <sstream>
#include <iostream>
#include <chrono>
#include <fstream>

#include <confd_lib.h>
#include <confd_cdb.h>
#include <confd_dp.h>
#include <confd_maapi.h>
#include "rwuagent_startup.hpp"
#include "rwuagent_confd.hpp"
#include "rw-vcs.pb-c.h"
#include "rw-base.pb-c.h"
#include "reaper_client.h"

using namespace rw_uagent;

using VcsInfo = RWPB_T_MSG(RwBase_data_Vcs_Info);

StartupHandler::StartupHandler(Instance* inst)
  : instance_(inst),
    memlog_buf_(
        inst->get_memlog_inst(),
        "StartupHandler",
        reinterpret_cast<intptr_t>(this) )
{
}

static void config_mgmt_start_cb(
  rwdts_xact_t* xact,
  rwdts_xact_status_t* xact_status,
  void* user_data)
{
  auto* self = static_cast<StartupHandler*>(user_data);
  self->config_mgmt_start( xact, xact_status );
}

static void retry_mgmt_start_cb(void* user_data)
{
  auto* self = static_cast<StartupHandler*>(user_data);
  self->start_mgmt_instance();
}

void StartupHandler::start_mgmt_instance()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "start management server" );

  RWPB_T_PATHSPEC(RwVcs_input_Vstart) start_act = *(RWPB_G_PATHSPEC_VALUE(RwVcs_input_Vstart));
  RWPB_M_MSG_DECL_INIT(RwVcs_input_Vstart, start_act_msg);

  start_act_msg.parent_instance = const_cast<char*>(instance_->rwvcs()->identity.rwvm_name);
  start_act_msg.component_name = const_cast<char*>(get_component_name());

  std::string log("Starting management instance. ");
  log += "Parent instance: " + std::string(start_act_msg.parent_instance)
       + ". Component name: " + start_act_msg.component_name;
  RW_MA_INST_LOG(instance_, InstanceCritInfo, log.c_str());

  auto* xact = rwdts_api_query_ks(instance_->dts_api(),
                                  (rw_keyspec_path_t*)&start_act,
                                  RWDTS_QUERY_RPC,
                                  RWDTS_XACT_FLAG_NOTRAN,
                                  config_mgmt_start_cb,
                                  this,
                                  &start_act_msg.base);
  RW_ASSERT(xact);
}

void StartupHandler::config_mgmt_start(
    rwdts_xact_t* xact,
    rwdts_xact_status_t* xact_status)
{
  RW_MA_INST_LOG(instance_, InstanceDebug, "confd vstart response");
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "confd vstart result",
           RWMEMLOG_ARG_PRINTF_INTPTR("st=%" PRIdPTR, (intptr_t)xact_status->status),
           RWMEMLOG_ARG_PRINTF_INTPTR("dts xact=0x%" PRIXPTR, (intptr_t)xact) );

  switch (xact_status->status) {
    case RWDTS_XACT_COMMITTED:
      RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "confd vstart complete");
      RW_MA_INST_LOG(instance_, InstanceCritInfo, "confd started");
      inst_ready_ = true;
      break;

    default:
    case RWDTS_XACT_FAILURE:
    case RWDTS_XACT_ABORTED: {
      RW_MA_INST_LOG(instance_, InstanceError, "Unable to start management instance, retrying");
      auto when = dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * 5);
      rwsched_dispatch_after_f(instance_->rwsched_tasklet(),
                               when,
                               rwsched_dispatch_get_main_queue(instance_->rwsched()),
                               this,
                               retry_mgmt_start_cb);
      break;
    }
  }
}


void StartupHandler::start_tasks_ready_timer()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "start readiness timer");
  rwsched_tasklet_ptr_t tasklet = instance_->rwsched_tasklet();
  RW_MA_INST_LOG(instance_, InstanceInfo, "Starting timer to wait for critical tasklets");

  tasks_ready_timer_ = rwsched_dispatch_source_create(
      tasklet,
      RWSCHED_DISPATCH_SOURCE_TYPE_TIMER,
      0,
      0,
      rwsched_dispatch_get_main_queue(instance_->rwsched()));

  rwsched_dispatch_source_set_event_handler_f(
      tasklet,
      tasks_ready_timer_,
      StartupHandler::tasks_ready_timer_expire_cb);

  rwsched_dispatch_set_context(
      tasklet,
      tasks_ready_timer_,
      this);

  rwsched_dispatch_source_set_timer(
      tasklet,
      tasks_ready_timer_,
      dispatch_time(DISPATCH_TIME_NOW, CRITICAL_TASKLETS_WAIT_TIME),
      0,
      0);

  rwsched_dispatch_resume(tasklet, tasks_ready_timer_);
}

void StartupHandler::tasks_ready_timer_expire_cb(void* ctx)
{
  RW_ASSERT(ctx);
  static_cast<StartupHandler*>(ctx)->tasks_ready_timer_expire();
}

void StartupHandler::tasks_ready_timer_expire()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "ready timer expired, start mgmt server anyway");
  RW_MA_INST_LOG(instance_, InstanceError,
                 "Critical tasks not ready after 5 minutes, continuing");
  tasks_ready();
}

void StartupHandler::stop_tasks_ready_timer()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "stop ready timer",
           RWMEMLOG_ARG_PRINTF_INTPTR("t=%" PRIdPTR, (intptr_t)tasks_ready_timer_));
  if (tasks_ready_timer_) {
    rwsched_tasklet_ptr_t tasklet = instance_->rwsched_tasklet();
    rwsched_dispatch_source_cancel(tasklet, tasks_ready_timer_);
    rwsched_dispatch_release(tasklet, tasks_ready_timer_);
  }
  tasks_ready_timer_ = nullptr;
}

void StartupHandler::tasks_ready_cb(void* ctx)
{
  RW_ASSERT(ctx);
  auto self = static_cast<StartupHandler*>(ctx);
  RW_MA_INST_LOG(self->instance_, InstanceCritInfo, "Critical tasklets are in running state.");

  self->tasks_ready();
}

void StartupHandler::tasks_ready()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "tasks ready");
  RW_ASSERT (!inst_ready_);

  stop_tasks_ready_timer();
  start_mgmt_instance();
}

rw_status_t StartupHandler::wait_for_critical_tasklets()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "wait for critical tasklets");
  RW_ASSERT (!inst_ready_);

  rw_status_t ret = RW_STATUS_SUCCESS;
  // Wait for critical tasklets to come in Running state
  RW_ASSERT(instance_->dts_api());
  
  start_tasks_ready_timer();
  cb_data_ = rwdts_api_config_ready_register(
                instance_->dts_api(),
                tasks_ready_cb,
                this);
  
  return ret;
}


//----------------------------------------------------------------------------------------------
//
void ConfdStartupHandler::start_confd_phase_1_cb(void* ctx)
{
  RW_ASSERT(ctx);
  static_cast<ConfdStartupHandler*>(ctx)->start_confd_phase_1();
}

void ConfdStartupHandler::start_confd_reload_cb(void* ctx)
{
  RW_ASSERT(ctx);
  static_cast<ConfdStartupHandler*>(ctx)->start_confd_reload();
}

void ConfdStartupHandler::start_confd_phase_2_cb(void* ctx)
{
  RW_ASSERT(ctx);
  static_cast<ConfdStartupHandler*>(ctx)->start_confd_phase_2();
}

#define CREATE_STATE(S1, S2, Func)                                      \
    std::make_pair(std::make_pair(S1, S2), &ConfdStartupHandler::Func)

#define ADD_STATE(S1, S2, Func)                 \
  state_mc_.insert(                             \
          CREATE_STATE(S1, S2, Func))

ConfdStartupHandler::ConfdStartupHandler(Instance* instance):
                     StartupHandler(instance)
{
  RWMEMLOG (memlog_buf_, RWMEMLOG_MEM2, "ConfdStartupHandler ctor");

  ADD_STATE (PHASE_0,       PHASE_1, start_confd_phase_1_cb);
  ADD_STATE (TRANSITIONING, PHASE_1, start_confd_phase_1_cb);
  ADD_STATE (PHASE_1,       RELOAD,  start_confd_reload_cb);
  ADD_STATE (RELOAD,        PHASE_2, start_confd_phase_2_cb);
  ADD_STATE (TRANSITIONING, PHASE_2, start_confd_phase_2_cb);
}

#undef CREATE_STATE
#undef ADD_STATE


void ConfdStartupHandler::create_proxy_manifest_config()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "create proxy manifest cfg" );
  RW_MA_INST_LOG(instance_, InstanceDebug, "Create manifest entry configuration");

  std::string error_out;
  rw_yang::XMLDocument::uptr_t req;
  std::string xml(manifest_cfg_);

  if (!instance_->unique_ws_) {
    char hostname[MAX_HOSTNAME_SZ];
    hostname[MAX_HOSTNAME_SZ - 1] = 0;
    int res = gethostname(hostname, MAX_HOSTNAME_SZ - 2);
    RW_ASSERT(res != -1);

    unsigned long seconds_since_epoch =
          std::chrono::duration_cast<std::chrono::seconds>
                  (std::chrono::system_clock::now().time_since_epoch()).count();

    std::ostringstream oss;
    oss << "confd_ws." << &hostname[0] << "." << seconds_since_epoch;
    confd_dir_ = std::move(oss.str());
  } else {
    confd_dir_ = instance_->mgmt_workspace_.c_str();
  }

  auto pos = xml.find('$');
  RW_ASSERT(pos != std::string::npos);
  xml.replace(pos, 1, confd_dir_.c_str());
  req = std::move(instance_->xml_mgr()->create_document_from_string(xml.c_str(), error_out, false));

  // ATTN: We are not publishing this data since
  // there is no subscriber for rw-manifest.
  // What happens is, when the vstart RPC is fired,
  // the specified parent instance reads this
  // configuration entry from uAgent
  // and creates a registration for it at runtime.
  instance_->dom()->merge(req.get());

  RW_MA_INST_LOG(instance_, InstanceInfo, "Manifest entry configuration created successfully");
}


rw_status_t ConfdStartupHandler::initialize_conn()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "initialize connection" );
  RW_ASSERT(curr_phase() == PHASE_0);
  std::string tmp_log;

  close_sockets();

  sock_ = socket(instance_->confd_addr_->sa_family, SOCK_STREAM | SOCK_CLOEXEC, 0);
  RW_ASSERT(sock_ >= 0);

  auto ret = maapi_connect(sock_, instance_->confd_addr_, instance_->confd_addr_size_);
  if (ret != CONFD_OK) {
    tmp_log="MAAPI connect failed: " + std::string(confd_lasterr()) + ". Retrying.";
    RW_MA_INST_LOG (instance_, InstanceError, tmp_log.c_str());
    RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "failed maapi_connect" );
    return RW_STATUS_FAILURE;
  }

  RW_MA_INST_LOG(instance_, InstanceNotice, "MAAPI connection succeeded");

  read_sock_ = socket(instance_->confd_addr_->sa_family, SOCK_STREAM, 0);
  RW_ASSERT(read_sock_ >= 0);

  ret = cdb_connect(read_sock_, CDB_READ_SOCKET, instance_->confd_addr_,
      instance_->confd_addr_size_);
  if (ret != CONFD_OK) {
    tmp_log="CDB read socket connection failed. Retrying. " + std::string(confd_lasterr());
    RW_MA_INST_LOG (instance_, InstanceError, tmp_log.c_str());
    RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "failed cdb_connect" );
    return RW_STATUS_FAILURE;
  }

  return RW_STATUS_SUCCESS;
}

void ConfdStartupHandler::close_sockets()
{
  if (sock_ >= 0) close(sock_);
  sock_ = -1;

  if (read_sock_ >= 0) close(read_sock_);
  read_sock_ = -1;
}

void ConfdStartupHandler::proceed_to_next_state()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "proceed to next state");
  RW_ASSERT(next_phase() != DONE);

  rwsched_dispatch_async_f(
      instance_->rwsched_tasklet(),
      rwsched_dispatch_get_main_queue(instance_->rwsched()),
      this,
      state_mc_[state_]);
}

void ConfdStartupHandler::retry_phase_cb(ConfdStartupHandler::CB cb)
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "retry phase cb");

  auto when = dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC / 2LL);
  rwsched_dispatch_after_f(
        instance_->rwsched_tasklet(),
        when,
        rwsched_dispatch_get_main_queue(instance_->rwsched()),
        this,
        cb);

  return;
}

#define OK_RETRY(retval, err_msg) \
  if (retval != CONFD_OK) { \
    const char* s_ = err_msg; \
    RW_MA_INST_LOG (instance_, InstanceError, s_); \
    RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "start confd err", \
             RWMEMLOG_ARG_STRCPY_MAX(s_,128) ); \
    return retry_phase_cb(cb); \
  }

#define SET_STATE(curr_state, next_state) \
  state_.first = curr_state;              \
  state_.second = next_state;

void ConfdStartupHandler::start_confd_phase_1()
{
  RWMEMLOG (memlog_buf_, RWMEMLOG_MEM2, "start confd phase 1");
  RW_MA_INST_LOG (instance_, InstanceInfo, "start confd phase 1");

  RW_ASSERT (next_phase() == PHASE_1);
  std::string tmp_log;

  auto cb = state_mc_[state_];
  RW_ASSERT (cb);

  if (curr_phase() == PHASE_0) {
    auto ret = maapi_start_phase(sock_, PHASE_1, 0/*async*/);
    OK_RETRY (ret,
              (tmp_log="Maapi start phase1 failed. Retrying. "
                        + std::string(confd_lasterr())).c_str());

    SET_STATE (TRANSITIONING, PHASE_1);
    return retry_phase_cb(cb);
  }

  if ((curr_phase() == TRANSITIONING) && (next_phase() == PHASE_1)) {
    struct cdb_phase cdb_phase;

    auto ret = cdb_get_phase(read_sock_, &cdb_phase);
    OK_RETRY (ret,
              (tmp_log="CDB get phase failed. Current phase 0. Retrying. "
                        + std::string(confd_lasterr())).c_str());

    if (cdb_phase.phase == PHASE_1) {
      SET_STATE (PHASE_1, RELOAD);
    } else {
      return retry_phase_cb(cb);
    }
  }

  RW_MA_INST_LOG(instance_, InstanceCritInfo,
      "Configuration management phase-1 finished. Starting config subscription");

  instance_->setup_confd_subscription();
}

void ConfdStartupHandler::start_confd_reload()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "start confd reload phase");
  RW_MA_INST_LOG(instance_, InstanceInfo, "start confd reload phase");
  RW_ASSERT (next_phase() == RELOAD);

  instance_->start_confd_reload();
  SET_STATE (RELOAD, PHASE_2);
}

void ConfdStartupHandler::start_confd_phase_2()
{
  RWMEMLOG (memlog_buf_, RWMEMLOG_MEM2, "start confd phase 2");
  RW_MA_INST_LOG (instance_, InstanceInfo, "start confd phase 2");

  RW_ASSERT (next_phase() == PHASE_2);
  std::string tmp_log;

  auto cb = state_mc_[state_];
  RW_ASSERT (cb);

  if (curr_phase() == RELOAD) {
    auto ret = maapi_start_phase(sock_, PHASE_2, 0/*async*/);
    OK_RETRY (ret,
        (tmp_log="Maapi start phase2 failed. Retrying. "
                 + std::string(confd_lasterr())).c_str());

    SET_STATE (TRANSITIONING, PHASE_2);
    return retry_phase_cb(cb);
  }

  if ((curr_phase() == TRANSITIONING) && (next_phase() == PHASE_2)) {
    struct cdb_phase cdb_phase;
    auto ret = cdb_get_phase(read_sock_, &cdb_phase);
    OK_RETRY (ret,
        (tmp_log="CDB get phase failed. Current phase 1. Retrying. "
                 + std::string(confd_lasterr())).c_str());

    if (cdb_phase.phase == PHASE_2) {
      SET_STATE (PHASE_2, DONE);
    } else {
      return retry_phase_cb(cb);
    }
  }

  std::string rift_root = getenv("RIFT_INSTALL");
  if (!rift_root.length()) {
    rift_root = "/";
  }
  auto confd_init_file = std::string(rift_root) + "/" + confd_dir_ +
    std::string("/") + CONFD_INIT_FILE;

  struct stat fst;
  if (stat(confd_init_file.c_str(), &fst) != 0) {
    RW_MA_INST_LOG (instance_, InstanceCritInfo,
        "Creating agent confd init file");
    std::fstream in(confd_init_file, std::ios::out);
    RW_ASSERT_MESSAGE(in.good(), "Failed to create confd init file");
    in.close();
  }

  RW_MA_INST_LOG(instance_, InstanceCritInfo,
      "Configuration management startup complete. Northbound interfaces are enabled.");
  return;
}

#undef OK_RETRY
#undef SET_STATE

// ATTN: What is this? - it is unused
std::vector<std::string> ConfdStartupHandler::get_log_files()
{
  const char* rift_root = getenv("RIFT_INSTALL");
  if (nullptr == rift_root) {
    rift_root = "/";
  }
  std::string confd_log_dir = std::string(rift_root) + "/" + confd_dir_ +
    "/usr/local/confd/var/confd/log/";

  std::vector<std::string> logs;
  logs.emplace_back(confd_log_dir + "confd.log");
  logs.emplace_back(confd_log_dir + "devel.log");
  logs.emplace_back(confd_log_dir + "netconf.log");

  return logs;
}

