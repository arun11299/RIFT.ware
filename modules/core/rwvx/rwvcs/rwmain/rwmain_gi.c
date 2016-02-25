
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 *
 */

#include <rw-manifest.pb-c.h>
#include <rwlib.h>
#include <rwsched_main.h>
#include <rwtasklet.h>
#include <rwvcs_defs.h>
#include <rwvcs_rwzk.h>
#include <rwvx.h>

#include "rwmain.h"
#include "rwmain_gi.h"


static struct rwmain_gi * rwmain_gi_ref(struct rwmain_gi * rwmain_gi) {
  rwmain_gi->_refcnt++;
  return rwmain_gi;
}

static void rwmain_gi_unref(struct rwmain_gi * rwmain_gi) {
  rwmain_gi->_refcnt--;
  if (!rwmain_gi->_refcnt) {
    struct rwmain_tasklet * rt;
    struct rwmain_tasklet * bkup;

    RW_SKLIST_FOREACH_SAFE(rt, struct rwmain_tasklet, &(rwmain_gi->tasklets), _sklist, bkup)
      rwmain_tasklet_free(rt);

    rwtasklet_info_unref(rwmain_gi->tasklet_info);
    rwvx_instance_free(rwmain_gi->rwvx);
    free(rwmain_gi);
  }
}

G_DEFINE_BOXED_TYPE(rwmain_gi_t,
                    rwmain_gi,
                    rwmain_gi_ref,
                    rwmain_gi_unref);


static void sanitize_manifest(vcs_manifest * manifest) {
  if (!manifest->init_phase) {
    manifest->init_phase = (vcs_manifest_init *)malloc(sizeof(vcs_manifest_init));
    RW_ASSERT(manifest->init_phase);
    vcs_manifest_init__init(manifest->init_phase);
  }

  if (!manifest->init_phase->settings) {
    manifest->init_phase->settings = (vcs_manifest_settings *)malloc(sizeof(vcs_manifest_settings));
    RW_ASSERT(manifest->init_phase->settings);
    vcs_manifest_settings__init(manifest->init_phase->settings);
  }

  if (!manifest->init_phase->settings->rwmsg) {
    manifest->init_phase->settings->rwmsg = (vcs_manifest_rwmsg *)malloc(sizeof(vcs_manifest_rwmsg));
    RW_ASSERT(manifest->init_phase->settings->rwmsg);
    vcs_manifest_rwmsg__init(manifest->init_phase->settings->rwmsg);
  }

  if (!manifest->init_phase->settings->rwvcs) {
    manifest->init_phase->settings->rwvcs = (vcs_manifest_rwvcs *)malloc(sizeof(vcs_manifest_rwvcs));
    RW_ASSERT(manifest->init_phase->settings->rwvcs);
    vcs_manifest_rwvcs__init(manifest->init_phase->settings->rwvcs);
  }
  manifest->init_phase->settings->rwvcs->has_collapse_each_rwprocess = true;
  manifest->init_phase->settings->rwvcs->collapse_each_rwprocess = true;
  manifest->init_phase->settings->rwvcs->has_collapse_each_rwvm = true;
  manifest->init_phase->settings->rwvcs->collapse_each_rwvm = true;
}

struct rwmain_gi * rwmain_gi_new(rwpb_gi_RwManifest_Manifest * manifest_box) {
  rw_status_t status;
  struct rwmain_gi * rwmain_gi;
  struct rwvx_instance_s * rwvx;
  struct rwvcs_instance_s * rwvcs;
  struct rwtasklet_info_s * tinfo;
  vcs_manifest * manifest;

  rwmain_gi = (struct rwmain_gi *)malloc(sizeof(struct rwmain_gi));
  if (!rwmain_gi) {
    RW_ASSERT(0);
    return NULL;
  }

  RW_SKLIST_PARAMS_DECL(
      tasklets_sklist_params,
      struct rwmain_tasklet,
      instance_name,
      rw_sklist_comp_charptr,
      _sklist);
  RW_SKLIST_INIT(&(rwmain_gi->tasklets), &tasklets_sklist_params);

  rwvx = rwvx_instance_alloc();
  if (!rwvx) {
    RW_ASSERT(0);
    goto free_gi;
  }
  rwvcs = rwvx->rwvcs;

  RW_ASSERT(manifest_box->box.message->descriptor == RWPB_G_MSG_PBCMD(RwManifest_Manifest));
  manifest = (vcs_manifest *)manifest_box->box.message;
  rwvcs->pb_rwmanifest = (vcs_manifest *)protobuf_c_message_duplicate(
      NULL,
      &manifest->base,
      manifest->base.descriptor);
  if (!rwvcs->pb_rwmanifest) {
    RW_ASSERT(0);
    goto free_rwvx;
  }
  sanitize_manifest(rwvcs->pb_rwmanifest);

  status = rwcal_rwzk_zake_init(rwvx->rwcal_module);
  if (status != RW_STATUS_SUCCESS) {
    RW_ASSERT(0);
    goto free_rwvx;
  }

  status = rwvcs_rwzk_seed_auto_instance(rwvcs, 1000, NULL);
  if (status != RW_STATUS_SUCCESS) {
    RW_ASSERT(0);
    goto free_rwvx;
  }

