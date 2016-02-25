
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


#include <getopt.h>
#include <unistd.h>
#include <sys/resource.h>

#include <rwdts.h>
#include <rwmsg_clichan.h>
#include <rwtasklet.h>
#include <rwtrace.h>
#include <rwvcs.h>
#include <rwvcs_manifest.h>
#include <rwvcs_rwzk.h>

#include "rwdts_int.h"

#include "rwmain.h"

#define CHECK_DTS_MAX_ATTEMPTS 100
#define CHECK_DTS_FREQUENCY 100

struct wait_on_dts_cls {
  struct rwmain * rwmain;
  uint16_t attempts;
};

static rw_component_type component_type_str_to_enum(const char * type)
{
  if (!strcmp(type, "rwcollection"))
    return RWVCS_TYPES_COMPONENT_TYPE_RWCOLLECTION;
  else if (!strcmp(type, "rwvm"))
    return RWVCS_TYPES_COMPONENT_TYPE_RWVM;
  else if (!strcmp(type, "rwproc"))
    return RWVCS_TYPES_COMPONENT_TYPE_RWPROC;
  else if (!strcmp(type, "proc"))
    return RWVCS_TYPES_COMPONENT_TYPE_PROC;
  else if (!strcmp(type, "rwtasklet"))
    return RWVCS_TYPES_COMPONENT_TYPE_RWTASKLET;
  else {
    RW_ASSERT(0);
    return RWVCS_TYPES_COMPONENT_TYPE_RWCOLLECTION;
  }
}


/*
 * Schedule the next rwmain phase.  This will run on the scheduler in the
 * next interation.
 *
 * @param rwmain    - rwmain instance
 * @param next      - next phase function
 * @param frequency - times per second to execute timer, 0 to run only once
 * @param ctx       - if defined, context passed to the next phase, otherwise the
 *                    rwmain instance is passed.
 */
static void schedule_next(
    struct rwmain * rwmain,
    rwsched_CFRunLoopTimerCallBack next,
    uint16_t frequency,
    void * ctx)
{
  rwsched_CFRunLoopTimerRef cftimer;
  rwsched_CFRunLoopTimerContext cf_context;


  bzero(&cf_context, sizeof(rwsched_CFRunLoopTimerContext));
  cf_context.info = ctx ? ctx : rwmain;

  cftimer = rwsched_tasklet_CFRunLoopTimerCreate(
      rwmain->rwvx->rwsched_tasklet,
      kCFAllocatorDefault,
      CFAbsoluteTimeGetCurrent(),
      frequency ? 1.0 / (double)frequency : 0,
      0,
      0,
      next,
      &cf_context);

  rwsched_tasklet_CFRunLoopAddTimer(
      rwmain->rwvx->rwsched_tasklet,
      rwsched_tasklet_CFRunLoopGetCurrent(rwmain->rwvx->rwsched_tasklet),
      cftimer,
      rwmain->rwvx->rwsched->main_cfrunloop_mode);
}


static void init_rwtrace(rwvx_instance_ptr_t rwvx, char ** argv)
{
  rw_status_t status;
  char * cmdline;

  status = rwtrace_ctx_category_severity_set(
      rwvx->rwtrace,
      RWTRACE_CATEGORY_RWMAIN,
      RWTRACE_SEVERITY_DEBUG);
  RW_ASSERT(status == RW_STATUS_SUCCESS);
  status = rwtrace_ctx_category_destination_set(
      rwvx->rwtrace,
      RWTRACE_CATEGORY_RWMAIN,
      RWTRACE_DESTINATION_CONSOLE);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  cmdline = g_strjoinv(" | ", argv);
  RWTRACE_DEBUG(
      rwvx->rwtrace,
      RWTRACE_CATEGORY_RWMAIN,
      "main() entered, cmdline = \"%s\"",
      cmdline);
  g_free(cmdline);
}

static int main_function(int argc, char ** argv, char ** envp)
{
  rwmain_setup(argc, argv, envp);

  return 0;
}

