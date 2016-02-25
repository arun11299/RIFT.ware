
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 *
 */

#include <rwvcs_manifest.h>
#include <rwvcs_rwzk.h>

#include "rwmain.h"


/*
 * Check if a component name and instance id are refering to this instance of
 * rwmain.
 *
 * @param rwmain  - rwmain instance
 * @param id      - component id
 * @return        - true if the component_name and instance_id match the rwmain instance
 */
static inline bool is_this_instance(
    struct rwmain * rwmain,
    const char * id)
{
  bool r = 0;
  char * this_id;

  this_id = to_instance_name(rwmain->component_name, rwmain->instance_id);
  r = !strncmp(this_id, id, strlen(this_id));
  free(this_id);

  return r;
}

/*
 * Create a start action and pass it to rwvcs to start an instance of an component
 *
 * @param rwmain          - rwmaininstance
 * @param component_name  - name of the component to start
 * @param xact            - dts transaction
 * @param query           - dts query
 * @return                - RW_STATUS_SUCCESS on success, status from rwmain_action_run() otherwise
 */
static rw_status_t start_component(
    struct rwmain * rwmain,
    const char * component_name,
    const char * ip_addr,
    const char * parent_instance,
    rwdts_xact_t * xact,
    rwdts_query_handle_t query,
    char ** instance_name,
    vcs_manifest_component *m_component)
{
  int r;
  rw_status_t status;
  uint32_t new_instance_id;
  vcs_manifest_action action;
  vcs_manifest_action_start start;
  char * tmp_instance_name;

  vcs_manifest_action__init(&action);
  vcs_manifest_action_start__init(&start);

  *instance_name = NULL;

  r = asprintf(&action.name, "Start %s", component_name);
  RW_ASSERT(r != -1);

  start.component_name = strdup(component_name);
  RW_ASSERT(start.component_name);

  vcs_manifest_event **m_event= NULL;

  switch (m_component->component_type) {
    case RWVCS_TYPES_COMPONENT_TYPE_RWCOLLECTION:
      if (m_component
          && m_component->rwcollection->event_list
          && m_component->rwcollection->event_list->event) {
        m_event = m_component->rwcollection->event_list->event;
      }
      break;

    case RWVCS_TYPES_COMPONENT_TYPE_RWVM:
      if (m_component
          && m_component->rwvm->event_list
          && m_component->rwvm->event_list->event) {
        m_event = m_component->rwvm->event_list->event;
      }
      break;

    case RWVCS_TYPES_COMPONENT_TYPE_RWPROC:
      //m_component->rwproc
      break;

    case RWVCS_TYPES_COMPONENT_TYPE_PROC:
      //m_component->native_proc
      break;

    case RWVCS_TYPES_COMPONENT_TYPE_RWTASKLET:
      //m_component->rwtasklet
      break;

    default:
      RW_CRASH();
  }

  if (m_event
      && m_event[0]
      && m_event[0]->action
      && m_event[0]->action[0]
      && m_event[0]->action[0]->start) {
    vcs_manifest_action_start *m_start = NULL;
    m_start = m_event[0]->action[0]->start;
    start.python_variable = malloc(sizeof(char**)*(m_start->n_python_variable+(ip_addr!=NULL)));
    int i; for (i=0; i<m_start->n_python_variable; i++) {
      start.python_variable[i] = strdup(m_start->python_variable[i]);
    }
    start.has_config_ready = m_start->has_config_ready;
    start.config_ready = m_start->config_ready;
    if (ip_addr) {
      r = asprintf(&start.python_variable[i], "vm_ip_address = '%s'", ip_addr);
      RW_ASSERT(r != -1);
    }
    start.n_python_variable = m_start->n_python_variable+(ip_addr!=NULL);
  }
  status = rwvcs_rwzk_next_instance_id(rwmain->rwvx->rwvcs, &new_instance_id, NULL);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  r = asprintf(&start.instance_id, "%u", new_instance_id);
  RW_ASSERT(r != -1);

  tmp_instance_name = to_instance_name(component_name, new_instance_id);
  action.start = &start;

  rw_status_t rs = rwvcs_instance_update_child_state(
    rwmain,
    tmp_instance_name,
    parent_instance,
    m_component->component_type,
    RW_BASE_ADMIN_COMMAND_START);
  RW_ASSERT(rs == RW_STATUS_SUCCESS);

  status = rwmain_action_run(
      rwmain,
      parent_instance,
      &action);

  if (status != RW_STATUS_SUCCESS)
    goto done;

  status = rwvcs_rwzk_add_child(
      rwmain->rwvx->rwvcs,
      parent_instance,
      tmp_instance_name);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  *instance_name = tmp_instance_name;

done:
  action.start = NULL;
  protobuf_c_message_free_unpacked_usebody(NULL, &action.base);
  protobuf_c_message_free_unpacked_usebody(NULL, &start.base);

  return status;
}

