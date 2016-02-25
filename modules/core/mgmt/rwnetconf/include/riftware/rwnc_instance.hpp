
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_instance.hpp
 * @author Rajesh Velandy
 * @date 2014/05/30
 * @brief RW.Netconf client library instance definitions.
 */

#ifndef RWNC_INSTANCE_HPP_
#define RWNC_INSTANCE_HPP_

#include <rwnetconf.h>
#include <yangmodel.h>
#include <rw_xml.h>
#include <rw_cf_type_validate.hpp>

namespace rw_netconf {

class DataStore;

/**
 * @addtogroup RwNcInstance
 * @{
 */

/**
 * The client library instance object holds global data for the client
 * library, and possibly maintains references to objects that are not
 * otherwise owned.
 */
class Instance
{
  /* Use RW-CF memory tracking. */
  RW_CF_TYPE_CLASS_DECLARE_MEMBERS(rw_ncclnt_instance_t, rw_ncclnt_instance_ptr_t, Instance)

 public:
  /**
   * Create an instance of RW Netconf Client Library Instance - The
   * caller owns the returned instance
   *
   * @return "Shared" unique pointer to the instance.
   */
  static suptr_t create(
    /**
     * [in] Yang model.  The caller owns the model.  The library
     * assumes the model is going to live at least as long as the
     * library instance.
     */
    rw_yang::YangModel* model,

    /**
     * [in] RW.Trace instance.  The caller owns the trace instance.
     * The library assumes the trace instance is going to live at least
     * as long as the library instance.
     */
    rwtrace_ctx_t* trace_instance
  );

 protected:

  /**
   * Constructor.  @see create().
   */
  Instance(rw_yang::YangModel* model, rwtrace_ctx_t* trace_instance);

  /// Destructor.
  ~Instance();

  // Cannot copy
  Instance(const Instance&) = delete;
  Instance& operator=(const Instance&) = delete;

 public:

  /**
   * Get the current state of the instance.
   *
   * @return
   *  - RW_NCCLNT_STATE_INITIALIZED: instance is good
   *  - RW_NCCLNT_STATE_TERMINATED: instance has been terminated, you
   *    should stop using it
   */
  rw_ncclnt_instance_state_t get_state() const;

  /**
   * Get the yang model.
   * @return The yang model.  The owner of Instance owns the model.
   */
  rw_yang::YangModel* get_model() const;

  /**
   * Get the RW.Trace instance.
   * @return The trace instance.  The owner of Instance owns the trace
   *   instance.
   */
  rwtrace_ctx_t* get_trace_instance() const;

  /**
   * Get the allocator to use for objects in this instance.
   * @return The allocator.  The Instance owns the allocator; use
   *   retain to keep a reference.
   */
  CFAllocatorRef get_allocator() const;

  /**
   * Get a reference to the const candidate DataStore.
   * @return The DataStore.  Use retain to keep a reference.
   */
  const DataStore* get_candidate() const;

  /**
   * Get a reference to the const running DataStore.
   * @return The DataStore.  Use retain to keep a reference.
   */
  const DataStore* get_running() const;

  /**
   * Get a reference to the const startup DataStore.
   * @return The DataStore.  Use retain to keep a reference.
   */
  const DataStore* get_startup() const;

 private:

  /// YangModel reference for the Netconf client library
  rw_yang::YangModel *model_;
  // ATTN: the following variable can only hold a single instance of module 
  // Since the variable is owned by model and is easily accessible through 
  // the model, probably there is no reason to maintain this reference.
  /// YangModule reference for the Netconf Yang Module
  rw_yang::YangModule *ymod_;

  /// The state of this instance
  rw_ncclnt_instance_state_t state_;

  /// Allocator to be used with CF library
  /// ATTN: This is actually unused
  /// ATTN: Put this in a unique_ptr<>-like thing.  Can't be UniquishPtrRwMemoryTracking<>
  CFAllocatorRef allocator_;

  /// The candidate data store
  UniquishPtrRwMemoryTracking<DataStore>::suptr_t candidate_;

  /// The running data store
  UniquishPtrRwMemoryTracking<DataStore>::suptr_t running_;

  /// The startup datastore
  UniquishPtrRwMemoryTracking<DataStore>::suptr_t startup_;

  /// The logging/tracing instance handle
  rwtrace_ctx_t* rwtrace_;
};

RW_CF_TYPE_CLASS_DEFINE_INLINES(rw_ncclnt_instance_t, rw_ncclnt_instance_ptr_t, Instance)

/** @} */

} // namespace rw_netconf

RW_CF_TYPE_CLASS_DEFINE_TRAITS(rw_ncclnt_instance_t, rw_ncclnt_instance_ptr_t, rw_netconf::Instance)

#endif // RWNC_INSTANCE_HPP_
