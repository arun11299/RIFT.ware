
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

                                                
/**
 * @file rwnc_xml.cpp
 * @author Rajesh Velandy
 * @date 2014/06/13
 * @brief RW.Netconf client library XML Implementation
 */

#include "rwnc_xml_impl.hpp"

using namespace rw_netconf;

RW_CF_TYPE_CLASS_DEFINE_MEMBERS(
  "Netconf Client Library - XML",
  rw_ncclnt_xml_t,
  rw_ncclnt_xml_ptr_t,
  Xml);


Xml::Xml(Instance *instance) 
: instance_(instance->rw_mem_dupref())
{
  //std::cout << "Calling Xml::Xml() with instance " << instance << std::endl;
  RW_MEM_OBJECT_VALIDATE(instance);
}

Xml::~Xml() 
{
}

Xml::suptr_t Xml::create_xml_copy_string(Instance *instance, const char *str) 
{
  Xml *xml = new XmlStrImpl(instance, str);
  return Xml::suptr_t(xml);
}

Xml::suptr_t Xml::create_xml_const_string(Instance *instance, const char *str) 
{
  Xml *xml = new XmlConstStrImpl(instance, str);
  return Xml::suptr_t(xml);
}

Xml::suptr_t Xml::create_xml_copy_buffer(Instance *instance, const void *buffer, size_t len) 
{
  Xml *xml = new XmlStrImpl(instance, (const char*)buffer, len);
  return Xml::suptr_t(xml);
}

Xml::suptr_t Xml::create_xml_const_buffer(Instance *instance, const void *buffer, size_t len) 
{
  Xml *xml = new XmlConstStrImpl(instance, (const char*)buffer, len);
  return Xml::suptr_t(xml);
}

Instance* Xml::get_instance() const
{
  return instance_.get();
}
/*
 * RWNetconf Client Library C APIs -- Look in rwnetconf.h for definitions 
 */

void rw_ncclnt_xml_retain(rw_ncclnt_xml_t* xml)
{
  rw_mem_to_cpp_type(xml)->rw_mem_retain();
}

void rw_ncclnt_xml_release(rw_ncclnt_xml_t* xml)
{
  rw_mem_to_cpp_type(xml)->rw_mem_release();
}

void rw_ncclnt_xml_terminate(rw_ncclnt_xml_t* xml)
{
  /// ATTN: Neeed implementation
  RW_ASSERT(0);
}

rw_ncclnt_xml_t*  rw_ncclnt_xml_create_empty(rw_ncclnt_instance_t* instance)
{
  Xml::suptr_t xml = Xml::create_xml_copy_string(rw_mem_to_cpp_type(instance), "");
  return xml.release()->rw_mem_to_c_type();
}

rw_ncclnt_xml_t* rw_ncclnt_xml_create_copy_string(
    rw_ncclnt_instance_t* instance,
    const char* string
)
{
  Xml::suptr_t xml =
      Xml::create_xml_copy_string(rw_mem_to_cpp_type(instance), string);
  return xml.release()->rw_mem_to_c_type();
}

rw_ncclnt_xml_t* rw_ncclnt_xml_create_const_string(
    rw_ncclnt_instance_t* instance,
    const char* string
)
{
  Xml::suptr_t xml =
      Xml::create_xml_const_string(rw_mem_to_cpp_type(instance), string);
  return xml.release()->rw_mem_to_c_type();
}

rw_ncclnt_xml_t* rw_ncclnt_xml_create_copy_buffer(
    rw_ncclnt_instance_t* instance,
    const void* buff,
    size_t len
)
{
  Xml::suptr_t xml =
   Xml::create_xml_copy_buffer(rw_mem_to_cpp_type(instance), buff, len);
  return xml.release()->rw_mem_to_c_type();
}

rw_ncclnt_xml_t* rw_ncclnt_xml_create_const_buffer(
    rw_ncclnt_instance_t* instance,
    const void* buff,
    size_t len
)
{
  Xml::suptr_t xml =
      Xml::create_xml_const_buffer(rw_mem_to_cpp_type(instance), buff, len);
  return xml.release()->rw_mem_to_c_type();
}

