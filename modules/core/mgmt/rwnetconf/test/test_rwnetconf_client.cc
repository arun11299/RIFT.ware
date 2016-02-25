
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file test_rwnetconf_client.cc
 * @author Rajesh Velandy
 * @date 05/27/2014
 * @brief Unit tests for NC Client Library
 *
 */

#include <limits.h>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include <rwut.h>
#include <rwnc_instance.hpp>
#include <rwnc_xml.hpp>
#include <rwnc_filter.hpp>
#include <rwnc_session.hpp>
#include <rwnc_transport.hpp>
#include <rwnc_transport_direct.hpp>
#include <rwnc_user.hpp>
#include <rwnc_ds.hpp>
#include <rwnc_cbmgr_pollq.hpp>

#include <yangncx.hpp>
#include <rwlib.h>
#include <rwtrace.h>

#include "server_mock.hpp"
#include "transport_null.hpp"

using namespace rw_netconf;
using namespace testing;
using namespace rw_yang;

#define GTEST_XML_BLOB_CONST_STR 1
#define GTEST_XML_BLOB_COPY_STR 2
  //Create a new XML Blob instance
static   const char *xmlStr =
    "<breakfast_menu>"
      "<food>"
        "<name>Belgian Waffles</name>"
        "<price>$5.95</price>"
        "<description>"
          "Two of our famous Belgian Waffles with plenty of real maple syrup"
        "</description>"
        "<calories>650</calories>"
      "</food>"
      "<food>"
        "<name>Strawberry Belgian Waffles</name>"
        "<price>$7.95</price>"
        "<description>"
          "Light Belgian waffles covered with strawberries and whipped cream"
        "</description>"
        "<calories>900</calories>"
      "</food>"
      "<food>"
        "<name>Berry-Berry Belgian Waffles</name>"
        "<price>$8.95</price>"
        "<description>"
          "Light Belgian waffles covered with an assortment of fresh berries and whipped cream"
        "</description>"
        "<calories>900</calories>"
      "</food>"
      "<food>"
        "<name>French Toast</name>"
        "<price>$4.50</price>"
        "<description>"
          "Thick slices made from our homemade sourdough bread"
        "</description>"
        "<calories>600</calories>"
      "</food>"
      "<food>"
        "<name>Homestyle Breakfast</name>"
        "<price>$6.95</price>"
        "<description>"
          "Two eggs, bacon or sausage, toast, and our ever-popular hash browns"
        "</description>"
        "<calories>950</calories>"
      "</food>"
    "</breakfast_menu>";

static void test_xml_blob(uint32_t tcase)
{
  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());

  //Create a new client instance
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel.get(), nullptr);
  ASSERT_NE(instance, nullptr);

  // Check the instance state
  EXPECT_EQ(rw_ncclnt_instance_get_state(instance), RW_NCCLNT_STATE_INITIALIZED);


  rw_ncclnt_xml_t *clnt_xml = nullptr;
  if (tcase == GTEST_XML_BLOB_CONST_STR)  {
    clnt_xml = rw_ncclnt_xml_create_const_string(instance, xmlStr);
  } else  {
    clnt_xml = rw_ncclnt_xml_create_copy_string(instance, xmlStr);
  }
  ASSERT_NE(clnt_xml, nullptr);

    // Retain the reference once
  rw_ncclnt_xml_retain(clnt_xml);

  Xml *xml = rw_mem_to_cpp_type(clnt_xml);
  ASSERT_NE(xml, nullptr);

  std::stringstream sstrm;
  char buf[32];
  uint32_t idx = 0;

  sstrm << *xml;

  std::string expected_str("");
  if (tcase == GTEST_XML_BLOB_CONST_STR) {
    expected_str += std::string("XMLConstStrImpl:\n[xml_str_] = \"") +
                    xmlStr + "\"\n";
  } else {
    expected_str += std::string("XMLStrImpl:\n[xml_str_] = \"") +
                    xmlStr + "\"\n";
  }
  expected_str += std::string("[curr] = \"") + xmlStr + "\"\n";

  EXPECT_STREQ(expected_str.c_str(), sstrm.str().c_str());

  EXPECT_FALSE(rw_ncclnt_xml_consume_is_eof(clnt_xml));

  // Walk the XML object
  while (!rw_ncclnt_xml_consume_is_eof(clnt_xml)) {
    size_t out_len = 0;
    rw_ncclnt_xml_consume_buffer(clnt_xml, buf, sizeof(buf), &out_len);
    EXPECT_GE(32, out_len);
    EXPECT_EQ(0, memcmp(buf, xmlStr + idx, out_len));
    idx += out_len;
  }
  EXPECT_EQ(0, xml->bytes_left());
  EXPECT_TRUE(rw_ncclnt_xml_consume_is_eof(clnt_xml));

  // Retain the reference once
  rw_ncclnt_xml_retain(clnt_xml);

  // Finally Release the object
  rw_ncclnt_xml_release(clnt_xml);
}

TEST(RwNcInstance, CreatAndRelease)
{
  TEST_DESCRIPTION("Creates and Releases an clnt instance");

   /// Create YangModel
  rw_yang_model_t* model =  rw_yang_model_create_libncx();
  rw_yang::YangModel *ymodel = static_cast<rw_yang::YangModel*>(model);

  ASSERT_NE(ymodel, nullptr);

  //Create a new client instance
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel, nullptr);
  ASSERT_NE(instance, nullptr);

  // Check model.
  EXPECT_EQ(model, rw_ncclnt_instance_get_model(instance));

  // Check the instance state
  EXPECT_EQ(rw_ncclnt_instance_get_state(instance), RW_NCCLNT_STATE_INITIALIZED);

  // release the instance
  rw_ncclnt_instance_release(instance);
}