/*
 * Send a response to a start request.
 *
 * @param xact          - dts transaction
 * @param query         - dts query
 * @param key           - dts key
 * @param start_status  - result of the start request
 * @param instance_name - instance name of the launched instance.  If no instance was launched, pass NULL.
 */
void send_start_request_response(
  rwdts_xact_t * xact,
  rwdts_query_handle_t query,
  rw_status_t start_status,
  const char * instance_name)
{
  rw_status_t status;
  vcs_rpc_start_output output;
  static ProtobufCMessage * msgs[1];
  rwdts_member_query_rsp_t rsp;

  vcs_rpc_start_output__init(&output);

  output.has_rw_status = true;
  if (instance_name) {
    output.instance_name = strdup(instance_name);
    RW_ASSERT(output.instance_name);
  }

  msgs[0] = &output.base;

  memset (&rsp, 0, sizeof (rsp));

  rsp.n_msgs = 1;
  rsp.msgs = msgs;
  rsp.ks = (rw_keyspec_path_t *)RWPB_G_PATHSPEC_VALUE(RwVcs_output_Vstart),
  rsp.evtrsp = RWDTS_EVTRSP_ACK;

  status = rwdts_member_send_response(xact, query, &rsp);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  protobuf_free_stack(output);
}

static void start_component_nd_send_rsp(
    struct rwmain * rwmain,
    struct dts_start_stop_closure * cls,
    vcs_manifest_component *m_component)
{
  rw_status_t status;
  char * instance_name = NULL;

  RW_ASSERT(rwmain);
  RW_ASSERT(cls);
  RW_ASSERT(m_component);

  status = start_component(
        rwmain,
        cls->rpc_query,
        cls->ip_addr,
        cls->instance_name,
        cls->xact,
        cls->query,
        &instance_name,
        m_component);

  RWPB_T_MSG(RwBase_VcsInstance_Instance_ChildN) *child_n;
  child_n = (RWPB_T_MSG(RwBase_VcsInstance_Instance_ChildN)*)RW_MALLOC0(sizeof(*child_n));
  RWPB_F_MSG_INIT(RwBase_VcsInstance_Instance_ChildN, child_n);
  child_n->instance_name = instance_name;

  RWPB_T_MSG(RwBase_VcsInstance_Instance) *inst;
  inst = (RWPB_T_MSG(RwBase_VcsInstance_Instance)*)RW_MALLOC0(sizeof(*inst));
  RW_ASSERT(inst);
  RWPB_F_MSG_INIT(RwBase_VcsInstance_Instance, inst);
  inst->instance_name = to_instance_name(rwmain->component_name, rwmain->instance_id);
  RW_ASSERT(inst->instance_name);
  inst->child_n = (RWPB_T_MSG(RwBase_VcsInstance_Instance_ChildN)**)RW_MALLOC0(sizeof(*inst->child_n));
  inst->child_n[0] = child_n;
  inst->n_child_n = 1;

  rwdts_member_query_rsp_t rsp;
  ProtobufCMessage * msgs[1];
  memset (&rsp, 0, sizeof (rsp));
  rsp.msgs = msgs;
  rsp.msgs[0] = &inst->base;
  rsp.n_msgs = 1;
  status = rwdts_api_keyspec_from_xpath(rwmain->dts, VCS_GET(rwmain)->vcs_instance_xpath, &rsp.ks);
  RW_ASSERT(status == RW_STATUS_SUCCESS); 
  rsp.evtrsp = RWDTS_EVTRSP_ACK;

  status = rwdts_member_send_response(cls->xact, cls->query, &rsp);
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  // Free the protobuf
  if (inst) {
    protobuf_c_message_free_unpacked_usebody(NULL, (ProtobufCMessage*)inst);
  }

}

