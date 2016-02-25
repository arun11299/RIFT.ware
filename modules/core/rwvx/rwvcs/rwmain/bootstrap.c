
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 *
 */

#include <rwtrace.h>
#include <rwvcs_component.h>
#include <rwvcs_defs.h>
#include <rwvcs_manifest.h>
#include <rwvcs_rwzk.h>
#include <rwcal_vars_api.h>


#include "rwmain.h"

/*
 * Bootstrap an RWVM.  This will process the bootstrap-phase/rwvm section
 * of the manifest.  Once bootstraped, the RWVM should be completely joined
 * to the current RIFT environment.
 *
 * @param rwmain          - rwmain instance
 * @return                - rw status from rwvcs_action_run()
 */
static rw_status_t bootstrap_rwvm(struct rwmain * rwmain);

/*
 * Bootstrap a RWPROC.  This will process the bootstrap-phase/rwproc section
 * of the manifest.  Once bootstraped, the RWPROC should be completely joined
 * to the current RIFT environment.
 *
 * @param rwmain          - rwmain instance
 * @return                - rw status from rwvcs_action_run()
 */
static rw_status_t bootstrap_rwproc(struct rwmain * rwmain);


static rw_status_t bootstrap_rwvm(struct rwmain * rwmain)
{
  rw_status_t status;
  char * self_id = NULL;

  RWTRACE_INFO(
      rwmain->rwvx->rwtrace,
      RWTRACE_CATEGORY_RWMAIN,
      "Performing bootstrap of RWVM %s",
      rwmain->component_name);

  status = RW_STATUS_SUCCESS;
  if (!rwmain->rwvx->rwvcs->pb_rwmanifest->bootstrap_phase->rwvm)
    goto done;

  self_id = to_instance_name(rwmain->component_name, rwmain->instance_id);

  for (size_t i = 0; i < rwmain->rwvx->rwvcs->pb_rwmanifest->bootstrap_phase->rwvm->n_instances; ++i) {
    vcs_manifest_action action;
    vcs_manifest_action_start start;

    vcs_manifest_action__init(&action);
    vcs_manifest_action_start__init(&start);

    action.start = &start;
    start.component_name = rwmain->rwvx->rwvcs->pb_rwmanifest->bootstrap_phase->rwvm->instances[i]->component_name;
    start.has_config_ready = rwmain->rwvx->rwvcs->pb_rwmanifest->bootstrap_phase->rwvm->instances[i]->has_config_ready;
    start.config_ready = rwmain->rwvx->rwvcs->pb_rwmanifest->bootstrap_phase->rwvm->instances[i]->config_ready;

    status = rwmain_action_run(rwmain, self_id, &action);

    action.start = NULL;
    start.component_name = NULL;
    protobuf_free_stack(action);
    protobuf_free_stack(start);
  }

done:
  if (self_id)
    free(self_id);

  RWTRACE_INFO(
      rwmain->rwvx->rwtrace,
      RWTRACE_CATEGORY_RWMAIN,
      "Bootstrap of RWVM complete");

  return status;
}

static rw_status_t bootstrap_rwproc(struct rwmain * rwmain)
{
  rw_status_t status;
  char * self_id = NULL;

  RWTRACE_INFO(
      rwmain->rwvx->rwtrace,
      RWTRACE_CATEGORY_RWMAIN,
      "Performing bootstrap of RWPROC %s",
      rwmain->component_name);

  status = RW_STATUS_SUCCESS;
  if (!rwmain->rwvx->rwvcs->pb_rwmanifest->bootstrap_phase->rwproc)
    goto done;

  self_id = to_instance_name(rwmain->component_name, rwmain->instance_id);

  for (size_t i = 0; i < rwmain->rwvx->rwvcs->pb_rwmanifest->bootstrap_phase->rwproc->n_instances; ++i) {
    vcs_manifest_action action;
    vcs_manifest_action_start start;

    vcs_manifest_action__init(&action);
    vcs_manifest_action_start__init(&start);

    action.start = &start;
    start.component_name = rwmain->rwvx->rwvcs->pb_rwmanifest->bootstrap_phase->rwproc->instances[i]->component_name;
    start.has_config_ready = rwmain->rwvx->rwvcs->pb_rwmanifest->bootstrap_phase->rwproc->instances[i]->has_config_ready;
    start.config_ready = rwmain->rwvx->rwvcs->pb_rwmanifest->bootstrap_phase->rwproc->instances[i]->config_ready;

    status = rwmain_action_run(rwmain, self_id, &action);

    action.start = NULL;
    start.component_name = NULL;
    protobuf_free_stack(action);
    protobuf_free_stack(start);
  }

done:
  if (self_id)
    free(self_id);

  RWTRACE_INFO(
      rwmain->rwvx->rwtrace,
      RWTRACE_CATEGORY_RWMAIN,
      "Bootstrap of RWPROC complete");

  return status;
}

