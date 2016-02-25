
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_cbmgr.hpp
 * @author Tom Seidenberg
 * @date 2014/06/25
 * @brief RW.Netconf client library callback manager definitions.
 */

#ifndef RWNC_CBMGR_HPP_
#define RWNC_CBMGR_HPP_

#include <rwnetconf.h>
#include <functional>
#include <rw_cf_type_validate.hpp>

namespace rw_netconf {

// Forward declarations
class DataStore;
class Instance;
class CallbackManager;
class Transport;
struct TransportContext;
class User;
class Xml;


/**
 * @addtogroup RwNcCbMgr
 * @{
 */

/**
 * Callback Manager's callable type.  This function type defines the
 * signature of the callbacks made by the Callback Manager.  All
 * callbacks, by the time they are queued into the manager, must by
 * wrapped in this type.
 */
typedef std::function<void()> CbMgrCallback;


/// Callback Manager object.
/**
 * A callback manager object defines the interactions with the
 * application's concurrency model, allowing the client library to use
 * a single callback interface regardless of the application's callback
 * conventions.  Typically, exactly one callback manager exists per
 * application concurrency context, and the application specifies which
 * callback manager to use on every potentially-asynchronous API.
 *
 * The callback manager object is responsible for accepting callbacks
 * from the library and invoking them in or scheduling them with
 * appropriate application context.
 *
 * Abstract base class.
 *
 * Some notes about various callback interactions:
 *  - The C API takes a function pointer and context.
 *     - The C arguments are not suitable for CallbackManager, because
 *       they are in the form of a function pointer and context structure.
 *     - The C adapter API creates a C++ callable object for passing to
 *       the C++ client API.  This callable is also not suitable for
 *       Callback Manager, unless there are no return values from the
 *       operation itself passed into the callback.  The library must
 *       further bind this callable, probably after handling the
 *       (asynchronous) response.
 *  - The C++ Client API takes a callable object for the application,
 *    based on std::function<>, whose parameters are based on the return
 *    values from the operation.
 *     - For netconf requests, the Client API creates a TransportContext
 *       for passing to the transport layer.
 *        - ATTN: Who allocates this?
 *        - ATTN: Who owns this?
 *     - The application's callable gets moved into the
 *       TransportContext, which takes owenership of it.  As noted
 *       above, this callable is probably not suitable for Callback
 *       Manager.
 *     - Most (if not all) transport responses need library processing
 *       before making the application's callback.  Therefore, the
 *       client API creates its own callable for the transport layer
 *       response.  This callable probably will be queued to the
 *       CallbackManager, but only after being rebound to capture the
 *       transport layer return values.
 *     - The application's callable gets moved into the
 *       TransportContext, which takes owenership of it.
 *  - The C++ Transport API takes a callable object to make the
 *    client-layer callback.
 *     - This is the object that is ultimately scheduled through the
 *       CallbackManager, upon request completion.
 *     - The application's callback is probably not rebound any further,
 *       because the transport layer response is executed in the
 *       application's concurrency context.  The client API's callback
 *       will (probably) invoke the application's callback directly.
 *       If it cannot, then it might have to rebind it for queuing to
 *       the CallbackManager.
 *     - ATTN: How does ::terminate affect ownership of the callback's
 *       resources?
 *     - The TransportContext must take a reference to the
 *       CallbackManager, which is released when the callback is queued
 *       or if thge the TransportContext is destroyed without queuing.
 *     - If the request cannot be completed immediately, the transport
 *       layer must take ownership of the TransportContext.
 *     - Regardless, the transport layer is responsible for destroying
 *       the TransportContext.
 *
 * ATTN: Need a CF-release shared pointer?  CfReleasePtr<class>.  Could
 * base it on unique_ptr, with each new reference created getting its
 * own move-only unique_ptr
 */
class CallbackManager
{
 protected:
  /// Trivial constructor
  CallbackManager()
  {}

  /// Trivial destructor
  virtual ~CallbackManager()
  {}

  // Cannot copy.
  CallbackManager(const CallbackManager&) = delete;
  CallbackManager& operator=(const CallbackManager&) = delete;

  RW_CF_TYPE_CLASS_DECLARE_MEMBERS(rw_ncclnt_cbmgr_t, rw_ncclnt_cbmgr_ptr_t, CallbackManager)

 public:
  // Library APIs

  /**
   * Queue a callback via the manager.
   *
   * Use cases:
   * - Internal library usage.  All callbacks made by the library must
   *   use this interface.
   */
  virtual void queue_callback(
    /**
     * [in] The callback to queue.  The callback manager takes
     * ownership of the resources in the callback.  Supports move
     * semantics.
     */
    CbMgrCallback&& callback
  ) = 0;

 public:
  // Client APIs

  /**
   * Get the API instance reference.
   *
   * Use cases:
   * - Debug
   *
   * @return The API instance.  Not owned by the caller; use retain, if
   *   the reference will be kept.
   */
  virtual Instance* get_instance() const = 0;

  // ATTN: Terminate?

  /**
   * Invoke one callback.  This API may or may not have any effect on
   * the callback manager, depending on the underlying implementation,
   * as indicated by the return value.  If this API is supported, then
   * a single callback will be invoked (if one is available), in the
   * calling concurrency context.
   *
   * It is the application's responsibility to ensure that the current
   * context is acceptible for any callback that might be queued to the
   * callback manager.  If there is doubt, then the application likely
   * requires a callback manager per applicable context, the
   * application should avoid using this function, or the application
   * should use a different callback manager implementation.
   *
   * \sa rw_ncclnt_cbmgr_poll_once()
   *
   * @return
   *  - RW_STATUS_SUCCESS: One callback was made.
   *  - RW_STATUS_FAILURE: Not supported.
   *  - RW_STATUS_NOTFOUND: Supported, but no callbacks are queued.
   */
  virtual rw_status_t poll_once();

  /**
   * Invoke all callbacks.  This API may or may not have any effect on
   * the callback manager, depending on the underlying implementation,
   * as indicated by the return value.  If this API is supported, then
   * all available callbacks will be invoked, in the calling
   * concurrency context, in the order they were queued by the library.
   * This function might never return, if each callback causes another
   * callback to be generated.  If this scenario is possible for the
   * application, then the application should use
   * rw_ncclnt_cbmgr_poll_once() and implement its own throttling.
   *
   * See the concurrency caveats described in poll_once().
   *
   * \sa rw_ncclnt_cbmgr_poll_all()
   *
   * @return
   *  - RW_STATUS_SUCCESS: All callbacks were made.
   *  - RW_STATUS_FAILURE: Not supported.
   */
  virtual rw_status_t poll_all();
};

RW_CF_TYPE_CLASS_DEFINE_INLINES(rw_ncclnt_cbmgr_t, rw_ncclnt_cbmgr_ptr_t, CallbackManager)


/** @} */

} // namespace rw_netconf

RW_CF_TYPE_CLASS_DEFINE_TRAITS(rw_ncclnt_cbmgr_t, rw_ncclnt_cbmgr_ptr_t, rw_netconf::CallbackManager)

#endif /* RWNC_CBMGR_HPP_ */
