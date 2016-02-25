
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 *
 */

#ifndef __RWVCS_MANIFEST_H__
#define __RWVCS_MANIFEST_H__

#include "rwvcs.h"

__BEGIN_DECLS

/*
 * Load the specified manifest.
 *
 * @param rwvcs         - rwvcs instace
 * @param manifest_path - path to the manifest file
 * @return              - rw_status
 */
rw_status_t rwvcs_manifest_load(
    rwvcs_instance_ptr_t rwvcs,
    const char * manifest_path);

/*
 * Lookup a component definition in the manifest.
 *
 * @param rwvcs     - rwvcs instance
 * @param name      - name of the component
 * @param component - on success a pointer to the component definition.  This pointer
 *                    is owned by rwvcs.
 * @return          - RW_STATUS_SUCCESS
 *                    RW_STATUS_NOTFOUND if the name cannot be found
 */
rw_status_t rwvcs_manifest_component_lookup(
    rwvcs_instance_ptr_t rwvcs,
    const char * name,
    vcs_manifest_component ** m_component);

/*
 * Lookup an event in the specified event list.
 *
 * @param event_name  - name of the event to find
 * @param list        - event list to search
 * @param event       - on success a pointer to the event definition.  This pointer
 *                      is owned by by the event list.
 * @return            - RW_STATUS_SUCCESS
 *                      RW_STATUS_NOTFOUND if the name cannot be found
 */
rw_status_t rwvcs_manifest_event_lookup(
    char * event_name,
    vcs_manifest_event_list * list,
    vcs_manifest_event ** event);

/*
 * Check if a given component exists in the manifest.
 *
 * @param rwvcs - rwvcs instance
 * @param name  - name of the component to lookup.
 * @return      - true if the manifest contains that component, false otherwise.
 */
bool rwvcs_manifest_have_component(rwvcs_instance_ptr_t rwvcs, const char * name);


__END_DECLS
#endif