  rwvcs->identity.rwvm_instance_id = 1;
  rwvcs->identity.vm_ip_address = strdup("127.0.0.1");
  if (!rwvcs->identity.vm_ip_address) {
    RW_ASSERT(0);
    goto free_rwvx;
  }

  tinfo = (struct rwtasklet_info_s *)malloc(sizeof(struct rwtasklet_info_s));
  if (!tinfo) {
    RW_ASSERT(0);
    goto free_rwvx;
  }

  tinfo->rwsched_instance = rwvx->rwsched;
  tinfo->rwsched_tasklet_info = rwsched_tasklet_new(rwvx->rwsched);
  tinfo->rwtrace_instance = rwvx->rwtrace;
  tinfo->rwvx = rwvx;
  tinfo->rwvcs = rwvx->rwvcs;
  tinfo->identity.rwtasklet_instance_id = 1;
  tinfo->identity.rwtasklet_name = strdup("rwmain_gi");
  tinfo->rwlog_instance = rwlog_init("rwmain_gi-1");
  tinfo->rwmsg_endpoint = rwmsg_endpoint_create(
      1,
      1,
      1,
      tinfo->rwsched_instance,
      tinfo->rwsched_tasklet_info,
      tinfo->rwtrace_instance,
      rwvcs->pb_rwmanifest->init_phase->settings->rwmsg);
  tinfo->ref_cnt = 1;


  rwmain_gi->_refcnt = 1;
  rwmain_gi->tasklet_info = tinfo;
  rwmain_gi->rwvx = rwvx;
  return rwmain_gi;

free_rwvx:
  if (rwvx->rwvcs->pb_rwmanifest)
    free(rwvx->rwvcs->pb_rwmanifest);
  free(rwvx->rwvcs);
  free(rwvx);

free_gi:
  free(rwmain_gi);

  return NULL;
}

rw_status_t rwmain_gi_add_tasklet(
    struct rwmain_gi * rwmain_gi,
    const char * plugin_dir,
    const char * plugin_name)
{
  rw_status_t status;
  rwvcs_instance_ptr_t rwvcs;
  uint32_t instance_id;
  char * instance_name;
  struct rwmain_tasklet * tasklet;

  rwvcs = rwmain_gi->rwvx->rwvcs;


  status = rwvcs_rwzk_next_instance_id(rwvcs, &instance_id, NULL);
  if (status != RW_STATUS_SUCCESS) {
    RW_ASSERT(0);
    return RW_STATUS_FAILURE;
  }

  instance_name = to_instance_name(plugin_name, instance_id);

  tasklet = rwmain_tasklet_alloc(
      instance_name,
      instance_id,
      plugin_name,
      plugin_dir,
      rwvcs);
  RW_FREE(instance_name);
  instance_name = NULL;
  if (!tasklet)
    return RW_STATUS_FAILURE;


  tasklet->plugin_interface->instance_start(
      tasklet->plugin_klass,
      tasklet->h_component,
      tasklet->h_instance);

  status = RW_SKLIST_INSERT(&(rwmain_gi->tasklets), tasklet);
  if (status != RW_STATUS_SUCCESS) {
    RW_ASSERT(0);
    goto free_tasklet;
  }

  return status;

free_tasklet:
  rwmain_tasklet_free(tasklet);

  return RW_STATUS_FAILURE;
}

rwtasklet_info_t * rwmain_gi_get_tasklet_info(rwmain_gi_t * rwmain_gi)
{
  return rwmain_gi->tasklet_info;
}

struct rwtasklet_info_s * rwmain_gi_new_tasklet_info(
    struct rwmain_gi * rwmain_gi,
    const char * name,
    uint32_t id)
{
  struct rwtasklet_info_s * tinfo;
  char * instance_name;

  tinfo = (struct rwtasklet_info_s *)malloc(sizeof(struct rwtasklet_info_s));
  if (!tinfo) {
    RW_ASSERT(0);
    return NULL;
  }

  instance_name = to_instance_name(name, id);

  tinfo->rwsched_instance = rwmain_gi->rwvx->rwsched;
  tinfo->rwsched_tasklet_info = rwsched_tasklet_new(rwmain_gi->rwvx->rwsched);
  tinfo->rwtrace_instance = rwmain_gi->rwvx->rwtrace;
  tinfo->rwvx = rwmain_gi->rwvx;
  tinfo->rwvcs = rwmain_gi->rwvx->rwvcs;
  tinfo->identity.rwtasklet_instance_id = id;
  tinfo->identity.rwtasklet_name = strdup(name);
  tinfo->rwlog_instance = rwlog_init(instance_name);
  tinfo->rwmsg_endpoint = rwmsg_endpoint_create(
      1,
      id,
      1,
      tinfo->rwsched_instance,
      tinfo->rwsched_tasklet_info,
      tinfo->rwtrace_instance,
      rwmain_gi->rwvx->rwvcs->pb_rwmanifest->init_phase->settings->rwmsg);
  tinfo->ref_cnt = 1;

  free(instance_name);
  return tinfo;
}

