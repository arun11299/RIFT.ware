
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwuagent_proto_splitter.cpp
 *
 * Protobuf conversion support.
 */

#include "rwuagent.hpp"

using namespace rw_uagent;
using namespace rw_yang;


ProtoSplitter::ProtoSplitter(
    rwdts_xact_t* xact,
    YangModel *model,
    RWDtsFlag flag,
    XMLNode* root_node )
: xact_(xact),
  dts_flags_(flag),
  root_node_(root_node)
{
}

rw_xml_next_action_t ProtoSplitter::visit(
    XMLNode* node,
    XMLNode** path,
    int32_t level )
{
  if (!(level >=0 && level < RW_MAX_XML_NODE_DEPTH)) {
    return RW_XML_ACTION_TERMINATE;
  }

  if (node == root_node_) {
    node = root_node_->get_first_child();
  }

  // In case of empty root xml
  // for ex. in delete queries
  if (nullptr == node) {
    return RW_XML_ACTION_NEXT;
  }

  YangNode *yn = node->get_yang_node();

  if (nullptr == yn) {
    return RW_XML_ACTION_NEXT;
  }

  ProtobufCMessage *message;
  rw_yang_netconf_op_status_t ncs = node->to_pbcm (&message);
  if (RW_YANG_NETCONF_OP_STATUS_OK != ncs) {
    return RW_XML_ACTION_TERMINATE;
  }

  valid_ = true;
  rw_keyspec_path_t* keyspec = nullptr;

  ncs = node->to_keyspec(&keyspec);
  if (RW_YANG_NETCONF_OP_STATUS_OK != ncs) {
    protobuf_c_message_free_unpacked (nullptr, message);
    return RW_XML_ACTION_TERMINATE;
  }

  rw_keyspec_path_set_category (keyspec, NULL, RW_SCHEMA_CATEGORY_CONFIG);

  rwdts_xact_block_t *blk = rwdts_xact_block_create(xact_);
  RW_ASSERT(blk);

  rw_status_t rs = rwdts_xact_block_add_query_ks(blk, keyspec, 
                                                 RWDTS_QUERY_UPDATE, 
                                                 RWDTS_FLAG_ADVISE | dts_flags_, 
                                                 0, message);
  if (rs != RW_STATUS_SUCCESS) {
    rw_keyspec_path_free(keyspec, NULL );
    protobuf_c_message_free_unpacked (nullptr, message);
    return RW_XML_ACTION_TERMINATE;
  }

  rs = rwdts_xact_block_execute(blk, dts_flags_, NULL, NULL, NULL);
  RW_ASSERT(rs == RW_STATUS_SUCCESS);

  rw_keyspec_path_free(keyspec, NULL );
  protobuf_c_message_free_unpacked (nullptr, message);

  return RW_XML_ACTION_NEXT_SIBLING;
}

