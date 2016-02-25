
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/*
 * @file rwnc_transport.cpp
 * @author Tom Seidenberg
 * @date 2014/06/19
 * @brief RW.Netconf transport implementations.
 */

#include "rwnc_session_impl.hpp"

#include <rwnc_transport_direct.hpp>
#include <rwnc_user.hpp>
#include <rwnc_xml.hpp>
#include <rwnc_ds.hpp>
#include <rwnc_cbmgr.hpp>
#include <rwnc_instance.hpp>

#include <functional>
#include <memory>

using namespace rw_netconf;


/*****************************************************************************/
// C++ TransportContext Implementation

TransportContext::TransportContext(
  Instance* in_instance,
  Session* in_session,
  ClientCallback in_client_cb,
  CallbackManager* in_cbmgr,
  SessionCallback in_session_cb)
: instance(in_instance->rw_mem_dupref()),
  session(in_session->rw_mem_dupref()),
  message_id(),
  xapi_status(RW_YANG_NETCONF_OP_STATUS_FAILED),
  ynode(nullptr),
  client_cb(in_client_cb),
  cbmgr(in_cbmgr->rw_mem_dupref()),
  session_cb(in_session_cb)
{
  RW_MEM_OBJECT_VALIDATE(instance);
  RW_MEM_OBJECT_VALIDATE(session);
  RW_MEM_OBJECT_VALIDATE(cbmgr);
}

TransportContext::~TransportContext()
{
}


/*****************************************************************************/
// C++ TransportDirect Implementation

Session* TransportDirect::create_session(
  Instance* instance,
  Transport* forward_to,
  User* user)
{
  RW_MEM_OBJECT_VALIDATE(instance);
  RW_ASSERT(forward_to);
  RW_MEM_OBJECT_VALIDATE(user);

  // Create the TransportDirect first.  If that succeeds, then create
  // the Session.  Keep the local data in unique_ptr<>s until done, for
  // automatic clean-up.

  std::unique_ptr<TransportDirect> td(new TransportDirect(instance, forward_to));

  Session::suptr_t session(SessionImpl::create(instance, td.get(), user));
  td.release();

  // ATTN: Should return suptr_t
  return session.release();
}

TransportDirect::TransportDirect(
  Instance* instance,
  Transport* forward_to)
: instance_(instance->rw_mem_dupref()),
  forward_to_(forward_to)
{
  RW_ASSERT(instance_);
  RW_ASSERT(forward_to_);

  // No ownership of forward_to_ - the application owns it.

  RWTRACE_NCCLNTLIB_DEBUG(instance_, "Created NC Client TransportDirect - [0x%p]", this);
}

TransportDirect::~TransportDirect()
{
}

Instance* TransportDirect::get_instance() const
{
  return instance_.get();
}

rw_status_t TransportDirect::xmit_nc_get_config(
  const DataStore* config_source,
  Filter* filter,
  TransportContext::uptr_t context)
{
  return forward_to_->xmit_nc_get_config(config_source, filter, std::move(context));
}

rw_status_t TransportDirect::xmit_nc_edit_config(
  const DataStore* config_target,
  void* default_op,
  void* test_opt,
  void* error_opt,
  Xml* config,
  TransportContext::uptr_t context)
{
  return forward_to_->xmit_nc_edit_config(config_target, default_op, test_opt, error_opt, config, std::move(context));
}

Transport* TransportDirect::get_forward_to() const
{
  return forward_to_;
}

