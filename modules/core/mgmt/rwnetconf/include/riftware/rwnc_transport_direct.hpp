
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_transport_direct.hpp
 * @author Tom Seidenberg
 * @date 2014/06/19
 * @brief RW.Netconf Direct function call transport definitions
 */

#ifndef RWNC_TRANSPORT_DIRECT_HPP_
#define RWNC_TRANSPORT_DIRECT_HPP_

#include <rwnc_transport.hpp>

// ATTN: other C++ includes

namespace rw_netconf {

class Session;
class Instance;
class User;

/*****************************************************************************/
/**
 * @addtogroup RwNcTransport
 * @{
 */

/// Direct function call transport
/**
 * Direct function call transport implementation, which simply
 * dispatches to another Transport API implementation.  In theory, this
 * could allow chaining of multiple transports, but the only use case
 * of this class will be unit test, with a mock providing the server
 * side.
 *
 * ATTN: Does the connected-to server need to be a TransportDirect or a
 * Session?
 *
 * ATTN: Should we support both TransportDirect and Session as targets?
 */
class TransportDirect
: public Transport
{
 public:
  /// Constructor
  TransportDirect(
    /**
     * [in] The owning client library instance.
     */
    Instance* instance,

    /**
     * [in] The transport layer API that this transport connects to.
     */
    Transport* transport
  );

  /// Destructor
  ~TransportDirect();

  // Cannot copy.
  TransportDirect(const TransportDirect&) = delete;
  TransportDirect& operator=(const TransportDirect&) = delete;

 public:
  /**
   * Establish a pass-through TransportDirect instance to another local
   * Transport API instance, creating a Session for the new
   * TransportDirect and returning the Session.  This function creates
   * a new Transport object that simply forwards requests to the other
   * Transport.  The calls are made directly into the other Transport,
   * within the same execution context as the calls made to the new
   * Transport.
   *
   * Transports are not first class objects in the client library API,
   * so this function creates the TransportDirct and the Session it
   * will be bound to.  Client applications can only use the Session
   * directly.
   *
   * ATTN: Direct transport is (currently) C++ only, because it is only
   * needed by other C++ code (e.g., google test, and the transport
   * layer isn't part of the client library's public API.
   *
   * Use cases:
   * - RW.MgmtAgent when binding any RW.NCTransport to the agent proper.
   * - Unit tests.
   *
   * @return A Session object upon success.  If creation fails, an
   *   exception will be thrown.
   */
  static Session* create_session(
    /**
     * [in] The owning client library instance.  The created Session
     * will retain a reference to the instance.  Cannot be nullptr.
     */
    Instance* instance,

    /**
     * [in] The transport layer API that the direct-transport connects
     * to.  The new Session will have a new direct function call
     * Transport connected to this transport.  Cannot be nullptr.  The
     * application retains ownership.
     * ATTN: If application retains ownership, then we end up with
     * frequently varying ownership semantics.  That's crazy.  Maybe
     * Transports should also use CF ownership semantics, even though
     * that creates a maintenance burden...
     */
    Transport* transport,

    /**
     * [in] The user who owns the session.  The session will retain a
     * reference.  Cannot be nullptr.  If nullptr, use "current user"?
     */
    User* user
  );

 public:
  // Implement all base methods

  Instance* get_instance() const override;

  rw_status_t xmit_nc_get_config(
    const DataStore* config_source,
    Filter* filter,
    TransportContext::uptr_t context
  ) override;

  rw_status_t xmit_nc_edit_config(
    const DataStore* config_target,
    void* default_op,
    void* test_opt,
    void* error_opt,
    Xml* config,
    TransportContext::uptr_t context
  ) override;

 public:
  // Methods for just this class

  /**
   * Get the forward-to Transport.
   *
   * Use cases:
   * - Debug
   *
   * @return The forward-to Transport.
   */
  Transport* get_forward_to() const;

 private:

  /// The owning instance.  Retains a reference.
  UniquishPtrRwMemoryTracking<Instance>::suptr_t instance_;

  /// The transport API to forward to.  Retains a reference.
  Transport* forward_to_;
};


/** @} */

} /* namespace rw_netconf */

#endif /* RWNC_TRANSPORT_DIRECT_HPP_ */
