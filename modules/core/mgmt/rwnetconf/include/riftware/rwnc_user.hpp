
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

#ifndef RWNC_USER_HPP_
#define RWNC_USER_HPP_

#include <rwnetconf.h>
#include <string>
#include <rw_cf_type_validate.hpp>

namespace rw_netconf {

// Forward declarations
class Instance;

/**
 * @addtogroup RwNcUser
 * @{
 */

/**
 * The User class provides a object oriented library from which the externally
 * visible C and VALA API are derived from.
 */
class User
{
  /* Use RW-CF memory tracking. */
  RW_CF_TYPE_CLASS_DECLARE_MEMBERS(rw_ncclnt_user_t, rw_ncclnt_user_ptr_t, User);

 public:

  static User* create(Instance *instance);

  static User* create(Instance *instance, const char *name);

 public:

  /**
   * Set the name of a user object
   */
  void set_name (
      const char *name); /**< name of user */

  /**
   * Get the name of a user
   *
   * @return name of the user
   */
  const char *get_name() const;

 protected:
  // Cannot copy
  User(const User&) = delete;
  User& operator=(const User&) = delete;

  /**
   * Constructor that takes instance as the argument
   */
  User (
      Instance *instance);  /**< Instance that this user is part of */

  /**
   * Build a user with a given name
   */

  User (
      Instance *instance,  /**< Instance that this user is part of */
      const char* name);   /**< name of user */

  ~User();

 public:
  /// ATTN: User only holds a weak reference, at least for now.
  Instance           *instance_;  /**< The instance from which this user is
                                     allocated from */

  std::string name_;    /**< Name of the user */

};

RW_CF_TYPE_CLASS_DEFINE_INLINES(rw_ncclnt_user_t, rw_ncclnt_user_ptr_t, User);

/** @} */

} // namespace rw_netconf

RW_CF_TYPE_CLASS_DEFINE_TRAITS(rw_ncclnt_user_t, rw_ncclnt_user_ptr_t, rw_netconf::User);

#endif // RWNC_USER_HPP_
