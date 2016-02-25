
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_session.hpp
 * @author Tom Seidenberg
 * @date 2014/06/18
 * @brief RW.Netconf client library session definitions.
 */

#ifndef RWNC_SESSION_HPP_
#define RWNC_SESSION_HPP_

#include <rwnetconf.h>
#include <rw_cf_type_validate.hpp>
#include <functional>

namespace rw_netconf {

// Forward declarations
class CallbackManager;
class DataStore;
class Filter;
class Instance;
class Session;
class User;
class Xml;


/**
 * @addtogroup RwNcSession
 * @{
 */

/**
 * Client request callback type.  This function type defines the
 * callback made by the client library to the client application upon
 * receipt of a request's response.
 *
 * ATTN: Probably belongs in rwnetconf.h, along with any other
 * public-API class definitions.
 *
 * ATTN: Bindings will be needed for the various language interfaces:
 *  - C++: The binding can be anything compatible with std::function<>:
 *    function pointer, lambda, function object, et cetera...
 *  - C: The binding will take a rw_ncclnt_req_cb callback and a
 *    rw_ncclnt_context_t context.  Invocation will dispatch through
 *    the callback with the context.
 *  - Vala: The binding will be a delgate.  It is not entirely clear
 *    what form that will take; see delegate discussion in
 *    https://wiki.gnome.org/Projects/Vala/LegacyBindings
 *
 * Defines a callback object with the following call signature:
 *     void (*)(rw_yang_netconf_op_status_t, Xml*)
 */
typedef std::function<void(rw_yang_netconf_op_status_t, Xml*)> ClientCallback;


/// Session object.
/**
 * A session object contains the transport-independent session state.
 * A session is bound to a transport, but independent of it.  The
 * client application makes all netconf server requests via the session
 * object, which acts as a mediator between the client and the server.
 *
 * The session object is responsible for passing the request to the
 * transport layer for transmission to the server (typically very
 * minimal processing).  Upon completion, the session is further
 * responsible for validating the response and making the callback to
 * the client application.
 *
 * Abstract base class.
 *
 * Why is a session different from a transport?
 *  - Session needs to retain lock context.  (ATTN: Why are locks
 *    independent of transport?)
 *  - Session needs to retain user authentication data.  (ATTN:
 *    Actually, isn't the authenticated user data more directly
 *    associated with the transport, because the authentication methods
 *    are directly related to the transport?)
 *  - Simplified operations can be applied in common to all transports.
 *  - Feature lists and matching are common to all transports.
 *  - Request's response handlers have much code in common, which is
 *    better placed in a single class.
 *  - However, a session is is necessarily bound to a transport; it is
 *    impossible to have a session without a transport.  It is also
 *    impossible to move a session to another transport.
 *
 * ATTN: Eventually, the session state will also include any extra data
 * needed by the simplified APIs for managing multiple-RPC actions.
 *
 * ATTN: Currently, there is only one concrete Session definition.
 * Client API calls are converted to member function invocations on the
 * Session object.  The Session object allocates the context object,
 * sets up the request, and passes the request to the transport layer.
 * Upon completion of the request, Transport makes a (functor) callback
 * to the Session.
 *
 * ATTN: TGS: I think Session should contain the threading-model's
 * callback support.  Conceptually, each Session should be bound to a
 * thread resource, and it is up to the application code to determine
 * the threading model and bind Sessions to the correct callback
 * invocation mechanism.  The client library is responsible for
 * providing APIs for the various bindings.  Several bindings are
 * required:
 *  - Single thread, scheduler-less.  This is used in bare-bones unit
 *    tests, where a higher-order loop simply executes the callbacks.
 *    Callbacks are stored in futures and appended to lists.  A call
 *    into the object invokes all found callbacks.
 *  - Multi-thread CFRunloop rwsched.  Binds Sessions to a particular
 *    CF queue.
 *  - Multi-thread libdispatch rwsched.  Binds Sessions to a particular
 *    dispatch context.
 *  - GUI may need another kind of dispatch context, for making
 *    callbacks into python.
 *  - ATTN: Some kind of blocking API.
 */
class Session
{
  // Use RW-CF memory tracking and validation
  RW_CF_TYPE_CLASS_DECLARE_MEMBERS(rw_ncclnt_ses_t, rw_ncclnt_ses_ptr_t, Session);

 public:

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

  /**
   * Get the user associated with the session.
   *
   * Use cases:
   * - Debug
   *
   * @return The user.  Not owned by the caller; use retain, if the
   *   reference will be kept.
   */
  virtual User* get_user() const = 0;

  // ATTN: Disconnect?

  // ATTN: Terminate?


  #if 0
  /**
   * Send a RPC request to a server, using a specific XML blob for the
   * RPC body.  The library does no interpretation of this blob, other
   * than to (possibly) serialize it.  The caller is responsible for
   * making sure that it is a valid request.
   *
   * Because this API does not define the RPC in terms of a yang RPC
   * model node, the client library will not validate the response.
   * The entire response will be passed back to the client, and the
   * client is responsible for validating it.  The client library may
   * perform minimal XML syntax validation, but even that is not
   * guaranteed.
   *
   * Clients should not use this API if the RPC is available in the
   * client library API instance's yang model.  In that case, the
   * client should use the req_rpc_yang() interface, to ensure complete
   * netconf compliance.  This API is provided largely for debugging.
   *
   * The resulting netconf request will appear in the form:
   * <TT><PRE>
   *   <rpc message-id="XX" xmlns="urn:ietf...netconf:base:1.X">
   *     RPC_BODY
   * </PRE></TT>
   *
   * Use cases:
   * - Debug.
   *
   * @return
   * - RW_STATUS_SUCCESS - The request was enqueued.
   * - Others: request could not be enqueued; no callback will be made
   *   and no request was sent.
   */
  virtual rw_status_t req_rpc_xml(
    /**
     * [in] The RPC message body, identifying the RPC type.  Does not
     * include outer <rpc...> element.  The client generated the body
     * and is responsible for validating the response, because only the
     * client knows the message schema.
     * \n
     * Will be retained by the library, possibly until the entire
     * response has been received, so the caller must not modify the
     * blob after making this call, except to release its reference.
     */
    Xml* rpc_body,

    /**
     * [in] The client request complete callback.  Will be called only
     * if the request is queued successfully (indicated by
     * RW_STATUS_SUCCESS return).
     */
    ClientCallback callback
  ) = 0;


  /**
   * Send a RPC request to a server, using a specific RPC name and
   * namespace, and a given XML blob body.  The library does no
   * interpretation of the command name, namespace, or blob, other than
   * to (possibly) serialize the XML.
   *
   * Because this API does not define the RPC in terms of a yang RPC
   * model node, the client library will not validate the response.
   * The entire response will be passed back to the client, and the
   * client is responsible for validating it.  The client library may
   * perform minimal XML syntax validation, but even that is not
   * guaranteed.
   *
   * Clients should not use this API if the RPC name is available in
   * the client library API instance's yang model.  In that case, the
   * client should use the req_rpc_yang() interface, to ensure complete
   * netconf compliance.  This API is provided largely for debugging.
   *
   * The resulting netconf request will appear in the form:
   * <TT><PRE>
   *   <rpc message-id="XX" xmlns="urn:ietf...netconf:base:1.X">
   *     <NAME xmlns="NS">
   *       RPC_BODY
   *     </NAME>
   *   </rpc>
   * </PRE></TT>
   *
   * Use cases:
   * - General library support.
   *
   * @return
   * - RW_STATUS_SUCCESS - The request was enqueued.
   * - Others: request could not be enqueued; no callback will be made
   *   and no request was sent.
   */
  virtual rw_status_t req_rpc_name_ns(
    /**
     * [in] The RPC name, as a UTF-8 string.  Must be a valid
     * identifier according to the netconf protocol (although the
     * transport layer is not obligated to verify that).  Cannot be
     * nullptr.  Will either be copied or no longer needed by the
     * library upon return, so it may be stack allocated.
     */
    const char* name,

    /**
     * [in] The RPC's XML namespace, as a UTF-8 string.  Must not be
     * XML encoded - encoding will be performed by the library, if
     * necessary.  If nullptr, assumed to be
     * "urn:ietf:params:xml:ns:netconf:base:1.0".  Will either be
     * copied or no longer needed by the library upon return, so it may
     * be stack allocated.
     */
    const char* ns,

    /**
     * [in] The RPC message body contents.  Does not include outer
     * <rpc...> element, nor the RPC's name, as identified by the name
     * and ns arguments.  The client generated the body and is
     * responsible for validating the response, because only the client
     * knows the message schema.
     * \n
     * Will be retained by the library, possibly until the entire
     * response has been received, so the caller must not modify the
     * blob after making this call, except to release its reference.
     */
    Xml* rpc_body,

    /**
     * [in] The client request complete callback.  Will be called only
     * if the request is queued successfully (indicated by
     * RW_STATUS_SUCCESS return).
     */
    ClientCallback callback
  ) = 0;


  /**
   * Send a RPC request to a server, using a specified yang RPC
   * definition.  The library does no interpretation of the XML blob,
   * other than to (possibly) serialize the XML.  The caller is
   * responsible for making sure that it is a valid request.
   *
   * This is the preferred API to send a RPC request, due to the model
   * information provided by the yang node.
   *
   * The resulting netconf request will appear in the form:
   * <TT><PRE>
   *   <rpc message-id="XX" xmlns="urn:ietf...netconf:base:1.X">
   *     <RPC_NAME xmlns="RPC_NS">
   *       RPC_BODY
   *     </RPC_NAME>
   *   </rpc>
   * </PRE></TT>
   *
   * Use cases:
   * - General library support.
   *
   * @return
   * - RW_STATUS_SUCCESS - The request was enqueued.
   * - Others: request could not be enqueued; no callback will be made
   *   and no request was sent.
   */
  virtual rw_status_t req_rpc_yang(
    /**
     * [in] The yang model node of the RPC.  Must be RPC or RPC input.
     * Must be from the client library API instance's yang model.
     * Cannot be nullptr.
     */
    rw_yang_node_t* rpc_ynode,

    /**
     * [in] The RPC message body contents.  Does not include outer
     * <rpc...> element, nor the RPC's element, as identified by ynode.
     * The client generated the body, which may not be validated by the
     * library.  The library will validate the response according to
     * netconf rules.
     * \n
     * Will be retained by the library, possibly until the entire
     * response has been received.  So the caller must not modify the
     * blob after making this call, except to release its reference.
     */
    Xml* rpc_body,

    /**
     * [in] The client request complete callback.  Will be called only
     * if the request is queued successfully (indicated by
     * RW_STATUS_SUCCESS return).
     */
    ClientCallback callback
  ) = 0;
  #endif


  /**
   * Send a netconf get-config request to a server.  The data can be
   * limited by providing a filter.
   *
   * ATTN: Extensions API?  Will want to have "chunked" return value API.
   * Although, the "chunked" API probably needs to use a "simplified" API
   * because returning responses to a single request is not otherwise
   * netconf compliant.
   *
   * ATTN: Option to include a list of XML attributes to apply to the
   * get-config node?
   *
   * ATTN: Option to include a XML blob to append to the body,
   * following the ds and filter?
   *
   * The resulting netconf request will appear in the form:
   * <TT><PRE>
   *   <rpc message-id="XX" xmlns="urn:ietf...netconf:base:1.X">
   *     <get-config>
   *       <source>
   *         DATASTORE
   *       </source>
   *       <!-- optionally, if filter is not nullptr -->
   *       <filter type="FILTER_TYPE">
   *         FILTER
   *       </filter>
   *     </get-config>
   *   </rpc>
   * </PRE></TT>
   *
   * Use cases:
   * - General library support.
   *
   * @return
   * - RW_STATUS_SUCCESS - The request was enqueued.
   * - Others: request could not be enqueued; no callback will be made
   *   and no request was sent.
   */
  virtual rw_status_t req_nc_get_config(
    /**
     * [in] The configuration source.  Must be one of startup, running,
     * or candidate.  Cannot be nullptr or an URL datastore.  Will be
     * retained by the library, possibly until the entire response has
     * been received.  So the caller must not modify the object after
     * making this call, except to release its reference.
     */
    const DataStore* config_source,

    /**
     * [in] The filter description.  May be nullptr if no filtering is
     * desired.
     * \n
     * Will be retained by the library, possibly until the entire
     * response has been received.  So the caller must not modify the
     * blob after making this call, except to release its reference.
     */
    Filter* filter,

    /**
     * [in] The client request complete callback.  Will be called only
     * if the request is queued successfully (indicated by
     * RW_STATUS_SUCCESS return).
     */
    ClientCallback callback,

    /**
     * [in] The concurrency context to make the callback in.  Will
     * retain a reference if the request is queued successfully
     * (indicated by RW_STATUS_SUCCESS return).
     */
    CallbackManager* cbmgr
  ) = 0;


  /**
   * Send an edit-config request to a server, using a XML blob for the
   * changes to be made.  This API sends no default-operation parameter,
   * thus resulting in a merge.
   *
   * ATTN: Optional <default-operation>.
   * ATTN: Optional <test-option>.
   * ATTN: Optional <error-option>.
   *
   * The resulting netconf request will appear in the form:
   * <TT><PRE>
   *   <rpc message-id="XX" xmlns="urn:ietf...netconf:base:1.X">
   *     <edit-config>
   *       <target>
   *         DATASTORE
   *       </target>
   *       <!-- optionally, if default_op is not nullptr -->
   *         <default-operation>DEFAULT_OP</default-operation>
   *       <!-- optionally, if test_opt is not nullptr -->
   *         <test-option>TEST_OPT</test-option>
   *       <!-- optionally, if error_opt is not nullptr -->
   *         <error-option>ERROR_OPT</error-option>
   *       CONFIG
   *     </get-config>
   *   </rpc>
   * </PRE></TT>
   *
   * Use cases:
   * - General library support.
   *
   * @return
   * - RW_STATUS_SUCCESS - The request was enqueued.
   * - Others: request could not be enqueued; no callback will be made
   *   and no request was sent.
   */
  virtual rw_status_t req_nc_edit_config(
    /**
     * [in] The target configuration datastore.  Must be one of
     * running, or candidate.  Cannot be nullptr, startup, or an URL
     * datastore.  Will be retained by the library, possibly until the
     * entire response has been received.  So the caller must not
     * modify the object after making this call, except to release its
     * reference.
     */
    const DataStore* config_target,

    /**
     * [in] The default-operation.  One of merge, replace, or none.
     * May be nullptr, in which case no default-operation is included
     * in the request.  When no default-operation is specified, merge
     * is assumed.
     * \n
     * ATTN: Currently unsupported.  Complete the design.  Switch to
     * C++ class name when ready.
     */
    void* default_op,

    /**
     * [in] The test-option.  One of test-then-set, set, or test-only.
     * May be nullptr, in which case no test-option is included in the
     * request.
     * \n
     * ATTN: Currently unsupported.  Complete the design.  Switch to
     * C++ class name when ready.
     */
    void* test_opt,

    /**
     * [in] The error-option.  One of stop-on-error, continue-on-error,
     * or rollback-on-error.  May be nullptr, in which case no
     * error-option is included in the
     * request.
     * \n
     * ATTN: Currently unsupported.  Complete the design.  Switch to
     * C++ class name when ready.
     */
    void* error_opt,

    /**
     * [in] The configuration changes to be made.  Must be defined by a
     * root <config> node.  The client generated the body, which may
     * not be validated by the library.
     * \n
     * Will be retained by the library, possibly until the entire
     * response has been received.  So the caller must not modify the
     * blob after making this call, except to release its reference.
     */
    Xml* config,

    /**
     * [in] The client request complete callback.  Will be called only
     * if the request is queued successfully (indicated by
     * RW_STATUS_SUCCESS return).
     */
    ClientCallback callback,

    /**
     * [in] The concurrency context to make the callback in.  Will
     * retain a reference if the request is queued successfully
     * (indicated by RW_STATUS_SUCCESS return).
     */
    CallbackManager* cbmgr
  ) = 0;

 protected:
  /// Trivial constructor
  Session()
  {}

  /// Trivial destructor
  virtual ~Session()
  {}

  // Cannot copy.
  Session(const Session&) = delete;
  Session& operator=(const Session&) = delete;
};

RW_CF_TYPE_CLASS_DEFINE_INLINES(rw_ncclnt_ses_t, rw_ncclnt_ses_ptr_t, Session);

/** @} */

} // namespace rw_netconf

RW_CF_TYPE_CLASS_DEFINE_TRAITS(rw_ncclnt_ses_t, rw_ncclnt_ses_ptr_t, rw_netconf::Session);

#endif /* RWNC_SESSION_HPP_ */