TEST(RwNcInstance, Trace)
{
  TEST_DESCRIPTION("Test RW.Netconf library tracing");


  rwtrace_ctx_t* rwtrace = rwtrace_init();
  ASSERT_NE(rwtrace, nullptr);
  rw_status_t rs = rwtrace_ctx_category_destination_set(rwtrace,
                                                    RWTRACE_CATEGORY_RWNCCLNT,
                                                    RWTRACE_DESTINATION_CONSOLE);
  EXPECT_EQ(rs, RW_STATUS_SUCCESS);

  rs = rwtrace_ctx_category_severity_set(rwtrace,
                                     RWTRACE_CATEGORY_RWNCCLNT,
                                     RWTRACE_SEVERITY_CRIT);
  EXPECT_EQ(rs, RW_STATUS_SUCCESS);

  rs = rwtrace_ctx_category_severity_set(rwtrace,
                                     RWTRACE_CATEGORY_RWNCCLNT,
                                     RWTRACE_SEVERITY_DEBUG);
  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
   
  /// Create YangModel
  //rw_yang_model_t* model =  rw_yang_model_create_libncx_with_trace(rwtrace);
  rw_yang_model_t* model =  rw_yang_model_create_libncx();
  rw_yang::YangModel *ymodel = static_cast<rw_yang::YangModel*>(model);

  ASSERT_NE(ymodel, nullptr);

  //Create a new client instance
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel, rwtrace);
  ASSERT_NE(instance, nullptr);
  ASSERT_NE(rw_ncclnt_instance_get_trace_instance(instance), nullptr);

  // Set the trace severity to debug
  rs = rwtrace_ctx_category_severity_set(rwtrace,
                                     RWTRACE_CATEGORY_RWNCCLNT,
                                     RWTRACE_SEVERITY_DEBUG);
  EXPECT_EQ(rs, RW_STATUS_SUCCESS);

  // Log an event at level debug
  EXPECT_TRUE(ConsoleCapture::start_stdout_capture() >= 0);
  RWTRACE_NCCLNTLIB_DEBUG(rw_mem_to_cpp_type(instance),
                          "RWNC Client Instance Test log Debug");
  EXPECT_THAT(ConsoleCapture::get_capture_string(),
              HasSubstr("RWNC Client Instance Test log Debug"));

  // Set the trace severity to critical
  rs = rwtrace_ctx_category_severity_set(rwtrace,
                                     RWTRACE_CATEGORY_RWNCCLNT,
                                     RWTRACE_SEVERITY_CRIT);

  EXPECT_TRUE(ConsoleCapture::start_stdout_capture() >= 0);
  RWTRACE_NCCLNTLIB_DEBUG(rw_mem_to_cpp_type(instance),
                          "RWNC Client Instance Test log Debug");
  EXPECT_STREQ(ConsoleCapture::get_capture_string().c_str(), "");

  EXPECT_TRUE(ConsoleCapture::start_stdout_capture() >= 0);
  RWTRACE_NCCLNTLIB_CRIT(rw_mem_to_cpp_type(instance),
                         "RWNC Client Instance Test log Critical");
  EXPECT_THAT(ConsoleCapture::get_capture_string(),
              HasSubstr("RWNC Client Instance Test log Critical"));

  // release the instance
  rw_ncclnt_instance_release(instance);
  instance = nullptr;
  rwtrace_ctx_close(rwtrace);
}


TEST(RwNcXml, ConstStrBlob)
{
  TEST_DESCRIPTION("Tests the XML Const Str Blob class");
  test_xml_blob(GTEST_XML_BLOB_CONST_STR);
}

TEST(RwNcXml, CopyStrBlob)
{
  TEST_DESCRIPTION("Tests the XML Copy Str Blob class");
  test_xml_blob(GTEST_XML_BLOB_COPY_STR);
}


TEST(RwNcUser, Basic)
{
  TEST_DESCRIPTION ("Test the user object and C-API to it");
  const char *user_name = "John Smith";

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel.get(), nullptr);
  ASSERT_NE(instance, nullptr);

  rw_ncclnt_user_t* user1 = rw_ncclnt_user_create_self(instance);
  ASSERT_NE(user1, nullptr);

  const char *name = rw_ncclnt_user_get_username(user1);
  EXPECT_FALSE (strlen(name));

  rw_ncclnt_user_set_username(user1, user_name);

  EXPECT_STREQ (user_name, rw_ncclnt_user_get_username(user1));
  EXPECT_EQ (instance, rw_ncclnt_user_get_instance(user1));

  // Retain and release - should leave object in the same state
  rw_ncclnt_user_retain(user1);
  rw_ncclnt_user_release(user1);

  EXPECT_STREQ (user_name, rw_ncclnt_user_get_username(user1));
  EXPECT_EQ (instance, rw_ncclnt_user_get_instance(user1));

  //EXPECT_DEATH({rw_ncclnt_user_release(user1);rw_ncclnt_user_release(user1);}, "is not a CF object");
  rw_ncclnt_user_release(user1);

  user1 = rw_ncclnt_user_create_name(instance, user_name);
  ASSERT_NE(user1, nullptr);

  EXPECT_STREQ (user_name, rw_ncclnt_user_get_username(user1));
  EXPECT_EQ (instance, rw_ncclnt_user_get_instance(user1));

}


TEST(RwNcDataStore, Candidate)
{
  TEST_DESCRIPTION ("Test the Candidate Datastore object");

  const char *name = "candidate";

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel.get(), nullptr);
  ASSERT_NE(instance, nullptr);

  const rw_ncclnt_ds_t* ds = rw_ncclnt_ds_get_candidate(instance);
  ASSERT_NE(ds, nullptr);

  EXPECT_STREQ (name, rw_ncclnt_ds_get_name(ds));
  EXPECT_EQ (instance, rw_ncclnt_ds_get_instance(ds));
  EXPECT_EQ (nullptr,rw_ncclnt_ds_get_url (ds));

  // Retain and release - should leave object in the same state
  rw_ncclnt_ds_retain(ds);
  rw_ncclnt_ds_release(ds);

  EXPECT_STREQ (name, rw_ncclnt_ds_get_name(ds));
  EXPECT_EQ (instance, rw_ncclnt_ds_get_instance(ds));

  rw_ncclnt_ds_get_create_xml (ds);

  //EXPECT_DEATH({rw_ncclnt_ds_release(ds);rw_ncclnt_ds_release(ds);}, "is not a CF object");
  rw_ncclnt_ds_release(ds);
}

TEST(RwNcDataStore, Running)
{
  TEST_DESCRIPTION ("Test the Running Datastore object");

  const char *name = "running";

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel.get(), nullptr);
  ASSERT_NE(instance, nullptr);

  const rw_ncclnt_ds_t* ds = rw_ncclnt_ds_get_running(instance);
  ASSERT_NE(ds, nullptr);

  EXPECT_STREQ (name, rw_ncclnt_ds_get_name(ds));
  EXPECT_EQ (instance, rw_ncclnt_ds_get_instance(ds));
  EXPECT_EQ (nullptr,rw_ncclnt_ds_get_url (ds));

  // Retain and release - should leave object in the same state
  rw_ncclnt_ds_retain(ds);
  rw_ncclnt_ds_release(ds);

  EXPECT_STREQ (name, rw_ncclnt_ds_get_name(ds));
  EXPECT_EQ (instance, rw_ncclnt_ds_get_instance(ds));

  //EXPECT_DEATH({rw_ncclnt_ds_release(ds);rw_ncclnt_ds_release(ds);}, "is not a CF object");
  rw_ncclnt_ds_release(ds);
}

