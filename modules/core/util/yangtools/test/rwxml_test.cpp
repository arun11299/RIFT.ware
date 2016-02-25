 
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */




/**
 * @file rwxml_test.cpp
 * @author Vinod Kamalaraj
 * @date 2014/04/11
 * @brief XML wrapper library test program
 *
 * Unit tests for rw_xml_ximpl.cpp
 */

#include <limits.h>
#include <cstdlib>
#include <iostream>

#include "rwut.h"
#include "gmock/gmock.h"
#include "gtest/rw_gtest.h"

#include "confd_xml.h"
#include "rw_xml.h"
#include "yangtest_common.hpp"


using namespace rw_yang;
static void compare_xml_trees(XMLNode *old_node, XMLNode *new_node);

int g_argc;
char **g_argv;



TEST (RwXML, CreateDestroy)
{
  XMLManager::uptr_t mgr(xml_manager_create_xerces());
  TEST_DESCRIPTION ("Create and Destroy a DOM");

  XMLDocument::uptr_t doc(mgr->create_document());
  ASSERT_TRUE(doc.get());
  
  XMLNode *root = doc->get_root_node();
  ASSERT_TRUE (root);
  
  XMLNode *child_1 = root->add_child("level1_1");
  ASSERT_TRUE (child_1);

  XMLNode *child_2 = root->add_child("level1_2");
  ASSERT_TRUE (child_2);

  XMLNodeList::uptr_t list_1(doc->get_elements ("level1_1"));
  ASSERT_TRUE (list_1.get());
  ASSERT_EQ (1, list_1->length());

  XMLNode *dup = list_1->at(0);
  ASSERT_EQ (dup, child_1);

  list_1 = root->get_children();
  ASSERT_EQ (2, list_1->length());

  dup = root->find("level1_1");
  ASSERT_EQ (dup, child_1);

  dup = root->find("level1_2");
  ASSERT_EQ (dup, child_2);

  dup = root->find("level2_2");
  ASSERT_EQ (dup, nullptr);

  XMLNode *ns_child_1 = root->add_child("level1_1", nullptr, "rwtest/NS-1", "NS1");
  ASSERT_TRUE (ns_child_1);

  XMLNode *ns_child_2 = root->add_child("level1_2", nullptr, "rwtest/NS-1", "NS1");
  ASSERT_TRUE (ns_child_2);
  
  list_1 = std::move(doc->get_elements ("level1_1"));
  ASSERT_TRUE (list_1.get());
  ASSERT_EQ (1, list_1->length());

  dup = list_1->at(0);
  ASSERT_EQ (dup, child_1);

  list_1 = std::move(doc->get_elements ("level1_1", "rwtest/NS-1"));
  ASSERT_TRUE (list_1.get());
  ASSERT_EQ (1, list_1->length());

  dup = list_1->at(0);
  ASSERT_EQ (dup, ns_child_1);

  std::string tmp_str;
  std::string exp_str = "<root xmlns=\"http://riftio.com/ns/riftware-1.0/rw-base\"><level1_1/><level1_2/><NS1:level1_1 xmlns:NS1=\"rwtest/NS-1\"/><NS1:level1_2 xmlns:NS1=\"rwtest/NS-1\"/></root>";

  tmp_str = doc->to_string();
  EXPECT_EQ (tmp_str, exp_str);

  CFMutableStringRef cf = CFStringCreateMutable (NULL, 0);
  rw_xml_document_to_cfstring ((rw_xml_document_t *) doc.get(),
                               cf);

  char from_cf[500];

  CFStringGetCString (cf, from_cf, sizeof (from_cf), kCFStringEncodingUTF8);

  EXPECT_STREQ (tmp_str.c_str(), from_cf);
  std::string error_out;
  XMLDocument::uptr_t dup_doc = mgr->create_document_from_string (tmp_str.c_str(), error_out, false);

  list_1 = std::move(dup_doc->get_elements ("level1_1"));
  ASSERT_TRUE (list_1.get());
  ASSERT_EQ (1, list_1->length());

  dup = list_1->at(0);

  XMLNode *child_1_1 = child_1->add_child("level1_1_1", "chi_1_1-val0");
  ASSERT_TRUE (child_1_1);

  XMLNode *child_1_2 = child_1->add_child("level1_1_1", "chi_1_1-val2");
  ASSERT_TRUE (child_1_2);
  
  tmp_str = root->to_string();
  std::cout << tmp_str << std::endl;

  rw_xml_node_to_cfstring ((rw_xml_node_t*) root, cf);
  CFStringGetCString (cf, from_cf, sizeof (from_cf), kCFStringEncodingUTF8);

  EXPECT_STREQ (tmp_str.c_str(), from_cf);
  
  bool remove_status = child_1->remove_child(child_1_2);
  ASSERT_TRUE(remove_status);

  tmp_str = root->to_string();
  std::cout << "Original after removal = "<< std::endl<<tmp_str << std::endl;

  root = dup_doc->get_root_node();

  std::cout << "Dup doc " << std::endl;
  tmp_str = root->to_stdout();
  std::cout << std::endl;
  CFRelease(cf);
}

