/*
File:   C3PType.h
Author: J. Ian Lindsay
Date:   2023.10.19

Copyright 2016 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


The TCode enum and surrounding functions should not be needed outside of a few
  special-cases in parser/packer code. Most of it is abstracted away by type
  polymorphism in classes that need to distinguish types. Notably: C3PValue and
  KeyValuePair.

TODO: Might-should adopt some IANA standard code-spaces here? Is there a
  painless way to get better inter-op? Dig...
*/

#ifndef __C3P_TYPE_WRAPPER_H
#define __C3P_TYPE_WRAPPER_H

#include <inttypes.h>
#include <stddef.h>
class StringBuilder;

/*******************************************************************************
* Type codes, flags, and other surrounding fixed values.                       *
*******************************************************************************/
/**
* These are the different flags that might apply to a type. They are constants.
*/
#define TCODE_FLAG_NON_EXPORTABLE      0x01  // This type is not exportable to other systems.
#define TCODE_FLAG_VALUE_IS_PUNNED_PTR 0x02  // This type is small enough to fit inside a void* on this platform.
#define TCODE_FLAG_VARIABLE_LEN        0x04  // Some types do not have a fixed-length.
#define TCODE_FLAG_IS_NULL_DELIMITED   0x08  // Various string types are variable-length, yet self-delimiting.
#define TCODE_FLAG_RESERVED_2          0x10  //
#define TCODE_FLAG_LEGAL_FOR_ENCODING  0x20  // This type is a legal argument to (de)serializers.
#define TCODE_FLAG_RESERVED_1          0x40  // Reserved for future use.
#define TCODE_FLAG_RESERVED_0          0x80  // Reserved for future use.


/*
* A list of parameter types that are handled by the input parser.
* These should be supported in the type system, regardless of support in the
*   actual binary.
*/
enum class TCode : uint8_t {
  /* Primitives */
  NONE          = 0x00,    // Reserved. Denotes end-of-list.
  INT8          = 0x01,    // 8-bit integer
  INT16         = 0x02,    // 16-bit integer
  INT32         = 0x03,    // 32-bit integer
  UINT8         = 0x04,    // Unsigned 8-bit integer
  UINT16        = 0x05,    // Unsigned 16-bit integer
  UINT32        = 0x06,    // Unsigned 32-bit integer
  INT64         = 0x07,    // 64-bit integer
  INT128        = 0x08,    // 128-bit integer
  UINT64        = 0x09,    // Unsigned 64-bit integer
  UINT128       = 0x0A,    // Unsigned 128-bit integer
  BOOLEAN       = 0x0B,    // A boolean
  FLOAT         = 0x0C,    // A float
  DOUBLE        = 0x0D,    // A double
  BINARY        = 0x0E,    // A collection of bytes
  STR           = 0x0F,    // A null-terminated string

  /* Compound numeric types */
  VECT_2_FLOAT  = 0x30,    // A vector of floats in 2-space
  VECT_2_DOUBLE = 0x31,    // A vector of floats in 2-space
  VECT_2_INT8   = 0x32,    // A vector of signed integers in 2-space
  VECT_2_UINT8  = 0x33,    // A vector of unsigned integers in 2-space
  VECT_2_INT16  = 0x34,    // A vector of signed integers in 2-space
  VECT_2_UINT16 = 0x35,    // A vector of unsigned integers in 2-space
  VECT_2_INT32  = 0x36,    // A vector of signed integers in 2-space
  VECT_2_UINT32 = 0x37,    // A vector of unsigned integers in 2-space
  VECT_3_FLOAT  = 0x38,    // A vector of floats in 3-space
  VECT_3_DOUBLE = 0x39,    // A vector of floats in 3-space
  VECT_3_INT8   = 0x3A,    // A vector of signed integers in 3-space
  VECT_3_UINT8  = 0x3B,    // A vector of unsigned integers in 3-space
  VECT_3_INT16  = 0x3C,    // A vector of signed integers in 3-space
  VECT_3_UINT16 = 0x3D,    // A vector of unsigned integers in 3-space
  VECT_3_INT32  = 0x3E,    // A vector of signed integers in 3-space
  VECT_3_UINT32 = 0x3F,    // A vector of unsigned integers in 3-space
  VECT_4_FLOAT  = 0x40,    // A vector of floats in 4-space

