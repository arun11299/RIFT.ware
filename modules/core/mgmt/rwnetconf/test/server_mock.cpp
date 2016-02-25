
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file server_mock.cpp
 * @author Tom Seidenberg
 * @date 2014/06/20
 * @brief RW.Netconf server mock.
 */

#include "server_mock.hpp"

#include <rwnc_transport.hpp>
#include <rwnc_instance.hpp>
#include <rwnc_user.hpp>
#include <rwnc_xml.hpp>
#include <rwnc_ds.hpp>
#include <rwnc_session.hpp>
#include <rwnc_cbmgr.hpp>
#include <rwnc_filter.hpp>

#include <memory>

using namespace rw_netconf;
using namespace rw_yang;


Transport* ServerMock::create_server(
  Instance* instance,
  XMLManager* xml_mgr)
{
  RW_ASSERT(instance); // ATTN: CF
  RW_ASSERT(xml_mgr);

  ServerMock* mock = new ServerMock(instance, xml_mgr);
  return mock;
}

ServerMock::ServerMock(
  Instance* instance,
  XMLManager* xml_mgr,
  const char* file_name)
: instance_(instance->rw_mem_dupref()),
  xml_mgr_(xml_mgr),
  filter_(nullptr),
  fail_next_nc_req_with_nc_err(RW_YANG_NETCONF_OP_STATUS_NULL),
  fail_next_nc_req_with_status(RW_STATUS_SUCCESS)
{
  RW_ASSERT(instance_); // ATTN: CF
  RW_ASSERT(xml_mgr_);

  // No ownership of xml_mgr - the application owns it.

  // Create the dom.
  if (file_name == nullptr) {
    running_ = std::move(xml_mgr_->create_document());
  } else {
    running_ = std::move(xml_mgr_->create_document_from_file(file_name, false));
  }
}

ServerMock::~ServerMock()
{
  // No ownership of xml_mgr
  // doms auto-frees
}

Instance* ServerMock::get_instance() const
{
  return instance_.get();
}

rw_status_t ServerMock::xmit_nc_get_config(
  const DataStore* config_source,
  Filter* filter,
  TransportContext::uptr_t context)
{
  RW_MEM_OBJECT_VALIDATE(config_source);
  UNUSED(filter);

  RW_ASSERT(context);
  rw_yang::XMLDocument *dom = get_dom (config_source);
  
  // Check if the data store exists
  if (nullptr == dom) {
    return RW_STATUS_FAILURE;
  }

  TransportContext* tc;
  
  // Mock a request enqueue failure.
  if (RW_STATUS_SUCCESS != fail_next_nc_req_with_status) {
    rw_status_t rs = fail_next_nc_req_with_status;
    fail_next_nc_req_with_status = RW_STATUS_SUCCESS;
    return rs; // callback was not queued.
  }

  // Mock a request response failure with no XML
  if (RW_YANG_NETCONF_OP_STATUS_NULL != fail_next_nc_req_with_nc_err) {
    context->xapi_status = fail_next_nc_req_with_nc_err;
    tc = context.release();
    fail_next_nc_req_with_nc_err = RW_YANG_NETCONF_OP_STATUS_NULL;


    tc->cbmgr->queue_callback(std::bind(tc->session_cb, tc, nullptr));
    return RW_STATUS_SUCCESS; // callback was queued.
  }

  std::string xml_str = "";
  XMLNode *result = nullptr;

  if (filter != nullptr) {
    // Apply the  subrtree filter
    rw_ncclnt_xml_t *filter_xml = filter->get_xml();
    Xml* xml = rw_mem_to_cpp_type(filter_xml);
 
    RW_ASSERT(xml);
    rw_yang::XMLDocument::uptr_t filter_dom = rw_mem_to_cpp_type(filter_xml)->create_dom(xml_mgr_, false);
  
    if (dom->get_root_node() && filter_dom) {
      result = dom->get_root_node()->apply_subtree_filter_recursive(filter_dom->get_root_node());
    }
    filter_ = filter;
  }

  if (result != nullptr) {
    xml_str = result->to_string();
  } else {
    xml_str = dom->to_string();
  }

  std::cout << "XML Str = "<< xml_str << std::endl;
  
  Xml::suptr_t xml = Xml::create_xml_copy_string(instance_.get(), xml_str.c_str());
  context->xapi_status = RW_YANG_NETCONF_OP_STATUS_OK;
  tc = context.release();
  tc->cbmgr->queue_callback(std::bind(tc->session_cb, tc, xml.release()));
  return RW_STATUS_SUCCESS; // callback was queued.
}