static rwtasklet_info_ptr_t get_rwmain_tasklet_info(
    rwvx_instance_ptr_t rwvx,
    const char * component_name,
    int instance_id,
    uint32_t vm_instance_id)
{
  rwtasklet_info_ptr_t info;
  char * instance_name = NULL;
  int broker_instance_id;

  info = (rwtasklet_info_ptr_t)malloc(sizeof(struct rwtasklet_info_s));
  if (!info) {
    RW_ASSERT(0);
    goto err;
  }

  instance_name = to_instance_name(component_name, instance_id);
  if (!instance_name) {
    RW_ASSERT(0);
    goto err;
  }

  info->rwsched_instance = rwvx->rwsched;
  info->rwsched_tasklet_info = rwsched_tasklet_new(rwvx->rwsched);
  info->rwtrace_instance = rwvx->rwtrace;
  info->rwvx = rwvx;
  info->rwvcs = rwvx->rwvcs;

  info->identity.rwtasklet_instance_id = instance_id;
  info->identity.rwtasklet_name = strdup(component_name);
  info->rwlog_instance = rwlog_init(instance_name);

  broker_instance_id = 0;
  if (rwvx->rwvcs->pb_rwmanifest->init_phase->settings->rwmsg->multi_broker
      && rwvx->rwvcs->pb_rwmanifest->init_phase->settings->rwmsg->multi_broker->has_enable
      && rwvx->rwvcs->pb_rwmanifest->init_phase->settings->rwmsg->multi_broker->enable) {
    broker_instance_id = vm_instance_id ? vm_instance_id : 1;
  }

  info->rwmsg_endpoint = rwmsg_endpoint_create(
    1,
    instance_id,
    broker_instance_id,
    info->rwsched_instance,
    info->rwsched_tasklet_info,
    info->rwtrace_instance,
    rwvx->rwvcs->pb_rwmanifest->init_phase->settings->rwmsg);

  rwtasklet_info_ref(info);
  free(instance_name);

  return info;

err:
  if (info)
    free(info);

  if (instance_name)
    free(instance_name);

  return NULL;
}

static void rwmain_dts_handle_state_change(rwdts_api_t*  apih, 
                                           rwdts_state_t state,
                                           void*         ud)
{
}

