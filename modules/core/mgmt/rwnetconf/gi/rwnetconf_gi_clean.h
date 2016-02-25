
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 *
 */


typedef struct rw_ncclnt_token_datastore_s rw_ncclnt_token_datastore_t;


typedef struct rw_ncclnt_token_lock_s rw_ncclnt_token_lock_t;


typedef struct rw_ncclnt_token_commit_s rw_ncclnt_token_commit_t;


typedef struct rw_ncclnt_token_iterator_s rw_ncclnt_token_iterator_t;


typedef struct rw_ncclnt_context_s
{
  intptr_t data1;
  intptr_t data2;
} rw_ncclnt_context_t;





typedef struct rw_ncclnt_ssh_credentials_s rw_ncclnt_ssh_credentials_t;



typedef struct rw_xml_yang_node_t rw_xml_yang_node_t;


enum rw_ncclnt_status_t {
  UNDEFINED = 0
};


typedef enum rw_ncclnt_instance_state_e {
  RW_NCCLNT_STATE_NULL = 0,
  RW_NCCLNT_STATE_base = 0x33100,
  RW_NCCLNT_STATE_INITIALIZED,
  RW_NCCLNT_STATE_TERMINATED,
  RW_NCCLNT_STATE_end,

  RW_NCCLNT_STATE_FIRST = RW_NCCLNT_STATE_base+1,
  RW_NCCLNT_STATE_LAST = RW_NCCLNT_STATE_end-1,
  RW_NCCLNT_STATE_COUNT = RW_NCCLNT_STATE_end - RW_NCCLNT_STATE_base-1,

} rw_ncclnt_instance_state_t;

static inline bool_t rw_ncclnt_instance_state_is_good(rw_ncclnt_instance_state_t v)
{
  return (int)v >= (int)RW_NCCLNT_STATE_FIRST
         && (int)v <= (int)RW_NCCLNT_STATE_LAST;
}
static inline bool_t rw_ncclnt_instance_state_is_good_or_null(rw_ncclnt_instance_state_t v)
{
  return v == RW_NCCLNT_STATE_NULL
         || ( (int)v >= (int)RW_NCCLNT_STATE_FIRST
              && (int)v <= (int)RW_NCCLNT_STATE_LAST);
}
static inline size_t rw_ncclnt_instance_state_to_index(rw_ncclnt_instance_state_t v)
{
  return (size_t)((int)v - (int)RW_NCCLNT_STATE_FIRST);
}
static inline rw_ncclnt_instance_state_t rw_ncclnt_instance_state_from_index(size_t i)
{
  return (rw_ncclnt_instance_state_t)(i + (size_t)RW_NCCLNT_STATE_FIRST);
}

struct rw_ncclnt_instance_s;
typedef struct rw_ncclnt_instance_s rw_ncclnt_instance_t; typedef rw_ncclnt_instance_t *rw_ncclnt_instance_ptr_t;


rw_ncclnt_instance_t* rw_ncclnt_instance_create(





  rw_yang_model_t* model,







  rwtrace_ctx_t* trace_instance
);

void rw_ncclnt_instance_retain(
  const rw_ncclnt_instance_t* instance
);

void rw_ncclnt_instance_release(
  const rw_ncclnt_instance_t* instance
);

void rw_ncclnt_instance_terminate(
  rw_ncclnt_instance_t* instance
);

rw_ncclnt_instance_state_t rw_ncclnt_instance_get_state(
  const rw_ncclnt_instance_t* instance
);





rw_yang_model_t* rw_ncclnt_instance_get_model(
  const rw_ncclnt_instance_t* instance
);






rwtrace_ctx_t* rw_ncclnt_instance_get_trace_instance(
  const rw_ncclnt_instance_t* instance
);






CFAllocatorRef rw_ncclnt_instance_get_allocator(
  const rw_ncclnt_instance_t* instance
);

struct rw_ncclnt_user_s;
typedef struct rw_ncclnt_user_s rw_ncclnt_user_t; typedef rw_ncclnt_user_t *rw_ncclnt_user_ptr_t;


void rw_ncclnt_user_retain(
  const rw_ncclnt_user_t* user
);

void rw_ncclnt_user_release(
  const rw_ncclnt_user_t* user
);

rw_ncclnt_user_t* rw_ncclnt_user_create_self(
  rw_ncclnt_instance_t* instance
);

rw_ncclnt_user_t* rw_ncclnt_user_create_name(
  rw_ncclnt_instance_t* instance,
  const char* username
);

rw_ncclnt_instance_t* rw_ncclnt_user_get_instance(
  const rw_ncclnt_user_t* user
);

