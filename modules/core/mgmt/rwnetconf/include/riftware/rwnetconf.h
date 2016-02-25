
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnetconf.h
 * @author Tom Seidenberg
 * @date 2014/05/12
 * @brief RW.Netconf client library definitions
 */

#ifndef RWNETCONF_H__
#define RWNETCONF_H__

#include <rw_cf_type_validate.h>
#include <rw-base.pb-c.h>
#include <rwtrace.h>

/**
 * @mainpage
 *
 * The RW.Netconf Client Library defines a unified API for making
 * netconf (and netconf-like) requests.  The library provides an
 * abstraction layer that hides the details of the transport layer and
 * provides for some simplified operations.  The client library is
 * composed of 3 main APIs.
 *
 * API Overiview:
 *  - RW.Netconf C native API: This is what an application developer
 *    uses to access netconf.  This library provides access to all
 *    netconf operations, as well as to some simplifying
 *    meta-operations composed of several individual netconf
 *    operations.  The library maintains session state.
 *  - RW.Netconf Vala binding: This API provides a Vala interface on
 *    top of the C native API.  It allows the client library to be used
 *    in other languages besides C/C++.
 *  - RW.Netconf transport API: This is an internal RW.Neconf API that
 *    adapts a specific transport to the library.  Several transports
 *    will be developed to support requirements and test.
 *
 * Library Design Topics:
 *  - @ref RwNcPlan
 *  - @ref RwNcXapi
 *  - @ref RwNcStack
 *  - @ref RwNcXapiNcSsh
 *  - @ref RwNcXapiRwSched
 *  - @ref RwNcXapiRwMsg
 *  - @ref RwNcXapiDirect
 *
 * Library Utility APIs:
 *  - @ref RwNcInstance
 *  - @ref RwNcCbMgr
 *  - @ref RwNcDs
 *  - @ref RwNcFilter
 *  - @ref RwNcLock
 *  - @ref RwNcUser
 *  - @ref RwNcXml
 *  - @ref RwNcXmlGen
 *
 * Library Netconf APIs:
 *  - @ref RwNcSession
 *  - @ref RwNcApi
 *
 * Testing Information:
 *  - @ref RwNcUnitTest
 *
 * API goals:
 * - Support all current, applicable, RFCs and their associated commands:
 *   - <a href="http://tools.ietf.org/html/rfc5277">RFC 5277</a>:
 *     NETCONF notifications
 *      - create-subscription
 *   - <a href="http://tools.ietf.org/html/rfc5717">RFC 5717</a>:
 *     Partial lock of running config
 *      - partial-lock
 *      - partial-unlock
 *   - <a href="http://tools.ietf.org/html/rfc6020">RFC 6020</a>:
 *     Yang
 *   - <a href="http://tools.ietf.org/html/rfc6022">RFC 6022</a>:
 *     Yang module for NETCONF monitorring
 *      - get-schema
 *   - <a href="http://tools.ietf.org/html/rfc6241">RFC 6241</a>: NETCONF
 *      - get
 *      - get-config
 *      - edit-config
 *      - copy-config
 *      - delete-config
 *      - lock
 *      - unlock
 *      - kill-session
 *      - commit
 *      - cancel-commit
 *      - discard-changes
 *      - validate
 *      - rpc
 *   - <a href="http://tools.ietf.org/html/rfc6242">RFC 6242</a>:
 *     NETCONF/SSH
 *   - <a href="http://tools.ietf.org/html/rfc6243">RFC 6243</a>:
 *     NETCONF with-defaults
 *   - <a href="http://tools.ietf.org/html/rfc6536">RFC 6536</a>:
 *     NETCONF Access Control Model
 * - Support certain IETF drafts that are chartered for standardization:
 *   - <a href="http://tools.ietf.org/html/draft-ietf-netconf-restconf">
 *     draft-ietf-netconf-restconf</a>:
 *     RESTCONF Protocol
 *   - <a href="http://tools.ietf.org/html/draft-ietf-netconf-yang-patch">
 *     draft-ietf-netconf-yang-patch</a>:
 *     YANG Patch Media Type (used by RESTCONF)
 * - Support proposed rift NETCONF extensions
 *   - get with cursor support (conceivably extendable to get-config and rpc)
 *   - extend partial lock to a common candidate config
 * - Multi-transport
 *   - RW.NC over RW.Msg (users: RW.Cli, RW.GUI, RW.Test, debug tools)
 *   - RW.NC over NC/SSH (users: RW.Cli, RW.Test)
 *   - RW.NC direct call (users: RW.MgmtAgent, RW.Test)
 * - Support RW.CLI client over RW.Msg or NC
 * - Support RW.GUI client over RW.Msg
 * - Support Python clients via vala API
 * - Support internal RW.MgmtAgent client serving real NC sessions
 * - Design for test
 */

/**
 * @page RwNcPlan RW.Netconf Development Plan
 *
 * -# Task.Base.  Implement minimum necessary functionality to get
 *    unfiltered get-config and edit-config.  Artifacts:
 *     - Server mock capable of get-config and edit-config.
 *        - 2-3 days, if just API and uses canned XMLDocumentYang
 *        - ATTN: development time depends on how powerful the DOM is
 *     - Client unittest capable of get-config and edit-config.
 *     - Direct RW.NCTransport for get-config and edit-config.
 *     - RW.Netconf APIs:
 *        - rw_ncclnt_ds_create_get()
 *        - rw_ncclnt_ds_create_running()
 *        - rw_ncclnt_ds_is_get_config()
 *        - rw_ncclnt_ds_get_name()
 *        - rw_ncclnt_ds_retain()
 *        - rw_ncclnt_instance_create()
 *        - rw_ncclnt_instance_retain()
 *        - rw_ncclnt_req_edit_config()
 *        - rw_ncclnt_req_get_config()
 *        - rw_ncclnt_req_rpc_xml()
 *        - rw_ncclnt_ses_connect_api_server()
 *        - rw_ncclnt_ses_retain()
 *        - rw_ncclnt_user_create_name()
 *        - rw_ncclnt_user_create_self()
 *        - rw_ncclnt_user_retain()
 *        - rw_ncclnt_xml_consume_buffer()
 *        - rw_ncclnt_xml_consume_get_producer_error()
 *        - rw_ncclnt_xml_consume_is_eof()
 *        - rw_ncclnt_xml_create_const_string()
 *        - rw_ncclnt_xml_create_copy_string()
 *        - rw_ncclnt_xml_retain()
 *        - rw_ncclnt_xml_create_empty()
 *        - rw_ncclnt_xml_create_append()
 *        - rw_ncclnt_xml_create_composed()
 *
 * -# Task.Msg.Base.  As Task.Base is nearing completion,
 *    begin RW.Msg RW.NCTransport implementation.  Artifacts:
 *     - RW.Msg proto APIs defined.
 *     - Unittest is capable of running a canned tasklet test.
 *        - ATTN: This may be a research projeect...
 *     - RW.Msg RW.NCTransport for get-config and edit-config
 *     - RW.Netconf APIs:
 *        - rw_ncclnt_ds_get_create_xml()
 *        - rw_ncclnt_ses_connect_rwmsg()
 *
 * -# Task.Vala.  As Task.Msg.Base is nearing completion, or concurrent
 *    with the tasks, begin the Vala implementation of the library.
 *    Artifacts:
 *     - Vala definition file
 *     - Plugin library
 *     - Vala unittest
 *
 * -# Task.uAgent.  As soon as Task.Msg.Base is complete, begin
 *    implementing the server-side of RW.Netconf in RW.uAgent.  This
 *    support does not replace the current interfaces to RW.uAgent, but
 *    it is a transitional step towards that goal.  RW.uAgent should be
 *    library-ized so that the apps and client can be mocked.
 *    Artifacts:
 *     - RW.uAgent library-ized, suiatable for mocking apps and client.
 *     - RW.uAgent app mock (RW.uAgent already provides a dummy app).
 *     - RW.uAgent implementation of RW.Netconf API.
 *     - RW.uAgent get and get-config filter support
 *        - choice of subtree/xpath/limited-xpath: not clear
 *     - RW.uAgent+Netconf unittest, utilizing RW.Netconf client
 *       unittest, Direct RW.NCTransport, RW.uAgent library, and
 *       RW.uAgent app mock.
 *     - RW.Netconf APIs:
 *        - rw_ncclnt_ses_disconnect()
 *        - rw_ncclnt_req_get_filterx()
 *        - rw_ncclnt_filter_get_create_xml()
 *        - rw_ncclnt_filter_get_instance()
 *        - rw_ncclnt_filter_release()
 *        - rw_ncclnt_filter_retain()
 *
 * -# Task.uAgent.Msg.  As soon as Task.Msg.uAgent has
 *    implemented the RW.Netconf API, begin RW.uAgent integration with
 *    the server-side of RW.Msg RW.NCTransport.  Artifacts:
 *     - RW.uAgent integrated with RW.Msg RW.NCTransport.
 *     - Unittest with RW.Netconf test client and RW.uAgent server,
 *       over RW.Msg.
 *
 * -# Task.Cli.  As soon as Task.uAgent.Msg is complete, begin RW.Cli
 *    integration with RW.Netconf and RW.Msg RW.NCTransport.
 *    Artifacts:
 *     - RW.Cli integrated with RW.Msg RW.NCTransport.
 *     - Unittest with RW.Netconf test client and RW.uAgent server,
 *       over RW.Msg.
 *     - RW.Netconf APIs:
 *        - rw_ncclnt_ds_release()
 *        - rw_ncclnt_ses_release()
 *        - rw_ncclnt_user_release()
 *        - rw_ncclnt_xml_create_xml_yang_subtree()
 *        - rw_ncclnt_xml_release()
 *
 * -# Task.GUI.  As soon as Task.uAgent.Msg is complete, also begin
 *    RW.GUI integration with RW.Netconf and RW.Msg RW.NCTransport.
 *    ATTN: Assumes GUI server will use RW.Msg.
 *    Artifacts:
 *     - RW.GUI integrated with RW.Msg RW.NCTransport.
 *     - Unittest with RW.Netconf test client and RW.uAgent server,
 *       over RW.Msg.
 *     - RW.Netconf APIs:
 *        - rw_ncclnt_user_get_username()
 *        - rw_ncclnt_ses_get_user()
 *
 * -# Task.NC.SSH.  Once the RW.Msg RW.NCTransport is working with both
 *    RW.Cli and RW.GUIServer, we are ready to begin external netconf
 *    integration.  This will be a very long task, but it can be
 *    started at this point.
 *    Artifacts:
 *     - Library selection for client-side.
 *     - Library selection for server-side.
 *     - Client-side implementation of NC/SSH RW.NCTransport.
 *     - Server-side implementation of NC/SSH RW.NCTransport.
 *     - RW.Netconf APIs:
 *        - rw_ncclnt_ds_create_startup()
 *        - rw_ncclnt_ses_connect_nc_ssh()
 *        - rw_ncclnt_xml_consume_wait_producer()
 *        - rw_ncclnt_xml_consume_set_error()
 *        - rw_ncclnt_xml_create_xml_generator()
 *        - rw_ncclnt_xmlgen_get_consumer()
 *        - rw_ncclnt_xmlgen_produce_buffer_async()
 *        - rw_ncclnt_xmlgen_produce_buffer_sync()
 *        - rw_ncclnt_xmlgen_produce_get_consumer_error()
 *        - rw_ncclnt_xmlgen_produce_set_eof()
 *        - rw_ncclnt_xmlgen_produce_set_error()
 *        - rw_ncclnt_xmlgen_release()
 *        - rw_ncclnt_xmlgen_retain()
 *
 * -# Task.Debug.  As soon as Task.uAgent.Msg is complete, also begin
 *    development of debug-type interfaces and continue to fill out the
 *    API.
 *    Artifacts:
 *     - RW.Netconf APIs:
 *        - rw_ncclnt_ds_get_instance()
 *        - rw_ncclnt_ses_get_instance()
 *        - rw_ncclnt_user_get_instance()
 *        - rw_ncclnt_user_get_username()
 *        - rw_ncclnt_xml_get_instance()
 *        - rw_ncclnt_instance_terminate()
 *        - rw_ncclnt_ses_terminate()
 *        - rw_ncclnt_xml_terminate()
 *        - rw_ncclnt_instance_release()
 *
 * -# Task.uAgent.Cleanup.  As soon as Task.Cli and Task.GUI are
 *    complete and functioning correctly, remove the old RW.uAgent
 *    socket interfaces and RW.Msg APIs.  At this point, RW.uAgent is
 *    starting to morph into RW.MgmtAgent, and should probably get a
 *    rename...  Artifacts:
 *     - None.
 *
 * -# Continue implementing APIs as needed, until the end of time...
 *
 * -# ATTN: Will eventually need SAX-ish support in RW XML
 */

/**
 * @page RwNcStack RW.Netconf API Stack Overview
 *
 * The following diagram shows the complete RW.Netconf API stack.
 *
 * <TT><PRE>
 *    +------------------+-------------------+
 *    |                  | Python App        |
 *    |                  +-------------------+ <<< Class-ish python API
 *    |                  | Python Client Lib |
 *    |                  +-------------------+ <<< Vala APIs
 *    | C App            | Vala              |
 *    +------------------+-------------------+ <<< RW.Netconf C native API
 *    | RW NC Client Library Client APIs     |
 *    +--------------------------------------+ <<< RW.NCTransport API
 *    | RW NC Client Transport Plugin        |
 *    +--------------------------------------+
 *
 *    +--------------------------------------+ <<< RW.NCTransport API
 *    | RW NC Client Transport Plugin        |
 *    +---------+---------+--------+---------+ <<< Transport Client API
 *    | NC/SSH  | RWSched | RW.Msg | C Call  |
 *    +---------+---------+--------+---------+ <<< transport layer
 *
 *    +---------+---------+--------+---------+ <<< transport layer
 *    | NC/SSH  | RWSched | RW.Msg | C Call  |
 *    +---------+---------+--------+---------+ <<< Transport Server API
 *    | RW NC Server Transport Plugin        |
 *    +--------------------------------------+ <<< RW.NCTransport API
 *
 *    +--------------------------------------+ <<< RW.NCTransport API
 *    | RW NC Server Transport Plugin        |
 *    +------------------+-------------------+ <<< Application API
 *    | RW NC Server     | Server Mock       |
 *    +------------------+-------------------+
 * </PRE></TT>
 */

/**
 * @page RwNcXapiNcSsh RW.NCTransport NC/SSH
 *
 * Transport layer adaptor: Netconf over SSH: use real netconf over a
 * SSH connection to attach from outside the system.
 *
 * The selection of the netconf client and server libraries can be
 * deferred until we need to make SSH work, and can actually use
 * different libraries for client and server.  Modular approach also
 * allows us to change the libraries in the future, if that becomes
 * necessary.
 *
 * The adaptor translates the data structures into a format acceptible
 * to the library, which then serializes it for transport over the SSH
 * socket.  Adaptation of the library(s) socket API may be required, to
 * fit into our tasklet model.  This concern may motivate large-scale
 * rewrite of the libraries, or a proprietary implementation.
 *
 * Use cases:
 *  - External CLI client application
 *  - External utility client applications
 *  - RW.MgmtAgent Server SSH session handling
 *
 * Stack:
 * <TT><PRE>
 *    +---------------------------+ <<< RW.NCTransport API
 *    | NC/SSH RW.NCTransport     |
 *    +---------------------------+ <<< libnetconf/libncx/RW-proprietary library APIs
 *    | OSS NC/SSH client library |
 *    +---------------------------+ <<< socket client
 *               NC/SSH
 *    +---------------------------+ <<< socket server
 *    | OSS NC/SSH server library |
 *    +---------------------------+ <<< libnetconf/libncx/RW-proprietary library APIs
 *    | NC/SSH RWNC-X-SRVR        |
 *    +---------------------------+ <<< RW.NCTransport API
 * </PRE></TT>
 */