static struct rwmain * rwmain_alloc(
    rwvx_instance_ptr_t rwvx,
    const char * component_name,
    uint32_t instance_id,
    const char * component_type,
    const char * parent_id,
    const char * vm_ip_address,
    uint32_t vm_instance_id)
{
  rw_status_t status;
  int r;
  rwtasklet_info_ptr_t info = NULL;
  rwdts_api_t * dts = NULL;
  struct rwmain * rwmain = NULL;
  char * instance_name = NULL;


  rwmain = (struct rwmain *)malloc(sizeof(struct rwmain));
  if (!rwmain) {
    RW_ASSERT(0);
    goto err;
  }
  bzero(rwmain, sizeof(struct rwmain));

  /* If the component name wasn't specified on the command line, pull it
   * from the manifest init-phase.
   */
  if (!component_name) {
    char cn[1024];

    status = rwvcs_variable_evaluate_str(
        rwvx->rwvcs,
        "$rw_component_name",
        cn,
        sizeof(cn));
    if (status != RW_STATUS_SUCCESS) {
      RW_ASSERT(0);
      goto err;
    }

    rwmain->component_name = strdup(cn);
  } else {
    rwmain->component_name = strdup(component_name);
  }

  if (!rwmain->component_name) {
    RW_ASSERT(0);
    goto err;
  }


  /* If the instance id wasn't specified on the command line pull it from
   * the manifest init-phase if it is there, otherwise autoassign one.
   */
  if (instance_id == 0) {
    int id;

    status = rwvcs_variable_evaluate_int(
        rwvx->rwvcs,
        "$instance_id",
        &id);
    if (status == RW_STATUS_SUCCESS) {
      rwmain->instance_id = (uint32_t)id;
    } else {
      status = rwvcs_rwzk_next_instance_id(rwvx->rwvcs, &rwmain->instance_id, NULL);
      if (status != RW_STATUS_SUCCESS) {
        RW_ASSERT(0);
        goto err;
      }
    }
  } else {
    rwmain->instance_id = instance_id;
  }

  if (component_type) {
    rwmain->component_type = component_type_str_to_enum(component_type);
  } else {
    char ctype[64];
    status = rwvcs_variable_evaluate_str(
        rwvx->rwvcs,
        "$component_type",
        ctype,
        sizeof(ctype));
    if (status != RW_STATUS_SUCCESS) {
      RW_ASSERT(0);
      goto err;
    }
    rwmain->component_type = component_type_str_to_enum(ctype);
  }

  if (vm_instance_id > 0)
    rwmain->vm_instance_id = vm_instance_id;
  else if (rwmain->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWVM)
    rwmain->vm_instance_id = rwmain->instance_id;
  else
    RW_ASSERT(0);

  info = get_rwmain_tasklet_info(
      rwvx,
      rwmain->component_name,
      rwmain->instance_id,
      rwmain->vm_instance_id);
  if (!info) {
    RW_ASSERT(0);
    goto err;
  }

  // 10 hz with tolerance 64
  rwmain->rwproc_heartbeat = rwproc_heartbeat_alloc(10, 640);
  if (!rwmain->rwproc_heartbeat) {
    RW_ASSERT(0);
    goto err;
  }

  bzero(&rwmain->sys, sizeof(rwmain->sys));

  if (parent_id) {
    rwmain->parent_id = strdup(parent_id);
    if (!rwmain->parent_id) {
      RW_ASSERT(0);
      goto err;
    }
  }


  if (rwvx->rwvcs->pb_rwmanifest->init_phase->settings->rwvcs->collapse_each_rwvm) {
    r = asprintf(&rwmain->vm_ip_address, "127.%u.%u.1", rwmain->instance_id / 256, rwmain->instance_id % 256);
    if (r == -1) {
      RW_ASSERT(0);
      goto err;
    }
  } else if (vm_ip_address) {
    rwmain->vm_ip_address = strdup(vm_ip_address);
    if (!rwmain->vm_ip_address) {
      RW_ASSERT(0);
      goto err;
    }
    char *variable[0];
    r = asprintf(&variable[0], "vm_ip_address = '%s'", vm_ip_address);
    if (r == -1) {
      RW_ASSERT(0);
      goto err;
    }
    status = rwvcs_variable_list_evaluate(
        rwvx->rwvcs,
        1,
        variable);
    if (status != RW_STATUS_SUCCESS) {
      RW_ASSERT(0);
      goto err;
    }
    free(variable[0]);
  } else {
    char buf[32];

    status = rwvcs_variable_evaluate_str(
        rwvx->rwvcs,
        "$vm_ip_address",
        buf,
        sizeof(buf));
    if (status != RW_STATUS_SUCCESS) {
      RW_ASSERT(0);
      goto err;
    }

    rwmain->vm_ip_address = strdup(buf);
    if (!rwmain->vm_ip_address) {
      RW_ASSERT(0);
      goto err;
    }
  }


  rwvx->rwvcs->identity.vm_ip_address = strdup(rwmain->vm_ip_address);
  if (!rwvx->rwvcs->identity.vm_ip_address) {
    RW_ASSERT(0);
    goto err;
  }
  rwvx->rwvcs->identity.rwvm_instance_id = rwmain->vm_instance_id;

  instance_name = to_instance_name(rwmain->component_name, rwmain->instance_id);
  RW_ASSERT(instance_name!=NULL);
  dts = rwdts_api_new(
      info,
      (rw_yang_pb_schema_t *)RWPB_G_SCHEMA_YPBCSD(RwVcs),
      rwmain_dts_handle_state_change,
      NULL,
      NULL);

  if (!dts) {
    RW_ASSERT(0);
    goto err;
  }

  RW_SKLIST_PARAMS_DECL(
      procs_sklist_params,
      struct rwmain_proc,
      instance_name,
      rw_sklist_comp_charptr,
      _sklist);
  RW_SKLIST_INIT(&(rwmain->procs), &procs_sklist_params);

  RW_SKLIST_PARAMS_DECL(
      tasklets_sklist_params,
      struct rwmain_tasklet,
      instance_name,
      rw_sklist_comp_charptr,
      _sklist);
  RW_SKLIST_INIT(&(rwmain->tasklets), &tasklets_sklist_params);

  RW_SKLIST_PARAMS_DECL(
      multivms_sklist_params,
      struct rwmain_multivm,
      key,
      rw_sklist_comp_charbuf,
      _sklist);
  RW_SKLIST_INIT(&(rwmain->multivms), &multivms_sklist_params);


  rwmain->dts = dts;
  rwmain->tasklet_info = info;
  rwmain->rwvx = rwvx;
  r = asprintf(&VCS_GET(rwmain)->vcs_instance_xpath,
               VCS_INSTANCE_XPATH_FMT,
               instance_name);
  if (r == -1) {
    RW_ASSERT(0);
    goto err;
  }
  VCS_GET(rwmain)->instance_name = instance_name;

  goto done;

err:
  if (info) {
    rwsched_tasklet_free(info->rwsched_tasklet_info);
    free(info->identity.rwtasklet_name);
    rwmsg_endpoint_halt(info->rwmsg_endpoint);
    free(info);
  }

  if (dts)
    rwdts_api_deinit(dts);

  if (rwmain->component_name)
    free(rwmain->component_name);

  if (rwmain->parent_id)
    free(rwmain->parent_id);

  if (rwmain)
    free(rwmain);

done:

  return rwmain;
}