TEST(RwNcDataStore, Startup)
{
  TEST_DESCRIPTION ("Test the Startup Datastore object");

  const char *name = "startup";

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel.get(), nullptr);
  ASSERT_NE(instance, nullptr);

  const rw_ncclnt_ds_t* ds = rw_ncclnt_ds_get_startup(instance);
  ASSERT_NE(ds, nullptr);

  EXPECT_STREQ (name, rw_ncclnt_ds_get_name(ds));
  EXPECT_EQ (instance, rw_ncclnt_ds_get_instance(ds));
  EXPECT_EQ (nullptr,rw_ncclnt_ds_get_url (ds));

  // Retain and release - should leave object in the same state
  rw_ncclnt_ds_retain(ds);
  rw_ncclnt_ds_release(ds);

  EXPECT_STREQ (name, rw_ncclnt_ds_get_name(ds));
  EXPECT_EQ (instance, rw_ncclnt_ds_get_instance(ds));

  //EXPECT_DEATH({rw_ncclnt_ds_release(ds);rw_ncclnt_ds_release(ds);}, "is not a CF object");
  rw_ncclnt_ds_release(ds);
}

TEST(RwNcDataStore, Url)
{
  TEST_DESCRIPTION ("Test the Startup Datastore object");

  const char *name = "http://www.riftio.com/url_ds";

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel.get(), nullptr);
  ASSERT_NE(instance, nullptr);

  rw_ncclnt_ds_t* ds = rw_ncclnt_ds_create_url(instance, name);
  ASSERT_NE(ds, nullptr);

  EXPECT_EQ (nullptr, rw_ncclnt_ds_get_name(ds));
  EXPECT_EQ (instance, rw_ncclnt_ds_get_instance(ds));
  EXPECT_STREQ (name,rw_ncclnt_ds_get_url (ds));

  // Retain and release - should leave object in the same state
  rw_ncclnt_ds_retain(ds);
  rw_ncclnt_ds_release(ds);

  rw_ncclnt_ds_get_create_xml (ds);

  EXPECT_STREQ (name,rw_ncclnt_ds_get_url (ds));
  EXPECT_EQ (instance, rw_ncclnt_ds_get_instance(ds));

  /*
   * ATTN: Removed because the crashing fork was getting stuck in an
   * uninterruptible sleep - in exit().  WTF.  Tried unsuccessfully to
   * debug in gdb - the problem did not reproduce there, with or
   * without follow-fork
   */
  //EXPECT_DEATH({rw_ncclnt_ds_release(ds);rw_ncclnt_ds_release(ds);}, "is not a CF object");
  rw_ncclnt_ds_release(ds);
}

TEST(RwNcFilter, Subtree)
{
  TEST_DESCRIPTION ("Test a SubTree Filter object");

  const char *value = "<top xmlns=\"http://example.com/schema/1.2/config\"> "
      "<users> <user> <name/> "
      "</user> </users> </top>";

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel.get(), nullptr);
  ASSERT_NE(instance, nullptr);

  rw_ncclnt_filter_t* filter = rw_ncclnt_filter_create_subtree (instance);
  ASSERT_NE(filter, nullptr);

  EXPECT_NE (nullptr, rw_ncclnt_filter_get_create_xml(filter));
  EXPECT_EQ (instance, rw_ncclnt_filter_get_instance (filter));

  //EXPECT_DEATH({rw_ncclnt_filter_release(filter);rw_ncclnt_filter_release(filter);}, "is not a CF object");
  rw_ncclnt_filter_release(filter);

  filter = rw_ncclnt_filter_create_subtree_from_str (instance, value);
  ASSERT_NE(filter, nullptr);
  EXPECT_STREQ (value, rw_mem_to_cpp_type(filter)->get_value());
  EXPECT_NE (nullptr, rw_ncclnt_filter_get_create_xml(filter));
}

TEST(RwNcFilter, XPath)
{
  TEST_DESCRIPTION ("Test a SubTree Filter object");

  const char *value = "select=\"/t:top/t:users/t:user[t:name='fred']\"";

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel.get(), nullptr);
  ASSERT_NE(instance, nullptr);

  rw_ncclnt_filter_t* filter = rw_ncclnt_filter_create_xpath (instance);
  ASSERT_NE(filter, nullptr);

  rw_ncclnt_filter_retain(filter);

  rw_ncclnt_filter_release(filter);

  EXPECT_NE (nullptr, rw_ncclnt_filter_get_create_xml(filter));

  filter = rw_ncclnt_filter_create_xpath_from_str (instance,value);
  ASSERT_NE(filter, nullptr);
  EXPECT_STREQ (value, rw_mem_to_cpp_type(filter)->get_value());
  EXPECT_NE (nullptr, rw_ncclnt_filter_get_create_xml(filter));
}

