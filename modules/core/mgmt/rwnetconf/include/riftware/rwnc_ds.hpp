
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_ds.hpp
 * @author Vinod Kamalaraj
 * @date 2014/06/19
 * @brief User object used with netconf related SSH operations
 */

#ifndef RWNC_DS_HPP_
#define RWNC_DS_HPP_

#include <rwnetconf.h>
#include <rwnc_instance.hpp>
#include <string>
#include <rw_cf_type_validate.hpp>
#include "rwnc_xml.hpp"
typedef enum rwnc_ds_type_e {
  RWNC_DS_TYPE_STANDARD = 0x6192014,
  RWNC_DS_TYPE_URL,
} rwnc_ds_type_t;

namespace rw_netconf {

// Forward declarations
class Xml;

/**
 * @addtogroup RwNcDs
 * @{
 */

/**
 * The DataStore class provides a object oriented library from which
 * the externally visible C and VALA API are derived from.
 */
class DataStore
{
  // Uses RW-CF memory allocation and tracking
  RW_CF_TYPE_CLASS_DECLARE_MEMBERS(rw_ncclnt_ds_t, rw_ncclnt_ds_ptr_t, DataStore)

 public:

  /**
     Allocate and initialize a DataStore object.
     @return The new object, owned by the caller.
   */
  static suptr_t create (
    Instance *instance,  /**< Instance that this user is part of */
    const char* name,    /**< name of user */
    rwnc_ds_type_t type);

 public:

  /**
   * Get the API instance reference.
   *
   * @return The API instance.  Not owned by the caller; use retain, if
   *   the reference will be kept.
   */
  Instance* get_instance() const;

  /**
   * Get the name of a datastore
   *
   * @return name of the datastore if type is STANDARD
   * @return nullptr if the datastore represents a URL
   */
  const char* get_name() const;

  /**
   * Get the URL of a datstore
   *
   * @return name of the datastore if ds is a URL
   * @return nullptr if the datastore is standard
   */

  const char* get_url() const;

  /**
   * Get a XML that corresponds to the representation of this data store
   * in a NETCONF message
   *
   * @return XML object that represents an XML representation of this
   *         datastore
   */
  Xml::suptr_t get_xml() const;

 protected:

  /**
   * Build a data store with a given name and type
   */
  DataStore (
    Instance *instance,  /**< Instance that this user is part of */
    const char* name,    /**< name of user */
    rwnc_ds_type_t type);

  ~DataStore();

  // Cannot copy
  DataStore(const DataStore&) = delete;
  DataStore& operator=(const DataStore&) = delete;

 private:

  /**
   * Reference to the owning instance.  This is a weak reference
   * because the instance itself owns 3 DataStore objects, and so there
   * would be a circular reference relationship.
   */
  Instance* instance_;

  /** Name of the datastore */
  std::string name_;

  /** type of data store */
  rwnc_ds_type_t type_;
};

RW_CF_TYPE_CLASS_DEFINE_INLINES(rw_ncclnt_ds_t, rw_ncclnt_ds_ptr_t, DataStore)

/** @} */

} // namespace rw_netconf

RW_CF_TYPE_CLASS_DEFINE_TRAITS(rw_ncclnt_ds_t, rw_ncclnt_ds_ptr_t, rw_netconf::DataStore)

#endif // RWNC_DS_HPP_