/**
 * @page RwNcXapiRwSched RW.NCTransport RW.Sched
 *
 * Transport layer adaptor: RW.Sched: use RW.Sched to perform
 * same-process handling of both client and server.
 *
 * The adaptor makes no changes to the data beyond queuing the buffers
 * between threads.  Not sure if there are performance concerns with
 * locality, which may result in copying structures rather than making
 * the pointers available.
 *
 * Use cases:
 *  - Multi-threaded unit testing with mocked client or server
 *  - Library performance testing
 *  - RW.MgmtAgent Server dispatch of other transports (NC/SSH)
 *
 * Stack:
 * <TT><PRE>
 *    +---------------------------+ <<< RW.NCTransport API
 *    | RW.Sched RW.NCTransport   |
 *    +---------------------------+ <<< RW.Sched API
 *          RW.Sched Queuing
 *    +---------------------------+ <<< RW.Sched API
 *    | RW.Sched RWNC-X-SRVR      |
 *    +---------------------------+ <<< RW.NCTransport API
 * </PRE></TT>
 */

/**
 * @page RwNcXapiRwMsg RW.NCTransport RW.Msg
 *
 * Transport layer: Netconf over RW.Msg: uses largely netconf syntax
 * (except for the outermost layers - some liberties are taken with
 * packaging, to eliminate some XML processing), plus additional
 * metadata (authentication).
 *
 * The adaptor makes no changes to the data beyond serializing it to
 * protobuf messages for transport over RW.Msg.  Probably shares
 * substantially with RW.Sched transport.
 *
 * Use cases:
 *  - Internal CLI client
 *  - Internal web server client (?)
 *
 * Stack:
 * <TT><PRE>
 *    +---------------------------+ <<< RW.NCTransport API
 *    | RW.Msg RW.NCTransport     |
 *    +---------------------------+ <<< libnetconf/libncx/RW client library APIs
 *    | RW.Msg library            |
 *    +---------------------------+ <<< socket client
 *               NC/SSH
 *    +---------------------------+ <<< socket server
 *    | RW.Msg library            |
 *    +---------------------------+ <<< libnetconf/libncx/RW server library APIs
 *    | RW.Msg RWNC-X-SRVR        |
 *    +---------------------------+ <<< RW.NCTransport API
 * </PRE></TT>
 */

/**
 * @page RwNcXapiDirect RW.NCTransport Direct
 *
 * Transport layer adaptor: Direct function call: make direct function
 * call to another adaptor or a real client implementation.
 *
 * The adaptor makes no changes to the data and passes it directly to
 * the target function.
 *
 * Use cases:
 *  - Unit testing with mocked client or server
 *  - Library performance testing
 *
 * Stack:
 * <TT><PRE>
 *    +---------------------------+ <<< RW.NCTransport API
 *    | Direct RW.NCTransport     |
 *    +---------------------------+ <<< RW.NCTransport API
 * </PRE></TT>
 */


/**

      +---------------------------+ <<< RW.NCTransport API
      | NC/SSH client             |
      +---------------------------+ <<< libnetconf/libncx/RW client library APIs
      | OSS NC/SSH client library |
      +---------------------------+ <<< socket client
                 NC/SSH
      +---------------------------+ <<< socket server
      | OSS NC/SSH server library |
      +---------------------------+ <<< libnetconf/libncx/RW server library APIs
      | NC/SSH server             |
      +---------------------------+ <<< RW.NCTransport API
      | RW.Sched client           |
      +---------------------------+ <<< RW.Sched queuing APIs
            RW.Sched Queuing
      +---------------------------+ <<< RW.Sched callback APIs
      | RW.Sched server           |
      +---------------------------+ <<< RW.NCTransport API
      | RW.MgmtAgent              |
      +---------------------------+
 */


/**
   External CLI model (same as internal using NC/SSH): Just one API
   session and just one NC/SSH connection, bound to a specific user.
 *
   ATTN: The appropriate credentials to identify the user must be
   provided upon starting the CLI.  Defaults to current user.
 *
      +--------------------+
      | External RW.Cli    |
      +--------------------+
      | RW NC Client Lib   |
      | api instance       |
      | = = = = = = = = = =|
      | user               |
      | datastores         |
      | = = = = = = = = = =|
      | nc/ssh connection  |
      +--------------------+
      | NC/SSH Client Lib  |
      +--------------------+
              NC/SSH
      +--------------------+
      | NC/SSH Server Lib  |
      +--------------------+
      | RW NC Server Lib   |
      +--------------------+
      | RW.MgmtAgent       |
      +--------------------+
 */

/**
   Internal CLI model: Just one API session and just one connection
   over RW.Msg.  The NC-user is the invoking user.
 *
   The specifics of how the internal CLI authenticates the users or
   obtains their credentials is unspecified.  Presumably, the user has
   already been authenticated by virtue of being internal to the
   system.
 *
      +--------------------+
      | Internal RW.Cli    |
      +--------------------+
      | RW NC Client Lib   |
      | api instance       |
      | = = = = = = = = = =|
      | user               |
      | datastores         |
      | = = = = = = = = = =|
      | rwmsg connection   |
      +--------------------+
      | RW.Msg RW NC CLI    |
      +--------------------+
              RW.Msg
      +--------------------+
      | RW.Msg RW NC SRV    |
      +--------------------+
      | RW NC Server Lib   |
      +--------------------+
      | RW.MgmtAgent       |
      +--------------------+
 */

/**
   Internal NC/SSH server connection inside management agent: multiple
   API sessions and connections from external sources.  The NC-user has
   been authenticated by SSH prior to transferring socket to
   RW,MgmtAgent.
 *
      +--------------------+
      | NC/SSH Connection  |
      +--------------------+
      | RW NC Client Lib   |
      | api instance       |
      | = = = = = = = = = =|
      | user               |
      | datastores         |
      | = = = = = = = = = =|
      | sched connection   |
      +--------------------+
      | RWSched            |
      +--------------------+
      | RW NC Server Lib   |
      +--------------------+
      | RW.MgmtAgent       |
      +--------------------+
 */


/**
   External GUI server (same as internal using NC/SSH): One API
   session, with zero or more connections over real NETCONF, each
   connection bound to a specific user.
 *
   ATTN: The specifics of how the GUI authenticates the users or
   obtains their credentials is unspecified.
 *
   ATTN: This model presents a problem with re-authenticating the users
   between the GUI Server and RW.MgmtAgent.
 *
      +--------------------+ +--------------------+
      | GUI Browser        | | GUI Browser        |
      +--------------------+ +--------------------+
            HTTPS                   HTTPS
      +-------------------------------------------+
      | GUI Server                                |
      +-------------------------------------------+
      | RW NC Client Lib                          |
      | api instance                              |
      | = = = = = = = = = = = = = = = = = = = = = |
      | user1               : user2               |
      | datastore1          : datastore2          |
      | = = = = = = = = = = = = = = = = = = = = = |
      | NC/SSH connection   : NC/SSH connection   |
      +-------------------------------------------+
      | NC/SSH Client Lib                         |
      +-------------------------------------------+
              NC/SSH                NC/SSH
      +-------------------------------------------+
      | NC/SSH Server Lib                         |
      +-------------------------------------------+
      | RW NC Server Lib                          |
      +-------------------------------------------+
      | RW.MgmtAgent                              |
      +-------------------------------------------+
 */

/**
   Internal GUI: One API session with just one connection over
   RW.Msg, not bound to any specific user.
   ATTN: The specifics of how the GUI authenticates the users is
   unspecified.

      +--------------------+ +--------------------+
      | GUI Browser        | | GUI Browser        |
      +--------------------+ +--------------------+
            HTTPS                   HTTPS
      +-------------------------------------------+
      | GUI Server                                |
      +-------------------------------------------+
      | RW NC Client Lib                          |
      | api instance                              |
      | = = = = = = = = = = = = = = = = = = = = = |
      | user1               : user2               |
      | datastore1          : datastore2          |
      | = = = = = = = = = = = = = = = = = = = = = |
      | rwmsg connection                          |
      +-------------------------------------------+
      | RW.Msg RW NC CLI                           |
      +-------------------------------------------+
            RW.Msg
      +--------------------+
      | RW.Msg RW NC SRV    |
      +--------------------+
      | RW NC Server Lib   |
      +--------------------+
      | RW.MgmtAgent       |
      +--------------------+
 */

/**
   Client library unit-test: One API session with an internal
   connection to a mock server.
 *
      +--------------------+
      | Unit test          |
      +--------------------+
      | RW NC Client Lib   |
      | api instance       |
      | = = = = = = = = = =|
      | user               |
      | datastores         |
      | = = = = = = = = = =|
      | mock connection    |
      +--------------------+
      | RW NC Server Mock  |
      +--------------------+
 */

/**
   Management Agent unit-test: The unit test mocks a set of clients,
   which directly connect to the RW.MgmtAgent server.
 *
      +--------------------+
      | Unit test          |
      +--------------------+
      | RW NC Client Mock  |
      | api instance       |
      | = = = = = = = = = =|
      | user               |
      | datastores         |
      | = = = = = = = = = =|
      | mock connection    |
      +--------------------+
      | RW NC Server Lib   |
      +--------------------+
      | RW.MgmtAgent       |
      +--------------------+
 */

/**
   API first-class objects.
 *
   Session handle for library
    - What is the rationale for this?  Why can't I just make calls to
      obtain a client session?
       - Need a place to store libpeas/vala related data structures
    - This might not need to be first-class
 *
   Client session handle
    - Netconf is a session-based protocol - must track client sessions,
      and so the library must allocate a session object.
       - RFC 5277: notification subscriptions
       - RFC 5717: partial lock
       - RFC 6241: lock, user-specific candidate
 *
   User handle
    - Netconf is a session-based protocol that authenticates based on
      some user.  When operating over any transport other than NC/SSH,
      the library must track and/or provide an authentication token
       - RFC 6536: Netconf Access Control Model
 *
   Datastore
    - Does not need to be first-class.
    - However, it could be convenient, as it will allow a single API to
      support multiple requests.
    - By making first class, client API can use transport API to
      determine which datastores are available.
 *
   Lock
    - Does not, necessarily, need to be first-class
       - Could be a property of the session
    - However, it could be convenient, as it will allow a single API to
      support multiple requests.

 */

#include <rw_cf_type_validate.h>

#include <yangmodel.h>
#include <rw_xml.h>
#include <rwlib.h>

#include <stdint.h>
#include <sys/cdefs.h>

__BEGIN_DECLS

// Just some bytes
typedef struct rw_ncclnt_token_datastore_s {} rw_ncclnt_token_datastore_t;

// Just some bytes
typedef struct rw_ncclnt_token_lock_s {} rw_ncclnt_token_lock_t;

// Just some bytes
typedef struct rw_ncclnt_token_commit_s {} rw_ncclnt_token_commit_t;

// Just some bytes
typedef struct rw_ncclnt_token_iterator_s {} rw_ncclnt_token_iterator_t;

/// C API user data context.
typedef struct rw_ncclnt_context_s
{
  intptr_t data1;
  intptr_t data2;
} rw_ncclnt_context_t;

// Just some bytes
//typedef struct rw_yang_netconf_op_status_s {} rw_yang_netconf_op_status_t;

// Just some bytes
typedef struct rw_ncclnt_ssh_credentials_s {} rw_ncclnt_ssh_credentials_t;

// ATTN: TGS: I think it is wrong to define rw_xml_yang_node_t here - isn't this from rw_xml?
// Just some bytes
typedef struct rw_xml_yang_node_t {} rw_xml_yang_node_t;

// Status and error codes
enum rw_ncclnt_status_t {
  UNDEFINED = 0
};

/// state of the RW Netconf client instance
typedef enum rw_ncclnt_instance_state_e {
  RW_NCCLNT_STATE_NULL = 0,
  RW_NCCLNT_STATE_base = 0x33100,
  RW_NCCLNT_STATE_INITIALIZED,
  RW_NCCLNT_STATE_TERMINATED,
  RW_NCCLNT_STATE_end,

  RW_NCCLNT_STATE_FIRST = RW_NCCLNT_STATE_base+1,
  RW_NCCLNT_STATE_LAST  = RW_NCCLNT_STATE_end-1,
  RW_NCCLNT_STATE_COUNT = RW_NCCLNT_STATE_end - RW_NCCLNT_STATE_base-1,

} rw_ncclnt_instance_state_t;

static inline bool_t rw_ncclnt_instance_state_is_good(rw_ncclnt_instance_state_t v)
{
  return    (int)v >= (int)RW_NCCLNT_STATE_FIRST
         && (int)v <= (int)RW_NCCLNT_STATE_LAST;
}
static inline bool_t rw_ncclnt_instance_state_is_good_or_null(rw_ncclnt_instance_state_t v)
{
  return    v == RW_NCCLNT_STATE_NULL
         || (    (int)v >= (int)RW_NCCLNT_STATE_FIRST
              && (int)v <= (int)RW_NCCLNT_STATE_LAST);
}
static inline size_t rw_ncclnt_instance_state_to_index(rw_ncclnt_instance_state_t v)
{
  return (size_t)((int)v - (int)RW_NCCLNT_STATE_FIRST);
}
static inline rw_ncclnt_instance_state_t rw_ncclnt_instance_state_from_index(size_t i)
{
  return (rw_ncclnt_instance_state_t)(i + (size_t)RW_NCCLNT_STATE_FIRST);
}

// ATTN: Need function that converts these to rw_status_t


/*****************************************************************************/
/**
 * @defgroup RwNcInstance RW.Netconf Instance
 * @{
 *
 * RW.Netconf client library Instance state may include
 *  - plugin component reference
 *  - plugin instance data
 *  - references to other handles allocated via the instance
 *  - references to active connections
 *  - statistics
 *  - and other instance-related data
 *
 * The instance may own the other handles, or may assume the users of
 * other handles own them, depending on the usage model for the other
 * handles.
 *
 * The handle is a managed object.  The handle has no existence or
 * meaning outside of an active client library instance.
 *
 * ATTN: Recovery session?  Is that part of ses?
 */

/**
 * RW.Netconf client library instance handle.  1 per instance.
 */
struct rw_ncclnt_instance_s {};
RW_TYPE_DECL_ALL(rw_ncclnt_instance_s, rw_ncclnt_instance_t, rw_ncclnt_instance_ptr_t)
RW_CF_TYPE_CLASS_EXTERN(rw_ncclnt_instance_ptr_t)



/**
 * Create RW.Netconf client library instance.  This is a synchronous
 * interface and it may block (or, at least take noticeable wall time) -
 * in order to instantiate various plugins and/or load shared
 * libraries.  Presumably, this API only gets called once during
 * tasklet startup.  All other RW.Netconf APIs require an instance,
 * either explicitly or implicitly.
 *
 * ATTN: Should take yang model reference?  If none passed, creates its
 * own?
 *
 *  Passes Yangmodel as argument - Model is owned by the caller
 *
 * @return The instance handle.  Caller owns the handle.  ATTN: null on error?
 */
rw_ncclnt_instance_t* rw_ncclnt_instance_create(
  /**
   * [in] Yang model.  The caller owns the model.  The library assumes
   * the model is going to live at least as long as the library
   * instance.
   */
  rw_yang_model_t* model,

  /**
   * [in] RW.Trace instance.  The caller owns the trace instance.  The
   * library assumes the trace instance is going to live at least as
   * long as the library instance.  May be nullptr, which suppresses
   * all tracing.
   */
  rwtrace_ctx_t* trace_instance
);

/**
 * Retain a reference to a RW.Netconf client library instance.  The
 * original caller to rw_ncclnt_instance_create() should not do this - that
 * caller owns a reference upon return.
 *
 * ATTN: thread safety considerations?  I do not know if RWCF-types
 * using CFRetain and CFRelease are thread safe.
 *
 * Use cases:
 * - The library itself, whenever the application passes the instance
 *   to an API that runs asynchronously.
 * - The library APIs that create other objects, which hold internal
 *   references to the instance.
 */
void rw_ncclnt_instance_retain(
  const rw_ncclnt_instance_t* instance ///< [in] The instance to retain
);

/**
 * Release a reference to a RW.Netconf client library instance.  The
 * original caller to rw_ncclnt_instance_create() should do this when done
 * with netconf.  Presumably, most applications will hold their
 * references until tasklet/process exit.  When the last reference is
 * released, the instance object will be destroyed.
 *
 * ATTN: thread safety considerations?  I do not know if RWCF-types
 * using CFRetain and CFRelease are thread safe.
 *
 * Use cases:
 * - The library itself, when done with an asynchronously held handle.
 * - The library APIs that destroy other library objects.
 */