rw_status_t process_init_phase(struct rwmain * rwmain)
{
  rw_status_t status;
  rwvcs_instance_ptr_t rwvcs;

  rwvcs = rwmain->rwvx->rwvcs;

  if (rwvcs->pb_rwmanifest->init_phase->settings->rwvcs->no_autostart == false) {
    vcs_manifest_component *m_component;
    char * instance_name = NULL;

    instance_name = to_instance_name(rwmain->component_name, rwmain->instance_id);
    RW_ASSERT(*instance_name);

    // Lookup the component to start
    status = rwvcs_manifest_component_lookup(rwvcs, rwmain->component_name, &m_component);
    rwmain_trace_info(rwmain, "rwvcs_manifest_component_lookup %s", rwmain->component_name);
    RW_ASSERT(status == RW_STATUS_SUCCESS);

    if (m_component->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWVM) {
      RWVCS_LATENCY_CHK_PRE(rwmain->rwvx->rwsched);
      rwmain_rwvm_init(
          rwmain,
          m_component->rwvm,
          rwmain->component_name,
          rwmain->instance_id,
          instance_name,
          rwmain->parent_id);
      RWVCS_LATENCY_CHK_POST(rwmain->rwvx->rwtrace, RWTRACE_CATEGORY_RWMAIN,
                             rwmain_rwvm_init, "rwmain_rwvm_init:%s", instance_name);
    } else if (m_component->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWPROC) {
      RWVCS_LATENCY_CHK_PRE(rwmain->rwvx->rwsched);
      rwmain_rwproc_init(
          rwmain,
          m_component->rwproc,
          rwmain->component_name,
          rwmain->instance_id,
          instance_name,
          rwmain->parent_id);
      RWVCS_LATENCY_CHK_POST(rwmain->rwvx->rwtrace, RWTRACE_CATEGORY_RWMAIN,
                             rwmain_rwproc_init, "rwmain_rwproc_init:%s", instance_name);
    } else {
      rwmain_trace_crit(
          rwmain,
          "rwmain cannot start a component which is not a vm or process (%s)",
          m_component->component_name);
      RW_CRASH();
    }
  }

  return RW_STATUS_SUCCESS;
}

static void init_phase(rwsched_CFRunLoopTimerRef timer, void * ctx)
{
  rw_status_t status;
  struct rwmain * rwmain;

  rwmain = (struct rwmain *)ctx;

  rwmain_setup_cputime_monitor(rwmain);

  status = rwmain_setup_dts_registrations(rwmain);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  status = process_init_phase(rwmain);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  status = rwmain_setup_dts_rpcs(rwmain);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  rwsched_tasklet_CFRunLoopTimerRelease(
    rwmain->rwvx->rwsched_tasklet,
    timer);
}

/*
 * Called when we get a response from DTS with any additional component
 * definitions added to the inventory via runtime configuration.
 *
 * Adds any new components to the manifest held by rwvcs and then schedules
 * the init_phase.
 */