  /* Encoded buffers and semantic aliases to other types */
  URL           = 0x60,    // An alias of STR that carries the semantic 'URL'.
  JSON          = 0x61,    // A JSON object. Semantic layer on STR.
  CBOR          = 0x62,    // A CBOR object. Semantic layer on BINARY.
  LATLON        = 0x63,    // An alias of VECT_2_DOUBLE that indicates a point on a sphere.
  COLOR8        = 0x64,    // Alias of UINT8. 8-bit color data
  COLOR16       = 0x65,    // Alias of UINT16. 16-bit color data
  COLOR24       = 0x66,    // Alias of UINT32. 24-bit color data
  SI_UNIT       = 0x67,    // Alias of STR. A sequence of SIUnit enum values to describe a quantity.
  BASE64        = 0x68,    // Alias of STR that carries the semantic 'Base-64 encoded'.
  IPV4_ADDR     = 0x69,    // Alias of UINT32. An IP address, in network byte-order.

  /* Pointers to internal class instances */
  KVP           = 0xE0,    // A pointer to a KeyValuePair
  STR_BUILDER   = 0xE1,    // A pointer to a StringBuilder
  IDENTITY      = 0xE2,    // A pointer to an Identity class
  AUDIO         = 0xE3,    // A pointer to an audio stream
  IMAGE         = 0xE4,    // A pointer to an image class
  GEOLOCATION   = 0xE5,    // A pointer to a location class

  RESERVED      = 0xFE,    // Reserved for custom extension.
  INVALID       = 0xFF     // A code denoting TCode invalidity.
};


/* Quick inlines to facilitate moving into and out of serialization. */
inline uint8_t TcodeToInt(const TCode code) {   return (const uint8_t) code; };
inline TCode IntToTcode(const uint8_t code) {   return (const TCode) code;   };

const char* const typecodeToStr(const TCode);
const bool typeIsFixedLength(const TCode);
const int typeIsPointerPunned(const TCode);
const int sizeOfType(const TCode);
const int typeConversionRisk(const TCode FROM, const TCode INTO);



/* A binder object for consolidating parameters (should it be needed). */
typedef struct c3p_bin_binder_t {
  uint8_t*  buf;  // TCode::BINARY implies a second parameter (length). This
  uint32_t  len;  //   shim holds pointer and length as a single object.
} C3PBinBinder;


/*******************************************************************************
* Template for helper functions, and an interface for their concealment.       *
*******************************************************************************/

/*
* This class describes the interface to the type-wrapper helper functions. It
*   must be implemented (and instanced) as a template, but this is the API for
*   its use.
*/
class C3PType {
  public:
    virtual const TCode tcode()                                                    =0;
    virtual uint32_t    length(void* obj)                                          =0;
    virtual void        to_string(void* obj, StringBuilder*)                       =0;
    virtual int8_t      set_from(void* dest, const TCode SRC_TYPE, void* src)      =0;
    virtual int8_t      get_as(void* src, const TCode DEST_TYPE, void* dest)       =0;
    virtual int8_t      serialize(void* obj, StringBuilder*, const TCode FORMAT)   =0;
    virtual int8_t      deserialize(void* obj, StringBuilder*, const TCode FORMAT) =0;
};


/*
* The template below contains helper functions for each TCode.
* This is a concealed template for containment of per-type complexity in C3P's
*   type-wrapping system. Supported types should have a 1-to-1 implementation
*   for each TCode.
*/
template <class T> class C3PTypeConstraint : public C3PType {
  public:
    C3PTypeConstraint() {};
    ~C3PTypeConstraint() {};

    const TCode tcode();
    uint32_t    length(void* obj);
    void        to_string(void* obj, StringBuilder*);
    int8_t      set_from(void* dest, const TCode SRC_TYPE, void* src);
    int8_t      get_as(void* src, const TCode DEST_TYPE, void* dest);
    int8_t      serialize(void* obj, StringBuilder*, const TCode FORMAT);
    int8_t      deserialize(void* obj, StringBuilder*, const TCode FORMAT);
};

#endif  // __C3P_TYPE_WRAPPER_H
