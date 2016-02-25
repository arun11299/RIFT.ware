
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwuagent_instance.cpp
 *
 * Management agent instance.
 */

#include <cstdio> // for sprintf
#include <memory>


#include "rwvcs.h"
#include "rw_pb_schema.h"
 
#include "rwuagent.hpp"
#include "rwuagent_msg_client.h"

using namespace rw_uagent;
using namespace rw_yang;


RW_CF_TYPE_DEFINE("RW.uAgent RWTasklet Component Type", rwuagent_component_t);
RW_CF_TYPE_DEFINE("RW.uAgent RWTasklet Instance Type", rwuagent_instance_t);

const char *rw_uagent::uagent_yang_ns = "http://riftio.com/ns/riftware-1.0/rw-mgmtagt";

rwuagent_component_t rwuagent_component_init(void)
{
  rwuagent_component_t component = (rwuagent_component_t)RW_CF_TYPE_MALLOC0(sizeof(*component), rwuagent_component_t);
  RW_CF_TYPE_VALIDATE(component, rwuagent_component_t);

  return component;
}

void rwuagent_component_deinit(
    rwuagent_component_t component)
{
  RW_CF_TYPE_VALIDATE(component, rwuagent_component_t);
}

rwuagent_instance_t rwuagent_instance_alloc(
    rwuagent_component_t component,
    struct rwtasklet_info_s * rwtasklet_info,
    RwTaskletPlugin_RWExecURL *instance_url)
{
  // Validate input parameters
  RW_CF_TYPE_VALIDATE(component, rwuagent_component_t);
  RW_ASSERT(instance_url);

  // Allocate a new rwuagent_instance structure
  rwuagent_instance_t instance = (rwuagent_instance_t) RW_CF_TYPE_MALLOC0(sizeof(*instance), rwuagent_instance_t);
  RW_CF_TYPE_VALIDATE(instance, rwuagent_instance_t);
  instance->component = component;

  // Save the rwtasklet_info structure
  instance->rwtasklet_info = rwtasklet_info;

  // Allocate the real instance structure
  instance->instance = new Instance(instance);

  // Return the allocated instance
  return instance;
}

void rwuagent_instance_free(
    rwuagent_component_t component,
    rwuagent_instance_t instance)
{
  // Validate input parameters
  RW_CF_TYPE_VALIDATE(component, rwuagent_component_t);
  RW_CF_TYPE_VALIDATE(instance, rwuagent_instance_t);
}

rwtrace_ctx_t* rwuagent_get_rwtrace_instance(
    rwuagent_instance_t instance)
{
  RW_CF_TYPE_VALIDATE(instance, rwuagent_instance_t);
  return instance->rwtasklet_info ? instance->rwtasklet_info->rwtrace_instance : NULL;
}

void rwuagent_instance_start(
    rwuagent_component_t component,
    rwuagent_instance_t instance)
{
  RW_CF_TYPE_VALIDATE(component, rwuagent_component_t);
  RW_CF_TYPE_VALIDATE(instance, rwuagent_instance_t);
  RW_ASSERT(instance->component == component);
  RW_ASSERT(instance->instance);
  instance->instance->start();
}

void rw_management_agent_xml_log_cb(void *user_data,
                                    rw_xml_log_level_e level,
                                    const char *fn,
                                    const char *log_msg)
{
  /*
   * ATTN: These messages need to get back to the transaction that
   * generated them, so that they can be included in the NETCONF error
   * response, if any.
   *
   * ATTN: I think RW.XML needs more context when generating messages -
   * just binding the errors to manage is insufficient - need to
   * actually bind the messages to a particular client/xact.
   */
  auto *inst = static_cast<Instance*>(user_data);

  switch (level) {
    case RW_XML_LOG_LEVEL_DEBUG:
      RW_MA_DOMMGR_LOG (inst, DommgrDebug, fn, log_msg);
      break;
    case RW_XML_LOG_LEVEL_INFO:
      RW_MA_DOMMGR_LOG (inst, DommgrNotice, fn, log_msg);
      break;
    case RW_XML_LOG_LEVEL_ERROR:
      RW_MA_DOMMGR_LOG (inst, DommgrError, fn, log_msg);
      break;
    default:
      RW_ASSERT_NOT_REACHED();
  }
}

/**
 * Construct a uAgent instance.
 */
