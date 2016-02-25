
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwuagent_nb_req_msg.cpp
 *
 * Management agent northbound request handler for RW.Msg
 */

#include "rwuagent.hpp"

using namespace rw_uagent;
using namespace rw_yang;


NbReqMsg::NbReqMsg(
  Instance* instance,
  const NetconfReq* req,
  NetconfRsp_Closure rsp_closure,
  void* closure_data )
: NbReq(
    instance,
    "NbReqMsg",
    RW_MGMTAGT_NB_REQ_TYPE_RWDTS ),
  req_(req),
  rsp_closure_(rsp_closure),
  closure_data_(closure_data)
{
}

NbReqMsg::~NbReqMsg()
{
}

StartStatus NbReqMsg::execute()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "request");

  if (!req_->has_operation) {
    NbReq::respond( nullptr, "No operation" );
  }

  switch (req_->operation) {
    case nc_edit_config:{
      RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "netconf edit config",);
      RW_ASSERT (req_->has_edit_config_operation);
      RW_MA_NBREQ_LOG (this, ClientDebug, "Edit Config Operation", req_->xml_blob->xml_blob);

      auto edit_xact = new SbReqEditConfig(
          instance_,
          this,
          req_->xml_blob->xml_blob,
          req_->edit_config_operation );
      return edit_xact->start_xact();
    }
    case nc_default:
    case nc_rpc_exec: {
      RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "netconf rpc",);
      RW_MA_NBREQ_LOG (this, ClientDebug, "RPC Operation from Msg", req_->xml_blob->xml_blob);

      auto rpc_xact = new SbReqRpc(
          instance_,
          this,
          req_->xml_blob->xml_blob );
      return rpc_xact->start_xact();
    }
    case nc_get: {
      RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "netconf get",);
      RW_MA_NBREQ_LOG (this, ClientDebug, "Get Operation", req_->xml_blob->xml_blob);

      auto get_xact = new SbReqGet(
          instance_,
          this,
          req_->xml_blob->xml_blob );
      return get_xact->start_xact();
    }
    case nc_get_config: {
      RW_MA_NBREQ_LOG (this, ClientDebug, "Get Config", req_->xml_blob->xml_blob);
      return send_response( instance_->dom()->to_string() );
    }
    default:
      break;
  }

  return NbReq::respond( nullptr, "Unknown operation" );
}

StartStatus NbReqMsg::respond(
  SbReq* sbreq,
  rw_yang::XMLDocument::uptr_t rsp_dom )
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "respond with dom",
    RWMEMLOG_ARG_PRINTF_INTPTR("sbreq=0x%" PRIX64,(intptr_t)sbreq) );

  // Set the response from the dom
  std::string rsp_str;
  if (rsp_dom.get() && rsp_dom->get_root_node()) {
    rsp_str = rsp_dom->get_root_node()->to_string();
  }

  return send_response( rsp_str );
}

StartStatus NbReqMsg::respond(
  SbReq* sbreq,
  NetconfErrorList* nc_errors )
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "respond with error",
    RWMEMLOG_ARG_PRINTF_INTPTR("sbreq=0x%" PRIX64,(intptr_t)sbreq) );

  std::string rsp_str;
  nc_errors->to_xml(instance_->xml_mgr(), rsp_str);
  instance_->last_error_ = rsp_str;

  return send_response( rsp_str );
}

StartStatus NbReqMsg::send_response(
  std::string response )
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "send message response");
  XmlBlob blob;
  xml_blob__init( &blob );
  blob.xml_blob = (char*)response.c_str();

  NetconfRsp rsp;
  netconf_rsp__init( &rsp );
  rsp.xml_response = &blob;

  RW_MA_NBREQ_LOG (this, ClientDebug, "msg response", response.c_str());
  rsp_closure_( &rsp, closure_data_ );

  delete this;
  return StartStatus::Done;
}

