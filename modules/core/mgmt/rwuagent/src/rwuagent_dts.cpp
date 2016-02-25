
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwuagent_dts.cpp
 *
 * Management agent RW.DTS member API - basic DTS integration.
 */

#include "rwuagent.hpp"
#include "rwuagent_nb_req.hpp"
#include "rwuagent_sb_req.hpp"
#include "rwuagent_confd.hpp"

using namespace rw_uagent;
using namespace rw_yang;


DtsMember::DtsMember(
  Instance* instance)
: instance_(instance),
  memlog_buf_(
    instance_->get_memlog_inst(),
    "DtsMember",
    reinterpret_cast<intptr_t>(this))
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "created");

  api_ = rwdts_api_new(
    instance_->rwtasklet(),
    instance_->ypbc_schema(),
    &DtsMember::state_change_cb,
    this,
    nullptr);
  RW_ASSERT(api_);
}

DtsMember::~DtsMember()
{
  for (auto reg : all_dts_regs_) {
    rwdts_member_deregister(reg);
  }
}

RWDtsFlag DtsMember::get_flags()
{
  auto flags = flags_;
  flags_ = RWDTS_FLAG_NONE;
  return flags;
}

bool DtsMember::is_ready() const
{
  return api_ && ready_;
}

void DtsMember::trace_next()
{
  flags_ = RWDTS_FLAG_TRACE;
}

void DtsMember::state_change_cb(
  rwdts_api_t* apih,
  rwdts_state_t state,
  void* ud)
{
  RW_ASSERT(ud);
  auto dts = static_cast<DtsMember*>(ud);
  dts->state_change(state);
}

void DtsMember::state_change(
  rwdts_state_t state )
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "state change",
    RWMEMLOG_ARG_PRINTF_INTPTR("s=%" PRIu64, state) );

  switch (state)
  {
    case RW_DTS_STATE_INIT:
      dts_state_init();
      rwdts_api_set_state(api_, RW_DTS_STATE_REGN_COMPLETE);
      break;
    case RW_DTS_STATE_REGN_COMPLETE:
      break;
    case RW_DTS_STATE_CONFIG:
      dts_state_config();
      rwdts_api_set_state(api_, RW_DTS_STATE_RUN);
      break;
    case RW_DTS_STATE_RUN:
      // set the dynamic schema state to ready
      instance_->update_dyn_state(RW_MGMT_SCHEMA_APPLICATION_STATE_READY);
      instance_->startup_hndl()->wait_for_critical_tasklets();
      break;
    default:
      RWMEMLOG(memlog_buf_, RWMEMLOG_ALWAYS, "bad state change!" );
      RW_MA_INST_LOG( instance_, InstanceError, "Invalid DTS state" );
      break;
  }
}

void DtsMember::register_rpc()
{
  /*
   * Setup command handler.
   */
  RWPB_M_PATHSPEC_DECL_INIT(RwMgmtagt_input_MgmtAgent, keyspec_rpc);
  rw_keyspec_path_t *keyspec = (rw_keyspec_path_t*)&keyspec_rpc;
  rw_keyspec_path_set_category(keyspec, NULL, RW_SCHEMA_CATEGORY_RPC_INPUT);

  rwdts_member_event_cb_t reg_cb;
  RW_ZERO_VARIABLE(&reg_cb);
  reg_cb.cb.prepare = &DtsMember::rpc_cb;
  reg_cb.ud = (void*)this;

  rpc_reg_ = rwdts_member_register(
    NULL,
    api_,
    keyspec,
    &reg_cb,
    RWPB_G_MSG_PBCMD(RwMgmtagt_input_MgmtAgent),
    RWDTS_FLAG_PUBLISHER,
    NULL );
  RW_ASSERT(rpc_reg_);
}


void DtsMember::dts_state_init() 
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "dts state init");
  load_registrations(instance_->yang_model());
  register_rpc();
  ready_ = true;
}

void DtsMember::load_registrations(YangModel *model)
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "load registrations");

  YangNode *root = model->get_root_node();
  RW_ASSERT(root);
  YangNode *first_level = root->get_first_child();
  if (!first_level) {
    return;
  }

  while (nullptr != first_level) {
    // ATTN: So, who handles top-level leafy things?
    if (first_level->is_leafy() ||
        !(first_level->is_config()
          || first_level->is_notification()))
    {
      first_level = first_level->get_next_sibling();
      continue;
    }

    // ATTN: These seem like bugs. RPC is top level
    RW_ASSERT(!first_level->is_rpc());

    auto ypbc = first_level->get_ypbc_msgdesc();
    RW_ASSERT(ypbc);

    rw_keyspec_path_t *ks = nullptr;
    rw_status_t rs = rw_keyspec_path_create_dup(
      ypbc->schema_path_value, nullptr, &ks);
    RW_ASSERT(RW_STATUS_SUCCESS == rs);
    RW_ASSERT(ks);

    rwdts_member_reg_handle_t reg;
    rwdts_member_event_cb_t reg_cb = {};
    reg_cb.ud = this;

    uint32_t flag;

    if (first_level->is_config()) {
      rw_keyspec_path_set_category (ks, nullptr, RW_SCHEMA_CATEGORY_CONFIG);
      reg_cb.cb.prepare = &DtsMember::get_config_cb;
      flag = RWDTS_FLAG_PUBLISHER;

    } else if(first_level->is_notification()) {
      rw_keyspec_path_set_category (ks, nullptr, RW_SCHEMA_CATEGORY_NOTIFICATION);
      reg_cb.cb.prepare = &DtsMember::notification_recv_cb;
      flag = RWDTS_FLAG_SUBSCRIBER;

    } else {
      RW_ASSERT_NOT_REACHED();
    }

    reg = rwdts_member_register(
                    nullptr,
                    api_,
                    ks,
                    &reg_cb,
                    ypbc->pbc_mdesc,
                    flag,
                    nullptr);

    all_dts_regs_.push_back(reg);

    rw_keyspec_path_free (ks, NULL);
    first_level = first_level->get_next_sibling();
  }
}

