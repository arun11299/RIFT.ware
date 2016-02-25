
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file server_mock.hpp
 * @author Tom Seidenberg
 * @date 2014/06/20
 * @brief RW.Netconf server mock definitions.
 */

#ifndef RWNETCONF_SERVER_MOCK_HPP_
#define RWNETCONF_SERVER_MOCK_HPP_

#include <rwnetconf.h>
#include <rwnc_transport.hpp>
#include <rw_xml.h>

namespace rw_netconf {

/*****************************************************************************/
/**
 * @addtogroup RwNcUnitTest
 * @{
 */


/// Server mock
/**
 * Server mock transport implementation.  This defines a trivial server
 * that contains a bunch of public member variables that are useful for
 * handling requests from a transport layer test.
 *
ATTN:  A server mock implements the
     Tranport interface and provides a very simple (and, ultimately
     non-compliant) netconf server mock-up.  The mock-up is sufficient
     for testing the functioning of the client library and little else.

 */
class ServerMock
: public Transport
{
 public:
  /// Constructor
  ServerMock(
    /**
     * [in] The owning client library instance.
     */
    Instance* instance,

    /**
     * [in] The XML Manager for the DOM, and netconf handling.  Owned
     * by the application.  Cannot be nullptr.
     */
    rw_yang::XMLManager* xml_mgr,

   /** 
    * optional filename from which to load XML
    */
   
   const char *file_name = nullptr
 
    
  );

  /// Trivial destructor
  ~ServerMock();

  // Cannot copy.
  ServerMock(const ServerMock&) = delete;
  ServerMock& operator=(const ServerMock&) = delete;

 public:
  /**
   * Create a netconf server mock.
   *
   * @return A Session object upon success.  If creation fails, an
   *   exception will be thrown.
   */
  static Transport* create_server(
    /**
     * [in] The owning client library instance.  The created Session
     * will retain a reference to the instance.  Cannot be nullptr.
     */
    Instance* instance,

    /**
     * [in] The XML Manager for the DOM, and netconf handling.  Owned
     * by the application.  Cannot be nullptr.
     */
    rw_yang::XMLManager* xml_mgr
  );

  /**
   * Load a YANG module to use with the server mock.
   *
   * @return RW_STATUS_SUCCESS if the module gets loaded successfully
   * @return RW_STATUS_FAILURE if the module cannot be located or loaded
   */
  rw_status_t load_module (
      
      /**
       * [in] Name of the module to be loaded
       */

      const char *module_name);
  

  /**
   * Find the DOM associated with a data store
   *
   * @return DOM if it exists, nullptr otherwise
   */

  rw_yang::XMLDocument *get_dom (
      /**
       * [in] The data store to find
       */
      const DataStore *ds);
  
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
  // All data is public, to allow easy access from the client

  /// The owning instance.  Retains a reference.
  UniquishPtrRwMemoryTracking<Instance>::suptr_t instance_;

  /// XML document factory manager.  Owned by the application.
  rw_yang::XMLManager* xml_mgr_;

  /// The running configuration DOM.  Owned by the mock.
  rw_yang::XMLDocument::uptr_t running_;

  /// The candiate configuration DOM. Owned by the mock. Created on demand?
  rw_yang::XMLDocument::uptr_t candidate_;

  /// The startup configuration DOM. Owned by the mock. Created on demand?
  rw_yang::XMLDocument::uptr_t startup_;


  /// The last received filter request --- for use in google test
  //  The callee still owns the filter
  
  Filter *filter_;

  /**
   * When not set to RW_YANG_NETCONF_OP_STATUS_NULL, immediately
   * complete the next request with no XML and the indicated error.
   * Auto-resets to RW_YANG_NETCONF_OP_STATUS_NULL.
   */
  rw_yang_netconf_op_status_t fail_next_nc_req_with_nc_err;

  /**
   * When not set to RW_STATUS_SUCCESS, immediately return the status
   * without handling the request.  Auto-resets to RW_STATUS_SUCCESS.
   */
  rw_status_t fail_next_nc_req_with_status;

  /**
   * When not set to RW_STATUS_SUCCESS, immediately return the status
   * without handling the request.  Auto-resets to RW_STATUS_SUCCESS.
   */
  Filter*  get_filter();
};


/** @} */

} /* namespace rw_netconf */

#endif /* RWNETCONF_SERVER_MOCK_HPP_ */