rw_ncclnt_xml_t* rw_ncclnt_xml_create_composed(
    rw_ncclnt_instance_t* instance,
    const char* prefix,
    rw_ncclnt_xml_t* body,
    const char* suffix
)
{
  Xml::suptr_t xml = rw_mem_to_cpp_type(body)->create_composed(prefix, suffix);
  return xml.release()->rw_mem_to_c_type();
}
rw_ncclnt_xml_t* rw_ncclnt_xml_create_append(
    rw_ncclnt_instance_t* instance,
    rw_ncclnt_xml_t* prefix,
    rw_ncclnt_xml_t* suffix
)
{
  Xml::suptr_t xml =
      rw_mem_to_cpp_type(prefix)->append(rw_mem_to_cpp_type(suffix));
  return xml.release()->rw_mem_to_c_type();

}
rw_ncclnt_xml_t* rw_ncclnt_xml_create_xml_yang_subtree(
    rw_ncclnt_instance_t* instance,
    rw_xml_yang_node_t* node,
    unsigned depth_limit)
{
  //ATTN Need implementataion - Assert for now
  RW_ASSERT(0);
  return nullptr;
}

rw_ncclnt_xml_t* rw_ncclnt_xml_create_xml_subtree(
    rw_ncclnt_instance_t* instance,
    rw_xml_node_t* node,
    unsigned depth_limit)
{
  //ATTN Need implementataion - Assert for now
  RW_ASSERT(0);
  return nullptr;
}
rw_ncclnt_instance_t* rw_ncclnt_xml_get_instance(
    const rw_ncclnt_xml_t* xml
)
{
  //ATTN Need implementataion - Assert for now
  RW_ASSERT(0);
  return nullptr;
}
rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_buffer(
    rw_ncclnt_xml_t* xml,
    void* buffer,
    size_t buflen, 
    size_t* outlen) 
{
  Xml *xmlp = rw_mem_to_cpp_type(xml);

  xmlp->get_xml_str((char*)buffer, buflen, outlen);

  return RW_YANG_NETCONF_OP_STATUS_OK;
}

rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_buffer_pointer(
    rw_ncclnt_xml_t* xml,
    const void** outbuf,
    size_t* outlen
)
{
  //ATTN Need implementataion - Assert for now
  RW_ASSERT(0);
  return RW_YANG_NETCONF_OP_STATUS_OK;
}

rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_bytes(
    rw_ncclnt_xml_t* xml,
    size_t consume)
{
  //ATTN Need implementataion - Assert for now
  RW_ASSERT(0);
  return RW_YANG_NETCONF_OP_STATUS_OK;
}
rw_status_t rw_ncclnt_xml_consume_wait_producer(
    rw_ncclnt_xml_t* xml, 
    rw_ncclnt_xml_consume_wait_cb callback,
    const rw_ncclnt_context_t* context
)
{
  //ATTN Need implementataion - Assert for now
  RW_ASSERT(0);
  return RW_STATUS_SUCCESS;
}
rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_get_producer_error(
    rw_ncclnt_xml_t* xml 
)
{
  ///ATTN - For now no errors - return scuccess
  return RW_YANG_NETCONF_OP_STATUS_OK;
}

bool rw_ncclnt_xml_consume_is_eof(rw_ncclnt_xml_t* xml)
{
   Xml *xmlp = rw_mem_to_cpp_type(xml);
   return xmlp->eof();
}

void rw_ncclnt_xml_consume_set_error(
    rw_ncclnt_xml_t* xml,
    rw_yang_netconf_op_status_t status
)
{
  //ATTN Need implementataion - Assert for now
  RW_ASSERT(0);
  return;
}


rw_yang::XMLDocument::uptr_t Xml::create_dom(rw_yang::XMLManager *xml_mgr, bool validate)
{
  char tmp[2048];
  std::string xml_str;
  size_t filled_len = 0;


  while (!eof()) {
    get_xml_str (tmp, sizeof (tmp) - 1, &filled_len);
    xml_str += tmp;
  }

  std::string error_out;
  return xml_mgr->create_document_from_string (xml_str.c_str(), error_out, validate);  
}
