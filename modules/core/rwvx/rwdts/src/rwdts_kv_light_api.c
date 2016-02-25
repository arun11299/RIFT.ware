
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwdts_kv_light_api.c
 * @author Prashanth Bhaskar (Prashanth.Bhaskar@riftio.com)
 * @date 2014/10/02
 * @brief RWDTS KV Light API implementation
 */

#include "rwdts_kv_light_api.h"

rwdts_db_fn_table_t rwdts_fn_table[MAX_DB]  =
{
  { NULL, 
    rwdts_kv_redis_connect_api,
    rwdts_kv_redis_set_api,
    rwdts_kv_redis_get_api,
    rwdts_kv_redis_del_api,
    rwdts_kv_redis_disc_api,
    rwdts_kv_redis_set_hash_api,
    rwdts_kv_redis_get_hash_api,
    rwdts_kv_redis_del_hash_api,
    rwdts_kv_redis_hash_exists_api,
    rwdts_kv_redis_get_next_hash_api,
    rwdts_kv_redis_delete_tab_api,
    rwdts_kv_redis_get_hash_sernum_api,
    rwdts_kv_redis_get_all_api,
    rwdts_kv_redis_del_shard_entries_api,
    rwdts_kv_redis_set_pend_hash_api,
    rwdts_kv_redis_set_pend_commit_hash_api,
    rwdts_kv_redis_set_pend_abort_hash_api,
    rwdts_kv_redis_del_pend_hash_api,
    rwdts_kv_redis_del_pend_commit_hash_api,
    rwdts_kv_redis_del_pend_abort_hash_api,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
  },
  {
    rwdts_kv_bkd_open_api,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    rwdts_kv_bkd_set_api,
    rwdts_kv_bkd_get_api,
    rwdts_kv_bkd_del_api,
    rwdts_kv_get_bkd_cursor,
    rwdts_kv_get_next_bkd_api,
    rwdts_kv_remove_bkd,
    rwdts_kv_close_bkd
  }
};

void *
rwdts_kv_allocate_handle(rwdts_avail_db_t db)
{
  rwdts_kv_handle_t *handle;
  handle =  (rwdts_kv_handle_t *)malloc(sizeof(rwdts_kv_handle_t));
  handle->db_type = db;
  return (void *)handle;
}

rw_status_t
rwdts_kv_light_db_connect(rwdts_kv_handle_t *handle,
                          rwsched_instance_ptr_t sched_instance,
                          rwsched_tasklet_ptr_t tasklet_info,
                          char *uri, void *callbkfn, void *callbkdata)
{
  char local_uri[256];
  strcpy(local_uri, uri);
  char *local_uri_ptr = &local_uri[0];
  char *ip_address = strsep(&local_uri_ptr, ":");
  int port = atoi(local_uri_ptr);

  if (rwdts_fn_table[handle->db_type].rwdts_db_init_api) {
    return rwdts_fn_table[handle->db_type].rwdts_db_init_api(handle, sched_instance,
                                                             tasklet_info, ip_address,
                                                             (uint16_t)port, callbkfn,
                                                             callbkdata);
  } else {
    return RW_STATUS_FAILURE;
  }
}

rw_status_t
rwdts_kv_light_open_db(rwdts_kv_handle_t *handle, const char *file_name,
                       const char *program_name, FILE *error_file_pointer)
{
  if (rwdts_fn_table[handle->db_type].rwdts_db_open_api) {
    return rwdts_fn_table[handle->db_type].rwdts_db_open_api(handle,
                                                             file_name,
                                                             program_name,
                                                             error_file_pointer);
  }
  return RW_STATUS_FAILURE;
}

void
rwdts_kv_light_set_keyval(rwdts_kv_handle_t *handle, int db_num, void *key,
                          int key_len, void *val, int val_len, void *callbkfn,
                          void *callbk_data)
{
  if (rwdts_fn_table[handle->db_type].rwdts_db_set_api) {
    rwdts_fn_table[handle->db_type].rwdts_db_set_api(handle->kv_conn_instance,
                                                     db_num, (char *)key, key_len,
                                                     (char *)val, val_len,
                                                     callbkfn, callbk_data);
  }
  return;
}

void
rwdts_kv_light_get_val_from_key(rwdts_kv_handle_t *handle, int db_num, void *key,
                                int key_len, void *callbkfn, void *callbk_data)
{

  if (rwdts_fn_table[handle->db_type].rwdts_db_get_api) {
    rwdts_fn_table[handle->db_type].rwdts_db_get_api(handle->kv_conn_instance,
                                                     db_num, (char *)key, key_len,
                                                     callbkfn, callbk_data);
  }
  return;
}

void
rwdts_kv_light_del_keyval(rwdts_kv_handle_t *handle, int db_num, void *key,
                          int key_len, void *callbkfn, void *callbk_data)
{

  if (rwdts_fn_table[handle->db_type].rwdts_db_del_api) {
    rwdts_fn_table[handle->db_type].rwdts_db_del_api(handle->kv_conn_instance,
                                                     db_num, (char *)key, key_len,
                                                     callbkfn, callbk_data);
  }
  return;
}