static void on_vstart_inventory_update(
    rwdts_xact_t * xact,
    rwdts_xact_status_t* xact_status,
    void * ud)
{
  struct rwmain * rwmain;
  rwvcs_instance_ptr_t rwvcs;
  struct dts_start_stop_closure * cls;

  if (!xact_status->xact_done) {
    return;
  }

  cls = (struct dts_start_stop_closure *)ud;
  rwmain = cls->rwmain;
  rwvcs = cls->rwmain->rwvx->rwvcs;
  RW_CF_TYPE_VALIDATE(rwvcs, rwvcs_instance_ptr_t);

  if (xact_status->status == RWDTS_XACT_FAILURE || xact_status->status == RWDTS_XACT_ABORTED) {
    rwmain_trace_info(rwmain, "READ of inventory probably failed");
    rwdts_xact_unref(xact, __PRETTY_FUNCTION__, __LINE__);
    goto done;
  }

  rwmain_trace_info(rwmain, "Updating inventory");

  rwdts_query_result_t *qrslt = rwdts_xact_query_result(xact, 0);  
  while (qrslt) {
    vcs_manifest_inventory * ret_inventory;
    ret_inventory = (vcs_manifest_inventory*)(qrslt->message);
    RW_ASSERT(ret_inventory);
    if (ret_inventory->base.descriptor == RWPB_G_MSG_PBCMD(RwManifest_data_Manifest_Inventory)) {
      vcs_manifest_inventory * inventory;
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
    } else {
      vcs_manifest_op_inventory * ret_inventory = (vcs_manifest_op_inventory*)(qrslt->message);
      RW_ASSERT(ret_inventory->base.descriptor == RWPB_G_MSG_PBCMD(RwManifest_data_Manifest_OperationalInventory));
      vcs_manifest_inventory * inventory;
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
    }
    qrslt = rwdts_xact_query_result(xact, 0);
  }

  vcs_manifest_component *m_component = NULL;
  rw_status_t status = rwvcs_manifest_component_lookup(
      rwvcs,
      cls->rpc_query,
      &m_component);
  RW_ASSERT(status == RW_STATUS_SUCCESS);
  RW_ASSERT (m_component);
  RW_ASSERT (m_component->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWCOLLECTION);

  start_component_nd_send_rsp(rwmain, cls, m_component);

done:
  free(cls->instance_name);
  free(cls->rpc_query);
  if (cls->xact) {
    rwdts_xact_unref(cls->xact, __PRETTY_FUNCTION__, __LINE__);
    cls->xact = NULL;
  }
  if (cls->ip_addr) free(cls->ip_addr);
  free(cls);
}