static void on_op_inventory_update(rwdts_xact_t * xact, rwdts_xact_status_t* xact_status, void * ud)
{
  //rw_status_t status;
  struct rwmain * rwmain;
  rwvcs_instance_ptr_t rwvcs;
  vcs_manifest_op_inventory * ret_op_inventory;
  vcs_manifest_inventory * inventory;


  rwmain = (struct rwmain *)ud;
  rwvcs = rwmain->rwvx->rwvcs;
  RW_CF_TYPE_VALIDATE(rwvcs, rwvcs_instance_ptr_t);

  if (xact_status->status == RWDTS_XACT_FAILURE || xact_status->status == RWDTS_XACT_ABORTED) {
    rwmain_trace_info(rwmain, "Lookup of component probably failed");
    goto done;
  }

  rwmain_trace_info(rwmain, "Updating operational inventory");

  rwdts_query_result_t *qrslt = rwdts_xact_query_result(xact, 0);  
  while (qrslt) {
    ret_op_inventory = (vcs_manifest_op_inventory*)(qrslt->message);
    RW_ASSERT(ret_op_inventory);
    RW_ASSERT(ret_op_inventory->base.descriptor == RWPB_G_MSG_PBCMD(RwManifest_data_Manifest_OperationalInventory));

    inventory = rwvcs->pb_rwmanifest->inventory;
    for (size_t i = 0; i < ret_op_inventory->n_component; ++i) {
      // Any updates to the static manifest are going to be ignored.
      if (rwvcs_manifest_have_component(rwvcs, ret_op_inventory->component[i]->component_name)) {
        continue;
      }

      inventory->component = (vcs_manifest_component **)realloc(
          inventory->component,
          sizeof(vcs_manifest_component *) * (inventory->n_component + 1));
      RW_ASSERT(inventory->component);
      inventory->component[inventory->n_component] = (vcs_manifest_component*)protobuf_c_message_duplicate(
          NULL,
          &ret_op_inventory->component[i]->base,
          ret_op_inventory->component[i]->base.descriptor);
      inventory->n_component++;

      rwmain_trace_info(
          rwmain,
          "Updating operational inventory with %s",
          ret_op_inventory->component[i]->component_name);
    }
    qrslt = rwdts_xact_query_result(xact, 0);
  }

done:
  schedule_next(rwmain, init_phase, 0, NULL);
}

/*
 * Request any new components appended to the inventory via runtime
 * configuration from DTS.  Calls on_op_inventory_update to handle the response
 * after which the init_phase will be scheduled.
 */
static void request_op_inventory_update(rwsched_CFRunLoopTimerRef timer, void * ctx)
{
  struct rwmain * rwmain;
  vcs_manifest_component query_msg;
  rw_keyspec_path_t * query_key;
  rwdts_xact_t * query_xact;

  rwmain = (struct rwmain *)ctx;


  // The uagent will only publish the 2nd level right now no mater what we request.
  // RIFT-5261
  query_key = ((rw_keyspec_path_t *)RWPB_G_PATHSPEC_VALUE(RwManifest_data_Manifest_OperationalInventory));

  vcs_manifest_component__init(&query_msg);
  //query_msg.component_name = start_req->component_name;


  query_xact = rwdts_api_query_ks(
      rwmain->dts,
      query_key,
      RWDTS_QUERY_READ,
      0,
      on_op_inventory_update,
      rwmain,
      &query_msg.base);

  RW_ASSERT(query_xact);

  rwmain_trace_info(rwmain, "Requested Operational inventory update.");

  rwsched_tasklet_CFRunLoopTimerRelease(
    rwmain->rwvx->rwsched_tasklet,
    timer);
}

/*
 * Called when we get a response from DTS with any additional component
 * definitions added to the inventory via runtime configuration.
 *
 * Adds any new components to the manifest held by rwvcs and then schedules
 * the init_phase.
 */
