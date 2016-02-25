
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_user.hpp
 * @author Vinod Kamalaraj
 * @date 2014/06/19
 * @brief User object used with netconf related SSH operations
 */

#ifndef RWNC_FILTER_HPP_
#define RWNC_FILTER_HPP_

#include <rwnetconf.h>
#include <string>
#include <rw_cf_type_validate.hpp>

typedef enum rwnc_filter_type_e {
  RWNC_FILTER_TYPE_SUBTREE = 0x08102015,
  RWNC_FILTER_TYPE_XPATH,
} rwnc_filter_type_t;

namespace rw_netconf {

// Forward declarations
class Instance;

/**
 * @addtogroup RwNcFilter
 * @{
 */

/**
 * The Filter class provides a object oriented library from which the externally
 * visible C and VALA API are derived from.
 *
 * ATTN : Short term class structure.
 *    TODO: create right filter structures based on the string passed in during
 *          construction - iff multi-step construction of filters is required
 *    TODO: The base class needs to be abstract - with functionality moving to
 *          the XPATH and subtree classes. For now, the type and string will do.
 *          This implementation also allows for figuring out the use cases of
 *          filter in various client implementations - eg: CLI
 */
class Filter
{
  // Use RW-CF memory tracking and validation
  RW_CF_TYPE_CLASS_DECLARE_MEMBERS(rw_ncclnt_filter_t, rw_ncclnt_filter_ptr_t, Filter)

 public:

  /// ATTN: comments
  /// ATTN: use suptr_t
  static Filter* create(
    Instance *instance,
    rwnc_filter_type_t type,
    const char *value);

  /// ATTN: comments
  /// ATTN: use suptr_t
  static Filter* create(
    Instance *instance,
    rwnc_filter_type_t type);


 public:


  /**
   * Get the instance a filter belongs to
   * @return The instance that the filter belongs to
   */
  Instance *get_instance() const;

  /**
   * Get the string value of a filter
   * @return string value of the filter
   */
  const char *get_value() const;

  /**
   * Get the XML corresponding to a filter. In NETCONF requests, a XML node
   * represents the filter in get and get-config RPC operations.
   *
   * @return xml representation of a filter node.
   * @return nullptr if the value is not in valid state
   * ATTN: return suptr_t
   */
  rw_ncclnt_xml_t* get_xml() const;

 protected:

  /**
   * Create a filter given its type. It is assumed that the values for the
   * the filter is setup later.
   */
  Filter (
      Instance *instance,        /**< Instance that this filter is part of */
      rwnc_filter_type_t type);  /**< Type of filter - SUBTREE or XPATH */

  /**
   * Create a filter given the instance that it belongs to, its type, and
   * a string value representing the xml value of the filter.
   */
  Filter (
      Instance *instance,        /**< Instance that this filter is part of */
      rwnc_filter_type_t type,   /**< Type of filter - SUBTREE or XPATH */
      const char *value);        /**< A string representation in XML for the
                                    filters value */

  ~Filter();

  // Cannot copy
  Filter(const Filter&) = delete;
  Filter& operator=(const Filter&) = delete;

 private:

  /// The owning client library instance.
  UniquishPtrRwMemoryTracking<Instance>::suptr_t instance_;
  std::string         value_;      /**< String representation of the filter */
  rwnc_filter_type_t  type_;       /**< The type of filter */

};

RW_CF_TYPE_CLASS_DEFINE_INLINES(rw_ncclnt_filter_t, rw_ncclnt_filter_ptr_t, Filter)

/** @} */

} // namespace rw_netconf

RW_CF_TYPE_CLASS_DEFINE_TRAITS(rw_ncclnt_filter_t, rw_ncclnt_filter_ptr_t, rw_netconf::Filter)

#endif // RWNC_FILTER_HPP_