static void on_vstart_component_lookup_new(
    rwdts_xact_t * xact,
    rwdts_xact_status_t* xact_status,
    void * ud)
{
  struct rwmain * rwmain;
  rwvcs_instance_ptr_t rwvcs;
  struct dts_start_stop_closure * cls = NULL;
  vcs_manifest_component * ret_component;

  RW_STATIC_ASSERT(sizeof(RWPB_T_MSG(RwManifest_data_Manifest_Inventory_Component)) == sizeof(RWPB_T_MSG(RwManifest_data_Manifest_OperationalInventory_Component)));

  if (!xact_status->xact_done) {
    return;
  }

  cls = (struct dts_start_stop_closure *)ud;
  rwmain = cls->rwmain;
  rwvcs = cls->rwmain->rwvx->rwvcs;
  RW_CF_TYPE_VALIDATE(rwvcs, rwvcs_instance_ptr_t);

  if (xact_status->status == RWDTS_XACT_ABORTED ||
       xact_status->status == RWDTS_XACT_FAILURE) {
      rwdts_xact_unref(xact, __PRETTY_FUNCTION__, __LINE__);
    goto done;
  }

  rwdts_query_result_t *qrslt;
  qrslt = rwdts_xact_query_result(xact, 111111111111111);  
  if (!qrslt)
    qrslt = rwdts_xact_query_result(xact, 222222222222222);
  RW_ASSERT(qrslt);

  ret_component = (vcs_manifest_component*)(qrslt->message);
  RW_ASSERT(ret_component);
  RW_ASSERT((ret_component->base.descriptor == RWPB_G_MSG_PBCMD(RwManifest_data_Manifest_Inventory_Component))
            || (ret_component->base.descriptor == RWPB_G_MSG_PBCMD(RwManifest_data_Manifest_OperationalInventory_Component)));

  if (!rwvcs_manifest_have_component(rwvcs, cls->rpc_query)) {
    vcs_manifest_inventory * inventory;

    inventory = rwvcs->pb_rwmanifest->inventory;
    inventory->component = (vcs_manifest_component **)realloc(
        inventory->component,
        sizeof(vcs_manifest_component *) * (inventory->n_component + 1));
    RW_ASSERT(inventory->component);

    if (strcmp(cls->rpc_query, ret_component->component_name)) {
      rwmain_trace_crit(
          rwmain,
          "Returned inventory missing component definition for %s",
          cls->rpc_query);
      //status = RW_STATUS_NOTFOUND;
      goto done;
    }

#if 0
    // We are stealing the protobuf reference here so not need to free later on
    inventory->component[inventory->n_component] = ret_component;
#else
    inventory->component[inventory->n_component] = (vcs_manifest_component*)protobuf_c_message_duplicate(
          NULL,
          &ret_component->base,
          ret_component->base.descriptor);
#endif
    inventory->n_component++;

    rwmain_trace_info(
        rwmain,
        "Got component definition for %s",
        cls->rpc_query);
  } else {
    rwmain_trace_warn(
        rwmain,
        "Queried for %s but on response it is already in the in-memory manifest",
        cls->rpc_query);
  }

  /* Send one more READ to update the whole inventory in case of RWCOLLECTION */
  if (ret_component->component_type == RWVCS_TYPES_COMPONENT_TYPE_RWCOLLECTION) {
    rwdts_xact_t * query_xact;
    if (ret_component->base.descriptor == RWPB_G_MSG_PBCMD(RwManifest_data_Manifest_Inventory_Component)) {
      RWPB_T_PATHSPEC(RwManifest_data_Manifest_Inventory) pathspec;

      pathspec = (*RWPB_G_PATHSPEC_VALUE(RwManifest_data_Manifest_Inventory));

      rwmain_trace_info(
          rwmain,
          "(start rpc) updating inventory from DTS component=%s",
          cls->rpc_query);

      /* reuse the cls that came in */
      query_xact = rwdts_api_query_ks(rwmain->dts,
                            (rw_keyspec_path_t *) &pathspec,
                            RWDTS_QUERY_READ,
                            0,
                            on_vstart_inventory_update,
                            cls,
                            NULL); 
      RW_ASSERT(query_xact);
    } else {
      RWPB_T_PATHSPEC(RwManifest_data_Manifest_OperationalInventory) pathspec;

      pathspec = (*RWPB_G_PATHSPEC_VALUE(RwManifest_data_Manifest_OperationalInventory));

      rwmain_trace_info(
          rwmain,
          "(start rpc) updating inventory from DTS component=%s",
          cls->rpc_query);

      /* reuse the cls that came in */
      query_xact = rwdts_api_query_ks(rwmain->dts,
                            (rw_keyspec_path_t *) &pathspec,
                            RWDTS_QUERY_READ,
                            0,
                            on_vstart_inventory_update,
                            cls,
                            NULL); 
      RW_ASSERT(query_xact);
    }
    return;
  }

  start_component_nd_send_rsp(rwmain, cls, ret_component);

done:
  free(cls->instance_name);
  free(cls->rpc_query);
  if (cls->xact) {
    rwdts_xact_unref(cls->xact, __PRETTY_FUNCTION__, __LINE__);
    cls->xact = NULL;
  }
  if (cls->ip_addr) free(cls->ip_addr);
  free(cls);
}


/*
 * Handle DTS start RPC.  It is up to the receiving rwmain to decide if it needs
 * to take any action.  If not, RWDTS_ACTION_NA will be returned.
 *
 * When the start request specifies this rwmain as the destination parent, it
 * will build a new start event to send to rwvcs.  This will include generating
 * an instance-id to assign to the new instance.  On success, the instance-name
 * of the newly started component will be sent to the caller.
 *
 * @param xact  - dts transaction
 * @param query - dts query
 * @param evt   - dts event
 * @param key   - dts key
 * @param msg   - rpc start input message
 * @param ud    - rwmain instance
 * @return      - RWDTS_ACTION_OK if the request was handled here
 *                RWDTS_ACTION_NA if this rwmain doesn't care
 */
static rwdts_member_rsp_code_t on_vstart_new(
    const rwdts_xact_info_t * xact_info,
    RWDtsQueryAction action,
    const rw_keyspec_path_t * key,
    const ProtobufCMessage * msg,
    uint32_t credits,
    void *getnext_ptr)
{
  char * instance_name = NULL;
  rwdts_member_rsp_code_t dts_ret = RWDTS_ACTION_NA;
  struct rwmain * rwmain;
  rwvcs_instance_ptr_t rwvcs;
  vcs_rpc_start_input * start_req;

  RW_ASSERT(xact_info);
  rwmain = (struct rwmain *)xact_info->ud;
  rwvcs = rwmain->rwvx->rwvcs;
  RW_CF_TYPE_VALIDATE(rwvcs, rwvcs_instance_ptr_t);

  start_req = (vcs_rpc_start_input *)msg;

  instance_name = to_instance_name(rwmain->component_name, rwmain->instance_id);

  if (!rwvcs_rwzk_is_leader_of(rwvcs, instance_name, start_req->parent_instance)
      && !is_this_instance(rwmain, start_req->parent_instance)) {
    dts_ret = RWDTS_ACTION_NA;
    goto done;
  }
  free(instance_name);
  instance_name = NULL;

  send2dts_start_req(rwmain, xact_info, start_req->component_name);

  dts_ret = RWDTS_ACTION_ASYNC;

done:
  if (instance_name)
    free(instance_name);

  return dts_ret;
}

