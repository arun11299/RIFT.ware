
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file server_mock.hpp
 * @author Tom Seidenberg
 * @date 2014/06/23
 * @brief RW.Netconf null transport definitions.
 */

#ifndef RWNETCONF_TRANSPORT_NULL_HPP_
#define RWNETCONF_TRANSPORT_NULL_HPP_

#include <rwnc_transport.hpp>

namespace rw_netconf {

/*****************************************************************************/
/**
 * @addtogroup RwNcUnitTest
 * @{
 */

/// Null transport.
/**
 * Transport that fails all requests.  Useful when you otherwise need a
 * valid transport, but don't require one that works.
 */
class TransportNull
: public Transport
{
 public:
  /// Constructor
  TransportNull(
    /**
     * [in] The owning client library instance.
     */
    Instance* instance
  );

  /// Trivial destructor
  ~TransportNull();

  // Cannot copy.
  TransportNull(const TransportNull&) = delete;
  TransportNull& operator=(const TransportNull&) = delete;

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
};

/** @} */

} /* namespace rw_netconf */

#endif /* RWNETCONF_TRANSPORT_NULL_HPP_ */