void rw_ncclnt_instance_release(
  const rw_ncclnt_instance_t* instance ///< [in/out] The reference to release. Will be nulled.
);

/**
 * Terminate a RW.Netconf client library instance.  This function
 * closes an instance, which prevents it and all related objects
 * from being used again for processing.  It does not destroy the
 * objects, and does not release the caller's reference to the
 * instance.  This function may release underlying resources
 * immediately.  After making this call, the instance reference
 * should not be used again, except for passing to release.
 *
 * This API provides a way to arbitrarily stop processing netconf at
 * anytime, which may be useful for quick termination of a tasklet.
 *
 * ATTN: how does it ensure that all objects are released?
 *
 * Use cases:
 * - A tasklet wants to clean up.
 */
void rw_ncclnt_instance_terminate( //ATTNSTEP.Debug
  rw_ncclnt_instance_t* instance ///< [in] The instance to close
);

/**
 * Get the current state of the instance.
 *
 * @return
 *  - RW_NCCLNT_STATE_INITIALIZED: instance is good
 *  - RW_NCCLNT_STATE_TERMINATED: instance has been terminated, you
 *    should stop using it
 */
rw_ncclnt_instance_state_t  rw_ncclnt_instance_get_state(
  const rw_ncclnt_instance_t* instance ///< [in] The instance to query
);

/**
 * Get the yang model.
 * @return The yang model.  The owner of Instance owns the model.
 */
rw_yang_model_t* rw_ncclnt_instance_get_model(
  const rw_ncclnt_instance_t* instance ///< [in] The instance to query
);

/**
 * Get the RW.Trace instance.
 * @return The trace instance.  The owner of Instance owns the trace
 *   instance.
 */
rwtrace_ctx_t* rw_ncclnt_instance_get_trace_instance(
  const rw_ncclnt_instance_t* instance ///< [in] The instance to query
);

/**
 * Get the allocator to use for objects in this instance.
 * @return The allocator.  The Instance owns the allocator; use
 *   retain to keep a reference.
 */
CFAllocatorRef rw_ncclnt_instance_get_allocator(
  const rw_ncclnt_instance_t* instance ///< [in] The instance to query
);

/** @} */


/*****************************************************************************/
/**
 * @defgroup RwNcUser RW.Netconf User Descriptor
 * @{
 *
 * User handle for RW.Netconf client library.  Multiple references to a
 * single user are allowed.
 *
 * User state may include
 *  - reference to the client library instance
 *  - the user name
 *  - system user ID
 *  - system group membership list
 *  - activity timers
 *  - references to the owned (partial) locks
 *  - (references to) NACM group information
 *  - statistics
 *  - other user-related data
 *
 * The handle is a managed object that represents a user.  However, a
 * user exists independently of any transport session and any specific
 * handle.  Some aspects of user state may persist beyond the lifetime
 * of a transport session, either because they are configuration, to
 * provide a better user experience, or because of standards
 * requirements.
 *
 * ATTN: recovery user?
 * ATTN: super user?
 * ATTN: generic web-access read-only user?
 */

/**
 * RW.Netconf client library user handle.  1 per instance.
 */
struct rw_ncclnt_user_s {};
RW_TYPE_DECL_ALL(rw_ncclnt_user_s, rw_ncclnt_user_t, rw_ncclnt_user_ptr_t)
RW_CF_TYPE_CLASS_EXTERN(rw_ncclnt_user_ptr_t)

/**
 * Retain a reference to a RW.Netconf user.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Required to create a session.
 */
void rw_ncclnt_user_retain(
  const rw_ncclnt_user_t* user ///< [in] The user reference to retain
);

/**
 * Release a reference to a RW.Netconf user.  The reference should be
 * released when the user is no longer needed.  When the last reference
 * is released, the user object will be destroyed.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Done with creating a session.
 */
void rw_ncclnt_user_release(
  const rw_ncclnt_user_t* user ///< [in/out] The reference to release. Will be nulled.
);


/**
 * Create a user object based on the current user (getpid()).  This
 * does not need to be inside the RiftWare system.  This is a
 * synchronous interface, but it should not block or take noticeable
 * wall time, because it should just be a system call (or a few).
 *
 * Use cases:
 * - RW.Cli invoked externally.
 * - Maybe also RW.Cli invoked internally - ATTN: it is not clear how
 *   internal authentication would happen if netconf users do not map
 *   directly to VM host users.
 *
 * @return User handle.  Caller owns the handle.  ATTN: null on error?
 *   ATTN: Cannot fail?
 */
rw_ncclnt_user_t* rw_ncclnt_user_create_self( //ATTNSTEP.Base
  rw_ncclnt_instance_t* instance ///< [in] The owning library instance
);

/**
 * Create a user object with a specific name.  This API should only be
 * used internal to a RiftWare system, on a user that has already been
 * authenticated via some other method.
 *
 * Use cases:
 * - RW.MgmtAgent internals.
 * - RW.GUIServer after client authentication.
 * - Unit tests.
 *
 * @return User handle.  Caller owns the handle.  ATTN: null on error?
 *   ATTN: Cannot fail?
 */
rw_ncclnt_user_t* rw_ncclnt_user_create_name( //ATTNSTEP.Base
  rw_ncclnt_instance_t* instance, ///< [in] The owning library instance
  const char* username ///< [in] The username to create
);


/**
 * Get the library instance from a user reference.
 *
 * Use cases:
 * - Debug
 *
 * @return The library instance.  Not owned by the caller; use retain, if
 *   the reference will be kept.
 */
rw_ncclnt_instance_t* rw_ncclnt_user_get_instance( //ATTNSTEP.Debug
  const rw_ncclnt_user_t* user ///< [in] The user
);

/**
 * Get the username associated with a user reference.
 *
 * Use cases:
 * - Debug
 *
 * @return The username.  Not owned by the caller; make a copy if the
 *   string will be stored.
 */
const char* rw_ncclnt_user_get_username( //ATTNSTEP.GUI
  const rw_ncclnt_user_t* user ///< [in] The user
);

/**
 * Set the username associated with a user reference.
 *
 * Use cases:
 * -
 *
 * @return void
 */
void rw_ncclnt_user_set_username( //ATTNSTEP.GUI
    rw_ncclnt_user_t* user, ///< [in] The user
    const char *name
);

/** @} */


/*****************************************************************************/
/**
 * @defgroup RwNcCbMgr RW.Netconf Callback Manager
 * @{
 *
 * A Callback Manager is an abstraction that defines how callbacks will
 * be made from the RW.Netconf client library to the client
 * application.  A callback manager is the only part of the client
 * library that understands the threading or concurrency model of the
 * client application.  It allows the client library to make no
 * assuptions about how objects are passed between concurrent client
 * and library entities.  Interally, the library manages its own
 * concurrency.
 *
 * Internally, a callback manager maintains some kind of queue of
 * callbacks to be made to the application.  Callbacks from the library
 * are placed into futures, functors, or other similar abstration, and
 * then queued to the manager.  Internally, the manager ensures that
 * the callback is made from the correct client application context.
 *
 * Examples of Callback Manager implementations include:
 *  - Simple polled queue.  As the library makes callbacks, they are
 *    frozen into functors are queued for layer execution.  Execution
 *    occurs in a polling loop.  This model will be used for processes
 *    that have no other model, such as unit tests that do not make use
 *    or RW.Sched.
 *  - RW.Sched CF.  Library callbacks are scheduled through a RW.Sched
 *    CF Queue.
 *  - RW.Sched libdispatch.  Library callbacks are scheduled through a
 *    RW.Sched libdispatch queue.
 *  - Python.  It is not clear yet if this will be needed, but is may
 *    be required to have a python-specific callback manage that
 *    integrates with the interpretter to shedule the callbacks at an
 *    appropriate time.
 *  - Others, as identified.
 *
 * ATTN: need to identify the protential pitfalls with multi-language
 * callback managers.  For example, a callback into python needs to
 * occur at a point which is safe for the interpretter.  It is unclear
 * whether to us how thread-friendly the pythin interpretter core
 * really is, and how conccurrency can be applied in python.
 *
 * Callback Manager state may include
 *  - reference to the API instance
 *  - user-provided context
 *  - a queue of callbacks to be made
 *  - references to queued callbacks
 *  - internal messaging data
 *  - statistics
 *  - other concurrency-related data
 */

/**
 * Callback Manager Handle
 *
 * The handle is a managed object that represents a callback manager.
 * The callback manager does not exist independently of the handle;
 * when the handle is released, the callback manager (and, possibly,
 * any queued callbacks [although, htat is manager-dependent]) are
 * destroyed.  A callback manager cannot survive process failure.
 */
struct rw_ncclnt_cbmgr_s {};
RW_TYPE_DECL_ALL(rw_ncclnt_cbmgr_s, rw_ncclnt_cbmgr_t, rw_ncclnt_cbmgr_ptr_t)
RW_CF_TYPE_CLASS_EXTERN(rw_ncclnt_cbmgr_ptr_t)

/**
 * Retain a reference to a RW.Netconf callback manager.  When a
 * callback manager is created, the creating application will
 * automatically get one reference.  If the application wishes to share
 * the manager among several other objects, those other objects should
 * retain their own reference.  The library will not retain a reference
 * to the manager except while callbacks are waiting to be queued
 * (i.e., the time between request submission and response receipt).
 * The library will not retain a reference after queuing a callback,
 * even if the callback has not yet been dispatched.
 *
 * Use cases:
 *  - General library usage.  The application must specify how the
 *    library invokes callbacks to the application.
 */
void rw_ncclnt_cbmgr_retain(
  const rw_ncclnt_cbmgr_t* cbmgr ///< [in] The callback manager reference to retain
);

/**
 * Release a reference to a RW.Netconf callback manager.  The reference
 * should be released when the callback manager is no longer needed, or
 * when it has closed due to an error..  When the last reference is
 * released, the callback manager will be destroyed, possibly with all
 * uninvoked callbacks.
 *
 * Use cases:
 * - Closing a client library instance.
 * - Terminating a tasklet with associated callback managers.
 */
void rw_ncclnt_cbmgr_release(
  // ATTN: This, and all other release functions, should take pointer to pointer
  const rw_ncclnt_cbmgr_t* cbmgr ///< [in/out] The reference to release. Will be nulled.
);

/**
 * Terminate a RW.Netconf callback manager.  This function stops all
 * queued, but unsubmitted, callbacks from being invoked.  Please note
 * that this call DOES NOT guarantee that the underlying queuing
 * mechanism will not make any further callbacks - that behavior is
 * mechanism specific and may not be possible in all cases.  What this
 * API does guarantee is that any futher callback queuing attempts from
 * the library itself will be ignored.
 *
 * This API does not destroy the related objects, and does not release
 * the caller's reference to the callback manager.  This function may
 * release underlying implementation resources immediately.  After
 * making this call, the callback manager reference should not be used
 * again, except for passing to release.
 *
 * Please note - This API does not provide for a graceful close.  This
 * API provides a way to arbitrarily stop callbacks at anytime, which
 * may be useful for quick destruction of the callback manager.  But if
 * the callback context owns references to application resources, this
 * API can cause application resource leaks!  ATTN: Should there be a
 * way for the application to see these skipped callbacks, so that the
 * application can have the opportunity to release resources?
 *
 * Use cases:
 * - Closing a client library instance.
 * - Terminating a tasklet with associated callback managers.
 */
void rw_ncclnt_cbmgr_terminate(
  rw_ncclnt_cbmgr_t* cbmgr ///< [in] The callback manager reference to close
);

/**
 * Get the API instance from the callback manager reference.
 *
 * Use cases:
 * - Debug
 *
 * @return The API instance.  Not owned by the caller; use retain, if
 *   the reference will be kept.
 */
rw_ncclnt_instance_t* rw_ncclnt_cbmgr_get_instance(
  const rw_ncclnt_cbmgr_t* cbmgr ///< [in] The callback manager
);

/**
 * Create a polled-queue callback manager.  The callback manager will
 * queue all library callbacks into an internal queue.  The internal
 * queue will NOT be thread-safe - a single polled-queue callback
 * manager should only be used in a single threaded application.  The
 * callbacks to the application will be made only under application
 * control - the application mustcall a polling function, which will
 * invoke the oldest queue callback, if any.  No callbacks will be made
 * automatically.
 *
 * Use cases:
 * - Unit testing, without any RW.Sched support
 *
 * Anti-use cases:
 * - Unit tests running with RW.Sched support.
 * - RW.Cli
 * - RW.GUIServer
 *
 * @return The callback manager.  The caller already owns a reference.
 */
rw_ncclnt_cbmgr_t* rw_ncclnt_cbmgr_create_polled_queue(
  rw_ncclnt_instance_t* instance ///< [in] The owning API instance
);

/**
 * Invoke one callback from a callback manager.  This API may or may
 * not have any effect on the callback manager, depending on the
 * underlying implementation, as indicated by the return value.  If
 * this API is supported, then a single callback will be invoked (if
 * one is available), in the calling concurrency context.
 *
 * It is the application's responsibility to ensure that the current
 * context is acceptible for any callback that might be queued to the
 * callback manager.  If there is doubt, then the application likely
 * requires a callback manager per applicable context, it should avoid
 * using this function, or it should use a different callback manager
 * implementation.
 *
 * @return
 *  - RW_STATUS_SUCCESS: One callback was made.
 *  - RW_STATUS_FAILURE: Not supported.
 *  - RW_STATUS_NOTFOUND: Supported, but no callbacks are queued.
 */
rw_status_t rw_ncclnt_cbmgr_poll_once(
  rw_ncclnt_cbmgr_t* cbmgr ///< [in] The callback manager
);

/**
 * Invoke all callbacks from a callback manager.  This API may or may
 * not have any effect on the callback manager, depending on the
 * underlying implementation, as indicated by the return value.  If
 * this API is supported, then all available callbacks will be invoked,
 * in the calling concurrency context, in the order they were queued by
 * the library.  This function might never return, if each callback
 * causes another callback to be generated.  If this scenario is
 * possible for the application, then the application should use
 * rw_ncclnt_cbmgr_poll_once() and implement its own throttling.
 *
 * See the concurrency caveats described in
 * rw_ncclnt_cbmgr_poll_once().
 *
 * @return
 *  - RW_STATUS_SUCCESS: All callbacks were made.
 *  - RW_STATUS_FAILURE: Not supported.
 */
rw_status_t rw_ncclnt_cbmgr_poll_all(
  rw_ncclnt_cbmgr_t* cbmgr ///< [in] The callback manager
);

#if 0
/**
   ATTN: This is old, but still good information.  Need to change from
   being a property of the instance to being a callback manager and
   providing a create function.
 *
   ATTN: Also, does it makes sense for the library and/or session to
   each have their own callback managers?  Right now they have no need
   to perform asynchronous activities on their own, but they might need
   to someday?
 *
 * Bind an instance to a particular RW.Sched queue.  All callbacks
 * made for the instance will be made in the requested context.
 *
 * Use cases:
 * - RW.Cli for binding callbacks to the main thread.
 *
 * Anti-use cases:
 * - Unit tests might run in a single thread without RW.Sched support.
 * - RW.MgmtAgent, which will probably bind individual sessions to
 *   different RW.Sched queues.
 * - RW.GUIServer, which does not use RW.Sched.  ATTN: Is this true -
 *   if RW.GUIServer uses RW.Msg transport, then I think RW.GUIServer
 *   has to use RW.Sched.  Maybe RW.GUIServer uses a direct Unix-socket
 *   transport like the current RW.uAgent interface...  If so, then
 *   there probably needs to be a session-to-thread binding API in
 *   addition to a session-to-RW.Sched binding API.
 *
 * ATTN: Will this will create a dependency problem for unit tests?  If
 *   the code for RW.Netconf is in core/mgmt, then there is no build
 *   dependency problem.  But there still may be a linking problem -
 *   this API will require linking to rwvx, which we might want to
 *   suppress for unittests and GUI.
 *
 * @return
 * - RW_STATUS_SUCCESS: binding was successful.
 * - Others: binding was not successful.
 */
rw_status_t rw_ncclnt_instance_bind_rwsched(
  rw_ncclnt_instance_t* instance ///< [in] The instance to bind
  /// ATTN: RW.Sched args TBD
);
#endif