TEST (RwXML, Attributes)
{
  XMLManager::uptr_t mgr(xml_manager_create_xerces());
  TEST_DESCRIPTION ("Test Attribute support in RW XML");

  XMLDocument::uptr_t doc(mgr->create_document());
  ASSERT_TRUE(doc.get());
  
  XMLNode *root = doc->get_root_node();
  ASSERT_TRUE (root);
  
  XMLNode *child_1 = root->add_child("level1_1");
  ASSERT_TRUE (child_1);

  XMLNode *child_2 = root->add_child("level1_2");
  ASSERT_TRUE (child_2);

  XMLNodeList::uptr_t list_1(doc->get_elements ("level1_1"));
  ASSERT_TRUE (list_1.get());
  ASSERT_EQ (1, list_1->length());

  XMLNode *dup = list_1->at(0);
  ASSERT_EQ (dup, child_1);

  list_1 = root->get_children();
  ASSERT_EQ (2, list_1->length());

  XMLNode *ns_child_1 = root->add_child("level1_1", nullptr, "rwtest/NS-1", "NS1");
  ASSERT_TRUE (ns_child_1);

  XMLNode *ns_child_2 = root->add_child("level1_2", nullptr, "rwtest/NS-1", "NS1");
  ASSERT_TRUE (ns_child_2);
  
  list_1 = std::move(doc->get_elements ("level1_1"));
  ASSERT_TRUE (list_1.get());
  ASSERT_EQ (1, list_1->length());

  XMLAttributeList::uptr_t list_a1(child_1->get_attributes());
  ASSERT_TRUE (list_a1.get());
  ASSERT_EQ (0, list_a1->length());

  child_1->set_attribute("attr1", "value1");
  const char *ns = "http://www.riftio.com/namespace";
  child_1->set_attribute("attr2", "value2", ns, "ns");
  ASSERT_TRUE (child_1->has_attributes());
  ASSERT_TRUE (child_1->has_attribute("attr1"));
  //XMLAttribute::uptr_t attr = std::move(child_1->get_attribute("attr1"));
  //ASSERT_NE (nullptr, attr.get());
  list_a1 = std::move(child_1->get_attributes());
  ASSERT_TRUE (list_a1.get());
  ASSERT_EQ (2, list_a1->length());
#if 0
  for (uint32_t i = 0; i <  list_a1->length(); i++) {
    XMLAttribute *attr = list_a1->at(i);
    std::cout <<  attr->get_node_name().c_str() << " : " << attr->get_value().c_str() << std::endl;
  }
#endif

  EXPECT_STREQ(list_a1->at(0)->get_local_name().c_str(), "attr1");
  EXPECT_STREQ(list_a1->at(0)->get_value().c_str(), "value1");
  EXPECT_STREQ(list_a1->at(1)->get_local_name().c_str(), "attr2");
  EXPECT_STREQ(list_a1->at(1)->get_prefix().c_str(), "ns");
  EXPECT_STREQ(list_a1->at(1)->get_text_value().c_str(), "value2");
  EXPECT_STREQ(list_a1->at(1)->get_name_space().c_str(), "http://www.riftio.com/namespace");
  EXPECT_STREQ(list_a1->at(1)->get_value().c_str(), "value2");


  child_2->set_attribute("attr3", "value3");
  child_2->set_attribute("attr4", "value4");
  child_2->set_attribute("attr5", "value5");

  XMLAttributeList::uptr_t list_a2(child_2->get_attributes());
  ASSERT_TRUE (list_a2.get());
  ASSERT_EQ (3, list_a2->length());

  std::string tmp_str;
  std::string exp_str = "<root xmlns=\"http://riftio.com/ns/riftware-1.0/rw-base\"><level1_1 attr1=\"value1\" xmlns:ns=\"http://www.riftio.com/namespace\" ns:attr2=\"value2\"/><level1_2 attr3=\"value3\" attr4=\"value4\" attr5=\"value5\"/><NS1:level1_1 xmlns:NS1=\"rwtest/NS-1\"/><NS1:level1_2 xmlns:NS1=\"rwtest/NS-1\"/></root>";
  tmp_str = doc->to_string();
  ASSERT_EQ (tmp_str, exp_str);
}

