
/*
 * 
 * (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
 *
 */


/**
 * @file rwnc_xml.hpp
 * @author Rajesh Velandy
 * @date 2014/06/12
 * @brief RW.Netconf client library XML abstraction support defintions.
 */

#ifndef RWNC_XML_HPP_
#define RWNC_XML_HPP_

#include <rwlib.h>
#include <rw_xml.h>
#include <rwnetconf.h>

#include <rw_cf_type_validate.hpp>

#include <string>
#include <iostream>

namespace rw_netconf {

// Forward declarations
class Instance;

/**
 * @addtogroup RwNcXml
 * @{
 */

/**
 * Abstract class representing an XML blob in the RWNC Library.
 * XML blob is used by  RWNC Client library to pass arbitary XML snippets around.
 */
class Xml
: public rw_ncclnt_xml_t
{
  
  RW_CF_TYPE_CLASS_DECLARE_MEMBERS(rw_ncclnt_xml_t, rw_ncclnt_xml_ptr_t, Xml)
  
public:

  /// Construct an XML Blob object
  Xml(
    Instance *instance /// [in] RWNc client libraray instance
  );

  /// Destruct an XML Blob object
  virtual ~Xml();

  // Cannot copy
  Xml(const Xml&) = delete;
  Xml& operator=(const Xml&) = delete;

  /// Create a XML blob from a UTF-8 string.
  /// The string will be copied into the XML object, so it may be allocated on the stack
  static suptr_t create_xml_copy_string(
                                      Instance *instance, /// [in] RWNc client libraray instance
                                     const char *str ///< [in] the string from which the XML object need to be created
                                    );

  /// Create a XML blob from a UTF-8 buffer.
  /// The buffer will be copied into the XML object, so it may be allocated on the stack
  static suptr_t create_xml_copy_buffer(
                                      Instance *instance, /// [in] RWNc client libraray instance
                                     const void *buff,///< [in] the buffer from which the XML object need to be created
                                     size_t len ///< [in] optional buffer len -- If not specified len defaults to strlen(buffer)
                                    );

  /// Create an XML blob from a UTF-8 string. The string will be referred directly by the pointer
  /// passed in. Therefore it must live as long as the XML object.
  static suptr_t create_xml_const_string(
                                      Instance *instance, /// [in] RWNc client libraray instance
                                      const char *str ///< [in]  the string based on which XML object need to be generated. The string will be used by the XNL object
                                     );

  /// Create an XML blob from a UTF-8 buffer. The buffer will be referred directly by the pointer
  /// passed in. Therefore it must live as long as the XML object.
  static suptr_t create_xml_const_buffer(
                                      Instance *instance, /// [in] RWNc client libraray instance
                                      const void *buff, ///< [in]  the buffer based on which XML object need to be generated. The string will be used by the XNL object
                                      size_t len ///< [in] optional buffer len -- If not specified len defaults to strlen(buffer)
                                     );

  /// gets the UTF-8 XML string as a buffer - The caller owns the buffer
  /// cursor is advanced by the length of the returnred XML string
  /// which will be smaller of size-1 or bytes_left()
  //  The returned string will be null terminated when size > 0
  virtual char* get_xml_str(
      char *s,  ///< [in] pointer to the returned string- must be atlast size+1 long

      size_t size,      ///< [in] The size of the passed buffer
      size_t *outsize   ///< [in] The number of bytes returned
   ) = 0;

  /// Returns true if the stream has reached end of  the stream
  virtual bool eof() const = 0;

  /// Returns the remaining bytes in the XML blob  based on the cursor- 0 if end of the stream
  virtual size_t bytes_left() const= 0;

  /// Returns the  Network client API instance from the XML Blob
  // Not owned by the caller -- Use retain if the reference will be kept
  virtual Instance* get_instance() const;


  /// Overloaded << operator to print this object
  friend std::ostream&  operator <<(std::ostream& os, const Xml& xml);

  /** Create a new XML blob from a prefix string, this blob, and a suffix
   *  string.  The prefix and suffix provide enclosing XML fragment
   *  strings that are not proper XML by themselves, but together form a
   *  valid XML fragment.  The prefix and suffix strings are assumed valid
   *  and will not be validated.
   *
   *  The net result is that the XML blob gets inserted as a child of the XML
   *  fragment defined by the prefix and suffix strings.
   *
   *  For example, given the following inputs:
   *  @code
   *    prefix = "<root>";
   *    this blob === "<data>string</data>";
   *    suffix = "</root>";
   *  @endcode
   *
   *  The end result would be:
   *  @code
   *    result === "<root><data>string</data></root>";
   *  @endcode
   *
   *  @return new XML blob .  Caller owns the returned blob.
   */

  virtual suptr_t create_composed(
    const char *prefix,  ///< [in] The NUL-terminated prefix string
    const char *suffix    ///< [in] The NUL-terminated suffix string
  ) const = 0;

  /**
   * Create a new XML blob appending the passed suffix XML Blob.
   *
   * For example, given the following inputs:
   * @code
   *   this blob === "<data>string1</data>";
   *   suffix xml blob  === "<data>string2</data>";
   * @endcode
   *
   * The end result would be:
   * @code
   *   result === "<data>string1</data><data>string2</data>";
   * @endcode
   *
   * @return new XML blob .  Caller owns the returned blob.
   *
   */
   virtual suptr_t append(
     Xml *suffix_blob /// [in] suffix XML Blob
   ) const = 0;

  /**
   * Create a new XML blob appending the passed suffix XML string.
   *
   * For example, given the following inputs:
   * @code
   *   this blob === "<data>string1</data>";
   *   suffix xml string  === "<data>string2</data>";
   * @endcode
   *
   * The end result would be:
   * @code
   *   result === "<data>string1</data><data>string2</data>";
   * @endcode
   *
   * @return new XML blob .  Caller owns the returned blob.
   *
   */
   virtual suptr_t append(
     const char* xml_str /// [in] suffix XML string
   ) const = 0;

  /// Instance virtual print method  which prints the object to the passed stream-
  //  Implementation need to implement this
  virtual void print(std::ostream& os)  const = 0;

  /**
   * Create a RWXML dom based on this XML.
   * ATTN: This function will be deprecated when more nc::XML types are available
   *
   * @return uptr to newly created XML DOM
   */
  rw_yang::XMLDocument::uptr_t create_dom(
      rw_yang::XMLManager *mgr, 
      bool validate = true  /// [in] validate?
 ); /**< The XML Manager that manages the document */


protected:

  /// The  RWNC Client Instance
  UniquishPtrRwMemoryTracking<Instance>::suptr_t instance_;

};

RW_CF_TYPE_CLASS_DEFINE_INLINES(rw_ncclnt_xml_t, rw_ncclnt_xml_ptr_t, Xml)
/** @} */
} /* namespace rw_netconf */

RW_CF_TYPE_CLASS_DEFINE_TRAITS(rw_ncclnt_xml_t, rw_ncclnt_xml_ptr_t, rw_netconf::Xml)
#endif /* RWNC_XML_HPP_ */