TEST(RwNcXml, Composition)
{
  char buf[512];
  TEST_DESCRIPTION("Tests the XML Composition");

  //Create a new client instance
  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  rw_ncclnt_instance_t *instance = rw_ncclnt_instance_create(ymodel.get(), nullptr);
  ASSERT_NE(instance, nullptr);

  char xmlStr[] = "<child1>1</child1><child2>2</child2>";

  rw_ncclnt_xml_t *body = rw_ncclnt_xml_create_const_string(instance, xmlStr);

  ASSERT_NE(body, nullptr);

  char prefix[] = "<parent>";

  char suffix[] = "</parent>";

  rw_ncclnt_xml_t *result = rw_ncclnt_xml_create_composed(instance,
                                                          prefix,
                                                          body,
                                                          suffix);
  ASSERT_NE(result, nullptr);

  // Walk the XML object
  EXPECT_FALSE(rw_ncclnt_xml_consume_is_eof(result));

  memset(buf, 0, sizeof(buf));
  size_t out_len = 0;

  rw_ncclnt_xml_consume_buffer(result, buf, sizeof(buf), &out_len);
  EXPECT_EQ(strlen(xmlStr)+strlen(prefix)+strlen(suffix), out_len);

  EXPECT_STREQ("<parent><child1>1</child1><child2>2</child2></parent>", buf);

  char str1[] = "<parent1><child1>1</child1><child2>2</child2></parent1>";
  char str2[] = "<parent2><child1>1</child1><child2>2</child2></parent2>";

  rw_ncclnt_xml_t *xml1 = rw_ncclnt_xml_create_const_string(instance, str1);
  rw_ncclnt_xml_t *xml2 = rw_ncclnt_xml_create_const_string(instance, str2);

  // Test append
  result = rw_ncclnt_xml_create_append(instance, xml1, xml2);

  rw_ncclnt_xml_consume_buffer(result, buf, sizeof(buf), &out_len);
  EXPECT_EQ(strlen(str1)+strlen(str2), out_len);

  EXPECT_STREQ((std::string(str1)+str2).c_str(), buf);

  // release the body
  rw_ncclnt_xml_release(body);

  // Release the result
  rw_ncclnt_xml_release(result);
}

TEST(RwNcServerMock, PlainCreateDestroy)
{
  TEST_DESCRIPTION("Create and destroy ServerMock");

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  EXPECT_TRUE(ymodel.get());
  XMLManager::uptr_t xml_mgr = std::move(xml_manager_create_xerces(ymodel.get()));
  EXPECT_TRUE(xml_mgr.get());
  Instance::suptr_t instance(Instance::create(ymodel.get(),nullptr));
  EXPECT_TRUE(instance.get());

  std::unique_ptr<ServerMock> sm(new ServerMock(instance.get(), xml_mgr.get()));
  EXPECT_TRUE(sm.get());

  EXPECT_TRUE(sm->instance_.get());
  EXPECT_TRUE(sm->xml_mgr_);

  ASSERT_TRUE(sm->running_.get());
  EXPECT_TRUE(sm->running_->get_root_node());
  
  EXPECT_EQ(sm->get_instance(), instance.get());

  sm.reset();
  instance.reset();
  xml_mgr.reset();
  ymodel.reset();
}

TEST(RwNcTransportDirect, CreateDestroyPlain)
{
  TEST_DESCRIPTION("Create and destroy TransportDirect (without session)");

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  Instance::suptr_t instance(Instance::create(ymodel.get(),nullptr));
  EXPECT_TRUE(instance.get());

  TransportNull* null_trans = new TransportNull(instance.get());
  EXPECT_TRUE(null_trans);
  EXPECT_EQ(null_trans->get_instance(), instance.get());

  TransportDirect* direct_trans = new TransportDirect(instance.get(), null_trans);
  EXPECT_TRUE(direct_trans);
  EXPECT_EQ(direct_trans->get_instance(), instance.get());

  delete direct_trans;
  delete null_trans;

  instance.reset();
  ymodel.reset();
}

TEST(RwNcServerMock, CreateDestroyWithMock)
{
  TEST_DESCRIPTION("Create and destroy TransportDirect connected to ServerMock");

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  EXPECT_TRUE(ymodel.get());
  XMLManager::uptr_t xml_mgr = std::move(xml_manager_create_xerces(ymodel.get()));
  EXPECT_TRUE(xml_mgr.get());
  Instance::suptr_t instance(Instance::create(ymodel.get(),nullptr));
  EXPECT_TRUE(instance.get());

  std::unique_ptr<ServerMock> server_mock(new ServerMock(instance.get(), xml_mgr.get()));
  EXPECT_TRUE(server_mock.get());

  EXPECT_TRUE(server_mock->instance_.get());
  EXPECT_TRUE(server_mock->xml_mgr_);
  ASSERT_TRUE(server_mock->running_.get());
  EXPECT_TRUE(server_mock->running_->get_root_node());

  std::unique_ptr<TransportDirect> direct_trans(new TransportDirect(instance.get(), server_mock.get()));
  EXPECT_TRUE(direct_trans.get());
  EXPECT_EQ(direct_trans->get_instance(), instance.get());

  direct_trans.reset();
  server_mock.reset();
  instance.reset();
  xml_mgr.reset();
  ymodel.reset();
}

TEST(RwNcSession, CreateDestroyWithMockAndDirect)
{
  TEST_DESCRIPTION("Create and destroy Session with ServerMock and TransportDirect");

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  EXPECT_TRUE(ymodel.get());
  XMLManager::uptr_t xml_mgr = std::move(xml_manager_create_xerces(ymodel.get()));
  EXPECT_TRUE(xml_mgr.get());
  Instance::suptr_t instance(Instance::create(ymodel.get(),nullptr));
  EXPECT_TRUE(instance.get());

  std::unique_ptr<ServerMock> server_mock(new ServerMock(instance.get(), xml_mgr.get()));
  ASSERT_TRUE(server_mock.get());

  User* user = User::create(instance.get(), "nobody");
  ASSERT_TRUE(user);

  Session* ses = TransportDirect::create_session(instance.get(), server_mock.get(), user);
  ASSERT_TRUE(ses);
  EXPECT_EQ(ses->get_instance(), instance.get());
  EXPECT_EQ(ses->get_user(), user);

  ses->rw_mem_release();
  user->rw_mem_release();
  server_mock.reset();
  instance.reset();
  xml_mgr.reset();
  ymodel.reset();
}

TEST(RwNcCbMgr, CreateDestroyPollQ)
{
  TEST_DESCRIPTION("Create and destroy CallbackManagerPollQ");

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  ASSERT_TRUE(ymodel.get());
  Instance::suptr_t instance(Instance::create(ymodel.get(), nullptr));
  ASSERT_TRUE(instance.get());

  // Create a callback manager
  CallbackManager::suptr_t cbmgr(CallbackManagerPollQ::create(instance.get()));
  EXPECT_TRUE(cbmgr.get());

  cbmgr.reset();
  instance.reset();
  ymodel.reset();
}