/** @} */


/*****************************************************************************/
/**
 * @defgroup RwNcSession RW.Netconf Sessions
 * @{
 *
 * A Session is an authenticated session with a NC server entity, as
 * implemented by a specific RW.Netconf transport layer.  The server
 * entity may be a real netconf server, internal messaging to a RW
 * tasklet, or a server object in the same process.
 *
 * A connection is bound to a specific user, which may have been
 * authenticated by the application, or may need to be authenticated by
 * the server, depending on the transport.  A RW.Netconf client API
 * instances may have 0 or more connection sessions.  multiple
 * connections.
 *
 * ATTN: talk about locks
 * ATTN: talk about commit
 * ATTN: talk about message-id
 * ATTN: talk about capabilities
 *
 * ATTN: APIs for features
 * ATTN: APIs for capabilities
 * ATTN: APIs to get schemas
 *
 * Session state may include
 *  - reference to the API instance
 *  - RW.NCTransport reference
 *  - reference to the bound user
 *  - activity timers
 *  - references to the owned (partial) locks
 *  - internal messaging data
 *  - a reference to a server object
 *  - statistics
 *  - message id
 *  - other connection-related data
 */

/**
 * Session Handle
 *
 * The handle is a managed object that represents a connected session.
 * The session and connection do not exist independently of the handle;
 * when the handle is released, the session and connection are
 * destroyed.  A session cannot survive process failure.
 */
struct rw_ncclnt_ses_s {};
RW_TYPE_DECL_ALL(rw_ncclnt_ses_s, rw_ncclnt_ses_t, rw_ncclnt_ses_ptr_t)
RW_CF_TYPE_CLASS_EXTERN(rw_ncclnt_ses_ptr_t)


/**
 * Retain a reference to a RW.Netconf client session.  When a session
 * is created, the creating application will get a callback with the
 * session handle, but the handle is owned the by library at that point
 * in time.  Therefore, the application should return a reference to
 * the session from within the callback.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - The application, on successful session create callback.
 */
void rw_ncclnt_ses_retain(
  const rw_ncclnt_ses_t* ses ///< [in] The session reference to retain
);

/**
 * Release a reference to a RW.Netconf client session.  The reference
 * should be released when the session is no longer needed, or when it
 * has closed due to an error..  When the last reference is released,
 * the API object will be destroyed.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Closing a session.
 * - Terminating a tasklet with open sessions.
 */
void rw_ncclnt_ses_release(
  const rw_ncclnt_ses_t* ses ///< [in/out] The reference to release. Will be nulled.
);

/**
 * Terminate a RW.Netconf client session.  This function closes the
 * connection, which prevents it from being used again for more
 * requests.  It does not destroy the related objects, and does not
 * release the caller's reference to the session.  This function may
 * release underlying resources immediately.  After making this call,
 * the session reference should not be used again, except for passing
 * to release.
 *
 * This API provides a way to arbitrarily stop processing a session at
 * anytime, which may be useful for quick destruction of the session.
 * This API does not provide for a graceful close - @see
 * rw_ncclnt_ses_disconnect().
 *
 * ATTN: how does it ensure that all related objects are released?
 *
 * Use cases:
 * - Closing a session.
 * - Terminating a tasklet with open sessions.
 */
void rw_ncclnt_ses_terminate( //ATTNSTEP.Debug
  rw_ncclnt_ses_t* ses ///< [in] The session reference to close
);



/// ATTN: Better comments here
/// Callback for session creation.
typedef void (*rw_ncclnt_ses_connect_cb)(
  rw_ncclnt_context_t context, ///< [in] (Copy of) the context provided the original request
  rw_yang_netconf_op_status_t error, ///< [in] The error, if any.  _OK if successful
  rw_ncclnt_ses_t* ses ///< [in] On success, the newly created session handle
);

typedef struct rw_ncclnt_ses_connect_context_s
{
  /// The application context.  Will be returned in the callback.
  rw_ncclnt_context_t context;

  /// The callback function.
  rw_ncclnt_ses_connect_cb callback;

  /// The concurrency context to make the callback in.
  rw_ncclnt_cbmgr_ptr_t cbmgr;
} rw_ncclnt_ses_connect_context_t;


/**
 * Establish a connection to a server by using standard Netconf over
 * SSH - RFC 6242.  The server will authenticate the user, using the
 * provided credentials.
 *
 * ATTN: need to select NC/SSH library: libnetconf, libncx, or
 * proprietary.
 *
 * ATTN: ssh library may need its own callbacks, such as for
 * user-interactive or password
 *
 * Use cases:
 * - Establish a RW.Cli netconf over SSH session from outside RiftWare.
 *
 * @return
 * - RW_STATUS_SUCCESS - The request was enqueued, response will occur
 *   in callback.
 * - Others: request could not be satisfied; no callback will be made
 *   and no session was established.
 */
rw_status_t rw_ncclnt_ses_connect_nc_ssh( //ATTNSTEP.NC.SSH
  rw_ncclnt_instance_t* instance, ///< [in] The API instance to bind
  rw_ncclnt_user_t* user, ///< [in] The (current) (unauthenticated) user
  const char* host, ///< [in] The netconf server hostname
  rw_ncclnt_ssh_credentials_t* cred, ///< [in] The SSH credentials to use. ATTN: Put in user?
  rw_ncclnt_ses_connect_context_t context ///< [in] Callback and context information. Will make a copy and retain references.
);


#if 0
/**
 * Establish a connection to a RW.MgmtAgent server instance over RW.Msg
 * transport.  Transport APIs are serialized to protobuf messages and
 * sent via RW.Msg to the specified RW.MgmtAgent or RW.uAgent instance.
 *
 * Use cases:
 * - Internally invoked RW.Cli.
 * - RW.GUIServer if able to integrate with RW.Sched.
 * - System tests.
 *
 * @return
 * - RW_STATUS_SUCCESS - The request was enqueued, response will occur
 *   in callback.
 * - Others: request could not be satisfied; no callback will be made
 *   and no session was established.
 */
rw_status_t rw_ncclnt_ses_connect_rwmsg( //ATTNSTEP.RW.Msg.Base
  rw_ncclnt_instance_t* instance, ///< [in] The API instance to bind
  rw_ncclnt_user_t* user, ///< [in] The pre-authenticated user
  const rw_ncxapi_rwmsg_dest_t* rwmsg_dest, ///< [in] Definition of the RW.Msg destination.  ATTN: Contents TBD
  // ATTN: Logical RW.MgmtAgent ID?
  rw_ncclnt_ses_connect_context_t context ///< [in] Callback and context information. Will make a copy and retain references.
);
#endif


#if 0
/// Callback for session disconnect.
typedef void (*rw_ncclnt_ses_disconnect_cb)(
  rw_ncclnt_context_t context, ///< [in] (Copy of) the context provided the original request
  rw_yang_netconf_op_status_t error ///< [in] The error, if any.  _OK if successful
);

typedef struct rw_ncclnt_ses_disconnect_context_s
{
  /// The application context.  Will be returned in the callback.
  rw_ncclnt_context_t context;

  /// The callback function.
  rw_ncclnt_ses_disconnect_cb callback;

  /// The concurrency context to make the callback in.
  rw_ncclnt_cbmgr_ptr_t cbmgr;
} rw_ncclnt_ses_disconnect_context_t;

/**
 * Disconnect from a RW.MgmtAgent server.  This API does a graceful
 * close, as appropriate for the transport.  It does not release the
 * reference.
 *
 * Use cases:
 * - Close a connection for any normal reason.
 *
 * @return
 * - RW_STATUS_SUCCESS - The request was enqueued; disconnection status
 *   will be returned in the callback.
 * - Others: request could not be satisfied; no callback will be made
 *   and no session was established.
 */
rw_status_t rw_ncclnt_ses_disconnect( //ATTNSTEP.RW.uAgent
  rw_ncclnt_ses_t* ses, ///< [in] The session to disconnect
  rw_ncclnt_ses_disconnect_context_t context ///< [in] Callback and context information. Will make a copy and retain references.
);
#endif


/**
 * Get the API instance from the session reference.
 *
 * Use cases:
 * - Debug
 *
 * @return The API instance.  Not owned by the caller; use retain, if
 *   the reference will be kept.
 */
rw_ncclnt_instance_t* rw_ncclnt_ses_get_instance( //ATTNSTEP.Debug
  const rw_ncclnt_ses_t* ses ///< [in] The session
);

/**
 * Get the user associated with a session.
 *
 * Use cases:
 * - Debug
 *
 * @return The user.  Not owned by the caller; use retain, if the
 *   reference will be kept.
 */
rw_ncclnt_user_t* rw_ncclnt_ses_get_user( //ATTNSTEP.GUI
  const rw_ncclnt_ses_t* ses ///< [in] The session
);

#if 0
/**
 * Bind a session to a particular RW.Sched queue.  All callbacks made
 * for the session will be made in the requested context.
 *
 * Use cases:
 * - RW.MgmtAgent for binding callbacks for each session.
 *
 * Anti-use cases:
 * - Unit tests might run in a single thread without RW.Sched support.
 * - RW.Cli
 * - RW.GUIServer
 *
 * @return
 * - RW_STATUS_SUCCESS: binding was successful.
 * - Others: binding was not successful.
 */
rw_status_t rw_ncclnt_ses_bind_rwsched(
  rw_ncclnt_ses_t* ses ///< [in] The session
  /// ATTN: RW.Sched args TBD
);
#endif

/** @} */


/*****************************************************************************/
/**
 * @defgroup RwNcXml RW.Netconf XML Blobs
 * @{
 *
 * XML blob handle for NC client library.  A single blob has both a
 * producer and a consumer - the producer places data into the XML
 * blob, and the consumer reads it back out.  A blob can be consumed
 * exactly once - XML constants that are reused repeatedly require a
 * new XML blob for each use.
 *
 * XML blob handle state may include:
 *  - reference to the client library instance
 *  - a reference to the XML source
 *  - a cursor into the XML blob or stream
 *  - a reference to the generator
 *  - error status
 *  - statistics
 *
 * Multiple references to a blob are allowed.  Generally, the library
 * and the application code will both share one reference each to any
 * particular blob.  Additional references to a single blob do not make
 * much sense - the producer should have exactly one reference, and the
 * consumer should have exactly one reference.  Multiple producers
 * sharing a blob could easily make a malformed XML fragment.  Multiple
 * consumers sharing a blob would each get partial or null results.
 * XML blobs may be composed of other blobs, which requires taking
 * additional references.  Howeve,r in such a case, the composite blob
 * must be considered to take ownership of the embedded blob(s).
 *
 * ATTN: What about anyxml extraction from responses?  How is that
 * exposed as its own XML blob?  It cannot be directly parsed into a
 * DOM, because that assumes sufficient memory.
 *
 * A XML blob may take many forms: a const or run-time string or memory
 * buffer, a full-fledged plain-XML or yang-enhanced DOM, or a byte
 * stream generated by a file or transport.  The abstraction allows the
 * client to use the form they prefer and allows the transport to be
 * asynchronous.  At no time does the library absolutely require that
 * an entire XML document be stored in memory, although that may occur
 * for small documents or documents based on strings.
 *
 * A XML blob may used under any concurrency scheme under the following
 * strict circumstances:
 *  - A XML blob may be passed between consumer threads or queues only
 *    when there are no outstanding calls to any consume functions.
 *  - A XML blob may be passed between producer threads or queues only
 *    when there are no outstanding calls to any produce functions.
 *  - Release, retain, and terminate may be called at any time,
 *    although they are not guarenteed to take effect until any or all
 *    outstanding producer or consumer calls complete.
 * The library shall guarantee that these rules are followed for any
 * XML blobs passed into the library (netconf requests), or passed back
 * to the application (netconf responses).  Applications must ensure
 * that they also follow these rules.
 *
 * Because exactly one thread or queue will ever write to or read from
 * the same XML blob at the same time, the RW.Netconf XML
 * implementation need not provide synchronization within the set of
 * producer APIs, nor within the set of consumer APIs.  However, if the
 * production of XML is actually asynchronous with respect to the
 * consumption, then the internal state of the generator must have
 * suitable synchronization.  The users of the RW.Netconf library can
 * assume this synchronization is provided, and no extra steps are
 * needed other than following the concurrency rules, above.
 *
 * Rationale: Depending on the RW.NCTransport API, a XML blob may be
 * passed from the thread that created the blob to another thread via
 * the netconf client API.  For example, the direct function call or
 * RW.Sched transports could transfer the XML blob reference between
 * threads.
 *
   ATTN: establishing asynchrony between producer and consumer requires
   knowledge of the concurrency API.  Can unittests assume RW.Sched?
 *
   ATTN: Want to be able to put one RW XML/Yang dom within another, at
   a specified node.  How can we do that?
    - derive from rwxml a new class that maps one dom and contains
      decorations for another dom
    - change rwxml to use the decorator pattern, like I should have
      done originally, and then add a new decorator for
      "incorporated-by-reference" nodes.
    - annotate the yang to indicate when a reroot a dom...  is that
      even needed?
 */

/**
   Element:
     to visitor
     from visitor
     pbcmd
     pbcfd
     YangNode
     yang_path
     parent can be dynamically from another document

   ATTN: In order to pass XMLfragments between objects, it may be
   necessary to "give them back" when done with them.
    - This can be accomplished with a decorator that transfers
      ownership temporarily.  When the consumer destroys the node, the
      original owner gets the node back

   ATTN: Need to be able to apply operations to nodes - pass them an
   active object that produces outputs.
   ATTN: But the active object must be passable between tasklets, so it
   must be fundementally serializable.

   ATTN: Need to be able to walk a yang path with keys
   ATTN: Subtree filtering
   ATTN: XPATH filtering
   ATTN: NACM filtering

   ATTN: Need to be able to decorate a node with registrations
   ATTN: And, therefore, need ot be able to construct a registration
   ATTN: Which may mean being able to query for one's place in the tree

   ATTN: Need to be able to create a virtual tree
   ATTN: For candidate, copy-config to running, validate...

   ATTN: Need to be able to run map-reduce and xquery type operations on the dom

   ATTN: Need to be able to aggregate
   ATTN: Need to be able to add comments

   ATTN: Defaults need to appear in the DOM where defined, even if the
   node was not created.  ATTN: Are there cases where the default value
   can be deleted from a config?  Or can defaults only ever be
   replaced?

   actions:
     create
     change
     read
     delete

   ATTN: I think the registrations and NACM can be best acheived with
   decorators, because there is no interface change.
   ATTN: Redirect is also likely to benefit from decorator pattern.
   ATTN: Optmization nodes may also benefit from using the decorator
   pattern, as they, too, present the same interface

   ATTN: The nodes in the tree need to be able to share decorators?
   When a VNODE gets created, some of the other decorators might be
   needed....  Is a VNODE ALSO a decorator, but one that has two forks?

     +------------+
     | base YXMLN | <-- Contains the children, ynode pbdesc, data, attributes
     +------------+
           ^
     +------------+
     | NACM Dec   | <-- Applies CRUD filtering
     +------------+
           ^
     +------------+-----------+
     | Orig DOM   | VCOW DOM  | <- VCOW replaces children, maybe other things too
     +------------+-----------+
           ^            ^
           :       +-----------+
           :       | list perf |
           :       +-----------+
           :             ^
     +------------+ +-----------+
     | Node       | | VNode     |
     +------------+ +-----------+

   DOM:: apply operation
     node:: apply operation

 */


/**
 * XML Blob Handle
 */
struct rw_ncclnt_xml_s {};
RW_TYPE_DECL_ALL(rw_ncclnt_xml_s, rw_ncclnt_xml_t, rw_ncclnt_xml_ptr_t)
RW_CF_TYPE_CLASS_EXTERN(rw_ncclnt_xml_ptr_t)

/**
 * Retain a reference to a RW.Netconf XML fragment.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Library or application needs to hold on to a fragment passed from
 *   the other.
 */
void rw_ncclnt_xml_retain(
  rw_ncclnt_xml_t* xml ///< [in] The XML fragment reference to retain
);