const char* rw_ncclnt_user_get_username(
  const rw_ncclnt_user_t* user
);

void rw_ncclnt_user_set_username(
    rw_ncclnt_user_t* user,
    const char *name
);

struct rw_ncclnt_cbmgr_s;
typedef struct rw_ncclnt_cbmgr_s rw_ncclnt_cbmgr_t; typedef rw_ncclnt_cbmgr_t *rw_ncclnt_cbmgr_ptr_t;


void rw_ncclnt_cbmgr_retain(
  const rw_ncclnt_cbmgr_t* cbmgr
);

void rw_ncclnt_cbmgr_release(

  const rw_ncclnt_cbmgr_t* cbmgr
);

void rw_ncclnt_cbmgr_terminate(
  rw_ncclnt_cbmgr_t* cbmgr
);

rw_ncclnt_instance_t* rw_ncclnt_cbmgr_get_instance(
  const rw_ncclnt_cbmgr_t* cbmgr
);

rw_ncclnt_cbmgr_t* rw_ncclnt_cbmgr_create_polled_queue(
  rw_ncclnt_instance_t* instance
);

rw_status_t rw_ncclnt_cbmgr_poll_once(
  rw_ncclnt_cbmgr_t* cbmgr
);

rw_status_t rw_ncclnt_cbmgr_poll_all(
  rw_ncclnt_cbmgr_t* cbmgr
);

struct rw_ncclnt_ses_s;
typedef struct rw_ncclnt_ses_s rw_ncclnt_ses_t; typedef rw_ncclnt_ses_t *rw_ncclnt_ses_ptr_t;


void rw_ncclnt_ses_retain(
  const rw_ncclnt_ses_t* ses
);

void rw_ncclnt_ses_release(
  const rw_ncclnt_ses_t* ses
);

void rw_ncclnt_ses_terminate(
  rw_ncclnt_ses_t* ses
);





typedef void (*rw_ncclnt_ses_connect_cb)(
  rw_ncclnt_context_t context,
  rw_yang_netconf_op_status_t error,
  rw_ncclnt_ses_t* ses
);

typedef struct rw_ncclnt_ses_connect_context_s
{

  rw_ncclnt_context_t context;


  rw_ncclnt_ses_connect_cb callback;


  rw_ncclnt_cbmgr_ptr_t cbmgr;
} rw_ncclnt_ses_connect_context_t;

rw_status_t rw_ncclnt_ses_connect_nc_ssh(
  rw_ncclnt_instance_t* instance,
  rw_ncclnt_user_t* user,
  const char* host,
  rw_ncclnt_ssh_credentials_t* cred,
  rw_ncclnt_ses_connect_context_t context
);

rw_ncclnt_instance_t* rw_ncclnt_ses_get_instance(
  const rw_ncclnt_ses_t* ses
);

rw_ncclnt_user_t* rw_ncclnt_ses_get_user(
  const rw_ncclnt_ses_t* ses
);

struct rw_ncclnt_xml_s;
typedef struct rw_ncclnt_xml_s rw_ncclnt_xml_t; typedef rw_ncclnt_xml_t *rw_ncclnt_xml_ptr_t;


void rw_ncclnt_xml_retain(
  rw_ncclnt_xml_t* xml
);

void rw_ncclnt_xml_release(
  rw_ncclnt_xml_t* xml
);

void rw_ncclnt_xml_terminate(
  rw_ncclnt_xml_t* xml
);

rw_ncclnt_xml_t* rw_ncclnt_xml_create_empty(
  rw_ncclnt_instance_t* instance
);

rw_ncclnt_xml_t* rw_ncclnt_xml_create_copy_string(
  rw_ncclnt_instance_t* instance,
  const char* string
);

rw_ncclnt_xml_t* rw_ncclnt_xml_create_const_string(
  rw_ncclnt_instance_t* instance,
  const char* string
);

rw_ncclnt_xml_t* rw_ncclnt_xml_create_copy_buffer(
  rw_ncclnt_instance_t* instance,
  const void* buffer,
  size_t buflen
);

rw_ncclnt_xml_t* rw_ncclnt_xml_create_const_buffer(
  rw_ncclnt_instance_t* instance,
  const void* buffer,
  size_t buflen
);

rw_ncclnt_xml_t* rw_ncclnt_xml_create_composed(
  rw_ncclnt_instance_t* instance,
  const char* prefix,
  rw_ncclnt_xml_t* body,
  const char* suffix
);