rwdts_kv_table_handle_t *rwdts_kv_light_register_table(rwdts_kv_handle_t *handle,
                                                       int db_num)
{
  rwdts_kv_table_handle_t *tab_handle;

  tab_handle = malloc(sizeof(rwdts_kv_table_handle_t));
  tab_handle->db_num = db_num;

  tab_handle->handle = handle;

  tab_handle->scan_block = NULL;

  return tab_handle;
}

void rwdts_kv_light_deregister_table(rwdts_kv_table_handle_t *tab_handle)
{
  if (tab_handle->scan_block) {
    free(tab_handle->scan_block);
  }
  free(tab_handle);
  return;
}

void rwdts_kv_light_table_insert(rwdts_kv_table_handle_t *tab_handle, int serial_num,
                                 int shard_num, void *key, int key_len, void *val,
                                 int val_len, void *callbkfn, void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_set_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_set_api(tab_handle->handle,
                                                                      tab_handle->db_num,
                                                                      shard_num, key,
                                                                      key_len, serial_num,
                                                                      val, val_len,
                                                                      callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_table_get_by_key(rwdts_kv_table_handle_t *tab_handle,
                                     void *key, int key_len, void *callbkfn,
                                     void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_get_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_get_api(tab_handle->handle->kv_conn_instance,
                                                                      tab_handle->db_num,
                                                                      key, key_len,
                                                                      callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_table_delete_key(rwdts_kv_table_handle_t *tab_handle, int serial_num,
                                     int shard_num, void *key, int key_len,
                                     void *callbkfn, void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_del_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_del_api(tab_handle->handle,
                                                                      tab_handle->db_num, shard_num,
                                                                      key, key_len, serial_num,
                                                                      callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_tab_field_exists(rwdts_kv_table_handle_t *tab_handle,
                                     void *key, int key_len, void *callbkfn,
                                     void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_exists_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_exists_api(tab_handle->handle->kv_conn_instance,
                                                                         tab_handle->db_num,
                                                                         key, key_len,
                                                                         callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_get_next_fields(rwdts_kv_table_handle_t *tab_handle,
                                    int shard_num, void *callbkfn,
                                    void *callbk_data, void *next_block)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_get_next_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_get_next_api(tab_handle,
                                                                      shard_num, callbkfn,
                                                                      callbk_data);
  }
  return;
}

void rwdts_kv_light_del_table(rwdts_kv_table_handle_t *tab_handle,
                              void *callbkfn, void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_table_del_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_table_del_api(tab_handle->handle->kv_conn_instance,
                                                                       tab_handle->db_num,
                                                                       callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_table_get_sernum_by_key(rwdts_kv_table_handle_t *tab_handle,
                                            void *key, int key_len, void *callbkfn,
                                            void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_get_sernum_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_get_sernum_api(tab_handle->handle->kv_conn_instance,
                                                                             tab_handle->db_num,
                                                                             key, key_len,
                                                                             callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_tab_get_all(rwdts_kv_table_handle_t *tab_handle, void *callbkfn,
                                void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_table_get_all_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_table_get_all_api(tab_handle->handle,
                                                                           tab_handle->db_num,
                                                                           callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_delete_shard_entries(rwdts_kv_table_handle_t *tab_handle,
                                         int shard_num, void *callbkfn,
                                         void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_shard_delete_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_shard_delete_api(tab_handle->handle,
                                                                          tab_handle->db_num,
                                                                          shard_num, callbkfn,
                                                                          callbk_data);
  }
  return;
}

void rwdts_kv_light_table_xact_insert(rwdts_kv_table_handle_t *tab_handle, int serial_num,
                                      int shard_num, void *key, int key_len, void *val,
                                      int val_len, void *callbkfn, void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_set_pend_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_set_pend_api(tab_handle->handle,
                                                                           tab_handle->db_num,
                                                                           shard_num, key,
                                                                           key_len, serial_num,
                                                                           val, val_len,
                                                                           callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_table_xact_delete(rwdts_kv_table_handle_t *tab_handle, int serial_num,
                                      void *key, int key_len, void *callbkfn, void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_del_pend_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_del_pend_api(tab_handle->handle,
                                                                           tab_handle->db_num,
                                                                           key, key_len,
                                                                           serial_num,
                                                                           callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_api_xact_insert_commit(rwdts_kv_table_handle_t *tab_handle, int serial_num,
                                           int shard_num, void *key, int key_len,
                                           void *callbkfn, void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_set_pend_commit_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_set_pend_commit_api(tab_handle->handle,
                                                                                  tab_handle->db_num,
                                                                                  shard_num,
                                                                                  key, key_len,
                                                                                  serial_num,
                                                                                  callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_table_xact_delete_commit(rwdts_kv_table_handle_t *tab_handle, int serial_num,
                                             void *key, int key_len, void *callbkfn, void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_del_pend_commit_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_del_pend_commit_api(tab_handle->handle,
                                                                                  tab_handle->db_num,
                                                                                  key, key_len,
                                                                                  serial_num,
                                                                                  callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_api_xact_insert_abort(rwdts_kv_table_handle_t *tab_handle, int serial_num,
                                          int shard_num, void *key, int key_len,
                                          void *callbkfn, void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_set_pend_abort_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_set_pend_abort_api(tab_handle->handle,
                                                                                 tab_handle->db_num,
                                                                                 shard_num,
                                                                                 key, key_len,
                                                                                 serial_num,
                                                                                 callbkfn, callbk_data);
  }
  return;
}

void rwdts_kv_light_table_xact_delete_abort(rwdts_kv_table_handle_t *tab_handle, int serial_num,
                                            void *key, int key_len, void *callbkfn, void *callbk_data)
{
  if (rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_del_pend_abort_api) {
    rwdts_fn_table[tab_handle->handle->db_type].rwdts_db_hash_del_pend_abort_api(tab_handle->handle,
                                                                                 tab_handle->db_num,
                                                                                 key, key_len,
                                                                                 serial_num,
                                                                                 callbkfn, callbk_data);
  }
  return;
}

rw_status_t
rwdts_kv_light_file_set_keyval(rwdts_kv_handle_t *handle,
                               char *key, int key_len, char *val, int val_len)
{
  rw_status_t rs = RW_STATUS_FAILURE;
  if (rwdts_fn_table[handle->db_type].rwdts_file_db_set_api) {
    rs = rwdts_fn_table[handle->db_type].rwdts_file_db_set_api(handle->kv_conn_instance,
                                                               handle->file_name,
                                                               key, key_len,
                                                               val, val_len);
    handle->kv_conn_instance = NULL;
  }
  return rs;
}

rw_status_t
rwdts_kv_light_file_get_keyval(rwdts_kv_handle_t *handle,
                               char *key, int key_len, char **val, int *val_len)
{
  rw_status_t rs = RW_STATUS_FAILURE;
  if (rwdts_fn_table[handle->db_type].rwdts_file_db_get_api) {
    rs = rwdts_fn_table[handle->db_type].rwdts_file_db_get_api(handle->kv_conn_instance,
                                                               handle->file_name,
                                                               key, key_len,
                                                               val, val_len);
    handle->kv_conn_instance = NULL;
  }
  return rs;
}

rw_status_t
rwdts_kv_light_file_del_keyval(rwdts_kv_handle_t *handle,
                               char *key, int key_len)
{
  rw_status_t rs = RW_STATUS_FAILURE;
  if (rwdts_fn_table[handle->db_type].rwdts_file_db_del_api) {
    rs = rwdts_fn_table[handle->db_type].rwdts_file_db_del_api(handle->kv_conn_instance,
                                                               handle->file_name,
                                                               key, key_len);
    handle->kv_conn_instance = NULL;
  }
  return rs;
}

void*
rwdts_kv_light_file_get_cursor(rwdts_kv_handle_t *handle)
{
  if (rwdts_fn_table[handle->db_type].rwdts_file_db_get_cursor) {
    return rwdts_fn_table[handle->db_type].rwdts_file_db_get_cursor(handle->kv_conn_instance, handle->file_name);
  }
  return NULL;
}

rw_status_t
rwdts_kv_light_file_getnext(rwdts_kv_handle_t *handle, void **dbc, uint8_t **key,
                            size_t *key_len, uint8_t **val, size_t *val_len)
{
  if (rwdts_fn_table[handle->db_type].rwdts_file_db_getnext) {
    return rwdts_fn_table[handle->db_type].rwdts_file_db_getnext(handle->kv_conn_instance,
                                                                 dbc, key, key_len,
                                                                 val, val_len);
  }
  return RW_STATUS_FAILURE;
}

rw_status_t
rwdts_kv_light_file_remove(rwdts_kv_handle_t *handle)
{
  rw_status_t res = RW_STATUS_FAILURE;
  if (rwdts_fn_table[handle->db_type].rwdts_file_db_remove) {
    res = rwdts_fn_table[handle->db_type].rwdts_file_db_remove(handle->kv_conn_instance,
                                                               handle->file_name);
    handle->kv_conn_instance = NULL;
    return res;
  }
  return res;
}

rw_status_t
rwdts_kv_light_file_close(rwdts_kv_handle_t *handle)
{
  rw_status_t res = RW_STATUS_FAILURE;
  if (rwdts_fn_table[handle->db_type].rwdts_file_db_close) {
    res = rwdts_fn_table[handle->db_type].rwdts_file_db_close(handle->kv_conn_instance);
    handle->kv_conn_instance = NULL;
  }
  return res;
}
