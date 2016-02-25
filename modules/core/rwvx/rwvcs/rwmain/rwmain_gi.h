
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 *
 */

#ifndef __rwmain_gi_h__
#define __rwmain_gi_h__

#include <rw_sklist.h>
#include <rwvcs_defs.h>
#include <rwvx.h>
#include <rwtasklet.h>

__BEGIN_DECLS

#ifndef __GI_SCANNER__
struct rwmain_gi {
  int _refcnt;
  struct rwtasklet_info_s * tasklet_info;
  struct rwvx_instance_s * rwvx;
  rw_sklist_t tasklets;
};
#endif
typedef struct rwmain_gi rwmain_gi_t;

GType rwmain_gi_get_type(void);

///@cond GI_SCANNER
/**
 * rwmain_gi_new:
 * @manifest_box: (type RwManifestYang.Manifest) (transfer none)
 */
///@endcond
rwmain_gi_t * rwmain_gi_new(rwpb_gi_RwManifest_Manifest * manifest_box);

///@cond GI_SCANNER
/*
 * rwmain_gi_add_tasklet:
 * @rwmain_gi:
 * @plugin_dir:
 * @plugin_name:
 * returns: (type RwTypes.RwStatus) (transfer none)
 */
///@endcond
rw_status_t rwmain_gi_add_tasklet(
  rwmain_gi_t * rwmain_gi,
  const char * plugin_dir,
  const char * plugin_name);

///@cond GI_SCANNER
/**
 * rwmain_gi_get_tasklet:
 * returns: (type RwTasklet.Info) (transfer none)
 */
///@endcond
rwtasklet_info_t * rwmain_gi_get_tasklet_info(rwmain_gi_t * rwmain_gi);

///@cond GI_SCANNER
/**
 * rwmain_gi_new_tasklet:
 * returns: (type RwTasklet.Info) (transfer none)
 */
///@endcond
rwtasklet_info_t * rwmain_gi_new_tasklet_info(
    rwmain_gi_t * rwmain_gi,
    const char * name,
    uint32_t id);

__END_DECLS
#endif