void DtsMember::dts_state_config()
{
  // ATTN: Recovery, reconciliation code goes here ..
}


rwdts_member_rsp_code_t DtsMember::notification_recv_cb(
    const rwdts_xact_info_t * xact_info,
    RWDtsQueryAction action,
    const rw_keyspec_path_t * keyspec,
    const ProtobufCMessage * msg,
    uint32_t credits,
    void * get_next_key)
{
  RW_ASSERT(xact_info);
  RW_ASSERT(xact_info->ud);
  
  auto self = static_cast<DtsMember*>(xact_info->ud);
  
  return self->instance_->confd_daemon_->send_notification(msg);
}

rwdts_member_rsp_code_t DtsMember::get_config_cb(
  const rwdts_xact_info_t* xact_info,
  RWDtsQueryAction action,
  const rw_keyspec_path_t* ks,
  const ProtobufCMessage* msg,
  uint32_t credits,
  void* getnext_ptr )
{
  RW_ASSERT(xact_info);
  RW_ASSERT(xact_info->ud);
  DtsMember *dts = static_cast<DtsMember*>( xact_info->ud );

  return dts->get_config(
    xact_info->xact, xact_info->queryh, (rw_keyspec_path_t*)ks );
}


rwdts_member_rsp_code_t DtsMember::get_config(
  rwdts_xact_t* xact,
  rwdts_query_handle_t qhdl,
  const rw_keyspec_path_t* ks )
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "send config data");

  char ks_str[256] = "";
  rw_keyspec_path_get_print_buffer(
    ks, NULL, instance_->ypbc_schema(), ks_str, sizeof(ks_str));
  std::string log_str = "KeySpec: ";
  log_str += ks_str;
  RWMEMLOG_BARE(memlog_buf_, RWMEMLOG_MEM2,
    RWMEMLOG_ARG_STRNCPY(RWMEMLOG_ARG_SIZE_MAX_BYTES,ks_str) );

  // From the DOM, find if there is anything at ks.
  std::list<XMLNode*> found;
  XMLNode *root = instance_->dom()->get_root_node();
  RW_ASSERT(root);
  rw_yang_netconf_op_status_t ncrs = root->find (ks, found);

  if ((RW_YANG_NETCONF_OP_STATUS_OK != ncrs) || (found.empty())) {
    log_str += " not found";
    RW_MA_INST_LOG (instance_, InstanceNotice, log_str.c_str());
    return RWDTS_ACTION_OK;
  }

  log_str += " found " + std::to_string(found.size()) + " entries";
  RW_MA_INST_LOG (instance_, InstanceDebug, log_str.c_str());

  // The simpler way to send responses to DTS is one at a time.
  ProtobufCMessage *resp_msg[1];
  rwdts_member_query_rsp_t rsp = {};
  rsp.n_msgs = 1;
  rsp.msgs = resp_msg;
  rsp.evtrsp = RWDTS_EVTRSP_ASYNC;

  size_t i = 0;
  size_t total = found.size();

  for (auto it = found.begin(); it != found.end(); it++) {
    rw_keyspec_path_t*  spec_key = nullptr;

    ncrs = (*it)->to_keyspec (&spec_key);
    if (RW_YANG_NETCONF_OP_STATUS_OK != ncrs) {
      continue;
    }

    rsp.ks = spec_key;

    ProtobufCMessage *message;
    ncrs = (*it)->to_pbcm (&message);

    if (ncrs != RW_YANG_NETCONF_OP_STATUS_OK) {
      RW_MA_INST_LOG (instance_, InstanceError, "Protobuf conversion failed");
      rw_keyspec_path_free (spec_key, nullptr);
      return RWDTS_ACTION_NOT_OK;
    }

    resp_msg[0] = message;

    i++;
    if (i == total) {
      rsp.evtrsp = RWDTS_EVTRSP_ACK;
    }
    rwdts_member_send_response (xact, qhdl, &rsp);
    rw_keyspec_path_free (spec_key, NULL );
    protobuf_c_message_free_unpacked (nullptr, message);
  }
  return RWDTS_ACTION_OK;
}

rwdts_member_rsp_code_t DtsMember::rpc_cb(
  const rwdts_xact_info_t* xact_info,
  RWDtsQueryAction action,
  const rw_keyspec_path_t* ks_in,
  const ProtobufCMessage* msg,
  uint32_t credits,
  void* getnext_ptr )
{
  (void)credits;
  (void)getnext_ptr;

  RW_ASSERT(xact_info);
  rwdts_xact_t *xact = xact_info->xact;
  RW_ASSERT_TYPE(xact, rwdts_xact_t);

  auto dts_mgr = static_cast<DtsMember*>( xact_info->ud );
  return dts_mgr->rpc( xact_info, action, ks_in, msg );
}

rwdts_member_rsp_code_t DtsMember::rpc(
  const rwdts_xact_info_t* xact_info,
  RWDtsQueryAction action,
  const rw_keyspec_path_t* ks_in,
  const ProtobufCMessage* msg )
{
  RWMEMLOG_TIME_SCOPE(memlog_buf_, RWMEMLOG_MEM2, "receive rpc");
  auto nbreq = new NbReqDts( instance_, xact_info, action, ks_in, msg );
  auto ss = nbreq->execute();

  if (StartStatus::Async == ss) {
    return RWDTS_ACTION_ASYNC;
  }
  return RWDTS_ACTION_OK;
}