Instance::Instance(rwuagent_instance_t rwuai)
    : memlog_inst_("MgmtAgent", 200),
      memlog_buf_(
          memlog_inst_,
          "Instance",
          reinterpret_cast<intptr_t>(this)),
      rwuai_(rwuai),
      //ATTN: startup_hndler could be either for Confd or
      // libnetconf server. Currently defaulting to 
      // Confd.
      initializing_composite_schema_(true),
      startup_handler_(new ConfdStartupHandler(this))      
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "Instance constructor");
  RW_CF_TYPE_VALIDATE(rwuai_, rwuagent_instance_t);

  // ATTN: stdout?  is that for error logs?  Don't we want to capture that?
  confd_init ("rwUagent", stdout, CONFD_DEBUG);

  upgrade_ctxt_.serial_upgrade_q_ = rwsched_dispatch_queue_create(
      rwsched_tasklet(),
      "upgrade-queue",
      RWSCHED_DISPATCH_QUEUE_SERIAL);

  schema_load_q_ = rwsched_dispatch_queue_create(
      rwsched_tasklet(),
      "schema-load-queue",
      RWSCHED_DISPATCH_QUEUE_SERIAL);
}

/**
 * Destroy a uAgent instance.
 */
Instance::~Instance()
{
  // ATTN: close the messaging services and channels

  // ATTN: Iterate through all the Clients, destroying them

  rwmemlog_instance_dts_deregister(memlog_inst_, false/*dts_internal*/);

  // De-register the memlog instance of the xmlmgr.
  rwmemlog_instance_dts_deregister(xml_mgr_->get_memlog_inst(), false/*dts_internal*/);

  RW_CF_TYPE_VALIDATE(rwuai_, rwuagent_instance_t);
}

void Instance::async_start(void *ctxt)
{
  auto* self = reinterpret_cast<Instance*>(ctxt);
  auto& memlog_buf = self->memlog_buf_;

  RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "setup dom" );
  self->setup_dom("rw-mgmtagt-composite");

  RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "load schema" );
  // Load the schema specified at boot time
  self->ypbc_schema_ = rw_load_schema("librwuagent_yang_gen.so", "rw-mgmtagt-composite");
  RW_ASSERT(self->ypbc_schema_);

  RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "register schema" );
  self->yang_model()->load_schema_ypbc(self->ypbc_schema_);
  self->yang_model()->register_ypbc_schema(self->ypbc_schema_);

  rwsched_dispatch_async_f(self->rwsched_tasklet(),
                           rwsched_dispatch_get_main_queue(self->rwsched()),
                           self,
                           Instance::async_start_dts);
}


void Instance::async_start_dts(void *ctxt)
{
  auto* self = reinterpret_cast<Instance*>(ctxt);
  auto& memlog_buf = self->memlog_buf_;

  RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "start dts member" );
  self->dts_ = new DtsMember(self);

  RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "register rwmemlog" );
  rwmemlog_instance_dts_register( self->memlog_inst_,
                                  self->rwtasklet(),
                                  self->dts_->api() );

  RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "register XMLMgr rwmemlog instance" );
  rwmemlog_instance_dts_register( self->xml_mgr_->get_memlog_inst(),
                                  self->rwtasklet(),
                                  self->dts_->api() );

  // ATTN: Move this to a manager?
  RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "start dynamic schema driver" );
  self->schema_driver_.reset( new DynamicSchemaDriver(self, self->dts_->api()) );
  rwsched_dispatch_async_f(self->rwsched_tasklet(), // ATTN: Should be in constr?
                           rwsched_dispatch_get_main_queue(self->rwsched()),
                           self->schema_driver_.get(),
                           DynamicSchemaDriver::run);

  RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "register schema listener" );
  if (AgentDynSchemaHelper::register_for_dynamic_schema(self) != RW_STATUS_SUCCESS) {
    RW_MA_INST_LOG(self, InstanceError, "Error while registering for dynamic schema.");
    return;
  } 

  if (self->confd_addr_) {
    RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "configure confd proc" );
    self->startup_handler_->create_proxy_manifest_config();
  }

  // Instantiate the rw-msg interfaces
  RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "start messaging server" );
  self->msg_client_ = new MsgClient(self);

  rwsched_dispatch_async_f(self->rwsched_tasklet(),
                           rwsched_dispatch_get_main_queue(self->rwsched()),
                           self,
                           Instance::async_start_confd);

}

