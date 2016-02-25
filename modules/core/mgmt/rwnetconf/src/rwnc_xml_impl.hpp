
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */

                                                

/**
 * @file rwnc_xml_impl.hpp
 * @author Rajesh Velandy
 * @date 2014/06/13
 * @brief RW.Netconf client library XML Implementation classes
 */

#ifndef RWNC_XML_IMPL_HPP_
#define RWNC_XML_IMPL_HPP_

#include <rwnc_xml.hpp>
#include <rwnc_instance.hpp>


namespace rw_netconf {

/**
 * @addtogroup RwNcXml
 * @{
 */

/**
 * Utility class that implements an XML Blob based  on a string based 
 * implementation. This version of the implementation copies the passed
 * string, so the caller can release the passed string as soon as 
 * this object is created.
 */
class XmlStrImpl
:public Xml
{
 public:

  /// Constructor
  XmlStrImpl(
    Instance *instance,  /// < [in] RWNC Client Library instance
    const char* xml_str        /// > [in] XML string in UTF-8 formwat
  );
  
  /// Constructor
  XmlStrImpl(
    Instance *instance,  /// < [in] RWNC Client Library instance
    const char* buffer,  /// > [in] XML buffer in UTF-8 formwat
    size_t len           /// > [in] size of the XML buffer
  );
   

  /// Destroy XmlStrImpl object
  ~XmlStrImpl();

  // Cannot copy
  XmlStrImpl(const XmlStrImpl &) = delete;
  XmlStrImpl& operator = (const XmlStrImpl&) = delete;

  char* get_xml_str(
    char *s,        ///< [in] the buffer where the function returns the string. This should be at least size+1 long
    size_t size,    ///< [in] the requested size
    size_t *outsize ///< [in] the returned size
  );

  /// returns the number of bytes left in the object -- returns 0 if eof()
  size_t bytes_left() const;

  /// returns true when the  end of XML string is reached.
  bool eof() const;


  /// @see XML::create_composed(const char *prefix, const char *suffix)
  Xml::suptr_t create_composed(
    const char *prefix,  ///< [in] The NUL-terminated prefix string
    const char *suffix   ///< [in] The NUL-terminated suffix string
  ) const override;

  /// @see XML::append(Xml *suffix_blob)
  Xml::suptr_t append( 
    Xml *suffix_blob ///< [in] suffix XML blob
  ) const override;
  
  /// @see XML::append(const char* suffix_str)
  Xml::suptr_t append( 
    const char *xml_str ///< [in] suffix XML string
  ) const override;

  /// prints this object to  the passed output stream
  void print(std::ostream& os) const;

 protected:

  /// The  begining of the XML string
  char* xml_str_;
  
  /// The current position of the cursor
  char* curr_;
 
  /// The end of the XML string
  const char* end_;
};

/**
 * Utility class that implements an XML Blob based  on a string based 
 * implementation. This version of the implemeation uses the passed
 * string for storage, so the caller cannot release the passed string as soon as 
 * this object is alive.
 */
class XmlConstStrImpl
:public Xml
{
 public:

  /// Constructor
  XmlConstStrImpl(
    Instance *instance,  /// < [in] RWNC Client Library instance
    const char* xml_str  /// < [in] XML string in UTF-8 formwat
  );
  
  /// Constructor
  XmlConstStrImpl(
    Instance *instance,  /// < [in] RWNC Client Library instance
    const void* buff, /// < [in] XML string in UTF-8 formwat
    size_t len           /// < [in] length of the passed buffer
  );
   
  /// Destroy XmlConstStrImpl object
  ~XmlConstStrImpl();

  // Cannot copy
  XmlConstStrImpl(const XmlConstStrImpl &) = delete;
  XmlConstStrImpl& operator = (const XmlConstStrImpl&) = delete;
  
  char* get_xml_str(
    char *s,         ///< [in] the buffer where the function returns the string. This should be at least size+1 long
    size_t size,   ///< [in] the requested size+1
    size_t *outsize  ///< [in] the returned size
  );

  /// returns the number of bytes left in the object -- returns 0 if eof()
  size_t bytes_left() const;

  /// returns true when the  end of XML string is reached.
  bool eof() const;
  
  /// @see XML::create_composed(const char *prefix, const char *suffix)
  Xml::suptr_t create_composed(
    const char *prefix,  ///< [in] The NUL-terminated prefix string
    const char *suffix   ///< [in] The NUL-terminated suffix string
  ) const override;

  /// @see XML::append(Xml *suffix_blob)
  Xml::suptr_t append( 
    Xml *suffix_blob ///< [in] suffix XML blob
  ) const override;
  
  /// @see XML::append(const char* suffix_str)
  Xml::suptr_t append( 
    const char *xml_str ///< [in] suffix XML string
  ) const override;

  /// prints this object to the passed output stream
  void print(std::ostream& os) const;

 protected:

  /// The  begining of the XML string
  const char* xml_str_;
  
  /// The current position of the cursor
  char* curr_;
 
  /// The end of the XML string
  const char* end_;
  

};

/** @} */
} // namespace rw_netconf
#endif /* RWNC_XML_IMPL_HPP_ */