static void on_inventory_update(rwdts_xact_t * xact, rwdts_xact_status_t* xact_status, void * ud)
{
  //rw_status_t status;
  struct rwmain * rwmain;
  rwvcs_instance_ptr_t rwvcs;
  vcs_manifest_inventory * ret_inventory;
  vcs_manifest_inventory * inventory;


  rwmain = (struct rwmain *)ud;
  rwvcs = rwmain->rwvx->rwvcs;
  RW_CF_TYPE_VALIDATE(rwvcs, rwvcs_instance_ptr_t);

  if (xact_status->status == RWDTS_XACT_FAILURE || xact_status->status == RWDTS_XACT_ABORTED) {
    rwmain_trace_info(rwmain, "Lookup of component probably failed");
    goto done;
  }

  rwmain_trace_info(rwmain, "Updating inventory");

  rwdts_query_result_t *qrslt = rwdts_xact_query_result(xact, 0);  
  while (qrslt) {
    ret_inventory = (vcs_manifest_inventory*)(qrslt->message);
    RW_ASSERT(ret_inventory);
    RW_ASSERT(ret_inventory->base.descriptor == RWPB_G_MSG_PBCMD(RwManifest_data_Manifest_Inventory));

    inventory = rwvcs->pb_rwmanifest->inventory;
    for (size_t i = 0; i < ret_inventory->n_component; ++i) {
      // Any updates to the static manifest are going to be ignored.
      if (rwvcs_manifest_have_component(rwvcs, ret_inventory->component[i]->component_name)) {
        continue;
      }

      inventory->component = (vcs_manifest_component **)realloc(
          inventory->component,
          sizeof(vcs_manifest_component *) * (inventory->n_component + 1));
      RW_ASSERT(inventory->component);
      inventory->component[inventory->n_component] = (vcs_manifest_component*)protobuf_c_message_duplicate(
          NULL,
          &ret_inventory->component[i]->base,
          ret_inventory->component[i]->base.descriptor);
      inventory->n_component++;

      rwmain_trace_info(
          rwmain,
          "Updating inventory with %s",
          ret_inventory->component[i]->component_name);
    }
    qrslt = rwdts_xact_query_result(xact, 0);
  }

done:
  schedule_next(rwmain, request_op_inventory_update, 0, NULL);
}

/*
 * Request any new components appended to the inventory via runtime
 * configuration from DTS.  Calls on_inventory_update to handle the response
 * after which the init_phase will be scheduled.
 */
static void request_inventory_update(rwsched_CFRunLoopTimerRef timer, void * ctx)
{
  struct rwmain * rwmain;
  vcs_manifest_component query_msg;
  rw_keyspec_path_t * query_key;
  rwdts_xact_t * query_xact;

  rwmain = (struct rwmain *)ctx;


  // The uagent will only publish the 2nd level right now no mater what we request.
  // RIFT-5261
  query_key = ((rw_keyspec_path_t *)RWPB_G_PATHSPEC_VALUE(RwManifest_data_Manifest_Inventory));

  vcs_manifest_component__init(&query_msg);
  //query_msg.component_name = start_req->component_name;


  query_xact = rwdts_api_query_ks(
      rwmain->dts,
      query_key,
      RWDTS_QUERY_READ,
      0,
      on_inventory_update,
      rwmain,
      &query_msg.base);

  RW_ASSERT(query_xact);

  rwmain_trace_info(rwmain, "Requested inventory update.");

  rwsched_tasklet_CFRunLoopTimerRelease(
    rwmain->rwvx->rwsched_tasklet,
    timer);
}

/*
 * Timer that loops up to CHECK_DTS_MAX_ATTEMPTS times to see if we have a
 * connection to DTS yet.  If a connection has been made (in collapsed mode
 * this takes roughly 2ms) then an updated component inventory will be
 * requested.  If no connection is made, then the inventory update is skipped
 * and the init_phase is scheduled next.
 */
static void check_dts_connected(rwsched_CFRunLoopTimerRef timer, void * ctx)
{
  struct wait_on_dts_cls * cls;

  cls = (struct wait_on_dts_cls *)ctx;

  if (++cls->attempts >= CHECK_DTS_MAX_ATTEMPTS) {
    rwmain_trace_info(
        cls->rwmain,
        "No connection to DTS after %f seconds, skipping inventory update",
        (double)CHECK_DTS_MAX_ATTEMPTS * (1.0 / (double)CHECK_DTS_FREQUENCY));
    schedule_next(cls->rwmain, init_phase, 0, NULL);
    goto finished;
  }

  if (rwdts_get_router_conn_state(cls->rwmain->dts) == RWDTS_RTR_STATE_UP) {
    rwmain_trace_info(
        cls->rwmain,
        "Connected to DTS after %f seconds, scheduling inventory update",
        (double)cls->attempts * (1.0 / (double)CHECK_DTS_FREQUENCY));
    schedule_next(cls->rwmain, request_inventory_update, 0, NULL);
    goto finished;
  }

  return;

finished:
  rwsched_tasklet_CFRunLoopTimerRelease(cls->rwmain->rwvx->rwsched_tasklet, timer);
  free(cls);
}

