
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_instance.cpp
 * @author Rajesh Velandy
 * @date 2014/05/30
 * @brief RW.Netconf client library instance
 */

#include <rwnc_instance.hpp>
#include <rwnc_ds.hpp>
#include <rwtrace.h>

using namespace rw_netconf;
using namespace rw_yang;

/*****************************************************************************/
// C++ class API Implementations

RW_CF_TYPE_CLASS_DEFINE_MEMBERS(
  "Netconf Client Library - Instance",
  rw_ncclnt_instance_t,
  rw_ncclnt_instance_ptr_t,
  Instance);


Instance::suptr_t Instance::create(
  rw_yang::YangModel* model,
  rwtrace_ctx_t* trace_instance)
{
  return Instance::suptr_t(new Instance(model, trace_instance));
}

Instance::Instance(
  rw_yang::YangModel* model,
  rwtrace_ctx_t* rwtrace)
: model_(model),
  state_(RW_NCCLNT_STATE_INITIALIZED),
  allocator_(nullptr),
  rwtrace_(rwtrace)
{
  allocator_ = rw_cf_Allocator(RW_HASH_TYPE(rw_ncclnt_instance_ptr_t));

  // Load the netconf module
  ymod_ = model_->load_module("ietf-netconf");
  RW_ASSERT(ymod_);

  // ATTN: Take a reference to rwtrace?

  // Create the 3 standard data stores
  startup_ = std::move(DataStore::create (this, "startup", RWNC_DS_TYPE_STANDARD));
  candidate_ = std::move(DataStore::create (this, "candidate", RWNC_DS_TYPE_STANDARD));
  running_ = std::move(DataStore::create (this, "running", RWNC_DS_TYPE_STANDARD));

  RWTRACE_NCCLNTLIB_DEBUG(this, "Created RW.Netconf client - [0x%p]", this);
}

Instance::~Instance()
{
  CFRelease(allocator_);

  RWTRACE_NCCLNTLIB_DEBUG(this, "Destroyed RW.Netconf client - [0x%p]", this);
}

rw_ncclnt_instance_state_t Instance::get_state() const
{
  return state_;
}

rw_yang::YangModel* Instance::get_model() const
{
  return model_;
}

rwtrace_ctx_t* Instance::get_trace_instance()  const
{
  return rwtrace_;
}

CFAllocatorRef Instance::get_allocator() const
{
  return allocator_;
}

const DataStore* Instance::get_candidate() const
{
  return candidate_.get();
}

const DataStore* Instance::get_running() const
{
  return running_.get();
}

const DataStore* Instance::get_startup() const
{
  return startup_.get();
}


/*****************************************************************************/
// C Interface API Implementations

rw_ncclnt_instance_t* rw_ncclnt_instance_create(
  rw_yang_model_t* model,
  rwtrace_ctx_t* trace_instance)
{
  Instance::suptr_t instance = Instance::create(
    static_cast<YangModel*>(model),
    trace_instance);
  return instance.release()->rw_mem_to_c_type();
}

void rw_ncclnt_instance_retain(
  const rw_ncclnt_instance_t* instance)
{
  rw_mem_to_cpp_type(instance)->rw_mem_retain();
}

void rw_ncclnt_instance_release(
  const rw_ncclnt_instance_t* instance)
{
  rw_mem_to_cpp_type(instance)->rw_mem_release();
}

rw_ncclnt_instance_state_t rw_ncclnt_instance_get_state(
  const rw_ncclnt_instance_t *instance)
{
  RW_MEM_OBJECT_VALIDATE(instance);
  return rw_mem_to_cpp_type(instance)->get_state();
}

rw_yang_model_t* rw_ncclnt_instance_get_model(
  const rw_ncclnt_instance_t *instance)
{
  RW_MEM_OBJECT_VALIDATE(instance);
  return rw_mem_to_cpp_type(instance)->get_model();
}

rwtrace_ctx_t* rw_ncclnt_instance_get_trace_instance(
  const rw_ncclnt_instance_t *instance)
{
  RW_MEM_OBJECT_VALIDATE(instance);
  return rw_mem_to_cpp_type(instance)->get_trace_instance();
}

CFAllocatorRef rw_ncclnt_instance_get_allocator(
  const rw_ncclnt_instance_t *instance)
{
  RW_MEM_OBJECT_VALIDATE(instance);
  return rw_mem_to_cpp_type(instance)->get_allocator();
}
