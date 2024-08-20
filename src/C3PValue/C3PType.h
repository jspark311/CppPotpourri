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

TODO: Add cross-type support for TCode::STR. atoi(), toString(), etc...

TODO: There is inconsistency in this subsystem regarding property lookups.
  Mostly surrounding booleans. It would be nice to unconfuse this, and do it
  one way. Those schizophrenic parts of the API have been labled as such.
*/

#ifndef __C3P_TYPE_WRAPPER_H
#define __C3P_TYPE_WRAPPER_H

#include <inttypes.h>
#include <stddef.h>
#include "../Vector3.h"

// For-dec of class types to avoid including them.
class StringBuilder;
class C3PType;
class Identity;
class StopWatch;
class TimeSeriesBase;
class Image;
class KeyValuePair;


/*******************************************************************************
* Type codes, flags, and surrounding fixed values.                             *
*******************************************************************************/
/**
* These are the different flags that might apply to a type. They are constants.
*
* TODO: TCODE_FLAG_VALUE_IS_PUNNED_PTR, TCODE_FLAG_VALUE_BY_COPY, TCODE_FLAG_PTR_LEN_TYPE
*   are mirrored on a per-instance basis in C3PValue. It is confusing, at best.
*/
#define TCODE_FLAG_VALUE_BY_COPY       0x01  // This type should be deep-copied for this platform.
#define TCODE_FLAG_VALUE_IS_PUNNED_PTR 0x02  // This type is small enough to fit inside a void* on this platform. This includes other pointer types.
#define TCODE_FLAG_PTR_LEN_TYPE        0x04  // Some types consist of a pointer and an element count.
#define TCODE_FLAG_NULL_TERMIMNATED    0x08  // Various string types are variable-length, yet self-delimiting.
#define TCODE_FLAG_LEGAL_FOR_ENCODING  0x10  // This type is a legal argument to (de)serializers.
#define TCODE_FLAG_RESERVED_2          0x20  // Reserved for future use.
#define TCODE_FLAG_RESERVED_1          0x40  // Reserved for future use.
#define TCODE_FLAG_RESERVED_0          0x80  // Reserved for future use.

// C-style strings and their aliases have a common flag set.
// We give them the PUNNED_PTR flag to ensure that they are settable without
//   memory implications.
#define TCODE_FLAG_MASK_STRING_TYPE ( \
  TCODE_FLAG_VALUE_IS_PUNNED_PTR | TCODE_FLAG_NULL_TERMIMNATED)


/*
* A list of parameter types that are handled by C3P's interchange layer.
* These should be supported in the type system, regardless of support in the
*   actual binary.
*/
enum class TCode : uint8_t {
  /* Numeric primitives */
  NONE          = 0x00,    // Reserved. Denotes end-of-list in type strings.
  UINT8         = 0x01,    // Unsigned 8-bit integer
  UINT16        = 0x02,    // Unsigned 16-bit integer
  UINT32        = 0x03,    // Unsigned 32-bit integer
  UINT64        = 0x04,    // Unsigned 64-bit integer
  UINT128       = 0x05,    // Unsigned 128-bit integer
  INT8          = 0x06,    // 8-bit integer
  INT16         = 0x07,    // 16-bit integer
  INT32         = 0x08,    // 32-bit integer
  INT64         = 0x09,    // 64-bit integer
  INT128        = 0x0A,    // 128-bit integer
  BOOLEAN       = 0x0B,    // A boolean
  FLOAT         = 0x0C,    // A float
  DOUBLE        = 0x0D,    // A double

  /* Numeric vectors */
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

  /* Our basic notions of variable-length data. */
  BINARY        = 0x50,    // A collection of bytes
  STR           = 0x51,    // A null-terminated string

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
  CSV_STR       = 0x6A,    // Alias of STR that carries the semantic 'Comma Separated Values'.

  /* Pointers to internal class instances */
  KVP           = 0xE0,    // A pointer to a KeyValuePair
  STR_BUILDER   = 0xE1,    // A pointer to a StringBuilder
  IDENTITY      = 0xE2,    // A pointer to an Identity class
  AUDIO         = 0xE3,    // A pointer to an audio stream
  IMAGE         = 0xE4,    // A pointer to an image class
  GEOLOCATION   = 0xE5,    // A pointer to a location class
  STOPWATCH     = 0xE6,    // A pointer to a StopWatch class
  TRACE         = 0xE7,    // A pointer to a C3PTrace.
  CHECKLIST     = 0xE8,    // A pointer to an AsyncSequencer.
  TIMESERIES    = 0xE9,    // A pointer to a unit-controlled TimeSeries.

  RESERVED      = 0xFE,    // Reserved for custom extension.
  INVALID       = 0xFF     // A code denoting TCode invalidity.
};


/* Quick inlines to facilitate moving into and out of serialization. */
// TODO: Schizophrenic API, option #1 (global-scope fxns for type properties)
inline uint8_t TcodeToInt(const TCode code) {   return (const uint8_t) code; };
inline TCode IntToTcode(const uint8_t code) {   return (const TCode) code;   };
const char* const typecodeToStr(const TCode);
const bool typeIsFixedLength(const TCode);
const bool typeIsPointerPunned(const TCode);
const int sizeOfType(const TCode);

