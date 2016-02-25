
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_session_impl.cpp
 * @author Tom Seidenberg
 * @date 2014/06/19
 * @brief RW.Netconf client library session implementation
 */

#include "rwnc_session_impl.hpp"

#include <rwnc_xml.hpp>
#include <rwnc_transport.hpp>
#include <rwnc_user.hpp>
#include <rwnc_filter.hpp>
#include <rwnc_ds.hpp>
#include <rwnc_cbmgr.hpp>
#include <rwnc_instance.hpp>

#include <rw_cf_type_validate.hpp>

#include <functional>

using namespace rw_netconf;
using namespace std::placeholders;


/*****************************************************************************/
// C++ SessionImpl Implementations

Session::suptr_t SessionImpl::create(Instance* instance, Transport* transport, User* user)
{
  return suptr_t(new SessionImpl(instance, transport, user));
}

Instance* SessionImpl::get_instance() const
{
  return instance_.get();
}

User* SessionImpl::get_user() const
{
  return user_;
}

rw_status_t SessionImpl::req_nc_get_config(
  const DataStore* config_source,
  Filter* filter,
  ClientCallback callback,
  CallbackManager* cbmgr)
{
  RW_MEM_OBJECT_VALIDATE(config_source);
  if (filter) {
    RW_MEM_OBJECT_VALIDATE(filter);
    filter->rw_mem_retain();
  }
  RW_MEM_OBJECT_VALIDATE(cbmgr);

  TransportContext::uptr_t tc(
    new TransportContext(
      instance_.get(),
      this,
      callback,
      cbmgr,
      std::bind(&SessionImpl::rsp_cb_nc_get_config, this, _1, _2)));
  return transport_->xmit_nc_get_config(
    config_source,
    filter,
    std::move(tc));
}

rw_status_t SessionImpl::req_nc_edit_config(
  const DataStore* config_target,
  void* default_op,
  void* test_opt,
  void* error_opt,
  Xml* config,
  ClientCallback callback,
  CallbackManager* cbmgr)
{
  RW_MEM_OBJECT_VALIDATE(config_target);
  RW_ASSERT(nullptr == default_op); // ATTN: Currently unimplemented
  RW_ASSERT(nullptr == test_opt); // ATTN: Currently unimplemented
  RW_ASSERT(nullptr == error_opt); // ATTN: Currently unimplemented
  RW_ASSERT(config); // ATTN: CF
  RW_MEM_OBJECT_VALIDATE(cbmgr);

  TransportContext::uptr_t tc(
    new TransportContext(
      instance_.get(),
      this,
      callback,
      cbmgr,
      std::bind(&SessionImpl::rsp_cb_nc_edit_config, this, _1, _2)));
  return transport_->xmit_nc_edit_config(
    config_target,
    default_op,
    test_opt,
    error_opt,
    config,
    std::move(tc));
}

Transport* SessionImpl::get_transport() const
{
  return transport_;
}

void SessionImpl::rsp_cb_nc_get_config(
  TransportContext* tc,
  Xml* xml)
{
  TransportContext::uptr_t tc_uptr(tc);
  RW_ASSERT(tc);
  RW_ASSERT(tc->session.get() == this);

  // Transport layer failed?
  if (nullptr == xml) {
    RW_ASSERT(tc->xapi_status != RW_YANG_NETCONF_OP_STATUS_OK); // shouldn't be OK
    tc->client_cb(tc->xapi_status, nullptr);
    return;
  }

  /*
   * ATTN: Incomplete:
   *   parse response into dom
   *   if (parse failure) {
   *     local-side error
   *     make user's cb
   *     return
   *   }
   *
   *   if (nc-status == netconf error) {
   *     remote-side error
   *     make user's cb
   *     return
   *   }
   *
   *   validate response
   *   if (validate error) {
   *     netconf violation error
   *     make xml consumer from error response
   *     make user's cb
   *     return
   *   }
   *
   *   make xml consumer from response
   *   make user's cb
   */
  tc->client_cb(tc->xapi_status, xml);
}