rwdts_member_rsp_code_t do_vstart(
    const rwdts_xact_info_t* xact_info,
    struct rwmain * rwmain,
    char *component_name,
    char *ip_addr,
    char *parent_instance,
    char **child_instance)
{
  rw_status_t status;
  rwdts_member_rsp_code_t dts_ret = RWDTS_ACTION_NA;
  RW_ASSERT(rwmain);
  rwvcs_instance_ptr_t rwvcs;
  rwvcs = rwmain->rwvx->rwvcs;
  RW_CF_TYPE_VALIDATE(rwvcs, rwvcs_instance_ptr_t);
  RW_ASSERT(component_name);
  RW_ASSERT(parent_instance);

  char * instance_name = NULL;

  if (rwvcs_manifest_have_component(rwvcs, component_name)) {
    vcs_manifest_component *m_component = NULL;
    status = rwvcs_manifest_component_lookup(
        rwvcs,
        component_name,
        &m_component);
    RW_ASSERT(status == RW_STATUS_SUCCESS);
    status = start_component(
        rwmain,
        component_name,
        ip_addr,
        parent_instance,
        xact_info->xact, //NULL
        xact_info->queryh, //NULL,
        &instance_name,
        m_component);
    RW_ASSERT(status == RW_STATUS_SUCCESS);
    if (xact_info->xact && xact_info->queryh) {
    }
    dts_ret = RWDTS_ACTION_OK;
  } else {
    struct dts_start_stop_closure * cls = NULL;
    rwdts_xact_block_t *comp_blk;
    rwdts_xact_t *comp_xact = NULL;

    cls = (struct dts_start_stop_closure *)malloc(sizeof(struct dts_start_stop_closure));
    RW_ASSERT(cls);

    cls->rwmain = rwmain;
    cls->xact = xact_info->xact; //NULL
    rwdts_xact_ref(cls->xact, __PRETTY_FUNCTION__, __LINE__);
    cls->query = xact_info->queryh; //NULL
    cls->instance_name = strdup(parent_instance);
    RW_ASSERT(cls->instance_name);
    cls->rpc_query = strdup(component_name);
    if (ip_addr)
      cls->ip_addr = strdup(ip_addr);
    else
      cls->ip_addr = NULL;

    comp_xact = rwdts_api_xact_create(rwmain->dts, /*RWDTS_FLAG_TRACE |*/ RWDTS_XACT_FLAG_NOTRAN, on_vstart_component_lookup_new, cls);
    RW_ASSERT(comp_xact);

    comp_blk = rwdts_xact_block_create(comp_xact);
    RW_ASSERT(comp_blk);

    RWPB_T_PATHSPEC(RwManifest_data_Manifest_OperationalInventory_Component) pathspec2;
    pathspec2 = (*RWPB_G_PATHSPEC_VALUE(RwManifest_data_Manifest_OperationalInventory_Component));
    pathspec2.dompath.path002.key00.component_name = component_name;
    pathspec2.dompath.path002.has_key00 = true;

    status = rwdts_xact_block_add_query_ks(
        comp_blk,
        (rw_keyspec_path_t *)&pathspec2,
        RWDTS_QUERY_READ,
        RWDTS_FLAG_NONE,
        222222222222222,
        NULL);
    RW_ASSERT(status == RW_STATUS_SUCCESS);

    RWPB_T_PATHSPEC(RwManifest_data_Manifest_Inventory_Component) pathspec1;
    pathspec1 = (*RWPB_G_PATHSPEC_VALUE(RwManifest_data_Manifest_Inventory_Component));
    pathspec1.dompath.path002.key00.component_name = component_name;
    pathspec1.dompath.path002.has_key00 = true;

    status = rwdts_xact_block_add_query_ks(
        comp_blk,
        (rw_keyspec_path_t *)&pathspec1,
        RWDTS_QUERY_READ,
        RWDTS_FLAG_NONE,
        111111111111111,
        NULL);
    RW_ASSERT(status == RW_STATUS_SUCCESS);

    status = rwdts_xact_block_execute(comp_blk, RWDTS_XACT_FLAG_END, NULL, 0, NULL); /* go! */
    RW_ASSERT(status == RW_STATUS_SUCCESS);
    dts_ret = RWDTS_ACTION_ASYNC;
  }

  if (child_instance){
    *child_instance = instance_name;
    instance_name = NULL;
  }

  if (instance_name)
    free(instance_name);

  return dts_ret;
}