/* Global function that is the entry-point for the type API. */
C3PType* getTypeHelper(const TCode);

/*
* Inlines that return a TCode the represents that type in the argument.
* These are useful for greasing template-escape elsewhere.
*/
inline const TCode tcodeForType(int8_t) {              return TCode::INT8;           };
inline const TCode tcodeForType(int16_t) {             return TCode::INT16;          };
inline const TCode tcodeForType(int32_t) {             return TCode::INT32;          };
inline const TCode tcodeForType(int64_t) {             return TCode::INT64;          };
inline const TCode tcodeForType(uint8_t) {             return TCode::UINT8;          };
inline const TCode tcodeForType(uint16_t) {            return TCode::UINT16;         };
inline const TCode tcodeForType(uint32_t) {            return TCode::UINT32;         };
inline const TCode tcodeForType(uint64_t) {            return TCode::UINT64;         };
inline const TCode tcodeForType(bool) {                return TCode::BOOLEAN;        };
inline const TCode tcodeForType(float) {               return TCode::FLOAT;          };
inline const TCode tcodeForType(double) {              return TCode::DOUBLE;         };
inline const TCode tcodeForType(char*) {               return TCode::STR;            };
inline const TCode tcodeForType(const char*) {         return TCode::STR;            };
inline const TCode tcodeForType(Vector3<int8_t>*) {    return TCode::VECT_3_INT8;    };
inline const TCode tcodeForType(Vector3<int16_t>*) {   return TCode::VECT_3_INT16;   };
inline const TCode tcodeForType(Vector3<int32_t>*) {   return TCode::VECT_3_INT32;   };
inline const TCode tcodeForType(Vector3<uint8_t>*) {   return TCode::VECT_3_UINT8;   };
inline const TCode tcodeForType(Vector3<uint16_t>*) {  return TCode::VECT_3_UINT16;  };
inline const TCode tcodeForType(Vector3<uint32_t>*) {  return TCode::VECT_3_UINT32;  };
inline const TCode tcodeForType(Vector3<float>*) {     return TCode::VECT_3_FLOAT;   };
inline const TCode tcodeForType(Vector3<double>*) {    return TCode::VECT_3_DOUBLE;  };
inline const TCode tcodeForType(KeyValuePair*) {       return TCode::KVP;            };
inline const TCode tcodeForType(StringBuilder*) {      return TCode::STR_BUILDER;    };
inline const TCode tcodeForType(Identity*) {           return TCode::IDENTITY;       };
inline const TCode tcodeForType(Image*) {              return TCode::IMAGE;          };
inline const TCode tcodeForType(StopWatch*) {          return TCode::STOPWATCH;      };
inline const TCode tcodeForType(TimeSeriesBase*) {     return TCode::TIMESERIES;     };


/*******************************************************************************
* This is a binder object for consolidating pointer-length parameters into a
*   single object. Some binary types need this for ease of handling.
* Objects of this type are mainly internal to C3PType and
*   C3PValue, and will always be heap-allocated (and free'd) by those classes
*   when necessary.
* Buffer write-through semantics are abstracted by C3PType's API.
*******************************************************************************/
typedef struct c3p_bin_binder_t {
  uint8_t*  buf;    // TCode::BINARY implies a second parameter (length). This
  uint32_t  len;    //   shim holds pointer and length as a single object.
  //TCode     tcode;  // This allows alias support.
} C3PBinBinder;


/*******************************************************************************
* Template for helper functions, and an interface for their concealment.       *
*******************************************************************************/

/*
* This class describes the interface to the type-wrapper helper functions. It
*   must be implemented (and instanced) as a template, but this API for its use
*   doesn't require built-time knowledge about the specific type.
* This piece of the type subsystem should be const to facilitate memory
*   placement goals. Mere support for a type should not consume RAM. Only flash.
*/
class C3PType {
  public:
    const char* const NAME;
    const uint16_t    FIXED_LEN;
    const TCode       TCODE;

    // Implementation for a specific type needs to implement these.
    virtual uint32_t length(void* obj) =0;
    virtual void     to_string(void* obj, StringBuilder*) =0;
    virtual int8_t   representable_by(const TCode DEST_TYPE) =0;
    virtual int8_t   set_from(void* dest, const TCode SRC_TYPE, void* src) =0;
    virtual int8_t   get_as(void* src, const TCode DEST_TYPE, void* dest) =0;
    virtual int8_t   construct(void* obj, KeyValuePair*) =0;
    virtual int8_t   destruct(void* obj) =0;
    // TODO: Commented def will be _far_ easier on mem-constrained builds.
    //virtual int      serialize(void* obj, StringBuilder*, const TCode FORMAT, const uint32_t MAX_LEN)  =0;
    virtual int      serialize(void* obj, StringBuilder*, const TCode FORMAT)  =0;
    virtual int      deserialize(void* obj, StringBuilder*, const TCode FORMAT, const uint32_t OFFSET) =0;