rw_ncclnt_xml_t* rw_ncclnt_xml_create_append(
  rw_ncclnt_instance_t* instance,
  rw_ncclnt_xml_t* prefix,
  rw_ncclnt_xml_t* suffix
);

rw_ncclnt_xml_t* rw_ncclnt_xml_create_xml_yang_subtree(
  rw_ncclnt_instance_t* instance,
  rw_xml_yang_node_t* node,
  unsigned depth_limit
);

rw_ncclnt_xml_t* rw_ncclnt_xml_create_xml_subtree(
  rw_ncclnt_instance_t* instance,
  rw_xml_node_t* node,
  unsigned depth_limit
);

rw_ncclnt_instance_t* rw_ncclnt_xml_get_instance(
  const rw_ncclnt_xml_t* xml
);

rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_buffer(
  rw_ncclnt_xml_t* xml,
  void* buffer,
  size_t buflen,
  size_t* outlen
);

rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_buffer_pointer(
  rw_ncclnt_xml_t* xml,
  const void** outbuf,
  size_t* outlen
);

rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_bytes(
  rw_ncclnt_xml_t* xml,
  size_t consume
);

typedef void (*rw_ncclnt_xml_consume_wait_cb)(
  rw_ncclnt_context_t context,
  rw_yang_netconf_op_status_t error
);

typedef struct rw_ncclnt_xml_consume_wait_context_s
{

  rw_ncclnt_context_t context;


  rw_ncclnt_xml_consume_wait_cb callback;


  rw_ncclnt_cbmgr_ptr_t cbmgr;
} rw_ncclnt_xml_consume_wait_context_t;

rw_status_t rw_ncclnt_xml_consume_wait_producer(
  rw_ncclnt_xml_t* xml,
  rw_ncclnt_xml_consume_wait_context_t
);

rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_get_producer_error(
  rw_ncclnt_xml_t* xml
);

bool rw_ncclnt_xml_consume_is_eof(
  rw_ncclnt_xml_t* xml
);

void rw_ncclnt_xml_consume_set_error(
  rw_ncclnt_xml_t* xml,
  rw_yang_netconf_op_status_t status
);

typedef struct rw_ncclnt_xmlgen_s rw_ncclnt_xmlgen_t;

void rw_ncclnt_xmlgen_retain(
  const rw_ncclnt_xmlgen_t* xmlgen
);

void rw_ncclnt_xmlgen_release(
  const rw_ncclnt_xmlgen_t* xmlgen
);

void rw_ncclnt_xmlgen_terminate(
  rw_ncclnt_xmlgen_t* xmlgen
);

rw_ncclnt_xmlgen_t* rw_ncclnt_xml_create_xml_generator(
  rw_ncclnt_instance_t* instance

);

rw_ncclnt_instance_t* rw_ncclnt_xmlgen_get_instance(
  const rw_ncclnt_xmlgen_t* xmlgen
);

rw_ncclnt_xml_t* rw_ncclnt_xmlgen_get_consumer(
  rw_ncclnt_xmlgen_t* xmlgen
);

rw_yang_netconf_op_status_t rw_ncclnt_xmlgen_produce_buffer(
  rw_ncclnt_xmlgen_t* xmlgen,
  const void* buffer,
  size_t buflen,
  size_t* outlen
);

typedef void (*rw_ncclnt_xmlgen_produce_wait_cb)(
  rw_ncclnt_context_t context,
  rw_yang_netconf_op_status_t error
);

typedef struct rw_ncclnt_xmlgen_produce_wait_context_s
{

  rw_ncclnt_context_t context;


  rw_ncclnt_xmlgen_produce_wait_cb callback;


  rw_ncclnt_cbmgr_ptr_t cbmgr;
} rw_ncclnt_xmlgen_produce_wait_context_t;

rw_status_t rw_ncclnt_xmlgen_produce_wait_consumer(
  rw_ncclnt_xmlgen_t* xmlgen,
  rw_ncclnt_xmlgen_produce_wait_context_t
);

rw_yang_netconf_op_status_t rw_ncclnt_xmlgen_produce_get_consumer_error(
  rw_ncclnt_xmlgen_t* xmlgen
);

void rw_ncclnt_xmlgen_produce_set_eof(
  rw_ncclnt_xmlgen_t* xmlgen
);

void rw_ncclnt_xmlgen_produce_set_error(
  rw_ncclnt_xmlgen_t* xmlgen,
  rw_yang_netconf_op_status_t status
);

struct rw_ncclnt_ds_s;
typedef struct rw_ncclnt_ds_s rw_ncclnt_ds_t; typedef rw_ncclnt_ds_t *rw_ncclnt_ds_ptr_t;