TEST(RwNcCbMgr, PollQSimple)
{
  TEST_DESCRIPTION("CallbackManagerPollQ simple callback test");

  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  ASSERT_NE(nullptr,ymodel.get());
  Instance::suptr_t instance(Instance::create(ymodel.get(), nullptr));
  ASSERT_NE(nullptr,instance.get());

  // Create a callback manager
  CallbackManager::suptr_t cbmgr(CallbackManagerPollQ::create(instance.get()));
  EXPECT_NE(nullptr,cbmgr.get());

  // Use a lambda and a local variable to verify that the callbacks are working
  unsigned counter = 0;
  auto lambda = [&counter] (unsigned new_counter)
  {
    counter = new_counter;
  };
  EXPECT_EQ(counter,0);

  cbmgr->queue_callback(std::bind(lambda,1));
  EXPECT_EQ(counter,0);
  rw_status_t rs = cbmgr->poll_once();
  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
  EXPECT_EQ(counter, 1);

  rs = cbmgr->poll_once();
  EXPECT_EQ(rs, RW_STATUS_NOTFOUND);
  EXPECT_EQ(counter, 1);

  rs = cbmgr->poll_all();
  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
  EXPECT_EQ(counter, 1);

  cbmgr->queue_callback(std::bind(lambda,2));
  cbmgr->queue_callback(std::bind(lambda,33));
  cbmgr->queue_callback(std::bind(lambda,4));
  cbmgr->queue_callback(std::bind(lambda,555));
  EXPECT_EQ(counter, 1);

  rs = cbmgr->poll_once();
  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
  EXPECT_EQ(counter, 2);

  rs = cbmgr->poll_all();
  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
  EXPECT_EQ(counter, 555);

  cbmgr.reset();
  instance.reset();
  ymodel.reset();
}


TEST(RwNcApiMockDirect, MockFailures)
{
  TEST_DESCRIPTION("Test a couple of mock failures through the whole stack");

  //// All the following code is common - should find a way to abstract it


  // Yang model
  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  ASSERT_NE(nullptr,ymodel.get());

  // Client library instance
  Instance::suptr_t instance(Instance::create(ymodel.get(), nullptr));
  ASSERT_NE(nullptr,instance.get());
  rw_ncclnt_instance_t* c_instance = instance->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_instance);

  // Callback manager
  CallbackManager::suptr_t cbmgr(CallbackManagerPollQ::create(instance.get()));
  EXPECT_NE(nullptr,cbmgr.get());
  rw_ncclnt_cbmgr_t* c_cbmgr = cbmgr->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_cbmgr);

  // XML manager for server mock
  XMLManager::uptr_t xml_mgr = std::move(xml_manager_create_xerces(ymodel.get()));
  EXPECT_NE(nullptr,xml_mgr.get());

  // Server mock
  std::unique_ptr<ServerMock> server_mock(new ServerMock(instance.get(), xml_mgr.get()));
  ASSERT_NE(nullptr,server_mock.get());

  // Create a user for the connection
  UniquishPtrRwMemoryTracking<User>::suptr_t user(User::create(instance.get(), "nobody"));
  ASSERT_NE(nullptr,user.get());
  rw_ncclnt_user_t* c_user = user.get()->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_user);

  // Finally, create the direct-transport and associated session
  UniquishPtrRwMemoryTracking<Session>::suptr_t
    ses(TransportDirect::create_session(instance.get(), server_mock.get(), user.get()));
  ASSERT_NE(nullptr,ses.get());
  rw_ncclnt_ses_t* c_ses = ses.get()->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_ses);

  // Junk XML blob
  Xml::suptr_t empty_config_xml(Xml::create_xml_const_string(instance.get(), "<config/>"));


  //////////////////////////////////////////////////////////////////////////////////

  // Utility lambdas for catching errors
  unsigned callbacks = 0;
  bool expected_callback = true;
  rw_yang_netconf_op_status_t expected_status = RW_YANG_NETCONF_OP_STATUS_OK;
  rw_yang_netconf_op_status_t captured_status = RW_YANG_NETCONF_OP_STATUS_NULL;
  Xml* captured_xml = nullptr;
  auto callback_lambda = [&] (rw_yang_netconf_op_status_t rsp_status, Xml* rsp_xml)
  {
    ++callbacks;
    if (captured_xml) {
      captured_xml->rw_mem_release();
    }
    EXPECT_TRUE(expected_callback);
    EXPECT_EQ(expected_status, rsp_status);
    captured_status = rsp_status;
    captured_xml = rsp_xml;
    if (captured_xml) {
      captured_xml->rw_mem_retain();
    }
  };

  rw_status_t rs = RW_STATUS_SUCCESS;

  //////////////////////////////////////////////////////////////////////////////////

  unsigned expected_callbacks = 0;

  // Send a get-config, force an immediate server-side failure.
  server_mock->fail_next_nc_req_with_status = RW_STATUS_EXISTS;
  expected_callback = false;
  rs = ses->req_nc_get_config(
    instance->get_running(),
    nullptr /*filter*/,
    callback_lambda,
    cbmgr.get() );

  EXPECT_EQ(rs, RW_STATUS_EXISTS);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_NOTFOUND);
  EXPECT_EQ(nullptr, captured_xml);
  EXPECT_EQ(callbacks, expected_callbacks);


  // Send a get-config, force a response failure.
  server_mock->fail_next_nc_req_with_nc_err = RW_YANG_NETCONF_OP_STATUS_TOO_BIG;
  expected_status = RW_YANG_NETCONF_OP_STATUS_TOO_BIG;
  expected_callback = true;
  rs = ses->req_nc_get_config(
    instance->get_running(),
    nullptr /*filter*/,
    callback_lambda,
    cbmgr.get() );

  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_SUCCESS);
  ++expected_callbacks;
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_NOTFOUND);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(nullptr, captured_xml);
  EXPECT_EQ(captured_status, RW_YANG_NETCONF_OP_STATUS_TOO_BIG);


  // Send an edit-config, force an immediate server-side failure.
  server_mock->fail_next_nc_req_with_status = RW_STATUS_TIMEOUT;
  expected_callback = false;
  rs = ses->req_nc_edit_config(
    instance->get_candidate(),
    nullptr/*default_op*/,
    nullptr/*test_opt*/,
    nullptr/*error_opt*/,
    empty_config_xml.get(),
    callback_lambda,
    cbmgr.get() );

  EXPECT_EQ(rs, RW_STATUS_TIMEOUT);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_NOTFOUND);
  EXPECT_EQ(nullptr, captured_xml);
  EXPECT_EQ(callbacks, expected_callbacks);


  //////////////////////////////////////////////////////////////////////////////////

  empty_config_xml.reset();
  //EXPECT_DEATH({ses.reset();RW_MEM_OBJECT_VALIDATE(c_ses);}, "is not a CF object");
  ses.reset();
  //EXPECT_DEATH({user.reset();RW_MEM_OBJECT_VALIDATE(c_user);}, "is not a CF object");
  user.reset();
  server_mock.reset();
  xml_mgr.reset();
  //EXPECT_DEATH({cbmgr.reset();RW_MEM_OBJECT_VALIDATE(c_cbmgr);}, "is not a CF object");
  cbmgr.reset();
  //EXPECT_DEATH({instance.reset();RW_MEM_OBJECT_VALIDATE(c_instance);}, "is not a CF object");
  instance.reset();
  ymodel.reset();

}