/*
 * Initialize the zookeeper with a node for this component.  Used when the
 * component does not have a parent and therefore no one created the zookeeper
 * node yet.
 */
static rw_status_t update_zk(struct rwmain * rwmain)
{
  rw_status_t status;
  vcs_manifest_component * mdef;
  rwvcs_instance_ptr_t rwvcs;
  char * id = NULL;
  rw_component_info * ci = NULL;


  rwvcs = rwmain->rwvx->rwvcs;

  RW_ASSERT(!rwmain->parent_id);
  id = to_instance_name(rwmain->component_name, rwmain->instance_id);

  // As we only hit this point when the parent_id is NULL, we are pretty much
  // guaranteed that the definition has to be in the static manifest.
  status = rwvcs_manifest_component_lookup(rwvcs, rwmain->component_name, &mdef);
  if (status != RW_STATUS_SUCCESS) {
    RW_ASSERT(0);
    goto done;
  }


  if (mdef->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWVM) {
    ci = rwvcs_rwvm_alloc(
        rwmain->rwvx->rwvcs,
        rwmain->parent_id,
        rwmain->component_name,
        rwmain->instance_id,
        id);
    if (!ci) {
      RW_ASSERT(0);
      status = RW_STATUS_FAILURE;
      goto done;
    }

    ci->vm_info->vm_ip_address = strdup(rwmain->vm_ip_address);
    ci->vm_info->has_pid = true;
    ci->vm_info->pid = getpid();
    ci->has_state = true;
    ci->state = RW_BASE_STATE_TYPE_STARTING;

    if (mdef->rwvm && mdef->rwvm->has_leader) {
      ci->vm_info->has_leader = true;
      ci->vm_info->leader= mdef->rwvm->leader;
    }
  } else if (mdef->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWPROC) {
    ci = rwvcs_rwproc_alloc(
        rwvcs,
        rwmain->parent_id,
        rwmain->component_name,
        rwmain->instance_id,
        id);
    if (!ci) {
      RW_ASSERT(0);
      status = RW_STATUS_FAILURE;
      goto done;
    }

    ci->proc_info->has_pid = true;
    ci->proc_info->pid = getpid();
    ci->has_state = true;
    ci->proc_info->has_native = true;
    ci->proc_info->native = false;
    ci->state = RW_BASE_STATE_TYPE_STARTING;
  } else {
    RW_ASSERT(0);
    status = RW_STATUS_FAILURE;
    goto done;
  }

  status = rwvcs_rwzk_node_update(rwmain->rwvx->rwvcs, ci);
  if (status != RW_STATUS_SUCCESS) {
    RW_ASSERT(0);
    goto done;
  }


done:
  if (id)
    free(id);

  if (ci)
    protobuf_free(ci);

  return status;
}


rw_status_t rwmain_bootstrap(struct rwmain * rwmain)
{
  rw_status_t status;

  // If this instance has no parent then there is nothing in the zookeeper
  // yet.  We need to make a node so that anything started during bootstrap
  // can correctly link itself as a child.
  if (!rwmain->parent_id) {
    status = update_zk(rwmain);
    RW_ASSERT(status == RW_STATUS_SUCCESS);
  }

  if (rwmain->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWPROC)
    status = bootstrap_rwproc(rwmain);
  else if (rwmain->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWVM)
    status = bootstrap_rwvm(rwmain);
  else {
    status = RW_STATUS_NOTFOUND;
    RW_ASSERT(0);
    goto done;
  }

done:
  return status;
}
