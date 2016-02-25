
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

                                                
/**
 * @file rwnc_ds.cpp
 * @author Vinod Kamalaraj
 * @date 2014/06/19
 * @brief User object used with netconf related SSH operations
 */

#include "rwnc_xml_impl.hpp"
#include <rwnc_ds.hpp>
#include <rwnc_instance.hpp>

using namespace rw_netconf;

/*****************************************************************************/
// C++ class API Implementations

RW_CF_TYPE_CLASS_DEFINE_MEMBERS(
  "Netconf Client Library - Datastore",
  rw_ncclnt_ds_t,
  rw_ncclnt_ds_ptr_t,
  DataStore);

DataStore::suptr_t DataStore::create(
  Instance *instance,
  const char *name,
  rwnc_ds_type_t type)
{
  return suptr_t(new DataStore(instance, name, type));
}

Instance* DataStore::get_instance() const
{
  return instance_;
}

const char* DataStore::get_name() const
{
  if (type_ == RWNC_DS_TYPE_STANDARD) {
    return name_.c_str();
  }

  return nullptr;
}

const char* DataStore::get_url() const
{
  if (type_ == RWNC_DS_TYPE_URL) {
    return name_.c_str();
  }

  return nullptr;
}

Xml::suptr_t DataStore::get_xml() const
{
  std::string xml_str;

  switch (type_) {
    case RWNC_DS_TYPE_STANDARD:
      xml_str = "<" + name_ + "/>";
      break;

    case RWNC_DS_TYPE_URL:
      xml_str = "<url>" + name_ + "</url>";
      break;

    default:
      RW_ASSERT_NOT_REACHED();
  }

  return Xml::create_xml_copy_string(instance_, xml_str.c_str());
}

DataStore::DataStore(
  Instance *instance,
  const char *name,
  rwnc_ds_type_t type)
: instance_(instance),
  name_(name),
  type_(type)
{
}

DataStore::~DataStore()
{
}

/*****************************************************************************/
// C Interface API Implementations

void rw_ncclnt_ds_retain(const rw_ncclnt_ds_t* ds)
{
  rw_mem_to_cpp_type(ds)->rw_mem_retain();
}

void rw_ncclnt_ds_release(const rw_ncclnt_ds_t* ds)
{
  rw_mem_to_cpp_type(ds)->rw_mem_release();
}

rw_ncclnt_ds_t* rw_ncclnt_ds_create_url(
  rw_ncclnt_instance_t* instance,
  const char *url)
{
  RW_MEM_OBJECT_VALIDATE(instance);
  RW_ASSERT(url);
  DataStore::suptr_t ds(DataStore::create(rw_mem_to_cpp_type(instance), url, RWNC_DS_TYPE_URL));
  return ds.release()->rw_mem_to_c_type();
}

const rw_ncclnt_ds_t* rw_ncclnt_ds_get_startup(rw_ncclnt_instance_t* instance)
{
  RW_MEM_OBJECT_VALIDATE(instance);
  return rw_mem_to_cpp_type(instance)->get_startup()->rw_mem_to_c_type();
}

const rw_ncclnt_ds_t* rw_ncclnt_ds_get_running(rw_ncclnt_instance_t* instance)
{
  RW_MEM_OBJECT_VALIDATE(instance);
  return rw_mem_to_cpp_type(instance)->get_running()->rw_mem_to_c_type();
}

const rw_ncclnt_ds_t* rw_ncclnt_ds_get_candidate(rw_ncclnt_instance_t* instance)
{
  RW_MEM_OBJECT_VALIDATE(instance);
  return rw_mem_to_cpp_type(instance)->get_candidate()->rw_mem_to_c_type();
}

rw_ncclnt_instance_t* rw_ncclnt_ds_get_instance(const rw_ncclnt_ds_t* ds)
{
  return rw_mem_to_cpp_type(ds)->get_instance()->rw_mem_to_c_type();
}

const char* rw_ncclnt_ds_get_name(const rw_ncclnt_ds_t* ds)
{
  return rw_mem_to_cpp_type(ds)->get_name();
}

const char* rw_ncclnt_ds_get_url(const rw_ncclnt_ds_t* ds)
{
  return rw_mem_to_cpp_type(ds)->get_url();
}

rw_ncclnt_xml_t* rw_ncclnt_ds_get_create_xml(const rw_ncclnt_ds_t* ds)
{
  return (rw_mem_to_cpp_type(ds)->get_xml()).release();
}