/**
 * Release a reference to a RW.Netconf XML fragment.  The reference
 * should be released when the XML fragment is no longer needed.  When
 * the last reference is released, the XML fragment object will be
 * destroyed.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Done with a fragment.
 */
void rw_ncclnt_xml_release(
  rw_ncclnt_xml_t* xml ///< [in/out] The reference to release. Will be nulled.
);

/**
 * Terminate consumption of an RW.Netconf XML blob.  This function the
 * prevents the blob from being used again for more requests.  It might
 * release references to related objects and/or close any underlying
 * streams, but it does not release the caller's reference to the XML
 * blob.  After making this call, the XML blob reference should not be
 * used again, except for passing to release.
 *
 * This API provides a way to arbitrarily stop processing a XML blob at
 * anytime, which may be useful for quick destruction of the XML blob.
 *
 * Use cases:
 * - Terminating a netconf session with an active request or response.
 */
void rw_ncclnt_xml_terminate( //ATTNSTEP.Debug
  rw_ncclnt_xml_t* xml ///< [in] The XML blob to terminate
);


/**
 * Create an empty XML blob.  The blob will serialize to nothing.
 *
 * Use cases:
 * - Unit testing.
 * - Error returns.
 * - Default values.
 *
 * @return XML blob handle.  Caller owns the handle.  ATTN: null on
 *   error?  ATTN: Cannot fail?
 */
rw_ncclnt_xml_t* rw_ncclnt_xml_create_empty( //ATTNSTEP.Base
  rw_ncclnt_instance_t* instance ///< [in] The owning library instance
);

/**
 * Create a XML blob from a UTF-8 string.  The string will be copied
 * into the XML object, so it may be allocated on the stack.
 *
 * Use cases:
 * - Generating XML from external application strings, or from
 *   stack-local variables.
 *
 * @return XML blob handle.  Caller owns the handle.  ATTN: null on
 *   error?  ATTN: Cannot fail?
 */
rw_ncclnt_xml_t* rw_ncclnt_xml_create_copy_string( //ATTNSTEP.Base
  rw_ncclnt_instance_t* instance, ///< [in] The owning library instance
  const char* string ///< [in] The NUL-terminated string to copy
);

/**
 * Create a XML blob from a read-only, compile-time const UTF-8 string.
 * The string will be referred to directly by the pointer passed in;
 * therefore, it must live at least as long as the XML object.  This
 * API should really only be used for static const data, or other
 * global data that is expected to live forever.
 *
 * Use cases:
 * - Generating XML from static const strings.
 *
 * @return XML blob handle.  Caller owns the handle.  ATTN: null on
 *   error?  ATTN: Cannot fail?
 */
rw_ncclnt_xml_t* rw_ncclnt_xml_create_const_string(//ATTNSTEP.Base
  rw_ncclnt_instance_t* instance, ///< [in] The owning library instance
  const char* string ///< [in] The NUL-terminated string to reference
);

/**
 * Create a XML blob from a UTF-8 string buffer.  The string will be copied
 * into the XML object, so it may be allocated on the stack.
 *
 * Use cases:
 * - Generating XML from external application buffers, or from
 *   stack-local variables.
 *
 * @return XML blob handle.  Caller owns the handle.  ATTN: null on
 *   error?  ATTN: Cannot fail?
 */
rw_ncclnt_xml_t* rw_ncclnt_xml_create_copy_buffer(
  rw_ncclnt_instance_t* instance, ///< [in] The owning library instance
  const void* buffer, ///< [in] The length-counted buffer to copy
  size_t buflen ///< [in] The length of buffer
);

/**
 * Create a XML blob from a read-only, compile-time const UTF-8 string
 * buffer.  The string will be referred to directly by the pointer
 * passed in; therefore, it must live at least as long as the XML
 * object.  This API should really only be used for static const data,
 * or other global data that is expected to live forever.
 *
 * Use cases:
 * - Generating XML from static const buffers.
 *
 * @return XML blob handle.  Caller owns the handle.  ATTN: null on
 *   error?  ATTN: Cannot fail?
 */
rw_ncclnt_xml_t* rw_ncclnt_xml_create_const_buffer(
  rw_ncclnt_instance_t* instance, ///< [in] The owning library instance
  const void* buffer, ///< [in] The length-counted buffer to reference
  size_t buflen ///< [in] The length of buffer
);

/**
 * Create a XML blob from a prefix string, a XML blob, and a suffix
 * string.  The prefix and suffix provide enclosing XML fragment
 * strings that are not proper XML by themselves, but together form a
 * valid XML fragment.  The prefix and suffix strings will not be
 * validated.  The XML blob must be a valid XML fragment; it will not
 * be validated, either.  The net result is that the XML blob gets
 * inserted as a child of the XML fragment defined by the prefix and
 * suffix strings.
 *
 * For example, given the following inputs:
 * @code
 *   prefix = "<root>";
 *   blob === "<data>string</data>";
 *   suffix = "</root>";
 * @endcode
 *
 * The end result would be:
 * @code
 *   result === "<root><data>string</data></root>";
 * @endcode
 *
 * Use cases:
 * - Library composing RPC messages from rpc header, request body XML
 *   blob, and rpc trailer.
 *
 * @return XML blob handle.  Caller owns the handle.  Will retain a
 *   reference to body.  ATTN: null on error?  ATTN: Cannot fail?
 */
rw_ncclnt_xml_t* rw_ncclnt_xml_create_composed( //ATTNSTEP.Base
  rw_ncclnt_instance_t* instance, ///< [in] The owning library instance
  const char* prefix, ///< [in] The NUL-terminated prefix string
  rw_ncclnt_xml_t* body, ///< [in] The XML body.
  const char* suffix ///< [in] The NUL-terminated suffix string
);

/**
 * Create a XML blob from a prefix blob and a suffix blob.  The prefix
 * and suffix blobs must be a valid XML fragments, although they will
 * not be validated.  The net result is that the XML blobs become
 * siblings in the result blob.
 *
 * For example, given the following inputs:
 * @code
 *   prefix === "<data>string1</data>";
 *   suffix === "<data>string2</data>";
 * @endcode
 *
 * The end result would be:
 * @code
 *   result === "<data>string1</data><data>string2</data>";
 * @endcode
 *
 * Use cases:
 * - Library composing RPC message bodies from multiple parameters,
 *   such as get-config's datastore target and filter arguments.
 *
 * @return XML blob handle.  Caller owns the handle.  Will retain a
 *   reference to prefix and suffix.  ATTN: null on error?  ATTN:
 *   Cannot fail?
 */
rw_ncclnt_xml_t* rw_ncclnt_xml_create_append( //ATTNSTEP.Base
  rw_ncclnt_instance_t* instance, ///< [in] The owning library instance
  rw_ncclnt_xml_t* prefix, ///< [in] The prefix XML blob
  rw_ncclnt_xml_t* suffix ///< [in] The suffix XML blob
);

/**
 * Create a XML blob from a RW Yang-XML document fragment.  The XML
 * subtree will be incorporated into any output as a concatenation of
 * all nodes, in the tree's ordinary traversal order.
 *
 * The subtree MUST remain unmodified until the XML blob is destroyed.
 * For an application-created blob, it is sufficient for the
 * application to wait until the (the start of the) repsonse is
 * received for the RW.Netconf request API the blob was passed to.
 * ATTN: How do we know that happened for responses passed back to the
 * application?
 *
 * ATTN: How can the RW.Netconf library maintain a reference to the
 * node?
 *
 * Use cases:
 * - RW.Cli passing a parse tree-generated XML tree to a netconf API.
 *
 * @return XML blob handle.  Caller owns the handle.  ATTN: null on
 *   error?  ATTN: Cannot fail?
 */
rw_ncclnt_xml_t* rw_ncclnt_xml_create_xml_yang_subtree( //ATTNSTEP.Cli
  rw_ncclnt_instance_t* instance, ///< [in] The owning library instance
  rw_xml_yang_node_t* node, ///< [in] The subtree root node
  unsigned depth_limit ///< [in] An optional limit to the serialization depth; 0=unlimited
);

/**
 * Create a XML blob from a RW XML document fragment.  The XML subtree
 * will be incorporated into any output as a concatenation of all
 * nodes, in the tree's ordinary traversal order.
 *
 * The subtree MUST remain unmodified until the XML blob is destroyed.
 * For an application-created blob, it is sufficient for the
 * application to wait until the (the start of the) repsonse is
 * received for the RW.Netconf request API the blob was passed to.
 * ATTN: How do we know that happened for responses passed back to the
 * application?
 *
 * ATTN: How can the RW.Netconf library maintain a reference to the
 * node?
 *
 * Use cases:
 * - ATTN
 *
 * @return XML blob handle.  Caller owns the handle.  ATTN: null on
 *   error?  ATTN: Cannot fail?
 */
rw_ncclnt_xml_t* rw_ncclnt_xml_create_xml_subtree(
  rw_ncclnt_instance_t* instance, ///< [in] The owning library instance
  rw_xml_node_t* node, ///< [in] The subtree root node
  unsigned depth_limit ///< [in] An optional limit to the serialization depth; 0=unlimited
);

/**
 * Get the API instance from the XML blob reference.
 *
 * Use cases:
 * - Debug
 *
 * @return The API instance.  Not owned by the caller; use retain, if
 *   the reference will be kept.
 */
rw_ncclnt_instance_t* rw_ncclnt_xml_get_instance( //ATTNSTEP.Debug
  const rw_ncclnt_xml_t* xml ///< [in] The XML blob
);


/**
 * Consume (a portion of) a XML blob, by copying data from the blob
 * into the specified buffer.
 *
 * This function may be called from inside the
 * rw_ncclnt_xml_consume_wait_producer() callback, or when no
 * rw_ncclnt_xml_consume_wait_producer() callback is outstanding.  It is
 * a race condition to call this function between a call to
 * rw_ncclnt_xml_consume_wait_producer() and the resulting callback!
 *
 * Use cases:
 * - General library and application usage of XML.
 *
 * @return Status of the consumption:
 * - ATTN_OK upon success
 * - Others: errors
 */
rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_buffer( //ATTNSTEP.Base
  rw_ncclnt_xml_t* xml, ///< [in] The XML blob to consume from
  void* buffer, ///< [in] The buffer to copy to
  size_t buflen, ///< [in] The length of buffer
  size_t* outlen ///< [out] Upon success, the number of bytes copied
);

/*
 * Obtain a direct pointer to the current XML blob data.  This function
 * can be used to avoid a double copy during transmit, which may be
 * advantageous for some transport layers.
 *
 * This function may be called from inside the
 * rw_ncclnt_xml_consume_wait_producer() callback, or when no
 * rw_ncclnt_xml_consume_wait_producer() callback is outstanding.  It is
 * a race condition to call this function between a call to
 * rw_ncclnt_xml_consume_wait_producer() and the resulting callback!
 *
 * The returned pointer is only valid until the next
 * rw_ncclnt_xml_consume* call, or until return from the invoking
 * function.  In order to actually consume the bytes exposed by the
 * function, the caller must also call rw_ncclnt_xml_consume_bytes()
 *
 * ATTN: How can this API make synchronization guarantees?
 *
 * ATTN: How do you know that a generator supports this API?  Just try
 * it and see if it works?  Can the consumer loop be abstracted, such
 * that the choice between buffer and pointer based iteration becomes a
 * pluggable component?
 *
 * Use cases:
 * - Transport layer double copy elimination (e.g., passing to send()).
 *
 * @return Status of the consumption:
 * - ATTN_OK upon success
 * - Others: errors
 */
rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_buffer_pointer(
  rw_ncclnt_xml_t* xml, ///< [in] The XML blob to consume from
  const void** outbuf, ///< [out] Upon success, set to the direct buffer pointer
  size_t* outlen ///< [out] Upon success, the number of bytes in outbuf
);

/*
 * Consume a number of bytes, which were previously accessed via the
 * rw_ncclnt_xml_consume_buffer_pointer() API.
 *
 * Calls to this function must follow matching calls to
 * rw_ncclnt_xml_consume_buffer_pointer(), from the same calling
 * function invocation.
 *
 * Use cases:
 * - Transport layer double copy elimination (e.g., passing to send()).
 *
 * @return Status of the consumption:
 * - ATTN_OK upon success
 * - Others: errors
 */
rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_bytes(
  rw_ncclnt_xml_t* xml, ///< [in] The XML blob to consume from
  size_t consume ///< [in] The number of bytes to consume
);

/**
 * Callback for consumer waiting on producer to generate more XML data.
 * When the consumer wants more XML data, but there is no data
 * available, the consumer must call
 * rw_ncclnt_xml_consume_wait_producer().  When the producer finally
 * creates some more data (or closes, or errors), a callback of this
 * type will be made into the consumer.  The consumer will then
 * continue consuming data.
 */
typedef void (*rw_ncclnt_xml_consume_wait_cb)(
  rw_ncclnt_context_t context, ///< [in] (Copy of) the context provided the original request
  rw_yang_netconf_op_status_t error ///< [in] The error, if any.  _OK if successful
);

typedef struct rw_ncclnt_xml_consume_wait_context_s
{
  /// The application context.  Will be returned in the callback.
  rw_ncclnt_context_t context;

  /// The callback function.
  rw_ncclnt_xml_consume_wait_cb callback;

  /// The concurrency context to make the callback in.
  rw_ncclnt_cbmgr_ptr_t cbmgr;
} rw_ncclnt_xml_consume_wait_context_t;


/**
 * Wait for XML blob data to become available.  When data becomes
 * available the callback will be made.  This function can be called
 * even when data is already available, in which case the callback will
 * happen (almost) immediately.
 *
 * This function may be called from inside another
 * rw_ncclnt_xml_consume_wait_producer() callback, or when no
 * rw_ncclnt_xml_consume_wait_producer() callback is currently
 * outstanding.  It is a race condition to call this function between a
 * call to rw_ncclnt_xml_consume_wait_producer() and the resulting
 * callback!
 *
 * Use cases:
 * - NC/SSH netconf XML handling.
 *
 * @return
 * - RW_STATUS_SUCCESS - The request was enqueued; data available
 *   status will be returned in the callback.
 * - Others: request could not be satisfied; no callback will be made
 *   and no data will be provided.
 */
rw_status_t rw_ncclnt_xml_consume_wait_producer( //ATTNSTEP.NC.SSH
  rw_ncclnt_xml_t* xml, ///< [in] The XML blob to consume from
  rw_ncclnt_xml_consume_wait_context_t ///< [in] Context and callback. Will make a copy and retain references.
);



#if 0
/*
 * ATTN: There should probably be sax-ish APIs, so that we can stream
 * very large XML blobs, with some amount of yang-based validation
 * applied, where we don't have to parse the XML twice.
 *
 * ATTN: Maybe that also requires that there be native-DOM consumers,
 * where the consumer ends up with a DOM once consumption has
 * completed.  Somehow the sink would have to indicate what kind of
 * consumer it wanted when making/receiving the original netconf
 * request.
 */
// Check if there is sax data to read already available in the XML object,
// and return what it can
rw_status_t rw_ncclnt_xml_consume_sax_sync(rw_ncclnt_xml_t, rw_xml_node_t** node);

// Get more sax from the XML object
rw_status_t rw_ncclnt_xml_consume_sax_async(rw_ncclnt_xml_t, cb, context);
#endif


/*
 * Check if there is has been an error on the XML blob.  Once a blob
 * has an error, it will not return any more data and will always
 * return the same error status.
 *
 * Errors can happen on XML blobs if the blob is not completely
 * produced at create time, such as when it comes from a generator.
 *
 * Use cases:
 * - General good coding practice: check error state.
 *
 * @return Status of the consumption:
 * - ATTN_OK if the XML blob is okay
 * - Others: errors
 */
rw_yang_netconf_op_status_t rw_ncclnt_xml_consume_get_producer_error( //ATTNSTEP.Base
  rw_ncclnt_xml_t* xml ///< [in] The XML blob to check
);

/*
 * Check if the end of the XML blob has been reached.  When there is an
 * error, the blob will report EOF.
 *
 * Use cases:
 * - Fundemental API for XML blobs.
 *
 * @return true if there is no mode data; false if there is more data.
 */
bool rw_ncclnt_xml_consume_is_eof( //ATTNSTEP.Base
  rw_ncclnt_xml_t* xml ///< [in] The XML blob to check
);

