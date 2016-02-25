
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwuagent_sb_req_rpc.cpp
 *
 * Management agent southbound request support for RPC
 */

#include "rwuagent.hpp"

using namespace rw_uagent;
using namespace rw_yang;

extern StartStatus read_logs_and_send(Instance*, SbReqRpc*,
     const RWPB_T_MSG(RwMgmtagt_input_ShowAgentLogs)* );

SbReqRpc::SbReqRpc(
  Instance* instance,
  NbReq* nbreq,
  const char* xml_fragment )
: SbReq(
    instance,
    nbreq,
    RW_MGMTAGT_SB_REQ_TYPE_RPC,
    "SbReqRpcString",
    xml_fragment )
{
  RW_MA_SBREQ_LOG( this, __FUNCTION__, xml_fragment );
  std::string error_out;
  XMLDocument::uptr_t rpc_dom(
      instance_->xml_mgr()->create_document_from_string(xml_fragment, error_out, false) );

  if (!rpc_dom.get()) {
    RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "parse failed",
      RWMEMLOG_ARG_STRCPY_MAX(error_out.c_str(), RWMEMLOG_ARG_SIZE_MAX_BYTES));
    error_out = "Unable to parse XML string: " + error_out;
    add_error().set_error_message( error_out.c_str() );
    return;
  }

  auto root = rpc_dom->get_root_node();
  RW_ASSERT(root); // must have DOM
  auto node = root->get_first_element();
  if (!node) {
    add_error().set_error_message( "Did not find rpc root element" );
    return;
  }

  init_rpc_input( node );
}

SbReqRpc::SbReqRpc(
  Instance* instance,
  NbReq* nbreq,
  XMLDocument* rpc_dom )
: SbReq(
    instance,
    nbreq,
    RW_MGMTAGT_SB_REQ_TYPE_RPC,
    "SbReqRpcDom" )
{
  auto node = rpc_dom->get_root_node();
  RW_ASSERT(node); // must have DOM
  auto yn = node->get_yang_node();
  if (yn == instance_->yang_model()->get_root_node()) {
    node = node->get_first_element();
  }
  RW_ASSERT(node); // must have RPC

  std::string capture_temporary;
  RW_MA_SBREQ_LOG( this, __FUNCTION__, (capture_temporary=node->to_string()).c_str() );

  init_rpc_input( node );
}

void SbReqRpc::init_rpc_input(
  XMLNode* node )
{
  ProtobufCMessage* msg = nullptr;
  rw_yang_netconf_op_status_t ncs = node->to_rpc_input( &msg );
  rpc_input_.reset( msg );

  if (RW_YANG_NETCONF_OP_STATUS_OK != ncs) {
    // ATTN: Need to get XML error string out and back to client.
    add_error().set_error_message( "Could not convert DOM to RPC input message" );
    rpc_input_.reset( nullptr );
    return;
  }

  update_stats (RW_UAGENT_NETCONF_PARSE_REQUEST);
}

SbReqRpc::~SbReqRpc()
{
  if (xact_) {
    rwdts_xact_unref (xact_, __PRETTY_FUNCTION__, __LINE__);
    xact_ = nullptr;
  }
  instance_->update_stats( sbreq_type(), req_.c_str(), &statistics_ );
}

StartStatus SbReqRpc::start_xact_int()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "start rpc");
  if (nc_errors_.length()) {
    return done_with_error();
  }

  if (!rpc_input_.get()) {
    RW_MA_SBREQ_LOG (this, __FUNCTION__, "No message");
    return done_with_error( "No message" );
  }

  std::string ns = protobuf_c_message_descriptor_xml_ns( rpc_input_->descriptor );
  if (ns == uagent_yang_ns) {
    // ATTN: Need some other way rather than checking the element name.
    std::string rpcn = protobuf_c_message_descriptor_xml_element_name( rpc_input_->descriptor );
    if (rpcn == "show-system-info") {
      return start_ssi();
    } else if (rpcn == "show-agent-logs") {
      return show_agent_logs();
    }
  }

  /* Start the transcation on the main queue, both the
     dts xact and the local rpcs.
   */
  rwsched_dispatch_async_f(instance_->rwsched_tasklet(),
                           rwsched_dispatch_get_main_queue(instance_->rwsched()),
                           this,
                           start_xact_async);

  return StartStatus::Async;
}

