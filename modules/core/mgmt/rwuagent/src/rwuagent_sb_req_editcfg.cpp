
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwuagent_sb_req_editcfg.cpp
 *
 * Management agent southbound request support for configuration changes
 */

#include "rwuagent.hpp"

using namespace rw_uagent;
using namespace rw_yang;

SbReqEditConfig::SbReqEditConfig(
    Instance* instance,
    NbReq* nbreq,
    const char *xml_fragment,
    NetconfEditConfigOperations ec)
: SbReq(
    instance,
    nbreq,
    RW_MGMTAGT_SB_REQ_TYPE_EDITCONFIG,
    "SbReqEditConfigString" )
{
  // Build the DOM from the fragment.
  // Let any execptions propagate upwards - the clients that start a transaction
  // should handle it?
  RW_MA_SBREQ_LOG (this, __FUNCTION__, xml_fragment);

  std::string error_out;
  rw_yang::XMLDocument::uptr_t req =
      std::move(instance_->xml_mgr()->create_document_from_string(xml_fragment, error_out, false));

  if (ec_delete != ec) {
    delta_ = std::move(req);
  } else {
    delta_ = std::move(instance_->xml_mgr()->create_document(instance_->yang_model()->get_root_node()));

    XMLNode *node = req->get_root_node()->find_first_deepest_node();
    RW_ASSERT(node);

    rw_keyspec_path_t* key = nullptr;
    rw_yang_netconf_op_status_t ncrs = node->to_keyspec(&key);

    RW_ASSERT(ncrs == RW_YANG_NETCONF_OP_STATUS_OK);

    // Check if this config exists - fail if it doesnt
    std::list<XMLNode *> check;
    auto root = instance_->dom()->get_root_node();
    if (root) {
      ncrs = root->find(key, check);
      if ((RW_YANG_NETCONF_OP_STATUS_OK == ncrs) && check.size()) {

        rw_keyspec_path_set_category (key, NULL , RW_SCHEMA_CATEGORY_CONFIG);
        delete_ks_.push_back (rw_yang::XMLBuilder::uptr_ks(key));
        key = nullptr;
      }
      else {
        RW_MA_SBREQ_LOG (this, __FUNCTION__, "Delete node not found in agent DOM");
      }
    }
    if (nullptr != key) {
      rw_keyspec_path_free (key, NULL);
      key = nullptr;
    }
  }

  update_stats(RW_UAGENT_NETCONF_PARSE_REQUEST);
}

SbReqEditConfig::SbReqEditConfig(
    Instance* instance,
    NbReq* nbreq,
    XMLBuilder *builder)
: SbReq(
    instance,
    nbreq,
    RW_MGMTAGT_SB_REQ_TYPE_EDITCONFIG,
    "SbReqEditConfigDom" )
{
  delta_ = std::move(builder->merge_);
  delete_ks_.splice (delete_ks_.begin(), builder->delete_ks_);

  update_stats(RW_UAGENT_NETCONF_PARSE_REQUEST);
}

SbReqEditConfig::~SbReqEditConfig ()
{
  instance_->update_stats(sbreq_type(), req_.c_str(), &statistics_);
}

StartStatus SbReqEditConfig::start_xact_int()
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "xact edit-config start" );

  xact_ = rwdts_api_xact_create(
    dts_->api(),
    RWDTS_FLAG_ADVISE|RWDTS_FLAG_SUBSCRIBER|dts_flags_,
    dts_config_event_cb,
    this);

  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "xact dts created",
    RWMEMLOG_ARG_PRINTF_INTPTR("dts xact=0x%" PRIXPTR, (intptr_t)xact_) );

  auto *root_node = delta_->get_root_node();
  ProtoSplitter splitter(xact_, instance_->yang_model(),
                         dts_flags_, root_node);

  XMLWalkerInOrder walker(root_node->get_first_child());
  walker.walk(&splitter);

  // The transaction is all set to be shipped now?
  if (!(splitter.valid_ || delete_ks_.size())) {
    RW_MA_SBREQ_LOG (this, __FUNCTION__, "Skipping invalid edit config");
    return done_with_error( "Unable to convert edit-config to protobuf" );
  }

  for (auto &ks : delete_ks_) {
    rwdts_xact_block_t *blk = rwdts_xact_block_create(xact_);
    RW_ASSERT(blk);

    rw_status_t rs = rwdts_xact_block_add_query_ks(blk, ks.get(), RWDTS_QUERY_DELETE, RWDTS_FLAG_ADVISE | dts_flags_, 0, nullptr);
    if (rs != RW_STATUS_SUCCESS) {
      RW_MA_SBREQ_LOG (this, __FUNCTION__, "DTS Query add block failed");
      return done_with_error( "DTS operation failed" );
    }

    rs = rwdts_xact_block_execute(blk, dts_flags_, NULL, NULL, NULL);
    RW_ASSERT(rs == RW_STATUS_SUCCESS);
  }

  rw_status_t rs = rwdts_xact_commit(xact_);
  RW_MA_SBREQ_LOG (this, __FUNCTION__, "Executing transaction");
  RW_ASSERT(RW_STATUS_SUCCESS == rs);
  update_stats(RW_UAGENT_NETCONF_DTS_XACT_START);

  return StartStatus::Async;
}

void SbReqEditConfig::dts_config_event_cb(
  rwdts_xact_t *xact,
  rwdts_xact_status_t* xact_status,
  void *ud)
{
  if (xact_status->xact_done) {
    SbReqEditConfig *edit_cfg = static_cast<SbReqEditConfig *> (ud);
    RW_ASSERT (edit_cfg);
    edit_cfg->commit_cb (xact);
  }
}

void SbReqEditConfig::commit_cb(rwdts_xact_t *xact)
{
  RWMEMLOG(memlog_buf_, RWMEMLOG_MEM2, "dts xact callback",
    RWMEMLOG_ARG_PRINTF_INTPTR("dts xact=0x%" PRIXPTR, (intptr_t)xact_) );

  update_stats( RW_UAGENT_NETCONF_DTS_XACT_DONE );

  rwdts_xact_status_t status;
  auto rs = rwdts_xact_get_status (xact, &status);
  RW_ASSERT(RW_STATUS_SUCCESS == rs);

  switch (status.status) {
    case RWDTS_XACT_COMMITTED: {
      RW_MA_SBREQ_LOG (this, __FUNCTION__, "RWDTS_XACT_COMMITTED");
      XMLNode *root = instance_->dom()->get_root_node();

      // Delete nodes deleted by this transaction
      for (auto &ks : delete_ks_) {
        std::list<XMLNode *>found;
        rw_yang_netconf_op_status_t ncrs = root->find(ks.get(), found);

        if (ncrs != RW_YANG_NETCONF_OP_STATUS_OK ||
            !found.size()) {
          RW_MA_SBREQ_LOG (this, __FUNCTION__, "Error - config entry not found in DOM");
          break;
        }

        for (auto node : found) {
          XMLNode *parent = node->get_parent();
          RW_ASSERT(parent);
          parent->remove_child(node);
        }
      }
      // Merge delta with configuration DOM
      instance_->dom()->merge(delta_.get());

      delta_.release();
      delete_ks_.clear();
      update_stats( RW_UAGENT_NETCONF_RESPONSE_BUILT );
      done_with_success();
      return;
    }

    default:
    case RWDTS_XACT_FAILURE:
    case RWDTS_XACT_ABORTED: {
      RW_MA_SBREQ_LOG (this, __FUNCTION__, "RWDTS_XACT_ABORTED");
      rw_status_t rs;
      char *err_str = NULL;
      char *xpath = NULL;
      if (RW_STATUS_SUCCESS ==
          rwdts_xact_get_error_heuristic(xact, 0, &xpath, &rs, &err_str)) {
        RWMEMLOG(memlog_buf_, RWMEMLOG_MEM6, "Error response",
                 RWMEMLOG_ARG_PRINTF_INTPTR("sbreq=0x%" PRIX64,(intptr_t)this),
                 RWMEMLOG_ARG_PRINTF_INTPTR("dts xact=0x%" PRIXPTR, (intptr_t)xact) );
        NetconfErrorList nc_errors;
        NetconfError& err = nc_errors.add_error();
        if (xpath) {
          err.set_error_path(xpath);
          RW_FREE(xpath);
          xpath = NULL;
        }
        err.set_rw_error_tag(rs);
        if (err_str) {
          err.set_error_message(err_str);
          RW_FREE(err_str);
          err_str = NULL;
        }
        done_with_error(&nc_errors);
        return;
      }
      done_with_error( "Distributed transaction aborted or failed" );
      return;
    }
  }
}