/*
 * Indicate that the XML consumer has encountered an error, either with
 * the XML itself, or with the sink the XML is being sent to.  Once an
 * error has been set, EOF will always return true.  The consumer
 * should not attempt to consume more data after setting an error.
 * Check if there is has been an error on the XML blob.  Once a blob
 * has an error, it will not return any more data and will always
 * return the same error status.
 *
 * Use cases:
 * - NC/SSH netconf error handling.
 *
 * @return Status of the consumption:
 * - ATTN_OK if the XML blob is okay
 * - Others: errors
 */
void rw_ncclnt_xml_consume_set_error( //ATTNSTEP.NC.SSH
  rw_ncclnt_xml_t* xml, ///< [in] The XML blob to error
  rw_yang_netconf_op_status_t status ///< [in] The status to set.  Cannot be ATTN_OK.
);

/** @} */


/*****************************************************************************/
/**
 * @defgroup RwNcXmlGen RW.Netconf XML Blob Generators
 * @{
 *
 * An XML Blob Generator is an active object that generates XML data on
 * demand, asynchronously from XML consumption.  See @ref RwNcXml for
 * general information about XML blobs.
 *
 * There are four conceptual actors involved in an XML Blob Generator.
 * Two of the actors are actors are external to the RW.Netconf library
 * and/or the RiftWare system, while two are part of the library.  The
 * actors are:
 *  - Source: The original source of the XML blob (such as a file,
 *    socket, library interface, or string).  The source is outside of
 *    the client library.
 *  - Producer: The RW.Netconf library object that implements XML blob
 *    production, by interfacing with the external source.  Two
 *    examples of producers are the client-side NC/SSH transport
 *    response, and the server-side NC/SSH transport request.
 *  - Consumer: The RW.Netconf library or application object that
 *    implements XML blob consumption, by interfacing with the external
 *    sink.  Two examples of producers are the client-side NC/SSH
 *    transport request, and the server-side NC/SSH transport response.
 *  - Sink: The ultimate destination of the XML blob (such as a file,
 *    socket, library interface, or string).  The sink is outside of the
 *    client library.
 *
 * Actors and flows:
 * <TT><PRE>
 *          <--- FC ----          <--- FC ----          <--- FC ----
 *   Source              Producer              Consumer              Sink
 *          --- Data -->          --- Data -->          --- Data -->
 * </PRE></TT>
 *
 * The source and sink are connected via the producer and consumer.
 * Between each actor, data flows in the source to sink direction.  At
 * any particular actor, the data flow can be in one of three states:
 * waiting (empty), satisfied (flowing), or blocked (flowed-off).
 * Additionally, between each actor, backpressure is applied in the
 * sink to source direction.  Each actor can be in one of 2 flow
 * control states: flowed-off (not allowed to generate more data) or
 * flowed-on (allowed to generate more data).
 *
 * Source-Producer Interactions:
 *  - How does the source indicate to the producer that there is more data
 *    available?
 *     - It is up to the source implementation to decide this interface.
 *       For example, a socket source would just send() more data.
 *  - How does the source wait for the producer to catch up?
 *     - It is up to the source implementation to decide this interface.
 *       For example, a socket source could block on send(), or use
 *       epoll() to detect the ready-to-write condition.
 *  - How does the producer flow-off the source?
 *     - It is up to the producer implementation to decide this interface.
 *       For example, with a socket source, the producer would most likely
 *       just stop recv()ing data from the socket, probably by virtue of
 *       waiting on the consumer to accept more data.
 *  - How does the producer flow-on the source?
 *     - It is up to the producer implementation to decide this interface.
 *       For example, with a socket source, the producer would most likely
 *       just keep recv()ing data from the socket.
 *  - How does the producer wait for more data from the source?
 *     - It is up to the producer implementation to decide this interface.
 *       For example, with a socket source, the producer would
 *       (implicitly) use epoll() to detect the ready-to-read condition on
 *       the socket.  Meanwhile, it is not generating data for the
 *       consumer, which may also starve.
 *
   Producer-Consumer Interactions:
    - How does the producer indicate to the consumer that there is data
      available?
       - The producer calls rw_ncclnt_xmlgen_produce_buffer().  The
         generator takes care of proving that data to the consumer.
    - How does the producer wait for the consumer to catch up?
       - The producer calls rw_ncclnt_xmlgen_produce_wait_consumer().
         The generator takes care of making the callback to continue
         production, once the consumer has signalled a readiness to
         consume more data.
    - How does the consumer flow-off the producer?
       - By not consuming.  When the consumer is not consuming, it is
         most likely because it has been flowed-off by the sink.  At
         that time, the consumer is probably waiting for the sink to be
         ready to accept more data.  Eventually, if the producer
         continues to produce, there will come a point where the
         generator will not accept more data.  At that time, the
         producer will get an error when trying to produce more data,
         and will have to wait by calling
         rw_ncclnt_xmlgen_produce_wait_consumer().
    - How does the consumer flow-on the producer?
       - The consumer consumes some of the buffer, by calling either
         rw_ncclnt_xml_consume_buffer() or
         rw_ncclnt_xml_consume_bytes().  The generator takes care of
         proving the flow-on indication to the producer.
    - How does the consumer wait for more data from the producer?
       - The consumer makes a rw_ncclnt_xml_consume_wait_producer()
         call.  The generator takes care of making the callback to
         continue consumption, once the producer has produced more
         data.
 *
 * Consumer-Sink Interactions:
 *  - How does the consumer indicate to the sink that there is more data
 *    available?
 *     - It is up to the sink implementation to decide this interface.
 *       For example, a socket consumer would just send() more data.
 *  - How does the consumer wait for the sink to catch up?
 *     - It is up to the consumer implementation to decide this interface.
 *       For example, a socket sink, the consumer would (implicitly) use
 *       epoll() to detect the ready-to-write condition on the socket.
 *       Meanwhile, it is not consuming data fomr the producer, which may
 *       eventually cause it to flow off.
 *  - How does the sink flow-off the consumer?
 *     - It is up to the sink implementation to decide this interface.
 *       For example, with a socket consumer, the sink would most likely
 *       just stop recv()ing data from the socket.
 *  - How does the sink flow-on the consumer?
 *     - It is up to the sink implementation to decide this interface.
 *       For example, with a socket consumer, the sink would most likely
 *       just keep recv()ing data from the socket.
 *  - How does the sink wait for more data from the consumer?
 *     - It is up to the sink implementation to decide this interface.
 *       For example, with a socket consumer, the sink could block on
 *       recv(), or use epoll() to wait for the ready-to-read indication
 *       on the socket.
 *
   ATTN: Actually, the generator should be composed of 3 plugable adaptors:
    - Input adaptor: adapts the XML source to the generator's
      producer interface.  The Source-side maps to one of 5 models:
       - zero-copy (buffer generated by source is given to producer)
       - one-copy (buffer owned by source is copied into producer)
       - xml-dom (source is RW XML node tree)
       - yang-dom (source is RW XML-Yang tree)
       - sax-ish (source is generating stream of XML elements)
 *
    - Generator adaptor: manages the callbacks between Producer and
      Consumer.  Includes the threading/queuing model.  Notifications
      from source-side are passed to sink-side, using a mechanism that
      depends on the concurrency model.  Possible models include:
       - None (direct buffering, via calls into producer's callback)
       - RW.Sched, single queue - can only produce or consume on any
         given callback
       - RW.Sched, dual queue - can produce and consume simultaneously
       .
      The Generator adaptor also adapts the input to the output, if
      necessary.  The following adaptations are required:
 *
      +-----\---------++------------+------------+------------+------------+------------+
      | Input\ Output || zero-copy  | one-copy   | xml-dom    | yang-dom   | sax-ish    |
      +-------\=======++============+============+============+============+============+
      | zero-copy     || -          | serlz cbks | sax        | sax        | feed       |
      +---------------++------------+------------+------------+------------+------------+
      | one-copy      || mkbuf      | -          | sax        | sax        | feed       |
      +---------------++------------+------------+------------+------------+------------+
      | xml-dom       || enc+mkbuf  | enc+copy   | -          | import     | walk       |
      +---------------++------------+------------+------------+------------+------------+
      | yang-dom      || enc+mkbuf  | enc+copy   | -          | -          | walk       |
      +---------------++------------+------------+------------+------------+------------+
      | sax-ish       || enc+mkbuf  | enc+copy   | feed       | feed       | -          |
      +---------------++------------+------------+------------+------------+------------+
 *
      ATTN: If a DOM is passed from one to the other, then there isn't
      any thread saftey problem.  But if DOM remains owned by input,
      then you can't let output see the DOM!  You need to make a new
      one!
 *
      ATTN: It is possible to safely use the DOM in the two threads if
      the final produce/consume eof()s wait until all processing is
      complete.
 *
    - Output adaptor: adapts the XML source to the generator's
      producer interface.  The Source-side maps to one of 5 models:
       - zero-copy (buffer handed to sink)
       - one-copy (buffer copied into sink)
       - xml-node (sink wants a RW XML dom)
       - yang-node (sink wants a RW XML Yang dom)
       - sax-ish (sink will sax-parse the data)
 */

/**
 * XML Blob Generator Handle
 */
typedef struct rw_ncclnt_xmlgen_s {} rw_ncclnt_xmlgen_t;



/**
 * Retain a reference to a RW.Netconf XML generator.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Library or application needs to hold on to a fragment passed from
 *   the other.
 */
void rw_ncclnt_xmlgen_retain( //ATTNSTEP.NC.SSH
  const rw_ncclnt_xmlgen_t* xmlgen ///< [in] The XML generator reference to retain
);

/**
 * Release a reference to a RW.Netconf XML generator.  The reference
 * should be released when the XML generator is no longer needed.  When
 * the last reference is released, the XML generator object will be
 * destroyed.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Done with a fragment.
 */
void rw_ncclnt_xmlgen_release( //ATTNSTEP.NC.SSH
  const rw_ncclnt_xmlgen_t* xmlgen ///< [in/out] The reference to release. Will be nulled.
);

/**
 * Terminate consumption of an RW.Netconf XML generator.  This function
 * the prevents the blob from being used again for more requests.  It
 * might release references to related objects and/or close any
 * underlying streams, but it does not release the caller's reference
 * to the XML blob.  After making this call, the XML generator
 * reference should not be used again, except for passing to release.
 *
 * This API provides a way to arbitrarily stop processing a XML
 * generator at anytime, which may be useful for quick destruction of
 * the XML generator.
 *
 * Use cases:
 * - Terminating a netconf session with an active request or response.
 */
void rw_ncclnt_xmlgen_terminate(
  rw_ncclnt_xmlgen_t* xmlgen ///< [in] The XML generator to terminate
);



/**
 * Create a XML blob generator.  A generator produces the blob from
 * some kind of external input source, such as a socket, a library
 * interface, or a file.  The XML input source is expected to be
 * non-real time, and possibly larger than could fit into memory.
 *
 * In addition to the common XML blob consumer interfaces, a generator
 * also provides producer interfaces, which are used to place data into
 * the XML blob.  The producer and consumer are expected to run in
 * different contexts (threads and/or dispatch queues).
 *
 * Initially, the XML blob has indeterminate validity - errors might
 * not be detected until production ends, well after consumption began.
 * The consumer must handle unexpected termination of the XML blob due
 * to errors.  Similarly, the entire XML blob may never be consumed if
 * the consumer encounters an error, so the producer must be prepared
 * to prematurely stop production.
 *
 * ATTN: Will need some way to bind this generator to a RW.Sched queue
 * (or pair of queues?).  Maybe that's another API.
 *
 * ATTN: XML generators that have non-trivial state should be self
 * synchronizing.
 *
 * Use cases:
 * - NC/SSH transport netconf responses in client
 * - NC/SSH transport netconf requests in server
 *
 * @return XML blob generator handle.  Caller owns the handle.  ATTN:
 *   null on error?  ATTN: Cannot fail?
 */
rw_ncclnt_xmlgen_t* rw_ncclnt_xml_create_xml_generator( //ATTNSTEP.NC.SSH
  rw_ncclnt_instance_t* instance ///< [in] The owning library instance
  // ATTN: What extra arguments?
);


/**
 * Get the API instance from the XML blob generator.
 *
 * Use cases:
 * - Debug
 *
 * @return The API instance.  Not owned by the caller; use retain, if
 *   the reference will be kept.
 */
rw_ncclnt_instance_t* rw_ncclnt_xmlgen_get_instance(
  const rw_ncclnt_xmlgen_t* xmlgen ///< [in] The XML blob
);

/**
 * Get the XML consumer that is associated with a producer.
 *
 * Use cases:
 * - NC/SSH transport netconf responses in client
 * - NC/SSH transport netconf requests in server
 *
 * @return The API instance.  Not owned by the caller; use retain, if
 *   the reference will be kept.
 */
rw_ncclnt_xml_t* rw_ncclnt_xmlgen_get_consumer( //ATTNSTEP.NC.SSH
  rw_ncclnt_xmlgen_t* xmlgen ///< [in] The XML generator
);


/**
 * Produce (a portion of) a XML blob, by copying data to the blob from
 * the specified buffer.
 *
 * This function may be called from inside the
 * rw_ncclnt_xmlgen_produce_buffer_async() callback, or when no
 * rw_ncclnt_xmlgen_produce_buffer_async() callback is outstanding.  It
 * is a race condition to call this function between a call to
 * rw_ncclnt_xmlgen_produce_buffer_async() and the resulting callback!
 *
 * Use cases:
 * - NC/SSH netconf XML handling.
 *
 * @return Status of the production:
 * - ATTN_OK upon success
 * - Others: errors
 */
rw_yang_netconf_op_status_t rw_ncclnt_xmlgen_produce_buffer( //ATTNSTEP.NC.SSH
  rw_ncclnt_xmlgen_t* xmlgen, ///< [in] The XML generator
  const void* buffer, ///< [in] The buffer to copy from
  size_t buflen, ///< [in] The length of buffer
  size_t* outlen ///< [out] Upon success, the number of bytes copied
);


#if 0
/*
 * ATTN: It would be nice to have a buffer passing interface, where a
 * buffer generated by the producer can be passed to the consumer,
 * which could then pass the bytes directly to the sick, all without
 * making intermediate copies...  Not sure what that API will look like
 * here - maybe giving the buffer to the generator?
 */
rw_yang_netconf_op_status_t rw_ncclnt_xmlgen_produce_buffer_give(
  rw_ncclnt_xmlgen_t* xmlgen, ///< [in] The XML generator
  size_t* outlen ///< [out] The number of bytes in the buffer being given
  const void** outbuf, ///< [in/out] Upon success, buffer passed to the generator.  nulled upon success.
);
#endif


#if 0
/*
 * ATTN: There should probably be sax-ish APIs, so that we can stream
 * very large XML blobs, with some amount of yang-based validation
 * applied, where we don't have to parse the XML twice.
 *
 * ATTN: Maybe that also requires that there be native-DOM producers,
 * where the producer ends up with a DOM once production has
 * completed.  Somehow the sink would have to indicate what kind of
 * producer it wanted when making/receiving the original netconf
 * request.
 */
// Check if there is sax data to read already available in the XML object,
// and return what it can
rw_status_t rw_ncclnt_xmlgen_produce_sax_sync(rw_ncclnt_xmlgen_t, rw_xml_node_t** node);

// Get more sax from the XML object
rw_status_t rw_ncclnt_xmlgen_produce_sax_async(rw_ncclnt_xmlgen_t, cb, context);
#endif


/**
 * Callback for producer waiting on consumer to consumer more XML data.
 * When the producer wants to generate more XML data, but the consumer
 * is not consuming the XML data, the producer must call
 * rw_ncclnt_xmlgen_produce_wait_consumer().  When the consumer finally
 * consumes more data (or closes, or errors), a callback of this type
 * will be made into the producer.  The producer will then continue
 * producing data (or terminate).
 */
typedef void (*rw_ncclnt_xmlgen_produce_wait_cb)(
  rw_ncclnt_context_t context, ///< [in] (Copy of) the context provided the original request
  rw_yang_netconf_op_status_t error ///< [in] The error, if any.  _OK if successful
);

