/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 * Creation Date: 9/11/15
 * 
 */
#include <glib-object.h>

#include "rwdts.h"
#include "rwdts_api_gi.h"
#include "rwdts_int.h"
#include "rwdts_member_api.h"
#include "rwdts_query_api.h"
#include "rwmemlog.h"
#include "rwmemlog_mgmt.h"

#include "rw-mgmt-schema.pb-c.h"
#include "rw_file_update.h"
#include "rwdynschema.h"
#include "rwdynschema_load_schema.h"

static rwdynschema_dynamic_schema_registration_t *
rwdynschema_dynamic_schema_registration_ref(rwdynschema_dynamic_schema_registration_t * boxed);
static void
rwdynschema_dynamic_schema_registration_unref(rwdynschema_dynamic_schema_registration_t * boxed);

G_DEFINE_BOXED_TYPE(rwdynschema_dynamic_schema_registration_t,
                    rwdynschema_dynamic_schema_registration,
                    rwdynschema_dynamic_schema_registration_ref,
                    rwdynschema_dynamic_schema_registration_unref);

rwdynschema_dynamic_schema_registration_t *
rwdynschema_dynamic_schema_registration_ref(rwdynschema_dynamic_schema_registration_t * boxed)
{
  g_atomic_int_inc(&boxed->ref_count);
  return boxed;
}

void
rwdynschema_dynamic_schema_registration_unref(rwdynschema_dynamic_schema_registration_t * boxed)
{
  if (!g_atomic_int_dec_and_test(&boxed->ref_count)) {
    return;
  }

  int i = 0;
  for (; i < boxed->batch_size; ++i) {
    RW_FREE(boxed->module_names[i]);
    RW_FREE(boxed->fxs_filenames[i]);
    RW_FREE(boxed->so_filenames[i]);
    RW_FREE(boxed->yang_filenames[i]);
  }
  RW_FREE(boxed->module_names);
  RW_FREE(boxed->fxs_filenames);
  RW_FREE(boxed->so_filenames);
  RW_FREE(boxed->yang_filenames);
  
  g_free(boxed);
}

rwdynschema_dynamic_schema_registration_t *
rwdynschema_dynamic_schema_registration_new(char const * app_name,
                                            rwdts_api_t * dts_handle,
                                            void * app_instance,
                                            rwdynschema_app_sub_cb app_sub_cb)
{
  rwdynschema_dynamic_schema_registration_t *boxed =
      (rwdynschema_dynamic_schema_registration_t *)g_new0(rwdynschema_dynamic_schema_registration_t, 1);

  boxed->ref_count = 1;
    
  boxed->dts_handle = dts_handle;
  boxed->app_instance = app_instance;
  boxed->app_sub_cb = app_sub_cb;
  
  boxed->batch_size = 0;
  boxed->batch_capacity = 128;

  boxed->module_names = RW_MALLOC(boxed->batch_capacity * sizeof(char *));
  boxed->fxs_filenames = RW_MALLOC(boxed->batch_capacity * sizeof(char *));
  boxed->so_filenames = RW_MALLOC(boxed->batch_capacity * sizeof(char *));
  boxed->yang_filenames = RW_MALLOC(boxed->batch_capacity * sizeof(char *));  

  boxed->app_name = RW_STRDUP(app_name);
  boxed->memlog_instance = rwmemlog_instance_alloc(app_name, 0, 200);
  boxed->memlog_buffer = rwmemlog_instance_get_buffer(boxed->memlog_instance,
                                                      "dynamic_schema",
                                                      (intptr_t)(app_instance));

  return boxed;
}

void rwdynschema_grow_module_array(rwdynschema_dynamic_schema_registration_t * reg)
{
  
  reg->batch_capacity = reg->batch_capacity * 2;

  reg->module_names = RW_REALLOC(reg->module_names, reg->batch_capacity * sizeof(char *));
  reg->fxs_filenames = RW_REALLOC(reg->fxs_filenames, reg->batch_capacity * sizeof(char *));
  reg->so_filenames = RW_REALLOC(reg->so_filenames, reg->batch_capacity * sizeof(char *));
  reg->yang_filenames = RW_REALLOC(reg->yang_filenames, reg->batch_capacity * sizeof(char *));  
}

void rwdynschema_add_module_to_batch(rwdynschema_dynamic_schema_registration_t * reg,
                                     const char * module_name,
                                     const char * fxs_filename,
                                     const char * so_filename,
                                     const char * yang_filename)
{
  if (reg->batch_size >= reg->batch_capacity) {
    rwdynschema_grow_module_array(reg);
  }

  reg->module_names[reg->batch_size] = RW_STRDUP(module_name);
  reg->fxs_filenames[reg->batch_size] = RW_STRDUP(fxs_filename);
  reg->so_filenames[reg->batch_size] = RW_STRDUP(so_filename);
  reg->yang_filenames[reg->batch_size] = RW_STRDUP(yang_filename);

  ++reg->batch_size;
}

