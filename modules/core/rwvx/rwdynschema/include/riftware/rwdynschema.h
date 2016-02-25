/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 * Creation Date: 9/17/15
 * 
 */

#ifndef __RWDYNSCHEMA_H__
#define __RWDYNSCHEMA_H__

#include <glib.h>
#include <glib-object.h>

#include "rwdts.h"
#include "rwmemlog.h"
#include "yangmodel.h"

#ifdef __cplusplus
extern "C" {
#endif

  /*!
   * 
   */
  /// @cond GI_SCANNER
  /**
   * rwdynschema_app_sub_cb:
   * @app_instance: (type gpointer)
   * @numel:  
   * @module_names: (transfer full) (array length=numel) (allow-none):
   * @fxs_filenames: (transfer full) (array length=numel) (allow-none):
   * @so_filenames: (transfer full) (array length=numel) (allow-none):
   * @yang_filenames: (transfer full) (array length=numel) (allow-none):
   */
  /// @endcond
  typedef void (*rwdynschema_app_sub_cb)(
      void * app_instance,
      const int numel,
      char **module_names,
      char **fxs_filenames,
      char **so_filenames,
      char **yang_filenames);

  struct rwdynschema_dynamic_schema_registration_s {
#ifndef __GI_SCANNER__
    gint ref_count;

    rwdts_api_t * dts_handle;
    rwdts_member_reg_handle_t dts_registration;
    void * app_instance;
    rwdynschema_app_sub_cb app_sub_cb;
    char * app_name;

    int batch_size;
    int batch_capacity;
    char ** module_names;
    char ** fxs_filenames;
    char ** so_filenames;
    char ** yang_filenames;

    rwmemlog_instance_t * memlog_instance;
    rwmemlog_buffer_t * memlog_buffer;
#endif // __GI_SCANNER__
  };

  typedef struct rwdynschema_dynamic_schema_registration_s rwdynschema_dynamic_schema_registration_t;

#ifndef __GI_SCANNER__
  rwdynschema_dynamic_schema_registration_t *
  rwdynschema_dynamic_schema_registration_new(char const * app_name,
                                              rwdts_api_t * dts_handle,
                                              void * app_instance,
                                              rwdynschema_app_sub_cb app_sub_cb);
#endif // __GI_SCANNER__

  GType rwdynschema_dynamic_schema_registration_get_type(void);

  /*!
   * Register an app to get updates about dynamic schema changes.
   */
  /// @cond GI_SCANNER
  /**
   * rwdynschema_instance_register:
   * @dts_handle: (type RwDts.Api)
   * @app_sub_cb: (scope notified) (destroy app_instance_destructor) (closure app_instance)
   * @app_name:
   * @app_instance: (type gpointer)
   * @app_instance_destructor: (scope async) (nullable)
   * Returns: (transfer none)
   */
  /// @endcond
  rwdynschema_dynamic_schema_registration_t *
  rwdynschema_instance_register(rwdts_api_t * dts_handle,
                                rwdynschema_app_sub_cb app_sub_cb,
                                const char * app_name,
                                void * app_instance,
                                GDestroyNotify app_instance_destructor);

#ifndef __GI_SCANNER__
  /// @cond GI_SCANNER
  /**
   * rwdynschema_create_instance:
   * @app_sub_cb: (scope notified) (destroy app_instance_destructor) (closure app_instance)
   * @app_name:
   * @app_instance: (type gpointer)
   * @app_instance_destructor: (scope async) (nullable)
   * Returns: (transfer none)
   */
  /// @endcond
  void
  rwdynschema_register_instance(rwdynschema_dynamic_schema_registration_t * app_data,
                                rwdts_api_t * dts_handle);

  /// @cond GI_SCANNER
  /**
   * rwdynschema_create_instance:
   * @app_sub_cb: (scope notified) (destroy app_instance_destructor) (closure app_instance)
   * @app_name:
   * @app_instance: (type gpointer)
   * @app_instance_destructor: (scope async) (nullable)
   * Returns: (transfer none)
   */
  /// @endcond
  rwdynschema_dynamic_schema_registration_t *
  rwdynschema_create_instance(rwdynschema_app_sub_cb app_sub_cb,                              
                              const char * app_name,
                              void * app_instance,
                              GDestroyNotify app_instance_destructor);

  void rw_run_file_update_protocol(rwsched_instance_ptr_t sched,
                                   rwtasklet_info_ptr_t tinfo,
                                   rwsched_tasklet_ptr_t tasklet,
                                   rwdynschema_dynamic_schema_registration_t* app_data);

  bool rw_create_runtime_schema_dir();

  void rwdynschema_add_module_to_batch(rwdynschema_dynamic_schema_registration_t * reg,
                                       const char * module_name,
                                       const char * fxs_filename,
                                       const char * so_filename,
                                       const char * yang_filename);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif
