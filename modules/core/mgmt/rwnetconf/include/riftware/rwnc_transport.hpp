
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_transport.hpp
 * @author Tom Seidenberg
 * @date 2014/06/18
 * @brief RW.Netconf client library Transport API definitions
 */

#ifndef RWNC_TRANSPORT_HPP_
#define RWNC_TRANSPORT_HPP_

#include <rwnetconf.h>
#include <rwnc_session.hpp>

#include <functional>

namespace rw_netconf {

// Forward declarations
class Transport;
class TransportDirect;
struct TransportContext;


/*****************************************************************************/
/**
 * @page RwNcXapi RW.Netconf Transport layer API
 *
 * The transport layer API defines internal APIs used by the RW.Netconf
 * client library to abstract the different transport layers.  By
 * defining the transport layer as an abstraction, the same library can
 * be used in several different applications, making use of different
 * transport requirements.  The transport APIs basically map directly
 * to netconf operations.
 *
 * At the server-side of a transport implementation, the API may be
 * either the RW.Netconf client API or the RW.Netconf Transport Layer
 * API (as determined by the developer).  While this allows multiple
 * transports to be plugged into each other, possibly presenting some
 * interesting scenarios, the real motivation for this is that it
 * allows the client library to run directly on top of a server
 * implementation or mock.  This should accelerate developement both by
 * allowing testing of transports in isolation and also against the
 * same mocks.
 *
 * The transport API is actually private to the client library
 * implementation.  Clients should not use the transport API directly;
 * only the session layer (client library API) should make direct calls
 * into the transport layer.
 *
 * Transports implementing the RW.NCTransport API:
 *  - NC/SSH: Real netconf protocol over SSH
 *  - RW.Msg: Netconf syntax over internal RW.Msg transport
 *  - RW.Sched: Local queuing between threads in one process
 *  - Direct: Direct function call (no transport)
 *
 * Other Implementors of the RW.NCTransport API:
 *  - RW.MgmtAgent implements the API for real
 *  - The unittest framework implements a mock
 *
 * Responsibilities of the transport layer:
 *  - The transport layer is an XML consumer on the request side.  The
 *    session layer merely passes the XML blobs through to the
 *    transport layer.
 *  - The transport layer is an XML producer on the response side.  The
 *    session layer consumes the response XML in order to validate the
 *    response, building a DOM in the process.  The session layer then
 *    (conditionally) forwards (a portion of) the DOM, as an XML blob,
 *    on to the client.
 *  - The transport layer is not responsible for validating the
 *    response - that is the responsibility of the session layer.
 *  - The transport layer is responsible for encoding to and decoding
 *    from the transport layer.  For example, the NC/SSH transport must
 *    encode into and decode from the netconf chunking protocol.
 *  - The transport layer is responsible for multiplexing multiple
 *    sessions, if allowed and where appropriate.
 *
 * ATTN: Allow N:1 Session:Transport?  The use case is RW.Msg
 * transport, where multiple users could use a single transport
 * instance.  (However, we may want to spread the load across multiple
 * agents).
 *
 * ATTN: Need to include links to the implementations...
 *
 * (ATTN: How does the client ultimately indicate that the response DOM
 * can be deleted - how does that make it back into the library?)
 *
 * ATTN: Allow the client to specify an XML producer?  The transport then
 * uses the producer to produce the response?
 */


/*****************************************************************************/
/**
 * @defgroup RwNcTransport RW.Netconf Transport APIs
 * @{
 */

/// Message ID value.
typedef uint32_t rw_ncxapi_message_id_t;


/**
 * Client library Session request callback type.  This function type
 * defines the callback made by the transport layer to the client
 * library API layer upon receipt of a request's response.
 *
 * Defines a callback object with the following call signature:
 *     void (*)(TransportContext::uptr_t, Xml*)
 *
 * ATTN: Xml needs to be in a unique_ptr-like thing.
 *
 * ATTN: TGS: I had wanted to pass the context by unique_ptr<>, to
 * ensure ownership was properly transferred, BUT std::function<> ONLY
 * SUPPORTS copyable types.  We need to move away from
 * unique_ptr<>-based reference management and into our own class, that
 * will allow ownership transfers through std::function.
 */
typedef std::function<void(TransportContext*, Xml*)> SessionCallback;


/**
 * Transport API context.  These contexts are constructed by the
 * session layer and passed to the transport layer.  The transport
 * layer needs some of these fields to function.  Some of the fields
 * just being passed through for response handling, back to the client
 * library API layer or the all the way to the client.
 *
 * ATTN: shouldn't the client layer be entirely responsible for its own
 * data?  Why pollute the transport layer with client data?  If the
 * concern is memory allocation minimization, then the client layer
 * could hold the transport context in the client layer's structure.
 *
 * ATTN: some kind of user's preferred xml output?
 */
struct TransportContext
{
  typedef std::unique_ptr<TransportContext> uptr_t;

