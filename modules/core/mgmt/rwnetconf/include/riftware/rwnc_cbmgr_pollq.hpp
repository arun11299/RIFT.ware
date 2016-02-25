
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_cbmgr_pollq.hpp
 * @author Tom Seidenberg
 * @date 2014/06/26
 * @brief RW.Netconf client library polled-queue callback manager definitions.
 */

#ifndef RWNC_CBMGR_POLLQ_HPP_
#define RWNC_CBMGR_POLLQ_HPP_

#include <rwnc_cbmgr.hpp>
#include <list>

namespace rw_netconf {

/**
 * @addtogroup RwNcCbMgr
 * @{
 */

/// Polled-queue Callback Manager
/**
   Polled-queue callback manager.  This callback manager queues all
   callbacks internally.  The application (typically a unit test)
   invokes the callbacks by polling.
 */
class CallbackManagerPollQ
: public CallbackManager
{
 public:

  /// The list of requests.
  typedef CbMgrCallback cb_t;
  typedef std::list<cb_t> cb_list_t;

 public:
  /**
   * Create a polled-queue callback manager.  The callback manager will
   * queue all library callbacks into an internal queue.  The internal
   * queue will NOT be thread-safe.  The callbacks to the application
   * will be made only under application control - the application must
   * call a polling function, which will invoke the oldest queue
   * callback, if any.
   *
   * @see rw_ncclnt_cbmgr_create_polled_queue()
   *
   * Use cases:
   * - Unit testing, without any RW.Sched support
   *
   * Anti-use cases:
   * - Unit tests running with RW.Sched support.
   * - RW.Cli
   * - RW.GUIServer
   *
   * @return The callback manager.  The caller already owns a
   *   reference.
   */
  static suptr_t create(
    Instance* instance ///< [in] The owning API instance
  );

 public:
  // Implement base methods

  void queue_callback(CbMgrCallback&& callback) override;

  Instance* get_instance() const override;

  rw_status_t poll_once() override;

  // Use default poll_all().

 protected:
  /// Constructor
  CallbackManagerPollQ(
    /**
     * [in] The owning client library instance.  Will retain a
     * reference.
     */
    Instance* instance
  );

  /// Destructor
  ~CallbackManagerPollQ();

  // Cannot copy.
  CallbackManagerPollQ(const CallbackManagerPollQ&) = delete;
  CallbackManagerPollQ& operator=(const CallbackManagerPollQ&) = delete;

 private:

  /// The owning client library instance.
  UniquishPtrRwMemoryTracking<Instance>::suptr_t instance_;

  /// The queue of queued callbacks.
  cb_list_t cb_list_;
};

/** @} */

} // namespace rw_netconf

#endif /* RWNC_CBMGR_POLLQ_HPP_*/