    // TODO: Schizophrenic API, option #2 (flag accessors buried within a C3PType instance)
    const bool legal_for_encoding() {  return _all_flags_set(TCODE_FLAG_LEGAL_FOR_ENCODING);  };
    const bool null_terminated() {     return _all_flags_set(TCODE_FLAG_NULL_TERMIMNATED);    };
    const bool value_by_copy() {       return _all_flags_set(TCODE_FLAG_VALUE_BY_COPY);       };
    const bool is_ptr() {              return _all_flags_clear(TCODE_FLAG_VALUE_BY_COPY);     };
    const bool is_punned_ptr() {       return _all_flags_set(TCODE_FLAG_VALUE_IS_PUNNED_PTR); };
    const bool is_ptr_len() {          return _all_flags_set(TCODE_FLAG_PTR_LEN_TYPE);        };
    const bool is_fixed_length() {     return (FIXED_LEN > 0);    };

    // Static functions to facilitate lookup of common type properties.
    // TODO: Schizophrenic API, option #3 (a blend of options #1 and #2)
    static int8_t conversionRisk(const TCode, const TCode);
    static int8_t exportTypeMap(StringBuilder*, const TCode);
    static const bool is_numeric(const TCode);
    static const bool is_signed(const TCode);
    static const bool is_integral(const TCode);


  protected:
    C3PType(const char* const type_name, const uint16_t fixed_len, const TCode tcode, const uint8_t flags) :
      NAME(type_name), FIXED_LEN(fixed_len), TCODE(tcode), _FLAGS(flags) {};

    int8_t _type_blind_copy(void* src, void* dest, const TCode);
    int    _type_blind_serialize(void* obj, StringBuilder*, const TCode FORMAT);
    void   _type_blind_to_string(void* obj, StringBuilder*);
    bool   _pointer_safety_check(void* obj);


  private:
    const uint8_t _FLAGS;
    inline const bool _all_flags_set(const uint8_t MASK) {    return (MASK == (_FLAGS & MASK));  };
    inline const bool _all_flags_clear(const uint8_t MASK) {  return (0 == (_FLAGS & MASK));     };
    inline const bool _any_flags_set(const uint8_t MASK) {    return (0 != (_FLAGS & MASK));     };
    inline const bool _any_flags_clear(const uint8_t MASK) {  return (MASK != (_FLAGS & MASK));  };
};



/*
* The template below contains helper functions for each TCode.
* This is a concealed template for containment of per-type complexity in C3P's
*   type-wrapping system. Supported types should have a 1-to-1 implementation
*   for each TCode.
*
* Functions implemented in this header will be isomorphic for all TCodes, unless
*   over-ridden. The compiler will autogenerate such functions for each type
*   that doesn't provide its own implemention.
*/
template <class T> class C3PTypeConstraint : public C3PType {
  public:
    C3PTypeConstraint(const char* const type_name, const uint16_t fixed_len, const TCode tcode, const uint8_t flags) :
      C3PType(type_name, fixed_len, tcode, flags) {};
    ~C3PTypeConstraint() {};

    // These functions are optional overrides. An explicit type implementation
    //   that does not provide these will have autogenerated implementations.
    int8_t   construct(void* obj, KeyValuePair*) {  return -1;  };  // NOTE: If germane, must be implemented explicitly.
    int8_t   destruct(void* obj) {                  return -1;  };  // NOTE: If germane, must be implemented explicitly.
    uint32_t length(void* obj) {                                      return FIXED_LEN;                               };
    void     to_string(void* obj, StringBuilder* out) {               _type_blind_to_string(obj, out);                };
    int8_t   representable_by(const TCode DEST_TYPE) {                return ((DEST_TYPE == TCODE) ? 0 : -1);         };
    int8_t   set_from(void* dest, const TCode SRC_TYPE, void* src) {  return _type_blind_copy(src, dest, SRC_TYPE);   };
    int8_t   get_as(void* src, const TCode DEST_TYPE, void* dest) {   return _type_blind_copy(src, dest, DEST_TYPE);  };
    int      serialize(void* obj, StringBuilder* out, const TCode FORMAT) {  return _type_blind_serialize(obj, out, FORMAT);  };
    int      deserialize(void* obj, StringBuilder*,   const TCode FORMAT, const uint32_t OFFSET) {  return -1;  };


  private:
    // Use these functions instead of pointer dereference.
    // Values of some real types (particularly float and double) are
    //   highly-sensitive to alignment. These two functions allow the compiler
    //   to make whatever types's alignment concerns an independent matter from
    //   its storage address.
    T _load_from_mem(void* src) {
      T aligned;
      memcpy((void*) &aligned, src, length(src));
      return aligned;
    };

    bool _store_in_mem(void* dest, T src) {
      memcpy(dest, (void*) &src, length((void*) &src));
      return true;  // NOTE: Place-holder. This layer not responsible for null-checks.
    };
};

#endif  // __C3P_TYPE_WRAPPER_H
