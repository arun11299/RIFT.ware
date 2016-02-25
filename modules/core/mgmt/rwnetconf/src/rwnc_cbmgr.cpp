
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_cbmgr.cpp
 * @author Tom Seidenberg
 * @date 2014/06/25
 * @brief RW.Netconf client library Callback Manager base implementation
 */

#include "rwnc_cbmgr_pollq.hpp"

#include <rwnc_instance.hpp>

using namespace rw_netconf;


/*****************************************************************************/
// C++ CallbackManager API Implementations

RW_CF_TYPE_CLASS_DEFINE_MEMBERS(
  "Netconf Client Library - Callback Manager",
  rw_ncclnt_cbmgr_t,
  rw_ncclnt_cbmgr_ptr_t,
  CallbackManager);

rw_status_t CallbackManager::poll_once()
{
  // Default is unsupported
  return RW_STATUS_FAILURE;
}

rw_status_t CallbackManager::poll_all()
{
  while (1) {
    rw_status_t rs = poll_once();
    switch (rs) {
      case RW_STATUS_SUCCESS:
        // Successfully invoked one.  Keep trying for more...
        continue;
      case RW_STATUS_FAILURE:
        // Not supported.
        return rs;
      case RW_STATUS_NOTFOUND:
        // Supported, but empty.
        return RW_STATUS_SUCCESS;
      default:
        RW_ASSERT_NOT_REACHED();
    }
  }
}


/*****************************************************************************/
// C++ class CallbackManagerPollQ Implementations

CallbackManager::suptr_t CallbackManagerPollQ::create(
  Instance* instance)
{
  CallbackManager* cbmgr = new CallbackManagerPollQ(instance);
  return suptr_t(cbmgr);
}

void CallbackManagerPollQ::queue_callback(CbMgrCallback&& callback)
{
  cb_list_.emplace_back(callback);
}

Instance* CallbackManagerPollQ::get_instance() const
{
  return instance_.get();
}

rw_status_t CallbackManagerPollQ::poll_once()
{
  auto cbi = cb_list_.begin();
  if (cbi == cb_list_.end()) {
    return RW_STATUS_NOTFOUND;
  }

  (*cbi)();
  cb_list_.pop_front();
  return RW_STATUS_SUCCESS;
}

CallbackManagerPollQ::CallbackManagerPollQ(Instance* instance)
: instance_(instance->rw_mem_dupref())
{
  RW_MEM_OBJECT_VALIDATE(instance_);
}

CallbackManagerPollQ::~CallbackManagerPollQ()
{
}


/*****************************************************************************/
// C CallbackManager Implementations

void rw_ncclnt_cbmgr_retain(const rw_ncclnt_cbmgr_t* cbmgr)
{
  rw_mem_to_cpp_type(cbmgr)->rw_mem_retain();
}

void rw_ncclnt_cbmgr_release(const rw_ncclnt_cbmgr_t* cbmgr)
{
  rw_mem_to_cpp_type(cbmgr)->rw_mem_release();
}

void rw_ncclnt_cbmgr_terminate(rw_ncclnt_cbmgr_t* cbmgr)
{
  // ATTN: To be implemented.
  // ATTN: Stop all callbacks.
  //  - ATTN: That means getting the transport layer to stop all callbacks as well, doesn't it?
  UNUSED(cbmgr);
}

rw_ncclnt_instance_t* rw_ncclnt_cbmgr_get_instance(const rw_ncclnt_cbmgr_t* cbmgr)
{
  return rw_mem_to_cpp_type(cbmgr)->get_instance()->rw_mem_to_c_type();
}

rw_status_t rw_ncclnt_cbmgr_poll_once(rw_ncclnt_cbmgr_t* cbmgr)
{
  return rw_mem_to_cpp_type(cbmgr)->poll_once();
}

rw_status_t rw_ncclnt_cbmgr_poll_all(rw_ncclnt_cbmgr_t* cbmgr)
{
  return rw_mem_to_cpp_type(cbmgr)->poll_all();
}

/*****************************************************************************/
// C CallbackManagerPollQ Implementations

rw_ncclnt_cbmgr_t* rw_ncclnt_cbmgr_create_polled_queue(
  rw_ncclnt_instance_t* instance)
{
  CallbackManager::suptr_t cbmgr(CallbackManagerPollQ::create(rw_mem_to_cpp_type(instance)));
  return cbmgr.release()->rw_mem_to_c_type();
}