/*
  rwdts_query_result_t *qres;
 * Handle DTS stop RPC.  It is up to the receiving rwmain to decide if it needs
 * to take any action.  If not, RWDTS_ACTION_NA will be returned.
 *
 * When the stop requests matches this instance, rwmain will first request that
 * all of its children stop, wait for the responses and then stop itself.
 *
 * If the stop request is for a tasklet or native process owned by this instance,
 * rwmain will immediately terminate the target and respond to the caller.
 *
 * @param xact  - dts transaction
 * @param query - dts query
 * @param evt   - dts event
 * @param key   - dts key
 * @param msg   - rpc stop input message
 * @param ud    - rwmain instance
 * @return      - RWDTS_ACTION_OK if the request was handled here
 *                RWDTS_ACTION_NA if this rwmain doesn't care
 */
static rwdts_member_rsp_code_t on_vstop_new(
    const rwdts_xact_info_t * xact_info,
    RWDtsQueryAction action,
    const rw_keyspec_path_t * key,
    const ProtobufCMessage * msg,
    uint32_t credits,
    void *getnext_ptr)
{
  rw_status_t status;
  rwdts_member_rsp_code_t dts_ret = RWDTS_ACTION_NA;
  struct rwmain * rwmain;
  rwvcs_instance_ptr_t rwvcs;
  vcs_rpc_stop_input * stop_req;
  rw_component_info target_info;
  char * instance_name = NULL;
  rwdts_member_query_rsp_t rsp;

  rwmain = (struct rwmain *)xact_info->ud;
  rwvcs = rwmain->rwvx->rwvcs;
  RW_CF_TYPE_VALIDATE(rwvcs, rwvcs_instance_ptr_t);

  memset(&rsp, 0, sizeof(rsp));
  stop_req = (vcs_rpc_stop_input *)msg;

  rw_component_info__init(&target_info);

  status = rwvcs_rwzk_lookup_component(rwvcs, stop_req->instance_name, &target_info);
  // Target is already stopped.
  if (status == RW_STATUS_NOTFOUND) {
    dts_ret = RWDTS_ACTION_NA;
    goto done;
  }
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  instance_name = to_instance_name(rwmain->component_name, rwmain->instance_id);

  /*
   * Stopping components:
   *
   * An rwtasklet can directly be stopped if this is the parent rwproc.  The same goes for
   * a native linux process.
   *
   * An rwproc can also directly stop itself by calling
   * rwmain_stop_instance as any children will be in the same process
   * space.
   *
   * For all other components, each child must first be stopped.  Only once
   * this is done can the target component itself stop.
   */

  if (!rwvcs_rwzk_responsible_for(rwvcs, instance_name, stop_req->instance_name)
      && !is_this_instance(rwmain, stop_req->instance_name)) {
    dts_ret = RWDTS_ACTION_NA;
    goto done;
  }

  /*
   * If the target is a tasklet or native process, it is the responsibility of
   * the parent to stop it.
   * So, we're not the lead and the target isn't a tasklet or native process.
   * If the target isn't us or something we lead, get out.
   */
  if (target_info.component_type != RWVCS_TYPES_COMPONENT_TYPE_PROC
      && target_info.component_type != RWVCS_TYPES_COMPONENT_TYPE_RWTASKLET
      && !rwvcs_rwzk_is_leader_of(rwvcs, instance_name, stop_req->instance_name)
      && !is_this_instance(rwmain, stop_req->instance_name)) {
    dts_ret = RWDTS_ACTION_NA;
    goto done;
  }

  dts_ret = RWDTS_ACTION_OK;
  if (target_info.component_type == RWVCS_TYPES_COMPONENT_TYPE_RWTASKLET
      || target_info.component_type == RWVCS_TYPES_COMPONENT_TYPE_PROC) {
    send2dts_stop_req(rwmain, NULL, NULL, stop_req->instance_name);
  } else {
    send2dts_stop_req(rwmain, xact_info, stop_req->instance_name, NULL);
  }

  if (dts_ret == RWDTS_ACTION_OK)
    rsp.evtrsp = RWDTS_EVTRSP_ACK;

  /*
   * We hit this point in two different states.  If the stop request was fully
   * handled and it's time to report back, rsp.evtrsp will be set to ACK.  If
   * not, then we fell through all the cases above and a number of rpc calls
   * were sent to the children.  In this case we have nothing to send back
   * aside from alerting the router that this will be an async response.
   */
  {
    static ProtobufCMessage * msgs[1];
    vcs_rpc_stop_output * output = NULL;

    rsp.n_msgs = 0;
    rsp.ks = (rw_keyspec_path_t *)RWPB_G_PATHSPEC_VALUE(RwVcs_output_Vstop),
    rsp.msgs = NULL;

    if (rsp.evtrsp == RWDTS_EVTRSP_ACK) {
      output = (vcs_rpc_stop_output *)malloc(sizeof(vcs_rpc_stop_output));
      RW_ASSERT(output);

      vcs_rpc_stop_output__init(output);

      output->has_rw_status = true;
      output->rw_status = status;

      msgs[0] = &output->base;
      rsp.n_msgs = 1;
      rsp.msgs = msgs;
    }

    status = rwdts_member_send_response(xact_info->xact, xact_info->queryh, &rsp);
    RW_ASSERT(status == RW_STATUS_SUCCESS);

    if (output)
      protobuf_free(output);

  }
done:
  protobuf_free_stack(target_info);

  if (instance_name)
    free(instance_name);

  return dts_ret;
}

