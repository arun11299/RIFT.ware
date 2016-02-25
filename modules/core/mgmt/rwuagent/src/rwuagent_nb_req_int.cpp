
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwuagent_nb_req_int.cpp
 *
 * Management agent internal northbound request handler
 */

#include "rwuagent.hpp"

using namespace rw_uagent;
using namespace rw_yang;


NbReqInternal::NbReqInternal(
    Instance* instance,
    SbReqRpc* parent_rpc,
    const RwMgmtagt_PbRequest* pb_req )
: NbReq(
    instance,
    "NbReqInternal",
    RW_MGMTAGT_NB_REQ_TYPE_INTERNAL ),
  parent_rpc_(parent_rpc),
  pb_req_(pb_req)
{
  RW_ASSERT(parent_rpc_);
  RW_ASSERT(pb_req_);
  // ATTN: Own, copy, or ref pb_req?
}

NbReqInternal::~NbReqInternal()
{
}

StartStatus NbReqInternal::execute()
{
/*
  ATTN: TIME this
 */

  // Parse the keyspec and message data.
  UniquePtrKeySpecPath::uptr_t ks;
  UniquePtrProtobufCMessage<>::uptr_t msg;
  if (pb_req_->has_keyspec) {
    // Data is defined by a keyspec and message.
    rw_keyspec_path_t* new_ks = nullptr;
    ProtobufCMessage* new_msg = nullptr;
    auto rs = rw_keyspec_path_deserialize_msg_ks_pair(
      nullptr/*instance*/,
      instance_->ypbc_schema(),
      &pb_req_->data,
      &pb_req_->keyspec,
      &new_msg,
      &new_ks );
    ks.reset(new_ks);
    msg.reset(new_msg);
    if (RW_STATUS_SUCCESS != rs) {
      send_error(
        RW_YANG_NETCONF_OP_STATUS_BAD_ELEMENT,
        "Unable to parse keyspec or data" );
      return StartStatus::Done;
    }

  } else if (pb_req_->xpath) {
    // Data is defined by an xpath and message.
    RWMEMLOG( memlog_buf_, RWMEMLOG_MEM2, "xpath",
      RWMEMLOG_ARG_STRCPY_MAX(pb_req_->xpath, RWMEMLOG_ARG_SIZE_MAX_BYTES));

    ks.reset( rw_keyspec_path_from_xpath(
      instance_->ypbc_schema(),
      pb_req_->xpath,
      RW_XPATH_KEYSPEC,
      nullptr/*instance*/ ) );
    if (!ks.get()) {
      send_error(
        RW_YANG_NETCONF_OP_STATUS_BAD_ELEMENT,
        "Bad keyspec" );
      return StartStatus::Done;
    }

    const rw_yang_pb_msgdesc_t* msgdesc = rw_schema_pbcm_get_msg_msgdesc(
      nullptr/*instance*/,
      (ProtobufCMessage*)ks.get(),
      instance_->ypbc_schema() );
    RW_ASSERT(msgdesc);

    msg.reset( protobuf_c_message_unpack(
      nullptr/*instance*/,
      msgdesc->pbc_mdesc,
      pb_req_->data.len,
      pb_req_->data.data ) );
    if (!msg.get()) {
      send_error(
        RW_YANG_NETCONF_OP_STATUS_BAD_ELEMENT,
        "Bad data" );
      return StartStatus::Done;
    }
  } else {
    send_error(
      RW_YANG_NETCONF_OP_STATUS_MISSING_ELEMENT,
      "No key" );
    return StartStatus::Done;
  }

  rw_yang_netconf_op_status_t ncs;
  auto xml_dom(
      instance_->xml_mgr()->create_document_from_pbcm(
          msg.get(), ncs, true/*rooted*/, ks.get() ) );
  if (RW_YANG_NETCONF_OP_STATUS_OK != ncs) {
    send_error( ncs, "Protobuf conversion failed" );
    return StartStatus::Done;
  }

  SbReq* sbreq;
  switch (pb_req_->request_type) {
    case RW_MGMTAGT_PB_REQUEST_TYPE_EDIT_CONFIG: {
      NetconfEditConfigOperations eco = ec_merge;
      if (pb_req_->has_edit_type) {
        switch (pb_req_->edit_type) {
          case RW_MGMTAGT_PB_EDIT_TYPE_MERGE:
            eco = ec_merge;
            break;
          case RW_MGMTAGT_PB_EDIT_TYPE_DELETE:
            eco = ec_delete;
            break;
          default:
            send_error(
              RW_YANG_NETCONF_OP_STATUS_BAD_ELEMENT,
              "Bad edit-type" );
            return StartStatus::Done;
        }
      }

      auto str = xml_dom->to_string(); /*ATTN: stupidly expensive!*/
      RW_MA_NBREQ_LOG( this, ClientDebug, "Internal Request", str.c_str() );
      sbreq = new SbReqEditConfig( instance_, this, str.c_str(), eco );

      RWMEMLOG( memlog_buf_, RWMEMLOG_MEM2, "internal edit",
        RWMEMLOG_ARG_PRINTF_INTPTR("sbreq=0x%" PRIX64,(intptr_t)sbreq),
        RWMEMLOG_ARG_STRCPY_MAX(str.c_str(), RWMEMLOG_ARG_SIZE_MAX_BYTES) );
      break;
    }

    case RW_MGMTAGT_PB_REQUEST_TYPE_RPC: {
      auto str = xml_dom->to_string(); /*ATTN: stupidly expensive!*/
      RW_MA_NBREQ_LOG( this, ClientDebug, "Internal RPC", str.c_str() );
      sbreq = new SbReqRpc( instance_, this, str.c_str() );

      RWMEMLOG( memlog_buf_, RWMEMLOG_MEM2, "internal rpc",
        RWMEMLOG_ARG_PRINTF_INTPTR("sbreq=0x%" PRIX64,(intptr_t)sbreq),
        RWMEMLOG_ARG_STRCPY_MAX(str.c_str(), RWMEMLOG_ARG_SIZE_MAX_BYTES) );
      break;
    }

    default:
      send_error(
        RW_YANG_NETCONF_OP_STATUS_BAD_ELEMENT,
        "Bad request-type" );
      return StartStatus::Done;
  }

  // ATTN: Need to keep sbreq, to propagate aborts?
  return sbreq->start_xact();
}