void Instance::async_start_confd(void* ctxt)
{
  auto* self = reinterpret_cast<Instance*>(ctxt);
  auto& memlog_buf = self->memlog_buf_;

  if (self->initializing_composite_schema_) {
    // still loading composite schema, try again
    RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "waiting on composite to start confd" );
    rwsched_dispatch_async_f(self->rwsched_tasklet(),
                             rwsched_dispatch_get_main_queue(self->rwsched()),
                             self,
                             Instance::async_start_confd);
    return;
  }
 
  // initialize confd 
  if (self->confd_addr_) {

    RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "start confd data provider" );
    self->confd_config_ = new NbReqConfdConfig(self);

    RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "start confd daemon" );
    self->confd_daemon_ = new ConfdDaemon(self);

    RWMEMLOG( memlog_buf, RWMEMLOG_MEM2, "attempt confd connection" );
    self->try_confd_connection();
  }
}


void Instance::dyn_schema_dts_registration(void *ctxt)
{
  auto *self = reinterpret_cast<Instance*>(ctxt);
  
  while (self->tmp_models_.size() > 0) {
    self->dts_->load_registrations(self->tmp_models_.back().get());
    self->tmp_models_.pop_back();
  }
  self->initializing_composite_schema_ = false;
}

void Instance::start()
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "start agent");

  rw_status_t rwstatus;
  char cmdargs_var_str[] = "$cmdargs_str";
  char cmdargs_str[1024];
  int argc = 0;
  char **argv = NULL;
  gboolean ret;

  auto vcs_inst = rwvcs();
  if(   vcs_inst
        && vcs_inst->pb_rwmanifest
        && vcs_inst->pb_rwmanifest->bootstrap_phase
        && vcs_inst->pb_rwmanifest->bootstrap_phase->rwbaseschema
        && vcs_inst->pb_rwmanifest->bootstrap_phase->rwbaseschema->schema_name) {
    yang_schema_ = vcs_inst->pb_rwmanifest->bootstrap_phase->rwbaseschema->schema_name;
  } else {
    yang_schema_ = "rw-composite";
  }

  yang_schema_ = "rwuagent";

  std::string log_string;
  RW_MA_INST_LOG(this, InstanceInfo, (log_string = "Schema configured is " + yang_schema_).c_str());

  // HACK: This eventually need to come from management agent
  rwstatus = rwvcs_variable_evaluate_str(rwvcs(),
                                         cmdargs_var_str,
                                         cmdargs_str,
                                         sizeof(cmdargs_str));
  RW_ASSERT(RW_STATUS_SUCCESS == rwstatus);

  std::string log_str = "cmdargs_str";
  log_str += cmdargs_str;

  RW_MA_INST_LOG(this, InstanceCritInfo, log_str.c_str());

  ret = g_shell_parse_argv(cmdargs_str, &argc, &argv, NULL);
  RW_ASSERT(ret == TRUE);

  for (int j = 0; j < argc; ++j) {
    const char* arg = argv[j];
    RW_ASSERT(arg);
    char* cmd = nullptr;
    char* data = nullptr;
    char eos = '\0';
    int st = sscanf(arg, "%m[a-z_]:%ms%c", &cmd, &data, &eos);
    bool ok = false;
    if (st == 2 && cmd && cmd[0] && data && data[0]) {
      RW_ASSERT(cmd);
      RW_ASSERT(data);
      if (0 == strcmp (cmd, "module")) {
        ok = true;
        modules_.push_back (data);
        // load the module specified
      } else if (0 == strcmp (cmd, "confd_ws")) {
        ok = true;
        unique_ws_ = true;
        mgmt_workspace_ = data;
      }else if (0 == strcmp (cmd, "confd")) {
        unsigned v = 0;
        st = sscanf(data, "%u%c", &v, &eos);
        if (st == 1 && v > 0 && v < 65536) {
          confd_port_ = v;
          ok = true;
        } else if (0 == strcmp(data,"UID")) {
          confd_port_ = getuid() + CONFD_PORT;
          ok = true;
        } else if (0 == strcmp (data, "DEFAULT")) {
          confd_port_ = CONFD_PORT;
          ok = true;
        } else if (0 == strncmp (data, "AF_UNIX:", strlen("AF_UNIX:"))) {
          confd_unix_socket_ = data + strlen ("AF_UNIX:");
          if (   confd_unix_socket_.length()
                 && confd_unix_socket_.length() < sizeof (confd_unix_addr_.sun_path)) {
            memset (&confd_unix_addr_, 0, sizeof (confd_unix_addr_));
            strcpy (confd_unix_addr_.sun_path, confd_unix_socket_.c_str());
            confd_unix_addr_.sun_family = AF_UNIX;

            confd_addr_ = (struct sockaddr *) &confd_unix_addr_;
            confd_addr_size_ = sizeof (confd_unix_addr_);
            ok = true;
          }
        } else if (0 == strncmp(data, "AF_INET:", sizeof("AF_INET:")-1)) {
          char* ipstr = data + sizeof("AF_INET:")-1;
          char* portstr = strchr(ipstr, ':');
          if (portstr) {
            *portstr = 0;
            ++portstr;
            st = inet_aton(ipstr, &confd_inet_addr_.sin_addr);
            if (st != 0) {
              st = sscanf(portstr, "%u%c", &v, &eos);
              if (st == 1 && v > 0 && v < 65536) {
                confd_inet_addr_.sin_port = htons(v);
                ok = true;
              } else if (0 == strcmp(portstr,"UID")) {
                confd_inet_addr_.sin_port = htons(getuid() + CONFD_PORT);
                ok = true;
              } else if (0 == strcmp(portstr, "DEFAULT")) {
                confd_inet_addr_.sin_port = htons(CONFD_PORT);
                ok = true;
              }
              if (ok) {
                confd_inet_addr_.sin_family = AF_INET;
                confd_addr_ = (struct sockaddr *)&confd_inet_addr_;
                confd_addr_size_ = sizeof(confd_inet_addr_);
              }
            }
          }
        }
      } else if ((0 == strcmp (cmd, "ipport")) ||
                 (0 == strcmp (cmd, "sockpath")) ) {
        ok = true;
        // Not being used currently
      }

      if (confd_port_ && !confd_addr_) {
        confd_inet_addr_.sin_family = AF_INET;
        confd_inet_addr_.sin_port = htons(confd_port_);
        confd_inet_addr_.sin_addr.s_addr = INADDR_ANY;

        confd_addr_ = (struct sockaddr *) &confd_inet_addr_;
        confd_addr_size_ = sizeof (confd_inet_addr_);
      }
    }
    if (data) {
      free(data);
    }
    if (cmd) {
      free(cmd);
    }

    if (!ok) {
      log_str = "Bad uAgent init command in manifest";
      log_str += arg;

      RW_MA_INST_LOG(this, InstanceError, log_str.c_str());
    }
  }

  // set rift environment variables for rw.cli to connect to confd
  // ATTN: this should be read from the manifest when the agent generates confd.conf at runtime (RIFT-5059)
  rw_status_t const status = rw_setenv("NETCONF_PORT_NUMBER","2022");

  if (status != RW_STATUS_SUCCESS) {
    RW_MA_INST_LOG(this,
                   InstanceError,
                   "Couldn't set NETCONF port number in Rift environment variable");
  }

  // Set the instance name
  instance_name_ = rwtasklet()->identity.rwtasklet_name;

  // Create a concurrent dispatch queue for multi-threading.
  cc_dispatchq_ = rwsched_dispatch_queue_create(
      rwsched_tasklet(), "uagent-cc-queue", RWSCHED_DISPATCH_QUEUE_CONCURRENT);

  RW_ASSERT(cc_dispatchq_);

  /* ATTN: Can we free the argv strings */
  // g_strfreev(argv);
  //
  rwsched_dispatch_async_f(rwsched_tasklet(),
                           schema_load_q_,
                           this,
                           Instance::async_start);
}