StartStatus SbReqRpc::start_xact_locally()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "process local agent rpc");
  RWPB_M_MSG_DECL_SAFE_CAST(RwMgmtagt_input_MgmtAgent, mgmt_agent, rpc_input_.get());

  UniquePtrProtobufCMessage<>::uptr_t dup_msg;
  if (!mgmt_agent) {
    dup_msg.reset(
      protobuf_c_message_duplicate(
        nullptr,
        rpc_input_.get(),
        RWPB_G_MSG_PBCMD(RwMgmtagt_input_MgmtAgent) ) );
    if (!dup_msg.get()) {
      return done_with_error("Cannot coerce RPC to rw-mgmtagt:mgmt-agent");
    }
    mgmt_agent = RWPB_M_MSG_SAFE_CAST(RwMgmtagt_input_MgmtAgent, dup_msg.get());
  }

  if (mgmt_agent->clear_stats){
    instance_->confd_daemon_->clear_statistics();

    for (auto it = instance_->statistics_.begin(); it != instance_->statistics_.end(); it++) {
      it->reset();
    }
  } else if (mgmt_agent->dts_trace) {
    if (dts_) {
      dts_->trace_next();
    }
  } else if (mgmt_agent->load_yang_module) {
    instance_->start_upgrade(mgmt_agent->load_yang_module->n_module_name,
                             mgmt_agent->load_yang_module->module_name);
  } else if (mgmt_agent->dom_refresh_period) {
    if (mgmt_agent->dom_refresh_period->has_cli_dom_refresh_period) {
      instance_->cli_dom_refresh_period_msec_ = mgmt_agent->dom_refresh_period->cli_dom_refresh_period;
    }
    if (mgmt_agent->dom_refresh_period->has_nc_rest_dom_refresh_period) {
      instance_->nc_rest_refresh_period_msec_ = mgmt_agent->dom_refresh_period->nc_rest_dom_refresh_period;
    }
  } else if (mgmt_agent->pb_request) {
    return start_xact_pb_request();
  } else {
    return done_with_error( "No choice selected" );
  }

  return done_with_success(); // This function is always async now, so respond.
}

StartStatus SbReqRpc::start_xact_pb_request()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "process pb-request rpc");
  RWPB_M_MSG_DECL_CAST(RwMgmtagt_input_MgmtAgent, req, rpc_input_.get());
  RW_ASSERT(req->pb_request);

  // Send back into the agent as a new northbound request.
  auto nb_req_internal = new NbReqInternal(
    instance_,
    this,
    req->pb_request );
  return nb_req_internal->execute();
}

#define SSI_CONFD_ACTION_TIMEOUT 300 // ATTN:- This has to be changed to periodically push it.

StartStatus SbReqRpc::start_ssi()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "process show-system-information rpc");
  RWPB_M_MSG_DECL_CAST(RwMgmtagt_input_ShowSystemInfo, req, rpc_input_.get());

  // This will take significantly larger time. Increase the time-out on the NB
  // side.
  nbreq_->set_timeout(SSI_CONFD_ACTION_TIMEOUT);

  // Create a new ShowSysInfo and execute the command.
  ShowSysInfo *ssi = new ShowSysInfo(
      instance_, this, req);

  return ssi->execute();
}

StartStatus SbReqRpc::show_agent_logs()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "process show-agent-logs rpc");
  RWPB_M_MSG_DECL_CAST(RwMgmtagt_input_ShowAgentLogs, req, rpc_input_.get());
  nbreq_->set_timeout(SSI_CONFD_ACTION_TIMEOUT);

  return read_logs_and_send(instance_, this, req);
}

void SbReqRpc::start_xact_async(void *ud)
{
  SbReqRpc *rpc = static_cast<SbReqRpc*>(ud);
  RW_ASSERT (rpc);

  std::string ns = protobuf_c_message_descriptor_xml_ns( rpc->rpc_input()->descriptor );
  if (ns == uagent_yang_ns) {
    rpc->start_xact_locally();
    return;
  }

  rpc->start_xact_dts();
}

StartStatus SbReqRpc::start_xact_dts()
{
  auto msgdesc = rw_schema_pbcm_get_msg_msgdesc(
      nullptr, rpc_input_.get(), instance_->ypbc_schema() );
  RW_ASSERT(msgdesc);

  auto ks = msgdesc->schema_path_value;
  if (!ks) {
    // ATTN: Need element name
    return done_with_error( "Cannot get keyspec for RPC" );
  }

  xact_ = rwdts_api_query_ks(dts_->api(),
                             ks,
                             RWDTS_QUERY_RPC,
                             RWDTS_XACT_FLAG_NOTRAN | dts_flags_,
                             dts_complete_cb,
                             this,
                             rpc_input_.get());

  RW_ASSERT(xact_);
  rwdts_xact_ref (xact_, __PRETTY_FUNCTION__, __LINE__); // Take a ref always.
  RW_MA_SBREQ_LOG (this, __FUNCTION__, "Starting Transaction");
  update_stats (RW_UAGENT_NETCONF_DTS_XACT_START);
  return StartStatus::Async;
}

void SbReqRpc::internal_done(
  const rw_keyspec_path_t* ks,
  const ProtobufCMessage* msg )
{
  done_with_success( ks, msg );
}

void SbReqRpc::internal_done(
  NetconfErrorList* nc_errors )
{
  done_with_error( nc_errors );
}

void SbReqRpc::dts_cb_async(void* ud)
{
  SbReqRpc *rpc =  static_cast<SbReqRpc*>(ud);
  RW_ASSERT (rpc);
  rpc->done_with_success( rpc->dts_xact() );
}

void SbReqRpc::dts_complete_cb(
  rwdts_xact_t* xact,
  rwdts_xact_status_t* xact_status,
  void* ud)
{
  auto *rpc =  static_cast<SbReqRpc*>(ud);
  RW_ASSERT(rpc);

  switch (xact_status->status) {
    case RWDTS_XACT_COMMITTED: {
      RWMEMLOG(rpc->memlog_buf_, RWMEMLOG_MEM2, "dts query complete");

      RW_ASSERT (xact == rpc->dts_xact());
      rwsched_dispatch_async_f(rpc->instance()->rwsched_tasklet(),
                               rpc->instance()->cc_dispatchq(),
                               rpc,
                               dts_cb_async);
      break;
    }

    default:
    case RWDTS_XACT_FAILURE:
    case RWDTS_XACT_ABORTED: {
      RW_MA_SBREQ_LOG (rpc, __FUNCTION__, "RWDTS_XACT_ABORTED");
      rw_status_t rs;
      char *err_str = NULL;
      char *xpath = NULL;

      if (RW_STATUS_SUCCESS ==
          rwdts_xact_get_error_heuristic(xact, 0, &xpath, &rs, &err_str)) {
        RWMEMLOG(rpc->memlog_buf_, RWMEMLOG_MEM6, "Error response",
                 RWMEMLOG_ARG_PRINTF_INTPTR("sbreq=0x%" PRIX64,(intptr_t)rpc),
                 RWMEMLOG_ARG_PRINTF_INTPTR("dts xact=0x%" PRIXPTR, (intptr_t)xact) );

        NetconfErrorList nc_errors;
        NetconfError& err = nc_errors.add_error();

        if (xpath) {
          err.set_error_path(xpath);
          RW_FREE(xpath);
          xpath = NULL;
        }
        err.set_rw_error_tag(rs);
        if (err_str) {
          err.set_error_message(err_str);
          RW_FREE(err_str);
          err_str = NULL;
        }
        rpc->internal_done(&nc_errors);
        return;
      }
      rpc->done_with_error( "Distributed transaction aborted or failed" );
    }
  }
}