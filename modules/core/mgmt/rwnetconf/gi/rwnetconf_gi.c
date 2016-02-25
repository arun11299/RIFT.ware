
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/*
 * @file rwnetconf_gi.c
 * @author Garrett Regier
 * @date 2014/07/31
 * @brief GObject-Introspection bindable API for rwnetconf
 */

#include "rwnetconf_gi.h"


#define RWNETCONF_DEFINE_BOXED(type_name) \
static type_name##_t * _##type_name##_ref(type_name##_t *self) \
{ \
    type_name##_retain(self); \
    return self; \
} \
\
static void _##type_name##_unref(type_name##_t *self) \
{ \
    type_name##_release(self); \
} \
\
G_DEFINE_BOXED_TYPE(type_name##_t, type_name, \
                    _##type_name##_ref, \
                    _##type_name##_unref)


static const GEnumValue _rw_ncclnt_instance_state_enum_values[] = {
  { RW_NCCLNT_STATE_INITIALIZED, "INITIALIZED", "INITIALIZED" },
  { RW_NCCLNT_STATE_TERMINATED,  "TERMINATED",  "TERMINATED"  },
  { 0, NULL, NULL }
};

GType rw_ncclnt_instance_state_get_type(void)
{
  static GType type = 0; /* G_TYPE_INVALID */

  if (!type)
    type = g_enum_register_static("rw_ncclnt_instance_state_t", _rw_ncclnt_instance_state_enum_values);

  return type;
}


RWNETCONF_DEFINE_BOXED(rw_ncclnt_instance)
RWNETCONF_DEFINE_BOXED(rw_ncclnt_user)
RWNETCONF_DEFINE_BOXED(rw_ncclnt_cbmgr)
RWNETCONF_DEFINE_BOXED(rw_ncclnt_ses)
RWNETCONF_DEFINE_BOXED(rw_ncclnt_xml)
RWNETCONF_DEFINE_BOXED(rw_ncclnt_xmlgen)
RWNETCONF_DEFINE_BOXED(rw_ncclnt_ds)
RWNETCONF_DEFINE_BOXED(rw_ncclnt_filter)



static void
rw_ncclnt_ses_connect_full_cb_real(rw_ncclnt_context_t context,
                                   rw_yang_netconf_op_status_t error,
                                   rw_ncclnt_ses_t *ses)
{
  rw_ncclnt_ses_connect_full_cb cb;

  cb = (rw_ncclnt_ses_connect_full_cb) context.data1;

  cb (error, ses, (void *) context.data2);
}

rw_status_t
rw_ncclnt_ses_connect_nc_ssh_full(rw_ncclnt_instance_t* instance,
                                  rw_ncclnt_user_t* user,
                                  const char* host,
                                  rw_ncclnt_ssh_credentials_t* cred,
                                  rw_ncclnt_cbmgr_t *cbmgr,
                                  rw_ncclnt_ses_connect_full_cb callback,
                                  void *data)
{
  rw_ncclnt_ses_connect_context_t context;

  context.context.data1 = (intptr_t) callback;
  context.context.data2 = (intptr_t) data;
  context.callback = rw_ncclnt_ses_connect_full_cb_real;
  context.cbmgr = cbmgr;

  return rw_ncclnt_ses_connect_nc_ssh(instance, user, host, cred, context);
}


static void
rw_ncclnt_xml_consume_wait_full_cb_real(rw_ncclnt_context_t context,
                                        rw_yang_netconf_op_status_t error)
{
  rw_ncclnt_xml_consume_wait_full_cb cb;

  cb = (rw_ncclnt_xml_consume_wait_full_cb) context.data1;

  cb (error, (void *) context.data2);
}

rw_status_t
rw_ncclnt_xml_consume_wait_producer_full(rw_ncclnt_xml_t* xml,
                                         rw_ncclnt_cbmgr_t *cbmgr,
                                         rw_ncclnt_xml_consume_wait_full_cb callback,
                                         void *data)
{
  rw_ncclnt_xml_consume_wait_context_t context;

  context.context.data1 = (intptr_t) callback;
  context.context.data2 = (intptr_t) data;
  context.callback = rw_ncclnt_xml_consume_wait_full_cb_real;
  context.cbmgr = cbmgr;

  return rw_ncclnt_xml_consume_wait_producer(xml, context);
}



static void
rw_ncclnt_xmlgen_produce_wait_full_cb_real(rw_ncclnt_context_t context,
                                           rw_yang_netconf_op_status_t error)
{
  rw_ncclnt_xml_consume_wait_full_cb cb;

  cb = (rw_ncclnt_xml_consume_wait_full_cb) context.data1;

  cb (error, (void *) context.data2);
}

rw_status_t
rw_ncclnt_xmlgen_produce_wait_consumer_full(rw_ncclnt_xmlgen_t* xmlgen,
                                            rw_ncclnt_cbmgr_t *cbmgr,
                                            rw_ncclnt_xmlgen_produce_wait_full_cb callback,
                                            void *data)
{
  rw_ncclnt_xmlgen_produce_wait_context_t context;

  context.context.data1 = (intptr_t) callback;
  context.context.data2 = (intptr_t) data;
  context.callback = rw_ncclnt_xmlgen_produce_wait_full_cb_real;
  context.cbmgr = cbmgr;

  return rw_ncclnt_xmlgen_produce_wait_consumer(xmlgen, context);
}



static void
rw_ncclnt_nc_req_full_cb_real(rw_ncclnt_context_t context,
                              rw_yang_netconf_op_status_t error,
                              rw_ncclnt_xml_t* xml)
{
  rw_ncclnt_nc_req_full_cb cb;

  cb = (rw_ncclnt_nc_req_full_cb) context.data1;

  cb (error, xml, (void *) context.data2);
}


rw_status_t
rw_ncclnt_req_rpc_xml_full(rw_ncclnt_ses_t* ses,
                           rw_ncclnt_xml_t* rpc_body,
                           rw_ncclnt_cbmgr_t *cbmgr,
                           rw_ncclnt_nc_req_full_cb callback,
                           void *data)
{
  rw_ncclnt_nc_req_context_t context;

  context.context.data1 = (intptr_t) callback;
  context.context.data2 = (intptr_t) data;
  context.callback = rw_ncclnt_nc_req_full_cb_real;
  context.cbmgr = cbmgr;

  return rw_ncclnt_req_rpc_xml(ses, rpc_body, context);
}

rw_status_t
rw_ncclnt_req_rpc_name_ns_full(rw_ncclnt_ses_t* ses,
                               const char* name,
                               const char* ns,
                               rw_ncclnt_xml_t* rpc_body,
                               rw_ncclnt_cbmgr_t *cbmgr,
                               rw_ncclnt_nc_req_full_cb callback,
                               void *data)
{
  rw_ncclnt_nc_req_context_t context;

  context.context.data1 = (intptr_t) callback;
  context.context.data2 = (intptr_t) data;
  context.callback = rw_ncclnt_nc_req_full_cb_real;
  context.cbmgr = cbmgr;

  return rw_ncclnt_req_rpc_name_ns(ses, name, ns, rpc_body, context);
}

rw_status_t
rw_ncclnt_req_rpc_yang_full(rw_ncclnt_ses_t* ses,
                            rw_yang_node_t* ynode,
                            rw_ncclnt_xml_t* rpc_body,
                            rw_ncclnt_cbmgr_t *cbmgr,
                            rw_ncclnt_nc_req_full_cb callback,
                            void *data)
{
  rw_ncclnt_nc_req_context_t context;

  context.context.data1 = (intptr_t) callback;
  context.context.data2 = (intptr_t) data;
  context.callback = rw_ncclnt_nc_req_full_cb_real;
  context.cbmgr = cbmgr;

  return rw_ncclnt_req_rpc_yang(ses, ynode, rpc_body, context);
}

rw_status_t
rw_ncclnt_req_nc_get_config_full(rw_ncclnt_ses_t* ses,
                                 const rw_ncclnt_ds_t* config_source,
                                 rw_ncclnt_filter_t* filter,
                                 rw_ncclnt_cbmgr_t *cbmgr,
                                 rw_ncclnt_nc_req_full_cb callback,
                                 void *data)
{
  rw_ncclnt_nc_req_context_t context;

  context.context.data1 = (intptr_t) callback;
  context.context.data2 = (intptr_t) data;
  context.callback = rw_ncclnt_nc_req_full_cb_real;
  context.cbmgr = cbmgr;

  return rw_ncclnt_req_nc_get_config(ses, config_source, filter, context);
}

#if 0
rw_status_t
rw_ncclnt_req_nc_edit_config_full(rw_ncclnt_ses_t* ses,
                                  const rw_ncclnt_ds_t* target,
                                  void* default_op,
                                  void* test_opt,
                                  void* error_opt,
                                  rw_ncclnt_xml_t* config,
                                  rw_ncclnt_cbmgr_t *cbmgr,
                                  rw_ncclnt_nc_req_full_cb callback,
                                  void *data)
{
  rw_ncclnt_nc_req_context_t context;

  context.context.data1 = (intptr_t) callback;
  context.context.data2 = (intptr_t) data;
  context.callback = rw_ncclnt_nc_req_full_cb_real;
  context.cbmgr = cbmgr;

  return rw_ncclnt_req_nc_get_config(ses, target, default_op,
                                     test_op, error_opt, config, context);
}
#endif

rw_status_t
rw_ncclnt_req_nc_get_full(rw_ncclnt_ses_t* ses,
                          rw_ncclnt_filter_t* filter,
                          rw_ncclnt_cbmgr_t *cbmgr,
                          rw_ncclnt_nc_req_full_cb callback,
                          void *data)
{
  rw_ncclnt_nc_req_context_t context;

  context.context.data1 = (intptr_t) callback;
  context.context.data2 = (intptr_t) data;
  context.callback = rw_ncclnt_nc_req_full_cb_real;
  context.cbmgr = cbmgr;

  return rw_ncclnt_req_nc_get(ses, filter, context);
}