rw_status_t Instance::handle_dynamic_schema_update(const int batch_size,
                                                   const char * const * module_names,
                                                   const char * const * so_filenames)
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "handle dynamic schema");

  rw_status_t status = RW_STATUS_SUCCESS;
  update_dyn_state(RW_MGMT_SCHEMA_APPLICATION_STATE_WORKING);

  for (int i = 0; i < batch_size; ++i) {
    RW_ASSERT(module_names[i]);
    RW_ASSERT(so_filenames[i]);

    pending_schema_modules_.emplace_front(std::make_pair(module_names[i], so_filenames[i]));
  }
  // start confd in-service upgrade
  // ATTN: Signature to be changed based upon
  // the future of CLI command for upgrade
  if (initializing_composite_schema_) {
    // loading composite schema on boot
    perform_dynamic_schema_update();
  } else {   
    // normal dynamic schema operation
    start_upgrade(1, nullptr);
  }
  RW_MA_INST_LOG(this, InstanceInfo, "Dynamic schema update callback completed");

  return status;
}


rw_status_t Instance::perform_dynamic_schema_update()
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "perform dynamic schema");

  tmp_models_.reserve(pending_schema_modules_.size());                      

  for (const auto& mod_pair : pending_schema_modules_) {
    auto module_name = mod_pair.first.c_str();
    auto so_filename = mod_pair.second.c_str();

    const rw_yang_pb_schema_t* new_schema =  rw_load_schema(so_filename, module_name);
    RW_ASSERT(new_schema);
    
    // Scope for registering new keys in the module with
    // DTS
    auto *tmp_model = rw_yang::YangModelNcx::create_model();
    RW_ASSERT(tmp_model);
      
    tmp_model->load_schema_ypbc(new_schema);
    tmp_model->register_ypbc_schema(new_schema);
    tmp_models_.emplace_back(tmp_model);
    
    // Create new schema
    auto *merged_schema = rw_schema_merge(nullptr, ypbc_schema_, new_schema);

    if (!merged_schema) {
      std::string log_str;
      RW_MA_INST_LOG(this, InstanceError,
                     (log_str=std::string("Dynamic schema update for ")
                      + module_name + so_filename+ " failed").c_str());

      return RW_STATUS_FAILURE;
    }
  
    RW_MA_INST_LOG(this, InstanceDebug, "Load and merge completed.");

    load_module(module_name);
    // Overwrite the old schema with new
    ypbc_schema_ = merged_schema;
  }

  xml_mgr_->get_yang_model()->load_schema_ypbc(ypbc_schema_);
  xml_mgr_->get_yang_model()->register_ypbc_schema(ypbc_schema_);

  rwdts_api_set_ypbc_schema( dts_api(), ypbc_schema_ );

  rwsched_dispatch_async_f(rwsched_tasklet(),
                           rwsched_dispatch_get_main_queue(rwsched()),
                           this,
                           Instance::dyn_schema_dts_registration);

  pending_schema_modules_.clear();

  return fill_in_confd_info();
}