void rw_ncclnt_ds_retain(
  const rw_ncclnt_ds_t* ds
);

void rw_ncclnt_ds_release(
  const rw_ncclnt_ds_t* ds
);

rw_ncclnt_ds_t* rw_ncclnt_ds_create_url(
  rw_ncclnt_instance_t* instance,
  const char* url
);

const rw_ncclnt_ds_t* rw_ncclnt_ds_get_startup(
  rw_ncclnt_instance_t* instance
);

const rw_ncclnt_ds_t* rw_ncclnt_ds_get_running(
  rw_ncclnt_instance_t* instance
);

const rw_ncclnt_ds_t* rw_ncclnt_ds_get_candidate(
  rw_ncclnt_instance_t* instance
);

rw_ncclnt_instance_t* rw_ncclnt_ds_get_instance(
  const rw_ncclnt_ds_t* ds
);

const char* rw_ncclnt_ds_get_name(
  const rw_ncclnt_ds_t* ds
);

const char* rw_ncclnt_ds_get_url(
  const rw_ncclnt_ds_t* ds
);

rw_ncclnt_xml_t* rw_ncclnt_ds_get_create_xml(
  const rw_ncclnt_ds_t* ds
);

struct rw_ncclnt_filter_s;
typedef struct rw_ncclnt_filter_s rw_ncclnt_filter_t; typedef rw_ncclnt_filter_t *rw_ncclnt_filter_ptr_t;


void rw_ncclnt_filter_retain(
  const rw_ncclnt_filter_t* filter
);

void rw_ncclnt_filter_release(
  const rw_ncclnt_filter_t* filter
);

rw_ncclnt_instance_t* rw_ncclnt_filter_get_instance(
  const rw_ncclnt_filter_t* filter
);

rw_ncclnt_xml_t* rw_ncclnt_filter_get_create_xml(
    const rw_ncclnt_filter_t* filter
);

rw_ncclnt_filter_t* rw_ncclnt_filter_create_xpath(
    rw_ncclnt_instance_t* instance);

rw_ncclnt_filter_t* rw_ncclnt_filter_create_xpath_from_str(
    rw_ncclnt_instance_t* instance,

    const char *value);

rw_ncclnt_filter_t* rw_ncclnt_filter_create_subtree(
    rw_ncclnt_instance_t* instance);

rw_ncclnt_filter_t* rw_ncclnt_filter_create_subtree_from_str(
    rw_ncclnt_instance_t* instance,

    const char *value);

typedef void (*rw_ncclnt_nc_req_cb)(
  rw_ncclnt_context_t context,
  rw_yang_netconf_op_status_t error,
  rw_ncclnt_xml_t* xml
);

typedef struct rw_ncclnt_nc_req_context_s
{

  rw_ncclnt_context_t context;


  rw_ncclnt_nc_req_cb callback;


  rw_ncclnt_cbmgr_ptr_t cbmgr;
} rw_ncclnt_nc_req_context_t;

rw_status_t rw_ncclnt_req_rpc_xml(
  rw_ncclnt_ses_t* ses,
  rw_ncclnt_xml_t* rpc_body,
  rw_ncclnt_nc_req_context_t context
);

rw_status_t rw_ncclnt_req_rpc_name_ns(
  rw_ncclnt_ses_t* ses,
  const char* name,
  const char* ns,
  rw_ncclnt_xml_t* rpc_body,
  rw_ncclnt_nc_req_context_t context
);

rw_status_t rw_ncclnt_req_rpc_yang(
  rw_ncclnt_ses_t* ses,
  rw_yang_node_t* ynode,
  rw_ncclnt_xml_t* rpc_body,
  rw_ncclnt_nc_req_context_t context
);

rw_status_t rw_ncclnt_req_nc_get_config(
  rw_ncclnt_ses_t* ses,
  const rw_ncclnt_ds_t* config_source,
  rw_ncclnt_filter_t* filter,
  rw_ncclnt_nc_req_context_t context
);

rw_status_t rw_ncclnt_req_nc_edit_config(
  rw_ncclnt_ses_t* ses,
  const rw_ncclnt_ds_t* target,
  void* default_op,
  void* test_opt,
  void* error_opt,
  rw_ncclnt_xml_t* config,
  rw_ncclnt_nc_req_context_t context
);

rw_status_t rw_ncclnt_req_nc_get(
  rw_ncclnt_ses_t* ses,
  rw_ncclnt_filter_t* filter,


  rw_ncclnt_nc_req_context_t context
);
