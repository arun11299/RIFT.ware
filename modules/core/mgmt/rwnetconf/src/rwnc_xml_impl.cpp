
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

                                                

/**
 * @file rwnc_xml_impl.cpp
 * @author Rajesh Velandy
 * @date 2014/06/13
 * @brief RW.Netconf client library XML method implementation
 */

#include "rwnc_xml_impl.hpp"
#include <algorithm>
#include <iostream>

using namespace rw_netconf;

XmlStrImpl::XmlStrImpl(Instance *instance, const char* xml_str)
  :Xml(instance),
  end_(nullptr)
{
  RW_ASSERT(xml_str);
  xml_str_ = RW_MALLOC_TYPE(strlen(xml_str)+1, char);
  curr_ = (char*) xml_str_;
  RW_ASSERT(xml_str_);
  strcpy(xml_str_, xml_str);

  RWTRACE_NCCLNTLIB_DEBUG(instance_, "Create XmlStrImpl object - [0x%p]", this);
  // Set the end to the end of the string
  end_ = xml_str_ + strlen(xml_str);
}

XmlStrImpl::XmlStrImpl(Instance *instance, const char* xml_str, size_t len)
  :Xml(instance),
  end_(nullptr)
{
  RW_ASSERT(xml_str);
  xml_str_ = RW_MALLOC_TYPE(strlen(xml_str)+1, char);
  curr_ = (char*) xml_str_;
  RW_ASSERT(xml_str_);
  strcpy(xml_str_, xml_str);
  
  // Set the end to the end of the string
  end_ = xml_str_ + len;
}


char* XmlStrImpl::get_xml_str(char *s, size_t size, size_t *outlen)
{
  RW_ASSERT(s);
 
  if (size > 0) {
    *outlen = std::min(size-1, bytes_left());
  
    // Return len bytes from current
    strncpy(s, (const char*)curr_, *outlen);

    // Advance curr_
    curr_ += *outlen;
  } else {
    *outlen = 0;
  }
  s[*outlen] = '\0';
 // return the pointer
 return (s);
}

XmlStrImpl::~XmlStrImpl() 
{
  RW_FREE_TYPE(xml_str_, char);
  xml_str_  = nullptr;
  end_ = nullptr;
  curr_ = nullptr;
}

size_t XmlStrImpl::bytes_left() const
{
  RW_ASSERT(end_ >= curr_);
  return(end_ - curr_);
}

bool XmlStrImpl::eof() const
{
  return (end_ == curr_);
}

Xml::suptr_t  XmlStrImpl::create_composed(const char *prefix, const char *suffix) const
{
  std::string str = std::string(prefix) + xml_str_ +  suffix;
  return create_xml_copy_string(get_instance(), str.c_str());
}

Xml::suptr_t  XmlStrImpl::append(Xml *suffix) const
{
  char buffer[512];
  std::string str = std::string(xml_str_);
  size_t size = 0;

  while (!suffix->eof()) {
    str += suffix->get_xml_str(buffer, 512, &size);
  }

  return create_xml_copy_string(get_instance(), str.c_str());
}
Xml::suptr_t  XmlStrImpl::append(const char *xml_str) const
{
  return create_xml_copy_string(get_instance(), (std::string(xml_str_)+xml_str).c_str());
}

void XmlStrImpl::print(std::ostream& os) const
{
  os << "XMLStrImpl:" << std::endl;
  os << "[xml_str_] = \"";
  if (xml_str_) {
    os << xml_str_ ;
  } else {
    os << "nullptr";
  }
  os << "\"" << std::endl;

  os << "[curr] = \"";
  if (curr_) {
    os << curr_;
  } else {
    os << "nullptr";
  }
  os << "\"" << std::endl;
  return;
}


XmlConstStrImpl::XmlConstStrImpl(Instance *instance, const char* xml_str)
  :Xml(instance),
   xml_str_(xml_str),
   curr_((char*)xml_str)
{
  end_ = xml_str_ + strlen(xml_str);
  RWTRACE_NCCLNTLIB_DEBUG(instance_, "Create  XmlConstStrImpl object - [0x%p]", this);
}

XmlConstStrImpl::XmlConstStrImpl(Instance *instance, const void* buff, size_t len)
  :Xml(instance),
  end_(nullptr)
{
  xml_str_ = (char*)buff;
  end_ = xml_str_ + len;
  RWTRACE_NCCLNTLIB_DEBUG(instance_, "Create  XmlConstStrImpl object - [0x%p]", this);
}

XmlConstStrImpl::~XmlConstStrImpl() 
{
  xml_str_ = nullptr;
  end_ = nullptr;
  curr_ = nullptr;
}

char* XmlConstStrImpl::get_xml_str(char *s, size_t size, size_t *outlen) 
{
  RW_ASSERT(s);
 
  *outlen = std::min(size, bytes_left());
  
  // Return len bytes from curr_
  memcpy(s, (const char*)curr_, *outlen);
  s[*outlen] = 0;
  
  // Advance curr_
  curr_ += (*outlen);

 // return the pointer
 return (s);
}

size_t XmlConstStrImpl::bytes_left() const
{
  RW_ASSERT(end_ >= curr_);
  return(end_ - curr_);
}

bool XmlConstStrImpl::eof() const
{
  return (end_ == curr_);
}

Xml::suptr_t  XmlConstStrImpl::create_composed(const char *prefix, const char *suffix) const
{
  std::string str = std::string(prefix) + xml_str_ +  suffix;
  return create_xml_copy_string(get_instance(), str.c_str());
}

Xml::suptr_t  XmlConstStrImpl::append(Xml *suffix) const
{
  char buffer[512];
  std::string str = std::string(xml_str_);
  size_t size = 0;

  while (!suffix->eof()) {
    str += suffix->get_xml_str(buffer, 512, &size);
  }

  return create_xml_copy_string(get_instance(), str.c_str());
}

Xml::suptr_t  XmlConstStrImpl::append(const char *xml_str) const
{
  return create_xml_copy_string(get_instance(), (std::string(xml_str_)+xml_str).c_str());
}

void XmlConstStrImpl::print(std::ostream& os) const
{
  os << "XMLConstStrImpl:" << std::endl;
  os << "[xml_str_] = \"";
  if (xml_str_) {
    os << xml_str_ ;
  } else {
    os << "nullptr";
  }
  os << "\"" << std::endl;

  os << "[curr] = \"";
  if (curr_) {
    os << curr_;
  } else {
    os << "nullptr";
  }
  os << "\"" << std::endl;
  return;
}

namespace rw_netconf {
  std::ostream& operator<<(std::ostream& os, const Xml& xml)
  {
    xml.print(os);
    return os;
  }
}

