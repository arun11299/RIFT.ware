
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

                                                
/**
 * @file rwnc_user.cpp
 * @author Vinod Kamalaraj
 * @date 2014/06/19
 * @brief User object used with netconf related SSH operations
 */

#include <rwnc_user.hpp>
#include <rwnc_instance.hpp>

using namespace rw_netconf;

/*****************************************************************************/
// C++ class API Implementations

RW_CF_TYPE_CLASS_DEFINE_MEMBERS(
  "Netconf Client Library - User",
  rw_ncclnt_user_t,
  rw_ncclnt_user_ptr_t,
  User);

User* User::create(Instance *instance)
{
  return new User(instance);
}

User* User::create(Instance *instance, const char *name)
{
  return new User(instance, name);
}

User::User (Instance *instance)
    :instance_ (instance)
{
  // ATTN: Should get login name
}

User::User (Instance *instance, const char *name)
    :instance_ (instance),
     name_ (name)
{
  RWTRACE_NCCLNTLIB_DEBUG(instance_, "Created NC Client User object- [0x%p]", this); 
}

User::~User()
{
  RWTRACE_NCCLNTLIB_DEBUG(instance_, "Destructing NC Client User object- [0x%p]", this);
}

void User::set_name (const char *name)
{
  name_ = name;
  RWTRACE_NCCLNTLIB_DEBUG(instance_, "Set NC Client User object name to - [%s]", name_.c_str());
}

const char* User::get_name() const
{
  return name_.c_str();
}


/*****************************************************************************/
// C Interface API Implementations

void rw_ncclnt_user_retain(const rw_ncclnt_user_t* user)
{
  rw_mem_to_cpp_type(user)->rw_mem_retain();
}

void rw_ncclnt_user_release(const rw_ncclnt_user_t* user)
{
  rw_mem_to_cpp_type(user)->rw_mem_release();
}

rw_ncclnt_user_t* rw_ncclnt_user_create_self(rw_ncclnt_instance_t* instance)
{
  User *user = User::create (rw_mem_to_cpp_type(instance));
  return user->rw_mem_to_c_type();
}

rw_ncclnt_user_t* rw_ncclnt_user_create_name(rw_ncclnt_instance_t* instance,
                                             const char* username)
{
  User *user = User::create (rw_mem_to_cpp_type(instance), username);
  return user->rw_mem_to_c_type();
}

rw_ncclnt_instance_t* rw_ncclnt_user_get_instance(const rw_ncclnt_user_t* user)
{
  return rw_mem_to_cpp_type(user)->instance_->rw_mem_to_c_type();
}

const char* rw_ncclnt_user_get_username(const rw_ncclnt_user_t* user)
{
  return rw_mem_to_cpp_type(user)->get_name();
}

void rw_ncclnt_user_set_username(rw_ncclnt_user_t* user, const char* name)
{
  rw_mem_to_cpp_type(user)->set_name(name);
}

