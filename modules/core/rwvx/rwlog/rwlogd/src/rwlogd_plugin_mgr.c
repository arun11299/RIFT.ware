
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 *
 */

#include "rwlogd_plugin_mgr.h"
#include <dlfcn.h>
typedef rw_status_t (*startup_func_p)();
rw_status_t rwlogd_create_sink_dynamic(const char *library_name, 
                                       rwlogd_instance_ptr_t inst, 
                                       const char *name, 
                                       const char *host,const  int port)
{
  char *shared_library_name = NULL;
  int r = asprintf(&shared_library_name,
                   "lib%s.so", 
                   library_name);
  RW_ASSERT(r);
  //printf("Creating Sink %s with name %s\n", shared_library_name, name);
  void *handle = dlopen (shared_library_name, RTLD_NOW);
  startup_func_p start_funct = NULL;
  if(!handle)
  {
    fprintf(stderr, "Failed to load shared library %s\n", shared_library_name);
    RWLOG_ASSERT(handle);
  }
  char *start_function_name = NULL;
  r = asprintf(&start_function_name,
                   "start_%s",
                   library_name);
  RW_ASSERT(r);
  start_funct = (startup_func_p)dlsym(handle, start_function_name);
  if(!start_funct)
  {
    fprintf(stderr, "Failed to get start function %s\n", start_function_name);
    RWLOG_ASSERT(start_funct);
    return RW_STATUS_FAILURE;
  }
  free(start_function_name);
  free(shared_library_name);
  return start_funct(inst, name, host, port);
}

rw_status_t rwlogd_delete_sink_dynamic(const char *library_name,
                                       rwlogd_instance_ptr_t inst, 
                                       const char *name, 
                                       const char *host,const  int port)
{
  char *shared_library_name =NULL;
  int r = asprintf(&shared_library_name,
                   "lib%s.so",
                   library_name);
  RW_ASSERT(r);
  //printf("Deleting Sink %s with name %s\n", shared_library_name, name);
  void *handle = dlopen (shared_library_name, RTLD_NOW);
  startup_func_p stop_funct = NULL;
  if(!handle)
  {
    fprintf(stderr, "Failed to load shared library %s \n", shared_library_name);
    RWLOG_ASSERT(handle);
  }
  char *stop_function_name = NULL;
  r = asprintf(&stop_function_name,
                   "stop_%s",
                   library_name);
  RW_ASSERT(r);
  stop_funct = (startup_func_p)dlsym(handle, stop_function_name);
  if(!stop_funct)
  {
    fprintf(stderr, "Failed to get stop function %s \n", stop_function_name);
    RWLOG_ASSERT(stop_funct);
    return RW_STATUS_FAILURE;
  }
  free(stop_function_name);
  free(shared_library_name);
  return stop_funct(inst, name, host, port);
}