rw_status_t Instance::fill_in_confd_info()
{
  if (!initializing_composite_schema_
      && confd_load_schemas(confd_addr_, confd_addr_size_) != CONFD_OK) {
    RW_MA_INST_LOG (this, InstanceNotice,
                    "RW.uAgent - load of  Confdb schema failed.");
    return RW_STATUS_FAILURE;
  }

  bool ret = xml_mgr_->get_yang_model()->app_data_get_token(
      YANGMODEL_ANNOTATION_KEY, 
      YANGMODEL_ANNOTATION_CONFD_NS,
      &xml_mgr_->get_yang_model()->adt_confd_ns_);
  RW_ASSERT(ret);

  ret = xml_mgr_->get_yang_model()->app_data_get_token(
      YANGMODEL_ANNOTATION_KEY, 
      YANGMODEL_ANNOTATION_CONFD_NAME,
      &xml_mgr_->get_yang_model()->adt_confd_name_);
  RW_ASSERT(ret);

  if (!initializing_composite_schema_) {
    annotate_yang_model_confd();
  }

  return RW_STATUS_SUCCESS;
}

void Instance::close_cf_socket(rwsched_CFSocketRef s)
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "close cf socket");
  rwsched_instance_ptr_t sched = rwsched();
  rwsched_tasklet_ptr_t tasklet = rwsched_tasklet();
  rwsched_CFRunLoopRef runloop = rwsched_tasklet_CFRunLoopGetCurrent(tasklet);

  cf_src_map_t::iterator src = cf_srcs_.find (s);
  if (src == cf_srcs_.end()) {
    RW_MA_INST_LOG (this, InstanceError, "CF Socket not found in map");
    return;
  }

  rwsched_tasklet_CFRunLoopRemoveSource(tasklet,runloop,src->second,sched->main_cfrunloop_mode);
  cf_srcs_.erase(src);

  rwsched_tasklet_CFSocketRelease(tasklet, s);
}