StartStatus NbReqInternal::respond(
  SbReq* sbreq,
  rw_yang::XMLDocument::uptr_t rsp_dom )
{
  RWMEMLOG( memlog_buf_, RWMEMLOG_MEM2, "respond with dom",
    RWMEMLOG_ARG_PRINTF_INTPTR("sbreq=0x%" PRIX64,(intptr_t)sbreq) );

  switch (pb_req_->request_type) {
    case RW_MGMTAGT_PB_REQUEST_TYPE_EDIT_CONFIG:
      send_success( nullptr, nullptr );
      break;

    case RW_MGMTAGT_PB_REQUEST_TYPE_RPC: {
      auto root = rsp_dom->get_root_node();
      RW_ASSERT(root);
      auto node = root->get_first_element();
      RW_ASSERT(!node); // does not non-empty handle DOM responses at this time

      send_success( nullptr, nullptr );
      break;
    }

    default:
      RW_ASSERT_NOT_REACHED();
  }
  return StartStatus::Done;
}

StartStatus NbReqInternal::respond(
  SbReq* sbreq,
  const rw_keyspec_path_t* ks,
  const ProtobufCMessage* msg )
{
  RWMEMLOG( memlog_buf_, RWMEMLOG_MEM2, "respond with keyspec and message",
    RWMEMLOG_ARG_PRINTF_INTPTR("sbreq=0x%" PRIX64,(intptr_t)sbreq) );
  send_success( ks, msg );
  return StartStatus::Done;
}

StartStatus NbReqInternal::respond(
  SbReq* sbreq,
  NetconfErrorList* nc_errors )
{
  RWMEMLOG( memlog_buf_, RWMEMLOG_MEM2, "respond with error",
    RWMEMLOG_ARG_PRINTF_INTPTR("sbreq=0x%" PRIX64,(intptr_t)sbreq) );
  send_error( nc_errors );
  return StartStatus::Done;
}

void NbReqInternal::send_success(
  const rw_keyspec_path_t* ks,
  const ProtobufCMessage* msg )
{
  RWMEMLOG( memlog_buf_, RWMEMLOG_MEM2, "send success",
    RWMEMLOG_ARG_PRINTF_INTPTR("parent rpc sbreq=0x%" PRIX64,(intptr_t)parent_rpc_) );

  RWPB_M_MSG_DECL_INIT(RwMgmtagt_output_MgmtAgent_PbRequest, pb_rsp);
  UniquePtrProtobufCMessageUseBody<>::uptr_t pb_rsp_cleanup( &pb_rsp.base );

  if (ks) {
    auto rs = rw_keyspec_path_serialize_dompath( ks, nullptr, &pb_rsp.keyspec );
    if (RW_STATUS_SUCCESS == rs) {
      pb_rsp.has_keyspec = true;
      pb_rsp.xpath = rw_keyspec_path_to_xpath(
          ks, instance_->ypbc_schema(), nullptr );
    }
  }

  if (msg) {
    pb_rsp.data.data = protobuf_c_message_serialize(
        nullptr, msg, &pb_rsp.data.len );
    if (pb_rsp.data.data) {
      pb_rsp.has_data = true;
    }
  }

  RWPB_M_MSG_DECL_INIT(RwMgmtagt_output_MgmtAgent, rsp);
  rsp.pb_request = &pb_rsp;

  parent_rpc_->internal_done(
    &RWPB_G_PATHSPEC_VALUE(RwMgmtagt_output_MgmtAgent)->rw_keyspec_path_t,
    &rsp.base );

  delete this;
}

void NbReqInternal::send_error(
  rw_yang_netconf_op_status_t ncs,
  const std::string& err_str )
{
  NetconfErrorList nc_errors;
  nc_errors.add_error()
    .set_error_message( err_str.c_str() )
    .set_rw_error_tag( ncs );
  send_error( &nc_errors );
}

void NbReqInternal::send_error(
  NetconfErrorList* nc_errors )
{
  RWMEMLOG( memlog_buf_, RWMEMLOG_MEM2, "send error",
    RWMEMLOG_ARG_PRINTF_INTPTR("parent rpc sbreq=0x%" PRIX64,(intptr_t)parent_rpc_) );
  RW_ASSERT(parent_rpc_);

  std::string rsp_str;
  nc_errors->to_xml(instance_->xml_mgr(), rsp_str);

  RWPB_M_MSG_DECL_INIT(RwMgmtagt_output_MgmtAgent, rsp);
  RWPB_M_MSG_DECL_INIT(RwMgmtagt_output_MgmtAgent_PbRequest, pb_rsp);
  rsp.pb_request = &pb_rsp;
  pb_rsp.error = const_cast<char*>(rsp_str.c_str());

  parent_rpc_->internal_done(
    &RWPB_G_PATHSPEC_VALUE(RwMgmtagt_output_MgmtAgent)->rw_keyspec_path_t,
    &rsp.base );

  delete this;
}