typedef struct rw_ncclnt_xmlgen_produce_wait_context_s
{
  /// The application context.  Will be returned in the callback.
  rw_ncclnt_context_t context;

  /// The callback function.
  rw_ncclnt_xmlgen_produce_wait_cb callback;

  /// The concurrency context to make the callback in.
  rw_ncclnt_cbmgr_ptr_t cbmgr;
} rw_ncclnt_xmlgen_produce_wait_context_t;


/**
 * Wait to generate more XML data.  When the consumer begins consuming
 * again, the callback will be made.  This function can be called even
 * when the consumer is ready, in which case the callback will happen
 * (almost) immediately.
 *
 * This function may be called from inside another
 * rw_ncclnt_xmlgen_produce_buffer_async() callback, or when no
 * rw_ncclnt_xmlgen_produce_buffer_async() callback is currently
 * outstanding.  It is a race condition to call this function between a
 * call to rw_ncclnt_xmlgen_produce_buffer_async() and the resulting
 * callback!
 *
 * Use cases:
 * - NC/SSH netconf XML handling.
 *
 * @return
 * - RW_STATUS_SUCCESS - The request was enqueued; data available
 *   status will be returned in the callback.
 * - Others: request could not be satisfied; no callback will be made
 *   and no data will be provided.
 */
rw_status_t rw_ncclnt_xmlgen_produce_wait_consumer( //ATTNSTEP.NC.SSH
  rw_ncclnt_xmlgen_t* xmlgen, ///< [in] The XML generator
  rw_ncclnt_xmlgen_produce_wait_context_t ///< [in] Context and callback. Will make a copy and retain references.
);


/*
 * Check if there is has been an error on the consumer.  Once a
 * consumer has an error, the generator will not accept any more data
 * and will always return the same error status.rator.
 *
 * Use cases:
 * - NC/SSH netconf XML handling.
 *
 * @return Status of the production:
 * - ATTN_OK if the consumer is okay
 * - Others: errors
 */
rw_yang_netconf_op_status_t rw_ncclnt_xmlgen_produce_get_consumer_error( //ATTNSTEP.NC.SSH
  rw_ncclnt_xmlgen_t* xmlgen ///< [in] The XML generator to check
);

/*
 * Indicate that the end the XML blob has been reached.  The producer will
 * not add any more data to the generator.
 *
 * Use cases:
 * - NC/SSH netconf XML handling.
 */
void rw_ncclnt_xmlgen_produce_set_eof( //ATTNSTEP.NC.SSH
  rw_ncclnt_xmlgen_t* xmlgen ///< [in] The XML generator to check
);

/*
 * Indicate that the XML producer has encountered an error, either with
 * the XML itself, or with the sink the XML is being sent to.  Once an
 * error has been set, EOF will always return true.  The producer
 * should not attempt to produce more data after setting an error.
 * Check if there is has been an error on the XML blob.  Once a blob
 * has an error, it will not return any more data and will always
 * return the same error status.
 *
 * Use cases:
 * - NC/SSH netconf error handling.
 *
 * @return Status of the production:
 * - ATTN_OK if the XML blob is okay
 * - Others: errors
 */
void rw_ncclnt_xmlgen_produce_set_error( //ATTNSTEP.NC.SSH
  rw_ncclnt_xmlgen_t* xmlgen, ///< [in] The XML generator to error
  rw_yang_netconf_op_status_t status ///< [in] The status to set.  Cannot be ATTN_OK.
);

/** @} */


/*****************************************************************************/
/**
 * @defgroup RwNcDs RW.Netconf Datastore Identifier
 * @{
 *
 * A Datastore Identifier abstracts differences in the way the
 * datastores are referenced by the netconf protocol, and making the
 * library a little simpler to use.  Multiple references to a single
 * identifier are allowed.
 *
 * Datastore identifier state may include
 *  - a URL string
 *  - the identifier name string
 *
 * Datastores supported:
 *  - running
 *  - candidate
 *  - startup
 *  - A URL
 *
 * The handle is a managed object that represents a datastore identity,
 * which exists independently of the actual datastore.  The handles
 * only exist to allow the library to refer to datastores in a
 * consistent way.  Creation of a handle does not create the underlying
 * datastore - it only creates the handle the library needs to refer to
 * the datastore.  A handle can be created even for datastores that do
 * not exist, or that are unsupported by the server.  Destroying a
 * handle does not destroy the underlying datastore.  Multiple handles
 * may refer to the same conceptual datastore.
 */

/**
 * RW.Netconf Datastore Descriptor
 */
struct rw_ncclnt_ds_s {};
RW_TYPE_DECL_ALL(rw_ncclnt_ds_s, rw_ncclnt_ds_t, rw_ncclnt_ds_ptr_t)
RW_CF_TYPE_CLASS_EXTERN(rw_ncclnt_ds_ptr_t)


/**
 * Retain a reference to a RW.Netconf datastore identifier.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Required to access datastores.
 */
void rw_ncclnt_ds_retain(
  const rw_ncclnt_ds_t* ds ///< [in] The datastore identifier reference to retain
);

/**
 * Release a reference to a RW.Netconf datastore identifier.  The
 * reference should be released when the datastore identifier is no
 * longer needed.  When the last reference is released, the datastore
 * identifier object will be destroyed.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Done accessing datastores.
 */
void rw_ncclnt_ds_release(
  const rw_ncclnt_ds_t* ds ///< [in/out] The reference to release. Will be nulled.
);

/**
 * Create a datastore identifier for a URL.
 *
 * Use cases:
 * - copy-config, etc, with external URL
 *
 * @return Datastore handle.  Caller owns the handle.  ATTN: null on
 *   error?  ATTN: Cannot fail?
 */
rw_ncclnt_ds_t* rw_ncclnt_ds_create_url(//ATTNSTEP as needed
  rw_ncclnt_instance_t* instance, ///< [in] The owning library instance
  const char* url ///< [in] The URL.  Will make a copy.
);

/**
 * Get a datastore reference for the startup datastore.
 *
 * Use cases:
 * - RFC 6241 Distinct Startup Capability
 *
 * @return Datastore handle.  Owned by the library; use retain to keep
 *   a reference.
 */
const rw_ncclnt_ds_t* rw_ncclnt_ds_get_startup( //ATTNSTEP.NC.SSH
  rw_ncclnt_instance_t* instance ///< [in] The owning library instance
);

/**
 * Get a datastore reference for the running datastore.
 *
 * Use cases:
 * - RFC 6241 general support
 *
 * @return Datastore handle.  Owned by the library; use retain to keep
 *   a reference.
 */
const rw_ncclnt_ds_t* rw_ncclnt_ds_get_running( //ATTNSTEP.Base
  rw_ncclnt_instance_t* instance ///< [in] The owning library instance
);

/**
 * Get a datastore reference for the candidate datastore.
 *
 * Use cases:
 * - RFC 6241 Candidate Configuration Capability
 *
 * @return Datastore handle.  Owned by the library; use retain to keep
 *   a reference.
 */
const rw_ncclnt_ds_t* rw_ncclnt_ds_get_candidate( //ATTNSTEP as needed
  rw_ncclnt_instance_t* instance ///< [in] The owning library instance
);

/**
 * Get the API instance from a datastore reference.
 *
 * Use cases:
 * - Debug
 *
 * @return The API instance.  Not owned by the caller; use retain, if
 *   the reference will be kept.
 */
rw_ncclnt_instance_t* rw_ncclnt_ds_get_instance( //ATTNSTEP.Debug
  const rw_ncclnt_ds_t* ds ///< [in] The datastore reference.
);

/**
 * Get the name associated with a datastore identifier.
 *
 * Use cases:
 * - Debug
 *
 * @return The name, if it is not an URL.  If it is a URL, NULL will be
 *   returned.  Not owned by the caller; make a copy if the string will
 *   be stored.
 */
const char* rw_ncclnt_ds_get_name( //ATTNSTEP.Base
  const rw_ncclnt_ds_t* ds ///< [in] The datastore reference.
);

/**
 * Get the URL associated with a datastore identifier.
 *
 * ATTN: Need to XML-encode the URL if it has special characters!
 *
 * Use cases:
 * - Debug
 *
 * @return The URL, if it is an URL.  If it is a name, NULL will be
 *   returned.  Not owned by the caller; make a copy if the string will
 *   be stored.
 */
const char* rw_ncclnt_ds_get_url(
  const rw_ncclnt_ds_t* ds ///< [in] The datastore reference.
);

/**
 * Create a XML fragment for the datastore, with the specified tag.
 * The tag will typcially be target or source.
 *
 * Use cases:
 * - General library usage.
 *
 * @return The XML blob.  Owned by the caller.  ATTN: null on error?
 *   ATTN: Cannot fail?
 */
rw_ncclnt_xml_t* rw_ncclnt_ds_get_create_xml( //ATTNSTEP.RW.Msg.Base
  const rw_ncclnt_ds_t* ds ///< [in] The datastore reference.
);

/** @} */


/*****************************************************************************/
/**
 * @defgroup RwNcFilter RW.Netconf Filter Descriptor
 * @{
 *
 * Filter Descriptors abstract a filter expression, allowing a filter
 * to be constructed piecemeal.  Multiple references to a single
 * expression are allowed.
 *
 * Supported filter types:
 *  - subtree filters
 *  - XPATH filters
 *
 * The handle is a managed object that represents a filter
 * configuration.  The handles only exist to allow the library to refer
 * to different kinds of filters in a consistent way.
 *
   ATTN: Details about the kinds of filters and how to build them
 */

/**
 * RW.Netconf Filter Handle
 */
struct rw_ncclnt_filter_s {};
RW_TYPE_DECL_ALL(rw_ncclnt_filter_s, rw_ncclnt_filter_t, rw_ncclnt_filter_ptr_t)
RW_CF_TYPE_CLASS_EXTERN(rw_ncclnt_filter_ptr_t)


/**
 * Retain a reference to a RW.Netconf filter.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Required to make use of filters.
 */
void rw_ncclnt_filter_retain(
  const rw_ncclnt_filter_t* filter ///< [in] The reference to retain
);

/**
 * Release a reference to a filter expression.  The reference should be
 * released when the filter expression is no longer needed.  When the
 * last reference is released, the filter expression object will be
 * destroyed.
 *
 * ATTN: thread safety considerations, see api.
 *
 * Use cases:
 * - Done using a filter.
 */
void rw_ncclnt_filter_release(
  const rw_ncclnt_filter_t* filter ///< [in/out] The reference to release. Will be nulled.
);

/**
 * Get the API instance from a filter reference.
 *
 * Use cases:
 * - Debug
 *
 * @return The API instance.  Not owned by the caller; use retain, if
 *   the reference will be kept.
 */
rw_ncclnt_instance_t* rw_ncclnt_filter_get_instance( //ATTNSTEP.Filter
  const rw_ncclnt_filter_t* filter ///< [in] The filter reference
);

/**
 * Create a XML fragment for the filter.  The XML will contain the
 * filter tag.
 *
 * Use cases:
 * - Needed to make use of filters.
 *
 * @return The XML blob.  Owned by the caller.
 * @return NULL if the filter was created without a value, and none was filled
 *
 */
rw_ncclnt_xml_t* rw_ncclnt_filter_get_create_xml( //ATTNSTEP.Filter
    const rw_ncclnt_filter_t* filter ///< [in] The filter reference
);

/**
 * Create a filter of type XPATH. The filter is empty, and operations
 * to set value have to be called before the filter becomes valid.
 *
 * Use cases:
 * - Needed for clients that can compose XPATH filters in multiple steps
 *
 * @return the created XPATH filter.
 */

rw_ncclnt_filter_t* rw_ncclnt_filter_create_xpath(
    rw_ncclnt_instance_t* instance); /**< [in] Instance to which the filter
                                         belongs to */
/**
 * Create a filter of type XPATH. The value of the filter is set to
 * reflect the XML snippet that is passed in as the value.
 *
 * Use cases:
 * - Needed for clients that can have a composed XPATH
 *
 * @return the created XPATH filter.
 */


rw_ncclnt_filter_t* rw_ncclnt_filter_create_xpath_from_str(
    rw_ncclnt_instance_t* instance,  /**< [in] Instance to which the filter
                                        belongs to */
    const char *value);              /**< [in] String value of the filter */


/**
 * Create a filter of type SUBTREE. The filter is empty, and operations
 * to set value have to be called before the filter becomes valid.
 *
 * Use cases:
 * - Needed for clients that can compose SUBTREE filters in multiple steps
 *
 * @return the created SUBTREE filter.
 */

rw_ncclnt_filter_t* rw_ncclnt_filter_create_subtree(
    rw_ncclnt_instance_t* instance); /**< [in] Instance to which the filter
                                         belongs to */
/**
 * Create a filter of type SUBTREE. The value of the filter is set to
 * reflect the XML snippet that is passed in as the value.
 *
 * Use cases:
 * - Needed for clients that can have a composed SUBTREE
 *
 * @return the created SUBTREE filter.
 */


rw_ncclnt_filter_t* rw_ncclnt_filter_create_subtree_from_str(
    rw_ncclnt_instance_t* instance,  /**< [in] Instance to which the filter
                                        belongs to */
    const char *value);              /**< [in] String value of the filter */




// ATTN: Need APIs to actually build a path, probably using yang
// ATTN: Two kinds of filters: XPATH and subtree filtering
// rw_ncclnt_filter_subtree...manipulate(api, node...)

/** @} */


/*****************************************************************************/
/**
 * @defgroup RwNcLock RW.Netconf Lock Handle
 * @{
 *
   Configuration datastore lock handle for NC client library.  Multiple
   references to a single datastore lock are allowed, although that
   isn't expected to be our model.  Datastore lock state may include a
   reference to the locking user, a reference to the locked datastore,
   the datastore elements that are locked, lock creation time, lock
   expiration time, inactivity timer, statistics, and other
   lock-related data.
 *
   The handle is a managed object that represents a datastore lock.
   However, a datastore lock exists independently of the API session and
   any specific handle.
 *
   ATTN: Is it needed for a datastore lock to live beyond a specific
   API session?
 */
#if 0
typedef struct rw_ncclnt_lock_s {} rw_ncclnt_lock_t;

// Retain reference to a lock object.
void rw_ncclnt_lock_retain(rw_ncclnt_lock_t);

// Release a reference to a lock.
void rw_ncclnt_lock_release(rw_ncclnt_lock_t);


// Obtain a lock on a datastore
// ATTN: Partial
void rw_ncclnt_lock_datastore(rw_ncclnt_ses_t, rw_ncclnt_ds_t, rw_ncclnt_lock_datastore_cb, context);

// Wait to obtain a lock on the datastore
// ATTN: Partial
void rw_ncclnt_lock_datastore_wait(rw_ncclnt_ses_t, rw_ncclnt_ds_t, wait_timeout_t, rw_ncclnt_lock_datastore_cb, context);

// Callback for datastore lock
// N.B. The user owns the reference to the lock
void (*rw_ncclnt_lock_datastore_cb)(context, error, rw_ncclnt_lock_t);


// Get the API session reference
// Result is valid until the lock reference is released
rw_ncclnt_instance_t rw_ncclnt_lock_get_instance(rw_ncclnt_lock_t);

// Get the lock's owning session reference, if any
// Result is valid until the lock reference is released
rw_ncclnt_ses_t rw_ncclnt_lock_get_ses(rw_ncclnt_lock_t);

// Get the lock's owning datastore reference, if any
// Result is valid until the lock reference is released
rw_ncclnt_datastore_t rw_ncclnt_lock_get_datastore(rw_ncclnt_lock_t);

// ATTN: Lookup partial lock info


// Unlock a datastore lock
void rw_ncclnt_lock_unlock(rw_ncclnt_lock_t, rw_ncclnt_lock_unlock_cb, context);

// Callback for datastore unlock
void (*rw_ncclnt_lock_unlock_cb)(context, error);
#endif

/** @} */


/*****************************************************************************/
/*
   Misc other data structures not needed yet
 */
#if 0

// Handle for an unconfirmed commit
// Should be managed object
typedef struct rw_ncclnt_commit_s {} rw_ncclnt_commit_t;

// Handle for an iterated get or rpc (get-first, get-config-first, rpc-start, *-next)
// Should be managed object
typedef struct rw_ncclnt_iterator_s {} rw_ncclnt_iterator_t;

// Handle for a notifiction subscription
// Should be managed object
typedef struct rw_ncclnt_subscription_s {} rw_ncclnt_subscription_t;

#endif


/*****************************************************************************/
/**
 * @defgroup RwNcApi RW.Netconf Client Request APIs
 * @{
 *
 * The Client Request APIs define the lowest-level requests that a
 * client can make to a server via the RW.Netconf Client Library.
 * These APIs map directly to the netconf protocol.  See the @ref
 * RwNcSimpleApi for information on a simplified interface where a
 * single call maps to multiple netconf operations.
 *
 * All request APIs have the same form, and all take the same callback.
 * Each request API takes the netconf RPC elements as arguments.  If
 * the request cannot be queued (such as when the session has already
 * been lost prior to calling the API), the API returns an error
 * immediately.  Otherwise, the request is queued and the response gets
 * returned in a callback.  If the request is accepted, the
 * response will be provided in via a callback.  The callback provides
 * the status of the request, and the result (if any).
 *
 * The parameters to a request API might be accessed asynchronously
 * from the point of the call (unless the documentation otherwise
 * indicates that the library will copy the argument).  Therefore, the
 * caller should consider all arguments to be const, thread-shared data
 * until the response is returned (or an error is returned by the
 * initial API call).  For example, any rw_ncclnt_xml_t objects will be
 * serialized to the transport layer; that serialization may take real
 * wall-time and occur via numerous independent callbacks made in the
 * transport's thread.  Therefore, the caller must not modify the
 * rw_ncclnt_xml_t!  Any such access could result in a data race.  The
 * preferred method for using these APIs is to simply release all
 * request objects after making a request.  The library will ensure
 * that the objects are properly disposed of when they are no longer
 * needed.  (ATTN: If the application passes a DOM iterator to the
 * library, the application MUST get a callback when the library is
 * done using the iterator.  The callback allows the application to
 * begin modifying the DOM again.  Otherwise, the application must give
 * the DOM to the library, and the library becomes responsible for
 * freeing it.)
 *
 * The response to a netconf operation might be very big.  The
 * rw_ncclnt_xml_t object allows the response to to be arbitrarily
 * large, and the caller MUST NOT assume that a single consume
 * operation is able to obtain the entire response.  Additionally, the
 * production of a response might actually continue AFTER the initial
 * response callback is made; this can happen if the response is too
 * large to buffer all at once.  For both of these reasons, the client
 * must allow for and accet that errors may be raised while reading the
 * XML blob!  In this case, the initial response callback will indicate
 * success, and any late-detected failure will be indicated through the
 * xml blob.
 *
 * ATTN: How does the caller obtain a rw_ncclnt_xml_t object with the
 * API that they prefer?  Some may prefer a DOM, while some may prefer a
 * string...  Can the caller pass the returned xml into an xmlgen_create
 * API to transcode to the preferred form?
 *
 * ATTN: Actually, I think the client library MUST parse and validate
 * the response XML, regardless of the format that the user prefers.
 * It would be best to know, ahead of time, which format the user
 * wants.  If the user wants a DOM, the validation should build that
 * for the user, instead of dumping the response into a string?  But
 * isn't a DOM needed for validation, regardless, and DOM preserves all
 * content in the response.  Therefore, it seems like putting the
 * response in a DOM might be best, because it allows the caller to get
 * the response in whatever form they would like.
 *
 * ATTN: The counter-example is a client that wants a s[t]ax-ish API.
 * If they don't want a DOM, then why make them pay for it?  If the
 * client can validate the document without putting it in a DOM, then
 * both the client and the library get what they need, efficiently.
 */


/**
 * Callback for a netconf response.  The callback occurs only if the
 * request was enqueued.  There are several possible outcomes:
 *  - The response was never received (timeout, transport failure).  In
 *    this case, error will indicate the kind of failure, and xml will
 *    be NULL.
 *  - A response was recieved, but it was malformed (invalid XML).  In
 *    this case, error will indicate the kind of failure, and xml will
 *    be NULL.
 *  - A response was recieved, but it contained one or more
 *    server-generated errors.  In this case, the status contains the
 *    error-tag of the first error, and xml contains the entire
 *    contents of the rpc-error element.
 *  - A response was recieved, but it failed validation.  In this case,
 *    the status contains ATTN: Define a specific error code for this,
 *    and xml contains the response (that failed validation).
 *  - A valid, successful response was recieved.  In this case, error
 *    will be _OK, and xml will contain the body of the response.
 *
 * In all cases in which xml is not NULL, the callback MUST NOT assume
 * that the complete response has been received until the entire XML
 * blob has been read.  The entire XML blob must be read because the
 * blob might not be fully produced prior to making the callback,
 * particularly if the entire response cannot be buffered all at once.
 *
 * ATTN: It would be nice if the callback for the initial response and
 * the XML consumer callback could be the same...  simplifies client
 * code.
 *
 * Example callback usage, non-retaining:
 *   @code
 *     void my_callback(context, error, xml) {
 *       if (error != RW_YANG_NETCONF_OP_STATUS_OK) {
 *         handle_error(context, error, xml);
 *       }
 *       handle_response(context, xml);
 *     }
 *   @endcode
 *
 * Example callback usage, XML-retaining only on success:
 *   @code
 *     void my_callback(context, error, xml) {
 *       if (error != RW_YANG_NETCONF_OP_STATUS_OK) {
 *         handle_error(context, error, xml);
 *       }
 *       my_context_t* ctx = (my_context_t*)context->v1;
 *       ctx->rsp_xml = xml;
 *       rw_ncclnt_xml_retain(xml);
 *       handle_response_async(ctx);
 *     }
 *   @endcode
 *
 * @param [in] context
 * (Copy of) the context provided the original request.  Owned
 * by the library; if the client needs to keep a copy, it should make
 * a copy prior to returning.
 *
 * @param [in] error
 * The error, if any.  RW_YANG_NETCONF_OP_STATUS_OK if
 * successful.
 *
 * @param [in] xml
 * The contents of the response body.  If the client wants to
 * keep the XML blob past the function's return, it must call
 * rw_ncclnt_xml_retain()!  Otherwise, the XML blob will be destroyed
 * after the return.  If the function retains a reference to the XML
 * blob, then the session WILL NOT receive any more responses until
 * the reference is released.  May be passed to another thread for
 * consumption, provided that only one thread consumes at any one
 * time, and that the passing takes place via an atomic operation.
 */
typedef void (*rw_ncclnt_nc_req_cb)(
  rw_ncclnt_context_t context,
  rw_yang_netconf_op_status_t error,
  rw_ncclnt_xml_t* xml
);

typedef struct rw_ncclnt_nc_req_context_s
{
  /// The application context.  Will be returned in the callback.
  rw_ncclnt_context_t context;

  /// The callback function.
  rw_ncclnt_nc_req_cb callback;

  /// The concurrency context to make the callback in.
  rw_ncclnt_cbmgr_ptr_t cbmgr;
} rw_ncclnt_nc_req_context_t;


/**
 * Send a RPC request to a server, using a specific XML blob for the
 * RPC body.  The library does no interpretation of this blob, other
 * than to (possibly) serialize it.  The caller is responsible for
 * making sure that it is a valid request.
 *
 * Because this API does not define the RPC in terms of a yang RPC
 * model node, the client library will not validate the response.  The
 * entire response will be passed back to the client, and the client is
 * responsible for validating it.  The client library may perform
 * minimal XML syntax validation, but even that is not guaranteed.
 *
 * Use cases:
 * - Debug.
 *
 * @return
 * - RW_STATUS_SUCCESS - The request was enqueued.
 * - Others: request could not be enqueued; no callback will be made
 *   and no request was sent.
 */
rw_status_t rw_ncclnt_req_rpc_xml(//ATTNSTEP.Base
  rw_ncclnt_ses_t* ses, ///< [in] The session
  rw_ncclnt_xml_t* rpc_body, ///< [in] The contents of the RPC body, does not include rpc tags
  rw_ncclnt_nc_req_context_t context ///< [in] Callback and context. Will make a copy and retain referneces.
);

/**
 * Send a RPC request to a server, using a specific RPC name and
 * namespace, and a given XML blob body.  The library does no
 * interpretation of the command name, namespace, or blob, other than
 * to (possibly) serialize the XML.
 *
 * Because this API does not define the RPC in terms of a yang RPC
 * model node, the client library will not validate the response.  The
 * entire response will be passed back to the client, and the client is
 * responsible for validating it.  The client library may perform
 * minimal XML syntax validation, but even that is not guaranteed.
 *
 * Use cases:
 * - General library support.
 *
 * @return
 * - RW_STATUS_SUCCESS - The request was enqueued.
 * - Others: request could not be enqueued; no callback will be made
 *   and no request was sent.
 */
rw_status_t rw_ncclnt_req_rpc_name_ns(
  rw_ncclnt_ses_t* ses, ///< [in] The session
  const char* name, ///< [in] The RPC name. Cannot be NULL.
  const char* ns, ///< [in] The namespace. If NULL, assumed to be ...:netconf:base:1.0
  rw_ncclnt_xml_t* rpc_body, ///< [in] The contents of the RPC body, does not contain name's tags
  rw_ncclnt_nc_req_context_t context ///< [in] Callback and context. Will make a copy and retain referneces.
);

/**
 * Send a RPC request to a server, using a specified yang RPC
 * definition.  The library does no interpretation of the XML blob,
 * other than to (possibly) serialize the XML.  The caller is
 * responsible for making sure that it is a valid request.
 *
 * Use cases:
 * - General library support.
 *
 * @return
 * - RW_STATUS_SUCCESS - The request was enqueued.
 * - Others: request could not be enqueued; no callback will be made
 *   and no request was sent.
 */
rw_status_t rw_ncclnt_req_rpc_yang(
  rw_ncclnt_ses_t* ses, ///< [in] The session
  rw_yang_node_t* ynode, ///< [in] The yang model node of the RPC.  Must be RPC or RPC input.
  rw_ncclnt_xml_t* rpc_body, ///< [in] The contents of the RPC body, does not include rpc tags
  rw_ncclnt_nc_req_context_t context ///< [in] Callback and context. Will make a copy and retain referneces.
);

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
 * Use cases:
 * - General library support.
 *
 * @return
 * - RW_STATUS_SUCCESS - The request was enqueued.
 * - Others: request could not be enqueued; no callback will be made
 *   and no request was sent.
 */
rw_status_t rw_ncclnt_req_nc_get_config(
  rw_ncclnt_ses_t* ses, ///< [in] The session
  const rw_ncclnt_ds_t* config_source, ///< [in] The configuration to retrieve
  rw_ncclnt_filter_t* filter, ///< [in] The filter description, may be NULL
  rw_ncclnt_nc_req_context_t context ///< [in] Callback and context. Will make a copy and retain referneces.
);

/**
   Send an edit-config request to a server, using a XML blob for the
   changes to be made.  This API sends no default-operation parameter,
   thus resulting in a merge.
 *
   Use cases:
   - General library support.
 *
   @return
   - RW_STATUS_SUCCESS - The request was enqueued.
   - Others: request could not be enqueued; no callback will be made
     and no request was sent.
 */
rw_status_t rw_ncclnt_req_nc_edit_config(
  rw_ncclnt_ses_t* ses, ///< [in] The session
  const rw_ncclnt_ds_t* target, ///< [in] The target datastore
  void* default_op, ///< [in] The default operation. May be nullptr. ATTN: Currently unsupprted.
  void* test_opt, ///< [in] The test option. May be nullptr. ATTN: Currently unsupprted.
  void* error_opt, ///< [in] The error option. May be nullptr. ATTN: Currently unsupprted.
  rw_ncclnt_xml_t* config, ///< [in] The XML blob defining the config changes
  rw_ncclnt_nc_req_context_t context ///< [in] Callback and context. Will make a copy and retain referneces.
);

/**
 * Send a netconf get request to a server.  The data can be limited by
 * providing a filter.
 *
 * ATTN: Extensions API?  Will want to have "chunked" return value API.
 * Although, the "chunked" API probably needs to use a "simplified" API
 * because returning responses to a single request is not otherwise
 * netconf compliant.
 *
 * Use cases:
 * - General library support.
 *
 * @return
 * - RW_STATUS_SUCCESS - The request was enqueued.
 * - Others: request could not be enqueued; no callback will be made
 *   and no request was sent.
 */
rw_status_t rw_ncclnt_req_nc_get( //ATTNSTEP.Base
  rw_ncclnt_ses_t* ses, ///< [in] The session
  rw_ncclnt_filter_t* filter, ///< [in] The filter description, may be NULL
  // ATTN: List of XML attributes to apply to the get node
  // ATTN: XML blob to append to the body, following the filter
  rw_ncclnt_nc_req_context_t context ///< [in] Callback and context. Will make a copy and retain referneces.
);

/** @} */


/*****************************************************************************/
/**
 * @defgroup RwNcSimpleApi RW.Netconf Client Simplified APIs
 * @{
 *
 * The simplified client APIs define meta-operations that can be
 * performed via a netconf session, that require multiple underlying
 * netconf requests.  These simplified APIs aim to make certain tasks
 * simpler to accomplish by factoring out common boiler-plate tasks.
 *
 * ATTN: The Simplified API has not yet been defined.
 *
 * ATTN: Simplified API should allow:
 *  - insert (based on model node)
 *  - append (based on model node)
 *  - remove (based on model node)
 *  - update (based on model node)
 *  - merge (based on model node)
 *  - get based on model node
 *
 * Use cases:
 *  - set a specific leaf
 *  - get a list of all known keys for a schema node
 *  - get a list of possible values for a leafref
 *  - ...
 */

/** @} */

/*
 * Wrapper Macros for  RWNC Client Library
 */
#define RWTRACE_NCCLNTLIB_IMPL(trace_, inst_, fmt_, ...) \
  do { \
    rwtrace_ctx_t* ti = (inst_)->get_trace_instance(); \
    if (ti) { \
      trace_(ti, RWTRACE_CATEGORY_RWNCCLNT, fmt_, ##__VA_ARGS__); \
    } \
  } while(0)

#define RWTRACE_NCCLNTLIB_DEBUG(inst_, fmt_, ...) \
  RWTRACE_NCCLNTLIB_IMPL(RWTRACE_DEBUG, (inst_), fmt_, ##__VA_ARGS__)

#define RWTRACE_NCCLNTLIB_INFO(inst_, fmt_, ...) \
  RWTRACE_NCCLNTLIB_IMPL(RWTRACE_INFO, (inst_), fmt_, ##__VA_ARGS__)

#define RWTRACE_NCCLNTLIB_CRITINFO(inst_, fmt_, ...) \
  RWTRACE_NCCLNTLIB_IMPL(RWTRACE_CRITINFO, (inst_), fmt_, ##__VA_ARGS__)

#define RWTRACE_NCCLNTLIB_NOTICE(inst_, fmt_, ...) \
  RWTRACE_NCCLNTLIB_IMPL(RWTRACE_NOTICE, (inst_), fmt_, ##__VA_ARGS__)

#define RWTRACE_NCCLNTLIB_WARN(inst_, fmt_, ...) \
  RWTRACE_NCCLNTLIB_IMPL(RWTRACE_WARN, (inst_), fmt_, ##__VA_ARGS__)

#define RWTRACE_NCCLNTLIB_ERROR(inst_, fmt_, ...) \
  RWTRACE_NCCLNTLIB_IMPL(RWTRACE_ERROR, (inst_), fmt_, ##__VA_ARGS__)

#define RWTRACE_NCCLNTLIB_CRIT(inst_, fmt_, ...) \
  RWTRACE_NCCLNTLIB_IMPL(RWTRACE_CRIT, (inst_), fmt_, ##__VA_ARGS__)

#define RWTRACE_NCCLNTLIB_ALERT(inst_, fmt_, ...) \
  RWTRACE_NCCLNTLIB_IMPL(RWTRACE_ALERT, (inst_), fmt_, ##__VA_ARGS__)

#define RWTRACE_NCCLNTLIB_EMERG(inst_, fmt_, ...) \
  RWTRACE_NCCLNTLIB_IMPL(RWTRACE_EMERG, (inst_),  fmt_, ##__VA_ARGS__)


__END_DECLS

#endif