static const char* ydt_top_ns = "http://riftio.com/ns/nc-client/nc-client";
static const char* ydt_top_prefix = "ncc";

TEST(RwNcApiMockDirect, EditConfig)
{
  TEST_DESCRIPTION("Test a couple of mock success through the whole stack");


  // Yang model
  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  ASSERT_NE(nullptr,ymodel.get());
  YangModel *ymd = ymodel.get();
  ASSERT_TRUE(ymd->load_module ("nc-client"));

    //// All the following code is common - should find a way to abstract it
  Instance::suptr_t instance(Instance::create(ymodel.get(), nullptr));
  ASSERT_NE(nullptr,instance.get());
  rw_ncclnt_instance_t* c_instance = instance->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_instance);

    // Callback manager
  CallbackManager::suptr_t cbmgr(CallbackManagerPollQ::create(instance.get()));
  EXPECT_NE(nullptr,cbmgr.get());
  rw_ncclnt_cbmgr_t* c_cbmgr = cbmgr->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_cbmgr);

  XMLManager::uptr_t xml_mgr = std::move(xml_manager_create_xerces(ymd));
  EXPECT_NE(nullptr,xml_mgr.get());

  // Server mock
  std::unique_ptr<ServerMock> server_mock(new ServerMock(instance.get(), xml_mgr.get()));
  ASSERT_NE(nullptr,server_mock.get());

    // Create a user for the connection
  UniquishPtrRwMemoryTracking<User>::suptr_t user(User::create(instance.get(), "nobody"));
  ASSERT_NE(nullptr,user.get());
  rw_ncclnt_user_t* c_user = user.get()->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_user);

    // Finally, create the direct-transport and associated session
  UniquishPtrRwMemoryTracking<Session>::suptr_t
    ses(TransportDirect::create_session(instance.get(), server_mock.get(), user.get()));
  ASSERT_NE(nullptr,ses.get());
  rw_ncclnt_ses_t* c_ses = ses.get()->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_ses);


  // Build a config string to send to the server mock
  XMLDocument::uptr_t test_dom(xml_mgr->create_document());
  ASSERT_TRUE(test_dom.get());

  XMLNode *m_root = test_dom->get_root_node();
  ASSERT_TRUE (m_root);

  XMLNode *top = m_root->add_child( "top", nullptr, ydt_top_ns, ydt_top_prefix );
  ASSERT_TRUE (top);

  XMLNode *top_a = top->add_child( "a", nullptr, ydt_top_ns, ydt_top_prefix );
  ASSERT_TRUE (top_a);

  XMLNode *cont_in_a = top_a->add_child( "cont-in-a", nullptr, ydt_top_ns, ydt_top_prefix );
  ASSERT_TRUE (cont_in_a);

  XMLNode *str_1_cont_in_a = cont_in_a->add_child( "str1", "fo", ydt_top_ns, ydt_top_prefix );
  ASSERT_TRUE (str_1_cont_in_a);

  XMLNode *num_1_cont_in_a = cont_in_a->add_child( "num1", "100", ydt_top_ns, ydt_top_prefix );
  ASSERT_TRUE (num_1_cont_in_a);

  std::string test_dom_str = test_dom->to_string();
  std::string cfg_str = test_dom_str;
  // add the config tag to the dom string
  cfg_str.insert (0, "<config>");
  cfg_str.append ("</config>");

  // build the config string
  Xml::suptr_t
      config_xml(Xml::create_xml_const_string(instance.get(), cfg_str.c_str()));

  // Send an edit-config, force an immediate server-side failure.
  rw_yang_netconf_op_status_t expected_status = RW_YANG_NETCONF_OP_STATUS_OK;
  rw_yang_netconf_op_status_t captured_status = RW_YANG_NETCONF_OP_STATUS_NULL;
  uint32_t callbacks = 0;
  Xml* captured_xml = nullptr;
  bool expected_callback = true;
    
  auto callback_lambda = [&] (rw_yang_netconf_op_status_t rsp_status, Xml* rsp_xml)
  {
    ++callbacks;
    if (captured_xml) {
      captured_xml->rw_mem_release();
    }
    EXPECT_TRUE(expected_callback);
    EXPECT_EQ(expected_status, rsp_status);
    captured_status = rsp_status;
    captured_xml = rsp_xml;
    if (captured_xml) {
      captured_xml->rw_mem_retain();
    }
  };

  rw_status_t rs = RW_STATUS_SUCCESS;
  //  bool expected_callback = true;
  unsigned expected_callbacks = 0;
  
  rs = ses->req_nc_edit_config(
    instance->get_running(),
    nullptr/*default_op*/,
    nullptr/*test_opt*/,
    nullptr/*error_opt*/,
    config_xml.get(),
    callback_lambda,
    cbmgr.get() );

  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_SUCCESS);
  ASSERT_NE(nullptr, captured_xml);
  captured_xml->rw_mem_release();
  captured_xml->rw_mem_release();
  captured_xml = nullptr;
  ++expected_callbacks;
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_NOTFOUND);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(nullptr, captured_xml);
  EXPECT_EQ(captured_status, RW_YANG_NETCONF_OP_STATUS_OK);

  // Check if the dom is the same
  std::string running_str = server_mock->running_->to_string();
  EXPECT_EQ (running_str, test_dom_str);


  // Send a get-config expect a response
  // server_mock->fail_next_nc_req_with_nc_err = RW_YANG_NETCONF_OP_STATUS_TOO_BIG;

  expected_callback = true;
  rs = ses->req_nc_get_config(
    instance->get_running(),
    nullptr /*filter*/,
    callback_lambda,
    cbmgr.get() );

  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_SUCCESS);
  ASSERT_NE(nullptr, captured_xml);

  char get_ret[1001];
  size_t ret_size;
  
  captured_xml->get_xml_str (get_ret, sizeof(get_ret) - 1, &ret_size);
  EXPECT_EQ (running_str, get_ret);
  captured_xml->rw_mem_release();
  captured_xml->rw_mem_release();
  captured_xml = nullptr;
  ++expected_callbacks;
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_NOTFOUND);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(nullptr, captured_xml);
  EXPECT_EQ(captured_status, RW_YANG_NETCONF_OP_STATUS_OK);

}

