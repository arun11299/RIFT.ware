
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file server_mock.cpp
 * @author Tom Seidenberg
 * @date 2014/06/23
 * @brief RW.Netconf null transport.
 */

#include "transport_null.hpp"

#include <rwnc_instance.hpp>
#include <rwnc_user.hpp>
#include <rwnc_xml.hpp>
#include <rwnc_ds.hpp>

#include <memory>

using namespace rw_netconf;


TransportNull::TransportNull(Instance* instance)
: instance_(instance->rw_mem_dupref())
{
  RW_ASSERT(instance_);
}

TransportNull::~TransportNull()
{
}

Instance* TransportNull::get_instance() const
{
  return instance_.get();
}

rw_status_t TransportNull::xmit_nc_get_config(
  const DataStore* config_source,
  Filter* filter,
  TransportContext::uptr_t context)
{
  UNUSED(config_source);
  UNUSED(filter);
  UNUSED(context);
  return RW_STATUS_FAILURE;
}

rw_status_t TransportNull::xmit_nc_edit_config(
  const DataStore* config_target,
  void* default_op,
  void* test_opt,
  void* error_opt,
  Xml* config,
  TransportContext::uptr_t context)
{
  UNUSED(config_target);
  UNUSED(default_op);
  UNUSED(test_opt);
  UNUSED(error_opt);
  UNUSED(config);
  UNUSED(context);
  return RW_STATUS_FAILURE;
}
