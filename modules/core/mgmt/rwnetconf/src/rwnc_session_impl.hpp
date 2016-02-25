
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_session_impl.hpp
 * @author Tom Seidenberg
 * @date 2014/06/18
 * @brief RW.Netconf client library session implementation
 */

#ifndef RWNC_SESSION_IMPL_HPP_
#define RWNC_SESSION_IMPL_HPP_

#include <rwnc_session.hpp>
#include <rwnc_transport.hpp>

namespace rw_netconf {

/**
 * @addtogroup RwNcSession
 * @{
 */

/// Concrete Session object.
/**
 * Concrete rw_netconf::Session.  All sessions should end up using
 * this.
 */
class SessionImpl
: public Session
{
 public:
  /// Create a session.
  static Session::suptr_t create(
    /**
     * [in] The owning client library instance.  Will retain a
     * reference.
     */
    Instance* instance,

    /**
     * [in] The associated transport layer API that this session uses
     * to communicate with the server.  The application retains
     * ownership.
     */
    Transport* transport,

    /**
     * [in] The user who owns the session.  The session will retain a
     * reference.  Cannot be nullptr.
     */
    User* user
  );

 public:
  // Implement all base methods

  Instance* get_instance() const override;

  User* get_user() const override;

  rw_status_t req_nc_get_config(
    const DataStore* config_source,
    Filter* filter,
    ClientCallback callback,
    CallbackManager* cbmgr
  ) override;

  rw_status_t req_nc_edit_config(
    const DataStore* config_target,
    void* default_op,
    void* test_opt,
    void* error_opt,
    Xml* config,
    ClientCallback callback,
    CallbackManager* cbmgr
  ) override;

 public:

  /**
   * Get the transport associated with the session.  This is not part
   * of the Session public API because Transport is internal to the
   * client library.
   *
   * Use cases:
   * - Debug
   *
   * @return The transport.  Not owned by the caller.
   */
  Transport* get_transport() const;

 protected:

  /// Constructor
  SessionImpl(
    /**
     * [in] The owning client library instance.  Will retain a
     * reference.
     */
    Instance* instance,

    /**
     * [in] The associated transport layer API that this session uses
     * to communicate with the server.  The application retains
     * ownership.
     * ATTN: If application retains ownership, then we end up with
     * frequently varying ownership semantics.  That's crazy.  Maybe
     * Transports should also use CF ownership semantics, even though
     * that creates a maintenance burden...
     */
    Transport* transport,

    /**
     * [in] The user who owns the session.  The session will retain a
     * reference.  Cannot be nullptr.
     */
    User* user
  );

  /// Destructor
  ~SessionImpl();

  // Cannot copy.
  SessionImpl(const SessionImpl&) = delete;
  SessionImpl& operator=(const SessionImpl&) = delete;

 private:

  /**
   * Handle response callback for get-config request from transport
   * layer.  Will occur in application's concurrency context.
   */
  void rsp_cb_nc_get_config(
    /**
     * [in] The transport context for the request.  This function takes
     * ownership.
     */
    TransportContext* tc,

    /**
     * [in] The XML blob for the response.  May be nullptr if there was
     * an error in the transport layer that resulted in a no valid
     * response being recieved.  The entire XMl blob may not have been
     * recieved from the server yet.
     */
    Xml* xml
  );

  /**
   * Handle response callback for edit-config request from transport
   * layer.  Will occur in application's concurrency context.
   */
  void rsp_cb_nc_edit_config(
    /**
     * [in] The transport context for the request.  This function takes
     * ownership.
     */
    TransportContext* tc,

    /**
     * [in] The XML blob for the response.  May be nullptr if there was
     * an error in the transport layer that resulted in a no valid
     * response being recieved.  The entire XMl blob may not have been
     * recieved from the server yet.
     */
    Xml* xml
  );

 private:

  /// The owning client library instance instance.
  UniquishPtrRwMemoryTracking<Instance>::suptr_t instance_;

  /// The transport associated with the session.  Owned.
  /// ATTN: If Transports can be shared, this must be shared_ptr, CF, or equivalent.
  /// ATTN: If Transports cannot be shared, then this should be a unique_ptr.
  Transport* transport_;

  /// The transport associated with the session.  Owned.
  /// ATTN: Use suptr_t
  User* user_;
};

/** @} */

} // namespace rw_netconf

#endif /* RWNC_SESSION_IMPL_HPP_*/
