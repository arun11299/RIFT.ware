/*
/ (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
/*/
/*!
 * @file rwdts_kv_bkd_interface.h
 * @brief Top level RWDTS KV LIGHT API header
 * @author Prashanth Bhaskar(Prashanth.Bhaskar@riftio.com)
 * @date 2014/10/02
 */
#ifndef __RWDTS_KV_BKD_INTERFACE_H
#define __RWDTS_KV_BKD_INTERFACE_H

#include "rwdts_kv_light_common.h"
#include "rwdts_redis.h"

__BEGIN_DECLS

rw_status_t rwdts_kv_bkd_open_api(rwdts_kv_handle_t *handle,
                                  const char *file_name,
                                  const char *program_name,
                                  FILE *error_file_pointer);

rw_status_t
rwdts_kv_bkd_set_api(void *instance, char *file_name, char *key, int key_len,
                     char *val, int val_len);

rw_status_t
rwdts_kv_bkd_get_api(void *instance, char *file_name, char *key, int key_len,
                     char **val, int *val_len);

rw_status_t
rwdts_kv_bkd_del_api(void *instance, char *file_name, char *key, int key_len);

void *
rwdts_kv_get_bkd_cursor(void *instance, char *file_name);

rw_status_t
rwdts_kv_get_next_bkd_api(void *instance, void **dbc, uint8_t **key, size_t *key_len,
                          uint8_t **val, size_t *val_len);

rw_status_t
rwdts_kv_remove_bkd(void *instance, const char *file_name);

rw_status_t
rwdts_kv_close_bkd(void *instance);

__END_DECLS

#endif /*__RWDTS_KV_BKD_INTERFACE_H */
