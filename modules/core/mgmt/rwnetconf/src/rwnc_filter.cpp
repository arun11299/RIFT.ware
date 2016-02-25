
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

                                                
/**
 * @file rwnc_filter.cpp
 * @author Vinod Kamalaraj
 * @date 2014/06/19
 * @brief User object used with netconf related SSH operations
 */

#include <rwnc_filter.hpp>
#include <rwnc_ds.hpp>
#include <rwnc_instance.hpp>

using namespace rw_netconf;

/*****************************************************************************/
// C++ class API Implementations

RW_CF_TYPE_CLASS_DEFINE_MEMBERS(
  "Netconf Client Library - Datastore",
  rw_ncclnt_filter_t,
  rw_ncclnt_filter_ptr_t,
  Filter);

Filter* Filter::create(
  Instance *instance,
  rwnc_filter_type_t type,
  const char *value)
{
  return new Filter(instance, type, value);
}

Filter* Filter::create(
  Instance *instance,
  rwnc_filter_type_t type)
{
  return new Filter(instance, type);
}

Instance* Filter::get_instance() const
{
  return instance_.get();
}

const char* Filter::get_value() const
{
  return value_.c_str();
}

rw_ncclnt_xml_t* Filter::get_xml() const
{
  std::string xml_str;

  switch (type_) {
    case RWNC_FILTER_TYPE_SUBTREE:
      xml_str = "<filter type=\"subtree\">" +  value_ +  "</filter>";
      break;
      
    case RWNC_FILTER_TYPE_XPATH:
      xml_str =  "<filter type=\"xpath\"" +  value_ + "/>";
      break;
      
    default:
      RWTRACE_NCCLNTLIB_INFO(instance_, "Invalid type[%d]  in get_xml()", type_);
      RW_ASSERT_NOT_REACHED();
  }

  return (rw_ncclnt_xml_create_copy_string(instance_->rw_mem_to_c_type(), xml_str.c_str()));
}

Filter::Filter (Instance *instance,
                rwnc_filter_type_t type,
                const char *value)
: instance_ (instance->rw_mem_dupref()),
  value_ (value),
  type_ (type)
{
  RWTRACE_NCCLNTLIB_DEBUG(instance_.get(), "Instantiating  filter with value [%s]", value);
}

Filter::Filter (Instance *instance,
                rwnc_filter_type_t type)
: instance_ (instance->rw_mem_dupref()),
  type_ (type)
{
 value_= "";
  RWTRACE_NCCLNTLIB_DEBUG(instance_.get(),"Instantiating empty filter");
}

Filter::~Filter()
{
}


/*****************************************************************************/
// C Interface API Implementations

void rw_ncclnt_filter_retain(const rw_ncclnt_filter_t* filter)
{
  rw_mem_to_cpp_type(filter)->rw_mem_retain();
}

void rw_ncclnt_filter_release(const rw_ncclnt_filter_t* filter)
{
  rw_mem_to_cpp_type(filter)->rw_mem_release();
}

rw_ncclnt_filter_t* rw_ncclnt_filter_create_subtree(rw_ncclnt_instance_t* instance)
{
  Filter *filter = Filter::create (rw_mem_to_cpp_type(instance),
                               RWNC_FILTER_TYPE_SUBTREE); 
  return filter->rw_mem_to_c_type();
}

rw_ncclnt_filter_t* rw_ncclnt_filter_create_subtree_from_str(rw_ncclnt_instance_t* instance,
                                                             const char *value)
{
  Filter *filter = Filter::create (rw_mem_to_cpp_type(instance),
                               RWNC_FILTER_TYPE_SUBTREE, value);
  return filter->rw_mem_to_c_type();
}

rw_ncclnt_filter_t* rw_ncclnt_filter_create_xpath(rw_ncclnt_instance_t* instance)
{
  Filter *filter = Filter::create (rw_mem_to_cpp_type(instance),
                               RWNC_FILTER_TYPE_XPATH);
  return filter->rw_mem_to_c_type();
}

rw_ncclnt_filter_t* rw_ncclnt_filter_create_xpath_from_str(rw_ncclnt_instance_t* instance,
                                                           const char *value)
{
  Filter *filter = Filter::create (rw_mem_to_cpp_type(instance),
                               RWNC_FILTER_TYPE_XPATH, value);
  return filter->rw_mem_to_c_type();
}

rw_ncclnt_instance_t* rw_ncclnt_filter_get_instance(const rw_ncclnt_filter_t* filter)
{
  return rw_mem_to_cpp_type(filter)->get_instance()->rw_mem_to_c_type();
}

rw_ncclnt_xml_t* rw_ncclnt_filter_get_create_xml(const rw_ncclnt_filter_t* filter)
{
  rw_ncclnt_xml_t *xml_p;
  xml_p = rw_mem_to_cpp_type(filter)->get_xml();
  return xml_p;
}