static void usage() {
  printf("rwmain [ARGUMENTS]\n");
  printf("\n");
  printf("REQUIRED ARGUMENTS:\n");
  printf("  -m,--manifest [PATH]        Path to the XML manifest definition.\n");
  printf("\n");
  printf("ARGUMENTS:\n");
  printf("  -n,--name [NAME]            Name of component being started.\n");
  printf("  -i,--instance [ID]          Instance ID of component being started.\n");
  printf("  -t,--type [TYPE]            Type of component being started.\n");
  printf("  -p,--parent [NAME]          Parent's instance-name.\n");
  printf("  -a,--ip_address [ADDRESS]   VM IP address.\n");
  printf("  -v,--vm_instance [ID]       VM instance id.\n");
  printf("  -h, --help                  This screen.\n");
  printf("\n");
  printf("ENVIRONMENT VARIABLES\n");
  printf("  RIFT_NO_SUDO_REAPER             Run the reaper as the current user\n");
  printf("  RIFT_PROC_HEARTBEAT_NO_REVERSE  Disable reverse heartbeating\n");
  return;
}

struct rwmain * rwmain_setup(int argc, char ** argv, char ** envp)
{
  rw_status_t status;
  rwvx_instance_ptr_t rwvx;
  struct rwmain * rwmain;
  struct wait_on_dts_cls * cls;

  char * manifest = NULL;
  char * component_name = NULL;
  char * component_type = NULL;
  char * parent = NULL;
  char * ip_address = NULL;
  uint32_t instance_id = 0;
  uint32_t vm_instance_id = 0;

  // We need to reset optind here and after processing the command line options
  // as successive calls to getopt_long will leave optind=1. So, reset here as
  // rwmain or dpdk may have already parsed arguments and reset after as dpdk
  // may need to do processing of its own.  See getopt(3).
  optind = 0;
  while (true) {
    int c;
    long int lu;

    static struct option long_options[] = {
      {"manifest",      required_argument,  0, 'm'},
      {"name",          required_argument,  0, 'n'},
      {"instance",      required_argument,  0, 'i'},
      {"type",          required_argument,  0, 't'},
      {"parent",        required_argument,  0, 'p'},
      {"ip_address",    required_argument,  0, 'a'},
      {"vm_instance",   required_argument,  0, 'v'},
      {"help",          no_argument,        0, 'h'},
      {0,               0,                  0,  0},
    };

    c = getopt_long(argc, argv, "m:n:i:t:p:a:v:h", long_options, NULL);
    if (c == -1)
      break;

    switch (c) {
      case 'm':
        manifest = strdup(optarg);
        RW_ASSERT(manifest);
        break;

      case 'n':
        component_name = strdup(optarg);
        RW_ASSERT(component_name);
        break;

      case 'i':
        errno = 0;
        lu = strtol(optarg, NULL, 10);
        RW_ASSERT(errno == 0);
        RW_ASSERT(lu > 0 && lu < UINT32_MAX);
        instance_id = (uint32_t)lu;
        break;

      case 't':
        component_type = strdup(optarg);
        RW_ASSERT(component_type);
        break;

      case 'p':
        parent = strdup(optarg);
        RW_ASSERT(parent);
        break;

      case 'a':
        ip_address = strdup(optarg);
        RW_ASSERT(ip_address);
        break;

      case 'v':
        errno = 0;
        lu = strtol(optarg, NULL, 10);
        RW_ASSERT(errno == 0);
        RW_ASSERT(lu > 0 && lu < UINT32_MAX);
        vm_instance_id = (uint32_t)lu;
        break;

      case 'h':
        usage();
        return 0;
    }
  }
  optind = 0;

  if (!manifest) {
    fprintf(stderr, "ERROR:  No manifest file specified.\n");
    exit(1);
  }

  rwvx = rwvx_instance_alloc();
  RW_ASSERT(rwvx);

  init_rwtrace(rwvx, argv);
  rwvx->rwvcs->envp = envp;
  rwvx->rwvcs->rwmain_exefile = strdup(argv[0]);
  RW_ASSERT(rwvx->rwvcs->rwmain_exefile);