TEST(RwNcApiMockDirect, MockSuccess)
{
  TEST_DESCRIPTION("Test a couple of mock success through the whole stack");

  //// All the following code is common - should find a way to abstract it

  // Yang model
  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  ASSERT_NE(nullptr,ymodel.get());

  // Client library instance
  Instance::suptr_t instance(Instance::create(ymodel.get(), nullptr));
  ASSERT_NE(nullptr,instance.get());
  rw_ncclnt_instance_t* c_instance = instance->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_instance);

  // Callback manager
  CallbackManager::suptr_t cbmgr(CallbackManagerPollQ::create(instance.get()));
  EXPECT_NE(nullptr,cbmgr.get());
  rw_ncclnt_cbmgr_t* c_cbmgr = cbmgr->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_cbmgr);

  // XML manager for server mock
  XMLManager::uptr_t xml_mgr = std::move(xml_manager_create_xerces(ymodel.get()));
  EXPECT_NE(nullptr,xml_mgr.get());

  // Server mock
  std::unique_ptr<ServerMock> server_mock(new ServerMock(instance.get(), xml_mgr.get()));
  ASSERT_NE(nullptr,server_mock.get());

  // Create a user for the connection
  UniquishPtrRwMemoryTracking<User>::suptr_t user(User::create(instance.get(), "nobody"));
  ASSERT_NE(nullptr,user.get());
  rw_ncclnt_user_t* c_user = user.get()->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_user);

  // Finally, create the direct-transport and associated session
  UniquishPtrRwMemoryTracking<Session>::suptr_t
    ses(TransportDirect::create_session(instance.get(), server_mock.get(), user.get()));
  ASSERT_NE(nullptr,ses.get());
  rw_ncclnt_ses_t* c_ses = ses.get()->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_ses);

  // Junk XML blob
  Xml::suptr_t empty_config_xml(Xml::create_xml_const_string(instance.get(), "<config/>"));


  //////////////////////////////////////////////////////////////////////////////////

  // Utility lambdas for catching errors
  unsigned callbacks = 0;
  bool expected_callback = true;
  rw_yang_netconf_op_status_t expected_status = RW_YANG_NETCONF_OP_STATUS_OK;
  rw_yang_netconf_op_status_t captured_status = RW_YANG_NETCONF_OP_STATUS_NULL;
  Xml* captured_xml = nullptr;
  auto callback_lambda = [&] (rw_yang_netconf_op_status_t rsp_status, Xml* rsp_xml)
  {
    ++callbacks;
    if (captured_xml) {
      captured_xml->rw_mem_release();
    }
    EXPECT_TRUE(expected_callback);
    EXPECT_EQ(expected_status, rsp_status);
    captured_status = rsp_status;
    captured_xml = rsp_xml;
    if (captured_xml) {
      captured_xml->rw_mem_retain();
    }
  };

  rw_status_t rs = RW_STATUS_SUCCESS;

  //////////////////////////////////////////////////////////////////////////////////

  unsigned expected_callbacks = 0;


  // Send a get-config expect a response
  // server_mock->fail_next_nc_req_with_nc_err = RW_YANG_NETCONF_OP_STATUS_TOO_BIG;

  expected_callback = true;
  rs = ses->req_nc_get_config(
    instance->get_running(),
    nullptr /*filter*/,
    callback_lambda,
    cbmgr.get() );

  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_SUCCESS);
  ASSERT_NE(nullptr, captured_xml);
  captured_xml->rw_mem_release();
  captured_xml->rw_mem_release();
  captured_xml = nullptr;
  ++expected_callbacks;
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_NOTFOUND);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(nullptr, captured_xml);
  EXPECT_EQ(captured_status, RW_YANG_NETCONF_OP_STATUS_OK);



  //////////////////////////////////////////////////////////////////////////////////

  empty_config_xml.reset();
  //EXPECT_DEATH({ses.reset();RW_MEM_OBJECT_VALIDATE(c_ses);}, "is not a CF object");
  ses.reset();
  //EXPECT_DEATH({user.reset();RW_MEM_OBJECT_VALIDATE(c_user);}, "is not a CF object");
  user.reset();
  server_mock.reset();
  xml_mgr.reset();
  //EXPECT_DEATH({cbmgr.reset();RW_MEM_OBJECT_VALIDATE(c_cbmgr);}, "is not a CF object");
  cbmgr.reset();
  //EXPECT_DEATH({instance.reset();RW_MEM_OBJECT_VALIDATE(c_instance);}, "is not a CF object");
  instance.reset();
  ymodel.reset();
}