void SessionImpl::rsp_cb_nc_edit_config(
  TransportContext* tc,
  Xml* xml)
{
  TransportContext::uptr_t tc_uptr(tc);
  RW_ASSERT(tc);
  RW_ASSERT(tc->session.get() == this);

  // Transport layer failed?
  if (nullptr == xml) {
    RW_ASSERT(tc->xapi_status != RW_YANG_NETCONF_OP_STATUS_OK); // shouldn't be OK
    tc->client_cb(tc->xapi_status, nullptr);
    return;
  }

  /*
   * ATTN: Incomplete:
   *   parse response into dom
   *   if (parse failure) {
   *     local-side error
   *     make user's cb
   *     return
   *   }
   *
   *   if (nc-status == netconf error) {
   *     remote-side error
   *     make user's cb
   *     return
   *   }
   *
   *   validate response
   *   if (validate error) {
   *     netconf violation error
   *     make xml consumer from error response
   *     make user's cb
   *     return
   *   }
   *
   *   make xml consumer from response
   *   make user's cb
   */
  tc->client_cb(tc->xapi_status, xml);
}

SessionImpl::SessionImpl(Instance* instance, Transport* transport, User* user)
: instance_(instance->rw_mem_dupref()),
  transport_(transport),
  user_(user)
{
  RW_MEM_OBJECT_VALIDATE(instance_);
  RW_ASSERT(transport_);
  RW_MEM_OBJECT_VALIDATE(user_);

  user_->rw_mem_retain(); // ATTN: convert to suptr_t
}

SessionImpl::~SessionImpl()
{
  // ATTN: Terminate transport?  I think it should do that first.
  delete transport_; // ATTN: CF-ize this?  It is not exported to anyone...
  user_->rw_mem_release();
}


/*****************************************************************************/
// C Session Implementation

RW_CF_TYPE_CLASS_DEFINE_MEMBERS(
  "Netconf Client Library - Session",
  rw_ncclnt_ses_t,
  rw_ncclnt_ses_ptr_t,
  Session);

void rw_ncclnt_ses_retain(const rw_ncclnt_ses_t* session)
{
  rw_mem_to_cpp_type(session)->rw_mem_retain();
}

void rw_ncclnt_ses_release(const rw_ncclnt_ses_t* session)
{
  rw_mem_to_cpp_type(session)->rw_mem_release();
}

void rw_ncclnt_ses_terminate(rw_ncclnt_ses_t* session)
{
  // ATTN: To be implemented.
  // ATTN: Stop all callbacks.
  //  - ATTN: That means getting the transport layer to stop all callbacks as well, doesn't it?
  UNUSED(session);
}

rw_ncclnt_instance_t* rw_ncclnt_ses_get_instance(const rw_ncclnt_ses_t* session)
{
  return rw_mem_to_cpp_type(session)->get_instance()->rw_mem_to_c_type();
}

rw_ncclnt_user_t* rw_ncclnt_ses_get_user(const rw_ncclnt_ses_t* session)
{
  return rw_mem_to_cpp_type(session)->get_user()->rw_mem_to_c_type();
}

rw_status_t rw_ncclnt_req_nc_get_config(
  rw_ncclnt_ses_t* ses,
  const rw_ncclnt_ds_t* config_source,
  rw_ncclnt_filter_t* filter,
  rw_ncclnt_nc_req_context_t context)
{
  RW_MEM_OBJECT_VALIDATE(ses);

  try {
    // The C++ API is responsible for validating, asserting, and retaining the arguments.
    return rw_mem_to_cpp_type(ses)->req_nc_get_config(
      rw_mem_to_cpp_type(config_source),
      rw_mem_to_cpp_type(filter),
      std::bind(context.callback, context.context, _1, _2),
      rw_mem_to_cpp_type(context.cbmgr));
  } catch (...) {
    // Do something about caught exception?
    return RW_STATUS_FAILURE;
  }
}

rw_status_t rw_ncclnt_req_nc_edit_config(
  rw_ncclnt_ses_t* ses,
  const rw_ncclnt_ds_t* target,
  void* default_op,
  void* test_opt,
  void* error_opt,
  rw_ncclnt_xml_t* config,
  rw_ncclnt_nc_req_context_t context)
{
  RW_MEM_OBJECT_VALIDATE(ses);

  try {
    // The C++ API is responsible for validating, asserting, and retaining the arguments.
    return rw_mem_to_cpp_type(ses)->req_nc_edit_config(
      rw_mem_to_cpp_type(target),
      default_op,
      test_opt,
      error_opt,
      static_cast<Xml*>(config),
      std::bind(context.callback, context.context, _1, _2),
      rw_mem_to_cpp_type(context.cbmgr));
  } catch (...) {
    // Do something about caught exception?
    return RW_STATUS_FAILURE;
  }
}