rwdts_member_rsp_code_t rwdynschema_app_commit(const rwdts_xact_info_t * xact_info,
                                               uint32_t n_crec,
                                               const rwdts_commit_record_t** crec)
{
  RW_ASSERT(xact_info);
  rwdts_xact_t *xact = xact_info->xact;
  RW_ASSERT_TYPE(xact, rwdts_xact_t);

  rwdynschema_dynamic_schema_registration_t * app_data =
      (rwdynschema_dynamic_schema_registration_t *) xact_info->ud;
  RW_ASSERT(app_data);

  rw_run_file_update_protocol(xact_info->apih->sched,
                              xact_info->apih->rwtasklet_info,
                              xact_info->apih->tasklet,
                              app_data);
                      
  app_data->batch_size = 0;

  return RWDTS_ACTION_OK;
}

rwdts_member_rsp_code_t
rwdynschema_app_prepare(const rwdts_xact_info_t * xact_info,
                        RWDtsQueryAction action,
                        const rw_keyspec_path_t * keyspec,
                        const ProtobufCMessage * msg,
                        uint32_t credits,
                        void * get_next_key)
{
  if (action == RWDTS_QUERY_UPDATE) {
    return RWDTS_ACTION_OK;
  }

  RW_ASSERT(xact_info);
  rwdts_xact_t *xact = xact_info->xact;
  RW_ASSERT_TYPE(xact, rwdts_xact_t);

  rwdynschema_dynamic_schema_registration_t * app_data =
      (rwdynschema_dynamic_schema_registration_t *) xact_info->ud;
  RW_ASSERT(app_data);

  RWPB_T_MSG(RwMgmtSchema_data_RwMgmtSchemaState_DynamicModules) * dynamic_modules
      = (RWPB_T_MSG(RwMgmtSchema_data_RwMgmtSchemaState_DynamicModules) *) msg;

  rwdynschema_add_module_to_batch(app_data,
                                  dynamic_modules->name,
                                  dynamic_modules->module->fxs_filename,
                                  dynamic_modules->module->so_filename,
                                  dynamic_modules->module->yang_filename);
  
  return RWDTS_ACTION_OK;
}

rwdynschema_dynamic_schema_registration_t *
rwdynschema_instance_register(rwdts_api_t * dts_handle,
                              rwdynschema_app_sub_cb app_sub_cb,
                              const char * app_name,
                              void * app_instance,
                              GDestroyNotify app_instance_destructor)
{
  // construct app data collection 
  rwdynschema_dynamic_schema_registration_t * app_data =
      rwdynschema_dynamic_schema_registration_new(app_name,
                                                  dts_handle,
                                                  app_instance,
                                                  app_sub_cb);

  const rw_yang_pb_schema_t* mgmt_schema = rw_load_schema("librwschema_yang_gen.so", "rw-mgmt-schema");
  rwdts_api_add_ypbc_schema(dts_handle, mgmt_schema);

  // register with DTS
  RWPB_T_PATHSPEC(RwMgmtSchema_data_RwMgmtSchemaState_DynamicModules) sub_keyspec_entry =
      (*RWPB_G_PATHSPEC_VALUE(RwMgmtSchema_data_RwMgmtSchemaState_DynamicModules));

  rw_keyspec_path_t *sub_keyspec = (rw_keyspec_path_t*)&sub_keyspec_entry;
  rw_keyspec_path_set_category((rw_keyspec_path_t*)sub_keyspec,
                               NULL ,
                               RW_SCHEMA_CATEGORY_DATA);
                               
  rwdts_member_event_cb_t reg_cb;
  RW_ZERO_VARIABLE(&reg_cb);

  reg_cb.cb.prepare = rwdynschema_app_prepare;
  reg_cb.cb.commit = rwdynschema_app_commit;
  reg_cb.ud = (void *)app_data;

  rwdts_member_reg_handle_t sub_registration = rwdts_member_register(NULL,
                                                                     dts_handle,
                                                                     sub_keyspec,
                                                                     &reg_cb,
                                                                     RWPB_G_MSG_PBCMD(
                                                                         RwMgmtSchema_data_RwMgmtSchemaState_DynamicModules),
                                                                     RWDTS_FLAG_SUBSCRIBER,
                                                                     NULL);
  
  // save DTS details for app
  app_data->dts_registration = sub_registration;

  // Create the runtime schema directories on this vm, if not already created, and load the schema
  rwsched_dispatch_async_f(app_data->dts_handle->tasklet,
                           rwsched_dispatch_get_main_queue(app_data->dts_handle->sched),
                           (void *)app_data,
                           rwdynschema_load_all_schema);

  return app_data;
}