rwdts_member_rsp_code_t do_vstop(
    const rwdts_xact_info_t * xact_info,
    struct rwmain * rwmain,
    char *stop_instance_name)
{
  rw_status_t status;
  rwdts_member_rsp_code_t dts_ret = RWDTS_ACTION_NA;
  rwvcs_instance_ptr_t rwvcs;
  rw_component_info target_info;
  char * instance_name = NULL;

  rwmain_trace_info(rwmain, "do_vstop stop_instance_name=%s\n",
          stop_instance_name);

  RW_ASSERT(stop_instance_name);
  rwvcs = rwmain->rwvx->rwvcs;
  RW_CF_TYPE_VALIDATE(rwvcs, rwvcs_instance_ptr_t);

  rw_component_info__init(&target_info);

  status = rwvcs_rwzk_lookup_component(rwvcs, stop_instance_name, &target_info);
  // Target is already stopped.
  if (status == RW_STATUS_NOTFOUND) {
    dts_ret = RWDTS_ACTION_NA;
    goto done;
  }
  RW_ASSERT(status == RW_STATUS_SUCCESS);

  instance_name = to_instance_name(rwmain->component_name, rwmain->instance_id);

  /*
   * If the target is a tasklet or native process, it is the responsibility of
   * the parent to stop it.
   */
   if (target_info.component_type == RWVCS_TYPES_COMPONENT_TYPE_PROC
      || target_info.component_type == RWVCS_TYPES_COMPONENT_TYPE_RWTASKLET) {
    status = rwvcs_instance_update_child_state(
      rwmain,
      target_info.instance_name,
      target_info.rwcomponent_parent,
      target_info.component_type,
      RW_BASE_ADMIN_COMMAND_STOP);
    RW_ASSERT(status == RW_STATUS_SUCCESS);

    status = rwmain_stop_instance(rwmain, &target_info);
    RW_ASSERT(status == RW_STATUS_SUCCESS);

    RWMAIN_DEREG_PATH_DTS(rwmain->dts, &target_info);

    status = rwvcs_instance_delete_child(
      rwmain,
      target_info.instance_name,
      target_info.rwcomponent_parent,
      target_info.component_type);
    RW_ASSERT(status == RW_STATUS_SUCCESS);

    dts_ret = RWDTS_ACTION_OK;
    goto send_resp;
  }

  /*
   * So, we're not the lead and the target isn't a tasklet or native process.
   * If the target isn't us or something we lead, get out.
   */
  if (!rwvcs_rwzk_is_leader_of(rwvcs, instance_name, stop_instance_name)
      && !is_this_instance(rwmain, stop_instance_name)) {
    dts_ret = RWDTS_ACTION_NA;
    goto done;
  }


  int remaining_children = 0;

  /*
   * Send a stop request to each child that is not a tasklet or native process.
   */
  for (int i = 0; i < target_info.n_rwcomponent_children; ++i) {
    rw_component_info child;

    status = rwvcs_rwzk_lookup_component(rwvcs, target_info.rwcomponent_children[i], &child);
    if (status == RW_STATUS_NOTFOUND) {
      continue;
    }
    RW_ASSERT(status == RW_STATUS_SUCCESS);

    if (child.component_type != RWVCS_TYPES_COMPONENT_TYPE_RWTASKLET
        && child.component_type != RWVCS_TYPES_COMPONENT_TYPE_PROC) {
      remaining_children++;

      send2dts_stop_req(rwmain, xact_info, target_info.rwcomponent_children[i], NULL);
    } else {
      status = rwvcs_instance_update_child_state(
        rwmain,
        child.instance_name,
        child.rwcomponent_parent,
        child.component_type,
        RW_BASE_ADMIN_COMMAND_STOP);
      RW_ASSERT(status == RW_STATUS_SUCCESS);

      status = rwmain_stop_instance(rwmain, &child);
      RW_ASSERT(status == RW_STATUS_SUCCESS);
    
      RWMAIN_DEREG_PATH_DTS(rwmain->dts, &child);

      status = rwvcs_instance_delete_child(
        rwmain,
        child.instance_name,
        child.rwcomponent_parent,
        child.component_type);
      RW_ASSERT(status == RW_STATUS_SUCCESS);

    }

    protobuf_c_message_free_unpacked_usebody(NULL, &child.base);
  }

  /*
   * No children left?  We can directly stop now then.
   */
  if (!remaining_children) {

    status = rwmain_stop_instance(rwmain, &target_info);
    RW_ASSERT(status == RW_STATUS_SUCCESS);

    RWMAIN_DEREG_PATH_DTS(rwmain->dts, &target_info);

    status = rwvcs_instance_delete_instance(rwmain);
    RW_ASSERT(status == RW_STATUS_SUCCESS);

    dts_ret = RWDTS_ACTION_OK;
  }

send_resp:
done:
  return dts_ret;
}