void Instance::setup_dom(const char *module_name)
{
  // ATTN: Split this func. XMLMgr and model stuff belongs in instance constr.
  //    ... the doc create and model load belong in separate funcs, and async
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "setup dom");

  RW_MA_INST_LOG (this, InstanceDebug, "Setting up configuration dom");
  RW_ASSERT (!xml_mgr_.get()); // ATTN:- Assumption that there is a single instance.
  xml_mgr_ = std::move(xml_manager_create_xerces());
  xml_mgr_->set_log_cb(rw_management_agent_xml_log_cb,this);

  // register app data for confd hash registration
  auto model = yang_model();
  bool ret = model->app_data_get_token(YANGMODEL_ANNOTATION_KEY, YANGMODEL_ANNOTATION_CONFD_NS,
                                       &model->adt_confd_ns_);
  RW_ASSERT(ret);

  ret =  model->app_data_get_token(YANGMODEL_ANNOTATION_KEY, YANGMODEL_ANNOTATION_CONFD_NAME,
                                   &model->adt_confd_name_);
  RW_ASSERT(ret);

  auto *module = load_module(module_name);
  module->mark_imports_explicit();

  dom_ = std::move(xml_mgr_->create_document());
}

YangModule* Instance::load_module(const char* module_name)
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "load new module",
                      RWMEMLOG_ARG_STRNCPY(32, module_name) );
  RW_MA_INST_LOG(this, InstanceDebug, "Reloading dom with new module");

  YangModule *module = yang_model()->load_module(module_name);
  // ATTN: Should not crash!
  RW_ASSERT(module);

  //ATTN: Need to call mark_imports_explicit for
  //dynamically loaded modules ??
  // locked_cache_set_flag_only

  return module;
}

static inline void recalculate_mean (uint32_t *mean,
                                     uint32_t old_count,
                                     uint32_t new_value)
{
  RW_ASSERT(mean);
  uint64_t sum = (*mean) * old_count;
  *mean = (sum + new_value)/(old_count + 1);
}


void Instance::update_stats(RwMgmtagt_SbReqType type,
                            const char *req,
                            RWPB_T_MSG(RwMgmtagt_SpecificStatistics_ProcessingTimes) *sbreq_stats)
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM7, "update stats");
  RW_ASSERT(type < RW_MGMTAGT_SB_REQ_TYPE_MAXIMUM);

  if (nullptr == statistics_[type].get()) {
    statistics_[type].reset(new OperationalStats());
    RWPB_F_MSG_INIT (RwMgmtagt_Statistics, &statistics_[type]->statistics);
    statistics_[type]->statistics.operation = type;
    gettimeofday (&statistics_[type]->start_time_, 0);
    statistics_[type]->statistics.has_processing_times = 1;
    statistics_[type]->statistics.has_request_count = 1;
    statistics_[type]->statistics.has_parsing_failed = 1;

  }

  OperationalStats *stats = statistics_[type].get();

  RW_ASSERT(stats->commands.size() <= RWUAGENT_MAX_CMD_STATS);

  if (RWUAGENT_MAX_CMD_STATS == stats->commands.size()) {
    stats->commands.pop_front();
  }
  CommandStat t;
  t.request = req;
  t.statistics = *sbreq_stats;

  stats->commands.push_back (t);
  stats->statistics.request_count++;
  // Update the instance level stats
  if (!sbreq_stats->has_transaction_start_time) {
    stats->statistics.parsing_failed++;
    return;
  }

  // the success count has to be the old success count for recalulating mean
  uint32_t success = stats->statistics.request_count - stats->statistics.parsing_failed - 1;

  RWPB_T_MSG(RwMgmtagt_Statistics_ProcessingTimes) *pt =
      &stats->statistics.processing_times;

  if (!success) {
    // set all the present flags
    pt->has_request_parse_time = 1;
    pt->has_transaction_start_time = 1;
    pt->has_dts_response_time = 1;
    pt->has_response_parse_time = 1;
  }

  recalculate_mean (&pt->request_parse_time, success, sbreq_stats->request_parse_time);
  recalculate_mean (&pt->transaction_start_time, success, sbreq_stats->transaction_start_time);
  recalculate_mean (&pt->dts_response_time, success, sbreq_stats->dts_response_time);
  recalculate_mean (&pt->response_parse_time, success, sbreq_stats->response_parse_time);
}