rw_yang::XMLDocument* ServerMock::get_dom(const DataStore *ds)
{
  rw_yang::XMLDocument *dom = nullptr;
  
  if (ds->get_name() == nullptr) {
    // URL based data stores do not exist yet
    return nullptr;
  }

  if (!strcmp (ds->get_name(), "running")) {
    dom = running_.get();
  } else if (!strcmp (ds->get_name(), "candidate")) {
    dom = candidate_.get();
  } else if (!strcmp (ds->get_name(), "startup")) {
    dom = startup_.get();
  }

  return dom;
}


rw_status_t ServerMock::xmit_nc_edit_config(
  const DataStore* config_target,
  void* default_op,
  void* test_opt,
  void* error_opt,
  Xml* config,
  TransportContext::uptr_t context)
{
  RW_MEM_OBJECT_VALIDATE(config_target);
  UNUSED(default_op);
  UNUSED(test_opt);
  UNUSED(error_opt);
  RW_ASSERT(config); // ATTN: CF
  RW_ASSERT(context.get());

  // Mock a request enqueue failure.
  if (RW_STATUS_SUCCESS != fail_next_nc_req_with_status) {
    rw_status_t rs = fail_next_nc_req_with_status;
    fail_next_nc_req_with_status = RW_STATUS_SUCCESS;
    return rs; // callback was not queued.
  }
  TransportContext* tc;
  // Mock a request response failure with no XML
  if (RW_YANG_NETCONF_OP_STATUS_NULL != fail_next_nc_req_with_nc_err) {
    context->xapi_status = fail_next_nc_req_with_nc_err;
    fail_next_nc_req_with_nc_err = RW_YANG_NETCONF_OP_STATUS_NULL;

    tc = context.release();
    tc->cbmgr->queue_callback(std::bind(tc->session_cb, tc, nullptr));
    return RW_STATUS_SUCCESS; // callback was queued.
  }

  // Check if the data store exists.
  rw_yang::XMLDocument *dom = get_dom (config_target);
  if (nullptr == dom) {
    context->xapi_status = RW_YANG_NETCONF_OP_STATUS_RESOURCE_DENIED;

    tc = context.release();
    tc->cbmgr->queue_callback(std::bind(tc->session_cb, tc, nullptr));
    return RW_STATUS_SUCCESS; // callback was queued.
  }

  // create a DOM from the snippet
  std::string xml_str;
  char tmp[1001];
  size_t ret_size = 0;

  while ((config->get_xml_str(tmp, sizeof(tmp) - 1, &ret_size))  && (ret_size)) {
    xml_str += tmp;
  }

  // remove the <config> and </config> tags
  xml_str.erase(0, strlen("<config>"));
  xml_str.erase(xml_str.length()- strlen ("</config>"), strlen ("</config>"));
  
  std::string error_out;
  rw_yang::XMLDocument::uptr_t merge =
      xml_mgr_->create_document_from_string(xml_str.c_str(), error_out, false);

  if (!merge.get()) {
    context->xapi_status = RW_YANG_NETCONF_OP_STATUS_MALFORMED_MESSAGE;

    tc = context.release();
    tc->cbmgr->queue_callback(std::bind(tc->session_cb, tc, nullptr));
    return RW_STATUS_SUCCESS; // callback was queued.
  }

  dom->merge(merge.get());
  
  
  // Not mocking failure - send success
  Xml::suptr_t xml = Xml::create_xml_const_string(instance_.get(), "<ok/>");
  context->xapi_status = RW_YANG_NETCONF_OP_STATUS_OK;
  tc = context.release();
  //  xml->rw_mem_dupref();
  tc->cbmgr->queue_callback(std::bind(tc->session_cb, tc, xml.release()));

  return RW_STATUS_SUCCESS;
}

rw_status_t ServerMock::load_module(const char *module_name)
{
  RW_ASSERT (xml_mgr_);
  
  YangModel* model = xml_mgr_->get_yang_model();

  RW_ASSERT (model);

  if (nullptr != model->load_module (module_name)) {
    return RW_STATUS_SUCCESS;
  }

  return RW_STATUS_FAILURE;
  
}

Filter* ServerMock::get_filter() 
{
  return filter_;
}