  /// Constructor.
  TransportContext(
    Instance* in_instance,
    Session* in_session,
    ClientCallback in_client_cb,
    CallbackManager* in_cbmgr,
    SessionCallback in_session_cb);

  /// Destructor.
  ~TransportContext();
  
  /**
   * The client library instance.
   */
  UniquishPtrRwMemoryTracking<Instance>::suptr_t instance;

  /**
   * The client session instance.  The request was issued using this
   * session, and the response will come back to this session.
   */
  UniquishPtrRwMemoryTracking<Session>::suptr_t session;

  /**
   * The message ID assigned to this request.  Will be assigned by the
   * transport layer; original value, if any, will be lost.
   */
  rw_ncxapi_message_id_t message_id;

  /**
   * The status of the request, as determined by the transport layer.
   * Will be initialized to RW_YANG_NETCONF_OP_STATUS_OK by the
   * transport layer prior to doing any work.  Will be reset by the
   * transport layer upon detection of any transport error that result
   * in no (or an incomplete) response from the server.
   */
  rw_yang_netconf_op_status_t xapi_status;

  /**
   * The yang node that defines the client API request.  The transport
   * layer (probably) does not make use of this field; it is included
   * in the context for use by session_cb(), so that it can
   * validate/handle the response.
   */
  rw_yang_node_t* ynode;

  /**
   * The callback for the response, from the client library back to the
   * client.  This is the user's callback.  Not used by the transport
   * layer; it is included in the context so that session_cb() can
   * invoke it.
   */
  ClientCallback client_cb;

  /**
   * The callback manager for invoking the session callback.
   *
   * Note: Do not release this pointer as soon as the transport layer
   * callback to the client layer has been queued.  The client layer
   * may need to perform more asynchronous work before making the
   * user's callback.
   */
  UniquishPtrRwMemoryTracking<CallbackManager>::suptr_t cbmgr;

  /**
   * The callback for the response, from the transport layer back to
   * the client library session layer.  This is not the user's
   * callback; this is used internally by the client library.
   * Presumably, this callback performs validation of complete
   * responses before makinfg the client callback.
   */
  SessionCallback session_cb;
};


/**
 * RW.Netconf Transport base class.  Defines the operations that
 * transports must implement.
 */
class Transport
{
 public:
  /// Trivial constructor
  Transport()
  {}

  /// Trivial destructor
  virtual ~Transport()
  {}

  // Cannot copy.
  Transport(const Transport&) = delete;
  Transport& operator=(const Transport&) = delete;

 public:

  /**
   * Get the API instance.
   *
   * Use cases:
   * - Debug
   *
   * @return The API instance.  Not owned by the caller; use retain, if
   *   the reference will be kept.
   */
  virtual Instance* get_instance() const = 0;

  // ATTN: Disconnect?

  // ATTN: Terminate?


  /**
   * Send a netconf get-config request over the transport layer.
   *
   * @see Session::req_nc_get_config()
   *
   * @return
   * - RW_STATUS_SUCCESS - The request was enqueued.
   * - Others: request could not be enqueued; no callback will be made
   *   and no request was sent.
   */
  virtual rw_status_t xmit_nc_get_config(
    /// [in] The configuration source.
    const DataStore* config_source,

    /**
     * [in] The filter description.  May be retained.
     */
    Filter* filter,

    /**
     * [in] The transport context.  Takes ownership.
     */
    TransportContext::uptr_t context
  ) = 0;

  /**
   * Send a netconf get-config request over the transport layer.
   *
   * @see Session::req_nc_get_config()
   *
   * @return
   * - RW_STATUS_SUCCESS - The request was enqueued.
   * - Others: request could not be enqueued; no callback will be made
   *   and no request was sent.
   */
  virtual rw_status_t xmit_nc_edit_config(
    /// [in] The target configuration datastore.
    const DataStore* config_target,

    /**
     * [in] The default-operation.
     * ATTN: Currently unsupported.  Complete the design.  Switch to
     * C++ class name when ready.
     */
    void* default_op,

    /**
     * [in] The test-option.
     * ATTN: Currently unsupported.  Complete the design.  Switch to
     * C++ class name when ready.
     */
    void* test_opt,

    /**
     * [in] The error-option.
     * ATTN: Currently unsupported.  Complete the design.  Switch to
     * C++ class name when ready.
     */
    void* error_opt,

    /// [in] The configuration changes to be made.
    Xml* config,

    /// [in] The transport context.  Takes ownership.
    TransportContext::uptr_t context
  ) = 0;
};


/** @} */

} /* namespace rw_netconf */

#endif /* RWNC_TRANSPORT_HPP__*/