static rwdts_member_rsp_code_t on_tracing(
    const rwdts_xact_info_t * xact_info,
    RWDtsQueryAction action,
    const rw_keyspec_path_t * key,
    const ProtobufCMessage * msg,
    uint32_t credits,
    void *getnext_ptr)
{
  rw_status_t status;
  rwdts_member_rsp_code_t dts_ret = RWDTS_ACTION_OK;
  struct rwmain * rwmain;
  RWPB_T_MSG(RwBase_input_Tracing) * input;

  rwmain= (struct rwmain *)xact_info->ud;
  RW_CF_TYPE_VALIDATE(rwmain->rwvx, rwvx_instance_ptr_t);

  input = (RWPB_T_MSG(RwBase_input_Tracing) *)msg;
  if (!input->has_set)
    goto done;

  if (!input->set.has_level)
    goto done;

  for (int i = 0 ; i < RWTRACE_CATEGORY_LAST ; i++) {
    status = rwtrace_ctx_category_severity_set(
        rwmain->tasklet_info->rwtrace_instance,
        i,
        input->set.level);
    RW_ASSERT(status == RW_STATUS_SUCCESS);
  }

  dts_ret = RWDTS_ACTION_OK;

done:

  return dts_ret;
}

rw_status_t rwmain_setup_dts_rpcs(struct rwmain * rwmain)
{
  rwdts_registration_t dts_registrations[] = {
    // RPC start
    {
      .keyspec = (rw_keyspec_path_t *)RWPB_G_PATHSPEC_VALUE(RwVcs_input_Vstart),
      .desc = RWPB_G_MSG_PBCMD(RwVcs_input_Vstart),
      .flags = RWDTS_FLAG_PUBLISHER|RWDTS_FLAG_SHARED,
      .callback = {
        .cb.prepare = &on_vstart_new,

       }
    },

    // RPC stop
    {
      .keyspec = (rw_keyspec_path_t *)RWPB_G_PATHSPEC_VALUE(RwVcs_input_Vstop),
      .desc = RWPB_G_MSG_PBCMD(RwVcs_input_Vstop),
      .flags = RWDTS_FLAG_PUBLISHER|RWDTS_FLAG_SHARED,
      .callback = {
        .cb.prepare = &on_vstop_new,
      }
    },

    // RPC Tracing
    {
      .keyspec = (rw_keyspec_path_t *)RWPB_G_PATHSPEC_VALUE(RwBase_input_Tracing),
      .desc = RWPB_G_MSG_PBCMD(RwBase_input_Tracing),
      .flags = RWDTS_FLAG_PUBLISHER|RWDTS_FLAG_SHARED,
      .callback = {
        .cb.prepare = &on_tracing,
      }
    },

    {
      .keyspec = NULL,
    },
  };

  for (int i = 0; dts_registrations[i].keyspec; ++i) {
    dts_registrations[i].callback.ud = (void *)rwmain;
    rwdts_member_reg_handle_t reg_handle = rwdts_member_register(
        NULL,
        rwmain->dts,
        dts_registrations[i].keyspec,
        &dts_registrations[i].callback,
        dts_registrations[i].desc,
        dts_registrations[i].flags,
        NULL);
    RW_ASSERT(reg_handle);
  }

  return RW_STATUS_SUCCESS;
}