  status = rwvcs_instance_init(rwvx->rwvcs, manifest, main_function);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  rwmain = rwmain_alloc(rwvx, component_name, instance_id, component_type, parent, ip_address, vm_instance_id);
  RW_ASSERT(rwmain);
  {
    rwmain->rwvx->rwvcs->apih = rwmain->dts;
    RW_SKLIST_PARAMS_DECL(
        config_ready_entries_,
        rwvcs_config_ready_entry_t,
        instance_name,
        rw_sklist_comp_charptr,
        config_ready_elem);
    RW_SKLIST_INIT(
        &(rwmain->rwvx->rwvcs->config_ready_entries), 
        &config_ready_entries_);
    rwmain->rwvx->rwvcs->config_ready_fn = rwmain_dts_config_ready_process;
  }
    

  if (rwmain->vm_ip_address) {
    rwmain->rwvx->rwvcs->identity.vm_ip_address = strdup(rwmain->vm_ip_address);
    RW_ASSERT(rwmain->rwvx->rwvcs->identity.vm_ip_address);
  }
  rwmain->rwvx->rwvcs->identity.rwvm_instance_id = rwmain->vm_instance_id;

  if (rwmain->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWVM) {
    RW_ASSERT(VCS_GET(rwmain)->instance_name);
    rwmain->rwvx->rwvcs->identity.rwvm_name = strdup(VCS_GET(rwmain)->instance_name);
  }
  else if (rwmain->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWPROC) {
    RW_ASSERT(parent);
    rwmain->rwvx->rwvcs->identity.rwvm_name = strdup(parent);
  }

#if 1
  {
    char instance_id_str[256];
    snprintf(instance_id_str, 256, "%u", rwmain->vm_instance_id);
    status = rw_setenv("RWVM_INSTANCE_ID", instance_id_str);
    RW_ASSERT(status == RW_STATUS_SUCCESS);
  }
#endif

#if 1
  if (rwmain->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWVM && parent!=NULL) {
    rwvcs_instance_ptr_t rwvcs = rwvx->rwvcs;
    struct timeval timeout = { .tv_sec = RWVCS_RWZK_TIMEOUT_S, .tv_usec = 0 };
    rw_component_info rwvm_info;
    char * instance_name = NULL;
    instance_name = to_instance_name(component_name, instance_id);
    RW_ASSERT(instance_name!=NULL);
    // Lock so that the parent can initialize the zk data before the child updates it
    status = rwvcs_rwzk_lock(rwvcs, instance_name, &timeout);
    RW_ASSERT(status == RW_STATUS_SUCCESS);
    printf("instance_nameinstance_nameinstance_nameinstance_nameinstance_name=%s\n", instance_name);
    status = rwvcs_rwzk_lookup_component(rwvcs, instance_name, &rwvm_info);
    RW_ASSERT(status == RW_STATUS_SUCCESS);
    RW_ASSERT(rwvm_info.vm_info!=NULL);
    rwvm_info.vm_info->has_pid = true;
    rwvm_info.vm_info->pid = getpid();
    status = rwvcs_rwzk_node_update(rwvcs, &rwvm_info);
    RW_ASSERT(status == RW_STATUS_SUCCESS);
    status = rwvcs_rwzk_unlock(rwvcs, instance_name);
    RW_ASSERT(status == RW_STATUS_SUCCESS);
    free(instance_name);
  } else if (rwmain->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWVM && parent == NULL) {
    struct rlimit rlimit;
    uint32_t core_limit;
    uint32_t file_limit;
    RW_ASSERT(getrlimit(RLIMIT_CORE, &rlimit) == 0);
    core_limit = rlimit.rlim_max;
    RW_ASSERT(getrlimit(RLIMIT_FSIZE, &rlimit) == 0);
    file_limit = rlimit.rlim_max;
    rwmain_trace_crit(
        rwmain,
        "getrlimit(RLIMIT_CORE)=%u getrlimit(RLIMIT_FSIZE)=%u get_current_dir_name()=%s",
        core_limit, file_limit, get_current_dir_name());
  }
#endif

  status = rwmain_bootstrap(rwmain);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  cls = (struct wait_on_dts_cls *)malloc(sizeof(struct wait_on_dts_cls));
  RW_ASSERT(cls);
  cls->rwmain = rwmain;
  cls->attempts = 0;

  schedule_next(rwmain, check_dts_connected, CHECK_DTS_FREQUENCY, cls);

  if (manifest)
    free(manifest);

  if (component_name)
    free(component_name);

  if (component_type)
    free(component_type);

  if (parent)
    free(parent);

  if (ip_address)
    free(ip_address);

  return rwmain;
}