TEST (RwXML, MappingFunction)
{
  EXPECT_DEATH (rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_NULL),"");
  EXPECT_EQ (RW_STATUS_SUCCESS, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_OK));
  EXPECT_EQ (RW_STATUS_EXISTS, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_IN_USE));
  EXPECT_EQ (RW_STATUS_EXISTS, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_DATA_EXISTS));

  EXPECT_EQ (RW_STATUS_NOTFOUND, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_INVALID_VALUE));
  EXPECT_EQ (RW_STATUS_NOTFOUND, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_MISSING_ATTRIBUTE));
  EXPECT_EQ (RW_STATUS_NOTFOUND, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_MISSING_ELEMENT));
  EXPECT_EQ (RW_STATUS_NOTFOUND, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_DATA_MISSING));

  EXPECT_EQ (RW_STATUS_OUT_OF_BOUNDS, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_TOO_BIG));
  EXPECT_EQ (RW_STATUS_OUT_OF_BOUNDS, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_BAD_ATTRIBUTE));
  EXPECT_EQ (RW_STATUS_OUT_OF_BOUNDS, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_BAD_ELEMENT));

  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_UNKNOWN_ATTRIBUTE));
  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_UNKNOWN_ELEMENT));
  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_UNKNOWN_NAMESPACE));
  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_ACCESS_DENIED));
  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_LOCK_DENIED));
  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_RESOURCE_DENIED));
  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_ROLLBACK_FAILED));
  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_OPERATION_NOT_SUPPORTED));
  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_MALFORMED_MESSAGE));
  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_OPERATION_FAILED));
  EXPECT_EQ (RW_STATUS_FAILURE, rw_yang_netconf_op_status_to_rw_status(RW_YANG_NETCONF_OP_STATUS_FAILED));
}
TEST (RwXML,XMLWalkerTest)
{
  TEST_DESCRIPTION("Verify the functioning of XMLWalker/XMLCopier");
  XMLManager::uptr_t mgr(xml_manager_create_xerces());
  ASSERT_TRUE(mgr.get());

  YangModel* model = mgr->get_yang_model();
  ASSERT_TRUE(model);

  YangModule* ydom_top = model->load_module("company");
  EXPECT_TRUE(ydom_top);

  std::ifstream fp;

  std::string file_name = get_rift_root() +
      std::string("/modules/core/util/yangtools/test/company.xml");

  XMLDocument::uptr_t doc(mgr->create_document_from_file(file_name.c_str(), false));
  ASSERT_TRUE(doc.get());

  XMLNode* root = doc->get_root_node();

  EXPECT_TRUE(root);

  EXPECT_STREQ(root->get_local_name().c_str(), "company");

  std::string error_out;
  XMLDocument::uptr_t to_doc(mgr->create_document_from_string("<root/>", error_out, false/*validate*/));
  ASSERT_TRUE(to_doc.get());
  
  ASSERT_TRUE(to_doc->get_root_node());

  XMLCopier copier(to_doc->get_root_node());

  XMLWalkerInOrder walker(root);

  walker.walk(&copier);

  to_doc->to_stdout();
  std::cout << std::endl;

  
  // Now walk the copied tree along side the original tree and make sure they match
  //
  //
  root = to_doc->get_root_node();
  EXPECT_STREQ(root->get_local_name().c_str(), "root");

  XMLNode *old_node = doc->get_root_node();
  
  XMLNode *new_node = root->get_first_child();

  EXPECT_STREQ(old_node->get_local_name().c_str(), new_node->get_local_name().c_str());

  ASSERT_NO_FATAL_FAILURE(compare_xml_trees(old_node, new_node));
}

static void compare_xml_trees(XMLNode *old_node, XMLNode *new_node)
{
  ASSERT_TRUE(old_node);
  ASSERT_TRUE(new_node);
 
  EXPECT_STREQ(old_node->get_local_name().c_str(), new_node->get_local_name().c_str());

  XMLNode *old_ch = old_node->get_first_child();
  XMLNode *new_ch = new_node->get_first_child();

  while (old_ch) {
    EXPECT_NE(old_ch, nullptr);
    EXPECT_NE(new_ch, nullptr);

    EXPECT_STREQ(old_ch->get_local_name().c_str(), new_ch->get_local_name().c_str());
    EXPECT_STREQ(old_ch->get_name_space().c_str(), new_ch->get_name_space().c_str());
    EXPECT_STREQ(old_ch->get_text_value().c_str(), new_ch->get_text_value().c_str());

    compare_xml_trees(old_ch, new_ch);

    old_ch = old_ch->get_next_sibling();
    new_ch = new_ch->get_next_sibling();
  } while (old_ch != nullptr);

}

void mySetup(int argc, char** argv)
{
  //::testing::InitGoogleTest(&argc,argv);

  /* Used by yangdom_test */
  g_argc = argc;
  g_argv = argv;

}

RWUT_INIT(mySetup);