TEST(RwNcApiMockDirect, SubtreeFiltering)
{
  TEST_DESCRIPTION("Test few subfree filter tests to server mock");

  //Create the XML File
 const char *filename; 

 char file_buf[128];
 char buffer[1028];

 filename = tmpnam(file_buf);

 FILE *fp = fopen(filename, "w");

 ASSERT_NE(nullptr, fp);

 int len = fputs(xmlStr, fp);

 EXPECT_GT(len, 0);

 fclose(fp);
  
  //// All the following code is common - should find a way to abstract it

  // Yang model
  std::unique_ptr<YangModel> ymodel(YangModelNcx::create_model());
  ASSERT_NE(nullptr,ymodel.get());

  // Client library instance
  Instance::suptr_t instance(Instance::create(ymodel.get(), nullptr));
  ASSERT_NE(nullptr,instance.get());
  rw_ncclnt_instance_t* c_instance = instance->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_instance);

  // Callback manager
  CallbackManager::suptr_t cbmgr(CallbackManagerPollQ::create(instance.get()));
  EXPECT_NE(nullptr,cbmgr.get());
  rw_ncclnt_cbmgr_t* c_cbmgr = cbmgr->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_cbmgr);

  // XML manager for server mock
  XMLManager::uptr_t xml_mgr = std::move(xml_manager_create_xerces(ymodel.get()));
  EXPECT_NE(nullptr,xml_mgr.get());

  // Server mock
  std::unique_ptr<ServerMock> server_mock(new ServerMock(instance.get(), xml_mgr.get(), filename));
  ASSERT_NE(nullptr,server_mock.get());

  // Create a user for the connection
  UniquishPtrRwMemoryTracking<User>::suptr_t user(User::create(instance.get(), "nobody"));
  ASSERT_NE(nullptr,user.get());
  rw_ncclnt_user_t* c_user = user.get()->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_user);

  // Finally, create the direct-transport and associated session
  UniquishPtrRwMemoryTracking<Session>::suptr_t
    ses(TransportDirect::create_session(instance.get(), server_mock.get(), user.get()));
  ASSERT_NE(nullptr,ses.get());
  rw_ncclnt_ses_t* c_ses = ses.get()->rw_mem_to_c_type();
  ASSERT_NE(nullptr,c_ses);

  // A Containment filter
  rw_ncclnt_filter_t *test_filter = rw_ncclnt_filter_create_subtree_from_str(c_instance, "<breakfast_menu><food/></breakfast_menu>");


  //////////////////////////////////////////////////////////////////////////////////

  // Utility lambdas for catching errors
  unsigned callbacks = 0;
  bool expected_callback = true;
  rw_yang_netconf_op_status_t expected_status = RW_YANG_NETCONF_OP_STATUS_OK;
  rw_yang_netconf_op_status_t captured_status = RW_YANG_NETCONF_OP_STATUS_NULL;
  Xml* captured_xml = nullptr;
  auto callback_lambda = [&] (rw_yang_netconf_op_status_t rsp_status, Xml* rsp_xml)
  {
    ++callbacks;
    if (captured_xml) {
      captured_xml->rw_mem_release();
    }
    EXPECT_TRUE(expected_callback);
    EXPECT_EQ(expected_status, rsp_status);
    captured_status = rsp_status;
    captured_xml = rsp_xml;
    if (captured_xml) {
      captured_xml->rw_mem_retain();
    }
  };

  rw_status_t rs = RW_STATUS_SUCCESS;

  //////////////////////////////////////////////////////////////////////////////////

  unsigned expected_callbacks = 0;


  // Send a get-config expect a response
  // server_mock->fail_next_nc_req_with_nc_err = RW_YANG_NETCONF_OP_STATUS_TOO_BIG;

  expected_callback = true;
  rs = ses->req_nc_get_config(
    instance->get_running(),
    rw_mem_to_cpp_type(test_filter), /*filter*/
    callback_lambda,
    cbmgr.get() );

  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_SUCCESS);
  ASSERT_NE(nullptr, captured_xml);
  std::cout << captured_xml << std::endl;
  captured_xml->rw_mem_release();
  captured_xml->rw_mem_release();
  captured_xml = nullptr;
  ++expected_callbacks;
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_NOTFOUND);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(nullptr, captured_xml);
  EXPECT_EQ(captured_status, RW_YANG_NETCONF_OP_STATUS_OK);

  Filter *srv_filter =  server_mock->get_filter();
  ASSERT_NE(srv_filter, nullptr);
  rw_ncclnt_xml_t* srv_xml = srv_filter->get_xml();
  ASSERT_NE(srv_xml, nullptr);
  Xml* srv_xml_cpp = rw_mem_to_cpp_type(srv_xml);
  ASSERT_NE(srv_xml_cpp, nullptr);


  size_t size = 0;
  EXPECT_STREQ(srv_xml_cpp->get_xml_str(buffer, sizeof(buffer), &size), "<filter type=\"subtree\"><breakfast_menu><food/></breakfast_menu></filter>");
  EXPECT_EQ(size, 72);

  rw_ncclnt_xml_release(srv_xml);
  srv_xml = nullptr;
  srv_xml_cpp = nullptr;
 
  rw_ncclnt_filter_release(test_filter);

  // A  subtree filter 
  test_filter = rw_ncclnt_filter_create_subtree_from_str(c_instance, "<breakfast_menu><food><name>Berry-Berry Belgian Waffles</name></food></breakfast_menu>");
  //////////////////////////////////////////////////////////////////////////////////

  expected_callbacks = 0;
  callbacks = 0;
  expected_callback = true;
  expected_status = RW_YANG_NETCONF_OP_STATUS_OK;
  captured_status = RW_YANG_NETCONF_OP_STATUS_NULL;
  captured_xml = nullptr;

  // Send a get-config expect a response
  // server_mock->fail_next_nc_req_with_nc_err = RW_YANG_NETCONF_OP_STATUS_TOO_BIG;

  expected_callback = true;
  rs = ses->req_nc_get_config(
    instance->get_running(),
    rw_mem_to_cpp_type(test_filter), /*filter*/
    callback_lambda,
    cbmgr.get() );

  EXPECT_EQ(rs, RW_STATUS_SUCCESS);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_SUCCESS);
  ASSERT_NE(nullptr, captured_xml);
  captured_xml->rw_mem_release();
  captured_xml->rw_mem_release();
  captured_xml = nullptr;
  ++expected_callbacks;
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(cbmgr->poll_once(), RW_STATUS_NOTFOUND);
  EXPECT_EQ(callbacks, expected_callbacks);
  EXPECT_EQ(nullptr, captured_xml);
  std::cout << captured_xml << std::endl;
  EXPECT_EQ(captured_status, RW_YANG_NETCONF_OP_STATUS_OK);

  srv_filter =  server_mock->get_filter();
  ASSERT_NE(srv_filter, nullptr);
  srv_xml = srv_filter->get_xml();
  ASSERT_NE(srv_xml, nullptr);
  srv_xml_cpp = rw_mem_to_cpp_type(srv_xml);
  ASSERT_NE(srv_xml_cpp, nullptr);
  EXPECT_STREQ(srv_xml_cpp->get_xml_str(buffer, sizeof(buffer), &size), "<filter type=\"subtree\"><breakfast_menu><food><name>Berry-Berry Belgian Waffles</name></food></breakfast_menu></filter>");
  EXPECT_EQ(size, 118);

  rw_ncclnt_xml_release(srv_xml);
  srv_xml = nullptr;
  srv_xml_cpp = nullptr;
  size = 0;
  unlink(filename);

  rw_ncclnt_filter_release(test_filter);

}

char **g_argv;
int g_argc;

void rwnetconf_tests(int argc, char **argv)
{
  g_argv = argv;
  g_argc = argc;

}
RWUT_INIT(rwnetconf_tests);
