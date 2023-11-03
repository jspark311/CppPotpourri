/*
File:   C3PType.cpp
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

This file contains the implementation of type constraints for our wrapped types.
*/


#include <stdint.h>
#include <string.h>
#include "C3PValue.h"
#include "KeyValuePair.h"
#include "../StringBuilder.h"
#include "../Identity/Identity.h"
#include "../StringBuilder.h"
#include "../TimerTools/TimerTools.h"

/* CBOR support should probably be required to parse/pack. */
#if defined(CONFIG_C3P_CBOR)
  #include "../cbor-cpp/cbor.h"
#endif


/*******************************************************************************
* Statics related to type support.
*******************************************************************************/
// Static initializers for our type map that gives us runtime type constraints.
static const C3PTypeConstraint<int8_t>          c3p_type_helper_int8(         "INT8",         1,  TCode::INT8,           (TCODE_FLAG_VALUE_IS_PUNNED_PTR));
static const C3PTypeConstraint<int16_t>         c3p_type_helper_int16(        "INT16",        2,  TCode::INT16,          (TCODE_FLAG_VALUE_IS_PUNNED_PTR));
static const C3PTypeConstraint<int32_t>         c3p_type_helper_int32(        "INT32",        4,  TCode::INT32,          (TCODE_FLAG_VALUE_IS_PUNNED_PTR));
static const C3PTypeConstraint<int64_t>         c3p_type_helper_int64(        "INT64",        8,  TCode::INT64,          (0));
static const C3PTypeConstraint<uint8_t>         c3p_type_helper_uint8(        "UINT8",        1,  TCode::UINT8,          (TCODE_FLAG_VALUE_IS_PUNNED_PTR));
static const C3PTypeConstraint<uint16_t>        c3p_type_helper_uint16(       "UINT16",       2,  TCode::UINT16,         (TCODE_FLAG_VALUE_IS_PUNNED_PTR));
static const C3PTypeConstraint<uint32_t>        c3p_type_helper_uint32(       "UINT32",       4,  TCode::UINT32,         (TCODE_FLAG_VALUE_IS_PUNNED_PTR));
static const C3PTypeConstraint<uint64_t>        c3p_type_helper_uint64(       "UINT64",       8,  TCode::UINT64,         (0));
static const C3PTypeConstraint<float>           c3p_type_helper_float(        "FLOAT",        4,  TCode::FLOAT,          (TCODE_FLAG_VALUE_IS_PUNNED_PTR));
static const C3PTypeConstraint<double>          c3p_type_helper_double(       "DOUBLE",       8,  TCode::DOUBLE,         (0));
static const C3PTypeConstraint<bool>            c3p_type_helper_bool(         "BOOL",         1,  TCode::BOOLEAN,        (TCODE_FLAG_VALUE_IS_PUNNED_PTR));
static const C3PTypeConstraint<char*>           c3p_type_helper_str(          "STR",          0,  TCode::STR,            (TCODE_FLAG_MASK_STRING_TYPE));
static const C3PTypeConstraint<StringBuilder>   c3p_type_helper_stringbuilder("STR_BLDR",     0,  TCode::STR_BUILDER,    (TCODE_FLAG_VALUE_IS_POINTER));
static const C3PTypeConstraint<Vector3i8>       c3p_type_helper_vect3_i8(     "VEC3_INT8",    3,  TCode::VECT_3_INT8,    (0));
static const C3PTypeConstraint<Vector3i16>      c3p_type_helper_vect3_i16(    "VEC3_INT16",   6,  TCode::VECT_3_INT16,   (0));
static const C3PTypeConstraint<Vector3i32>      c3p_type_helper_vect3_i32(    "VEC3_INT32",   12, TCode::VECT_3_INT32,   (0));
static const C3PTypeConstraint<Vector3u8>       c3p_type_helper_vect3_u8(     "VEC3_UINT8",   3,  TCode::VECT_3_UINT8,   (0));
static const C3PTypeConstraint<Vector3u16>      c3p_type_helper_vect3_u16(    "VEC3_UINT16",  6,  TCode::VECT_3_UINT16,  (0));
static const C3PTypeConstraint<Vector3u32>      c3p_type_helper_vect3_u32(    "VEC3_UINT32",  12, TCode::VECT_3_UINT32,  (0));
static const C3PTypeConstraint<Vector3f>        c3p_type_helper_vect3_float(  "VEC3_FLOAT",   12, TCode::VECT_3_FLOAT,   (0));
static const C3PTypeConstraint<Vector3f64>      c3p_type_helper_vect3_double( "VEC3_DOUBLE",  24, TCode::VECT_3_DOUBLE,  (0));
static const C3PTypeConstraint<Image>           c3p_type_helper_image(        "IMAGE",        0,  TCode::IMAGE,          (TCODE_FLAG_VALUE_IS_POINTER));
static const C3PTypeConstraint<KeyValuePair>    c3p_type_helper_kvp(          "KVP",          0,  TCode::KVP,            (TCODE_FLAG_VALUE_IS_POINTER));
static const C3PTypeConstraint<Identity*>       c3p_type_helper_identity(     "IDENTITY",     0,  TCode::IDENTITY,       (TCODE_FLAG_VALUE_IS_POINTER));
static const C3PTypeConstraint<StopWatch*>      c3p_type_helper_stopwatch(    "STOPWATCH",   sizeof(StopWatch),  TCode::STOPWATCH,      (TCODE_FLAG_VALUE_IS_POINTER));

// Type-indirected handlers (parameter binders).
static const C3PTypeConstraint<C3PBinBinder>   c3p_type_helper_ptrlen(        "BINARY",       0,  TCode::BINARY,         (TCODE_FLAG_PTR_LEN_TYPE | TCODE_FLAG_LEGAL_FOR_ENCODING));

//{TCode::SI_UNIT,        (TCODE_FLAG_MASK_STRING_TYPE),                             0,  "SI_UNIT",       (C3PType*) &c3p_type_helper_str            },
//{TCode::UINT128,        (0),                                                       16, "UINT128",       nullptr },
//{TCode::INT128,         (0),                                                       16, "INT128",        nullptr },
//{TCode::COLOR8,         (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          1,  "COLOR8",        nullptr },
//{TCode::COLOR16,        (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          2,  "COLOR16",       nullptr },
//{TCode::COLOR24,        (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          3,  "COLOR24",       nullptr },
//{TCode::BASE64,         (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "BASE64",        nullptr },
//{TCode::JSON,           (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "JSON",          nullptr },
//{TCode::CBOR,           (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "CBOR",          nullptr },
//{TCode::GEOLOCATION,    (TCODE_FLAG_VARIABLE_LEN),                                 0,  "GEOLOCATION",   nullptr},
//{TCode::VECT_2_FLOAT,   (0),                                                       0,  "VEC2_FLOAT"};
//{TCode::VECT_2_DOUBLE,  (0),                                                       0,  "VEC2_DOUBLE"};
//{TCode::VECT_2_INT8,    (0),                                                       0,  "VEC2_INT8"};
//{TCode::VECT_2_UINT8,   (0),                                                       0,  "VEC2_UINT8"};
//{TCode::VECT_2_INT16,   (0),                                                       0,  "VEC2_INT16"};
//{TCode::VECT_2_UINT16,  (0),                                                       0,  "VEC2_UINT16"};
//{TCode::VECT_2_INT32,   (0),                                                       0,  "VEC2_INT32"};
//{TCode::VECT_2_UINT32,  (0),                                                       0,  "VEC2_UINT32"};
//{TCode::VECT_4_FLOAT,   (0),                                                       16, "VEC4_FLOAT"},
//{TCode::IPV4_ADDR,      (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          4,  "IPV4_ADDR"};
//{TCode::URL,            (TCODE_FLAG_MASK_STRING_TYPE),                             1,  "URL"},
//{TCode::AUDIO,          (TCODE_FLAG_VARIABLE_LEN),                                 0,  "AUDIO"},


/**
* Given a type code, find and return the entire C3PType.
* If the type isn't here, we won't be able to handle it.
*
* @param  TCode the type code being asked about.
* @return The desired C3PType, or nullptr on "not supported".
*/
static const C3PType* _get_type_def(const TCode TC) {
  switch (TC) {
    case TCode::INT8:            return (const C3PType*) &c3p_type_helper_int8;
    case TCode::INT16:           return (const C3PType*) &c3p_type_helper_int16;
    case TCode::INT32:           return (const C3PType*) &c3p_type_helper_int32;
    case TCode::INT64:           return (const C3PType*) &c3p_type_helper_int64;
    case TCode::UINT8:           return (const C3PType*) &c3p_type_helper_uint8;
    case TCode::UINT16:          return (const C3PType*) &c3p_type_helper_uint16;
    case TCode::UINT32:          return (const C3PType*) &c3p_type_helper_uint32;
    case TCode::UINT64:          return (const C3PType*) &c3p_type_helper_uint64;
    case TCode::FLOAT:           return (const C3PType*) &c3p_type_helper_float;
    case TCode::DOUBLE:          return (const C3PType*) &c3p_type_helper_double;
    case TCode::BOOLEAN:         return (const C3PType*) &c3p_type_helper_bool;
    case TCode::STR:             return (const C3PType*) &c3p_type_helper_str;
    case TCode::STR_BUILDER:     return (const C3PType*) &c3p_type_helper_stringbuilder;
    case TCode::VECT_3_INT8:     return (const C3PType*) &c3p_type_helper_vect3_i8;
    case TCode::VECT_3_INT16:    return (const C3PType*) &c3p_type_helper_vect3_i16;
    case TCode::VECT_3_INT32:    return (const C3PType*) &c3p_type_helper_vect3_i32;
    case TCode::VECT_3_UINT8:    return (const C3PType*) &c3p_type_helper_vect3_u8;
    case TCode::VECT_3_UINT16:   return (const C3PType*) &c3p_type_helper_vect3_u16;
    case TCode::VECT_3_UINT32:   return (const C3PType*) &c3p_type_helper_vect3_u32;
    case TCode::VECT_3_FLOAT:    return (const C3PType*) &c3p_type_helper_vect3_float;
    case TCode::VECT_3_DOUBLE:   return (const C3PType*) &c3p_type_helper_vect3_double;
    case TCode::IMAGE:           return (const C3PType*) &c3p_type_helper_image;
    case TCode::KVP:             return (const C3PType*) &c3p_type_helper_kvp;
    case TCode::IDENTITY:        return (const C3PType*) &c3p_type_helper_identity;
    case TCode::STOPWATCH:       return (const C3PType*) &c3p_type_helper_stopwatch;
    case TCode::BINARY:          return (const C3PType*) &c3p_type_helper_ptrlen;

    case TCode::SI_UNIT:         return (const C3PType*) &c3p_type_helper_str;
    default:  break;
  }
  return nullptr;
}


/* Serialize the entire type map. */
int8_t C3PType::exportTypeMap(StringBuilder*, const TCode) {
  // TODO
  return -1;
}

/*******************************************************************************
* Support functions for dealing with type codes.                               *
*******************************************************************************/
/**
* Given a type code, return the string representation.
*
* @param  tc the type code being asked about.
* @return The string representation. Never NULL.
*/
const char* const typecodeToStr(const TCode tc) {
  const C3PType* def = _get_type_def(tc);
  if (nullptr != def) {
    return def->NAME;
  }
  return "UNKNOWN";
}

/**
* Does the given type code represent a type of fixed length.
*
* @param TC is the type code to evaluate.
* @return true if the type is a fixed-length.
*/
const bool typeIsFixedLength(const TCode TC) {
  const C3PType* def = _get_type_def(TC);
  if (nullptr != def) {
    return (0 != def->FIXED_LEN);
  }
  return false;
}

/**
* On a given ALU width, some types fit into the same space as a pointer.
* This function returns true if the given TCode represents such a type.
*
* @param  TCode the type code being asked about.
* @return The size of the type (in bytes), or -1 on failure.
*/
const bool typeIsPointerPunned(const TCode TC) {
  C3PType* def = getTypeHelper(TC);
  return ((nullptr != def) ? def->is_punned_ptr() : false);
}

/**
* Given type code, find size in bytes. Returns 0 for variable-length
*   arguments, since this is their minimum size.
*
* @param  TCode the type code being asked about.
* @return The size of the type (in bytes), or -1 on failure.
*/
const int sizeOfType(const TCode TC) {
  const C3PType* def = _get_type_def(TC);
  return ((nullptr != def) ? def->FIXED_LEN : -1);
}

/**
* Given type code, find the helper object.
*
* @param  TCode the type code being asked about.
* @return A pointer to the abstracted type handler.
*/
C3PType* getTypeHelper(const TCode TC) {
  return (C3PType*) _get_type_def(TC);
}


/*******************************************************************************
* C3PTypeConstraint
*******************************************************************************/

/*
* Base function that handles the trivial (common) case of value copy with no
*   conversion, and no awareness.
* For pointer types, this means a pointer (reference) copy. NOT a deep copy.
*/
int8_t C3PType::_type_blind_copy(void* src, void* dest, const TCode TEST_TCODE) {
  int8_t ret = -1;
  if ((nullptr != src) & (nullptr != dest)) {
    ret--;
    if (TEST_TCODE == TCODE) {
      const uint32_t COPY_LEN = length(src);
      memcpy(dest, src, COPY_LEN);
      ret = 0;
    }
  }
  return ret;
}


int C3PType::_type_blind_serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat(*((const char**) obj));
      break;
    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        // NOTE: This ought to work for any types where retaining portability
        //   isn't important.
        // TODO: Gradually convert types out of this block. As much as possible should
        //   be portable. VECT_3_FLOAT ought to be an array of floats, for instance.
        encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(TCODE));
        encoder.write_bytes((uint8_t*) obj, length(obj));
        ret = 0;
      }
      break;
    default:  break;
  }
  return ret;
}


void C3PType::_type_blind_to_string(void* obj, StringBuilder* out) {
  const uint32_t L_ENDER = length(obj);
  if (L_ENDER > 0) {  StringBuilder::printBuffer(out, (uint8_t*) obj, L_ENDER);  }
}



/*******************************************************************************
* C3PTypeConstraint
* The blocks below hold templated code that constrains our implemented types,
*   and manages each type in a place where it can be easilly disabled as a
*   type-wrapper candidate type.
*******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
/// int8_t
template <> void        C3PTypeConstraint<int8_t>::to_string(void* obj, StringBuilder* out) {  out->concatf("%d", *((int8_t*) obj));  }

template <> int8_t      C3PTypeConstraint<int8_t>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  int8_t* d = (int8_t*) dest;
  switch (SRC_TYPE) {
    case TCode::INT8:         *d = *((int8_t*) src);              return 0;
    case TCode::INT16:
      if ((*((int16_t*) src) <= INT8_MAX) & ((*((int16_t*) src) >= INT8_MIN))) {
        *d = (int8_t) *((int16_t*) src);
        return 0;
      }
      break;
    case TCode::INT32:
      if ((*((int32_t*) src) <= INT8_MAX) & ((*((int32_t*) src) >= INT8_MIN))) {
        *d = (int8_t) *((int32_t*) src);
        return 0;
      }
      break;
    case TCode::INT64:
      if ((*((int64_t*) src) <= INT8_MAX) & ((*((int64_t*) src) >= INT8_MIN))) {
        *d = (int8_t) *((int64_t*) src);
        return 0;
      }
      break;
    case TCode::UINT8:
      if (*((uint8_t*) src) <= INT8_MAX) {
        *d = (int8_t) *((uint8_t*) src);
        return 0;
      }
      break;
    case TCode::UINT16:
      if (*((uint16_t*) src) <= INT8_MAX) {
        *d = (int8_t) *((uint16_t*) src);
        return 0;
      }
      break;
    case TCode::UINT32:
      if (*((uint32_t*) src) <= INT8_MAX) {
        *d = (int8_t) *((uint32_t*) src);
        return 0;
      }
      break;
    case TCode::UINT64:
      if (*((uint64_t*) src) <= INT8_MAX) {
        *d = (int8_t) *((uint32_t*) src);
        return 0;
      }
      break;
    case TCode::BOOLEAN:      *d = (*((bool*) src) ? 1 : 0);      return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<int8_t>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  int8_t s = *((int8_t*) src);
  switch (DEST_TYPE) {
    case TCode::INT8:      *((int8_t*) dest) = s;                 return 0;
    case TCode::INT16:     *((int16_t*) dest) = (int16_t) s;      return 0;
    case TCode::INT32:     *((int32_t*) dest) = (int32_t) s;      return 0;
    case TCode::INT64:     *((int64_t*) dest) = (int64_t) s;      return 0;
    case TCode::UINT8:
      if (s >= 0) {
        *((uint8_t*) dest) = (uint8_t) s;
        return 0;
      }
      break;
    case TCode::UINT16:
      if (s >= 0) {
        *((uint16_t*) dest) = (uint16_t) s;
        return 0;
      }
      break;
    case TCode::UINT32:
      if (s >= 0) {
        *((uint32_t*) dest) = (uint32_t) s;
        return 0;
      }
      break;
    case TCode::UINT64:
      if (s >= 0) {
        *((uint64_t*) dest) = (uint64_t) s;
        return 0;
      }
      break;
    case TCode::BOOLEAN:    *((bool*) dest) = (0 != s);         return 0;
    case TCode::FLOAT:      *((float*) dest) = (s * 1.0F);      return 0;
    case TCode::DOUBLE:     *((double*) dest) = (s * 1.0D);     return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<int8_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_int(*((int8_t*) obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<int8_t>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}



////////////////////////////////////////////////////////////////////////////////
/// int16_t
template <> void        C3PTypeConstraint<int16_t>::to_string(void* obj, StringBuilder* out) {  out->concatf("%d", *((int16_t*) obj));  }

template <> int8_t      C3PTypeConstraint<int16_t>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  int16_t* d = (int16_t*) dest;
  switch (SRC_TYPE) {
    case TCode::INT8:       *d = (int16_t) *((int8_t*) src);     return 0;
    case TCode::INT16:      *d = (int16_t) *((int16_t*) src);    return 0;
    case TCode::INT32:
      if ((*((int32_t*) src) <= INT16_MAX) & ((*((int32_t*) src) >= INT16_MIN))) {
        *d = (int16_t) *((int32_t*) src);
        return 0;
      }
      break;
    case TCode::INT64:
      if ((*((int64_t*) src) <= INT16_MAX) & ((*((int64_t*) src) >= INT16_MIN))) {
        *d = (int16_t) *((int64_t*) src);
        return 0;
      }
      break;
    case TCode::UINT8:     *d = (int16_t) *((uint8_t*) src);     return 0;
    case TCode::UINT16:
      if (*((uint16_t*) src) <= INT16_MAX) {
        *d = (int16_t) *((uint16_t*) src);
        return 0;
      }
      break;
    case TCode::UINT32:
      if (*((uint32_t*) src) <= INT16_MAX) {
        *d = (int16_t) *((uint32_t*) src);
        return 0;
      }
      break;
    case TCode::UINT64:
      if (*((uint64_t*) src) <= INT16_MAX) {
        *d = (int16_t) *((uint64_t*) src);
        return 0;
      }
      break;
    case TCode::BOOLEAN:      *d = (*((bool*) src) ? 1 : 0);     return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<int16_t>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  int16_t s = *((int16_t*) src);
  switch (DEST_TYPE) {
    case TCode::INT8:
      if ((s <= INT8_MAX) & (s >= INT8_MIN)) {
        *((int8_t*) dest) = (int8_t) s;
        return 0;
      }
      break;
    case TCode::INT16:     *((int16_t*) dest) = (int16_t) s;      return 0;
    case TCode::INT32:     *((int32_t*) dest) = (int32_t) s;      return 0;
    case TCode::INT64:     *((int64_t*) dest) = (int64_t) s;      return 0;
    case TCode::UINT8:
      if ((s <= UINT8_MAX) & (s >= 0)) {
        *((uint8_t*) dest) = (uint8_t) s;
        return 0;
      }
      break;
    case TCode::UINT16:
      if (s >= 0) {
        *((uint16_t*) dest) = (uint16_t) s;
        return 0;
      }
      break;
    case TCode::UINT32:
      if (s >= 0) {
        *((uint32_t*) dest) = (uint32_t) s;
        return 0;
      }
      break;
    case TCode::UINT64:
      if (s >= 0) {
        *((uint64_t*) dest) = (uint64_t) s;
        return 0;
      }
      break;
    case TCode::BOOLEAN:    *((bool*) dest)   = (0 != s);       return 0;
    case TCode::FLOAT:      *((float*) dest)  = (s * 1.0F);     return 0;
    case TCode::DOUBLE:     *((double*) dest) = (s * 1.0D);     return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<int16_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_int(*((int16_t*) obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<int16_t>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// int32_t
template <> void        C3PTypeConstraint<int32_t>::to_string(void* obj, StringBuilder* out) {  out->concatf("%d", *((int32_t*) obj));  }

template <> int8_t      C3PTypeConstraint<int32_t>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  int32_t* d = (int32_t*) dest;
  switch (SRC_TYPE) {
    case TCode::INT8:       *d = (int32_t) *((int8_t*) src);     return 0;
    case TCode::INT16:      *d = (int32_t) *((int16_t*) src);    return 0;
    case TCode::INT32:      *d = (int32_t) *((int32_t*) src);    return 0;
    case TCode::INT64:
      if ((*((int64_t*) src) <= INT32_MAX) & (*((int64_t*) src) >= INT32_MIN)) {
        *d = (int32_t) *((uint64_t*) src);
        return 0;
      }
      break;
    case TCode::UINT8:      *d = (int32_t) *((uint8_t*) src);    return 0;
    case TCode::UINT16:     *d = (int32_t) *((uint16_t*) src);   return 0;
    case TCode::UINT32:
      if (*((uint32_t*) src) <= INT32_MAX) {
        *d = (int32_t) *((uint32_t*) src);
        return 0;
      }
      break;
    case TCode::UINT64:
      if (*((uint64_t*) src) <= INT32_MAX) {
        *d = (int32_t) *((uint64_t*) src);
        return 0;
      }
      break;
    case TCode::BOOLEAN:      *d = (*((bool*) src) ? 1 : 0);     return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<int32_t>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  int32_t s = *((int32_t*) src);
  switch (DEST_TYPE) {
    case TCode::INT8:
      if ((s <= INT8_MAX) & (s >= INT8_MIN)) {
        *((int8_t*) dest) = (int8_t) s;
        return 0;
      }
      break;
    case TCode::INT16:
      if ((s <= INT16_MAX) & (s >= INT16_MIN)) {
        *((int16_t*) dest) = (int16_t) s;
        return 0;
      }
      break;
    case TCode::INT32:     *((int32_t*) dest) = (int32_t) s;      return 0;
    case TCode::INT64:     *((int64_t*) dest) = (int64_t) s;      return 0;
    case TCode::UINT8:
      if ((s <= UINT8_MAX) & (s >= 0)) {
        *((uint8_t*) dest) = (uint8_t) s;
        return 0;
      }
      break;
    case TCode::UINT16:
      if ((s <= UINT16_MAX) & (s >= 0)) {
        *((uint16_t*) dest) = (uint16_t) s;
        return 0;
      }
      break;
    case TCode::UINT32:
      if (s >= 0) {
        *((uint32_t*) dest) = (uint32_t) s;
        return 0;
      }
      break;
    case TCode::UINT64:
      if (s >= 0) {
        *((uint64_t*) dest) = (uint64_t) s;
        return 0;
      }
      break;
    case TCode::BOOLEAN:    *((bool*) dest)   = (0 != s);       return 0;
    case TCode::FLOAT:      *((float*) dest)  = (s * 1.0F);     return 0;
    case TCode::DOUBLE:     *((double*) dest) = (s * 1.0D);     return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<int32_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_int(*((int32_t*) obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<int32_t>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// int64_t
/// TODO: Large integers aren't printf()-able under newlib-nano. Add a
///   lib-check/conf step to help handle this case?
template <> void        C3PTypeConstraint<int64_t>::to_string(void* obj, StringBuilder* out) {  out->concatf("%d", *((int64_t*) obj));  }

template <> int8_t      C3PTypeConstraint<int64_t>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  int64_t* d = (int64_t*) dest;
  switch (SRC_TYPE) {
    case TCode::INT8:
      if (*((int8_t*) src) >= 0) {
        *d = (uint64_t) *((int8_t*) src);
        return 0;
      }
      break;
    case TCode::INT16:
      if (*((int16_t*) src) >= 0) {
        *d = (uint64_t) *((int16_t*) src);
        return 0;
      }
      break;
    case TCode::INT32:
      if (*((int32_t*) src) >= 0) {
        *d = (uint64_t) *((int32_t*) src);
        return 0;
      }
      break;
    case TCode::INT64:      *d = (int64_t) *((int64_t*) src);    return 0;
    case TCode::UINT8:      *d = (int64_t) *((uint8_t*) src);    return 0;
    case TCode::UINT16:     *d = (int64_t) *((uint16_t*) src);   return 0;
    case TCode::UINT32:     *d = (int64_t) *((uint32_t*) src);   return 0;
    case TCode::UINT64:
      if (*((uint64_t*) src) <= INT64_MAX) {
        *d = (int64_t) *((uint64_t*) src);
        return 0;
      }
      break;
    case TCode::BOOLEAN:    *d = (*((bool*) src) ? 1 : 0);        return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<int64_t>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  int64_t s = *((int64_t*) src);
  switch (DEST_TYPE) {
    case TCode::INT8:
      if ((s <= INT8_MAX) & (s >= INT8_MIN)) {
        *((int8_t*) dest) = (int8_t) s;
        return 0;
      }
      break;
    case TCode::INT16:
      if ((s <= INT16_MAX) & (s >= INT16_MIN)) {
        *((int16_t*) dest) = (int16_t) s;
        return 0;
      }
      break;
    case TCode::INT32:
      if ((s <= INT32_MAX) & (s >= INT32_MIN)) {
        *((int32_t*) dest) = (int32_t) s;
        return 0;
      }
      break;
    case TCode::INT64:      *((int64_t*) dest) = (int64_t) s;    return 0;
    case TCode::UINT8:
      if ((s <= UINT8_MAX) & (s >= 0)) {
        *((uint8_t*) dest) = (uint8_t) s;
        return 0;
      }
      break;
    case TCode::UINT16:
      if ((s <= UINT16_MAX) & (s >= 0)) {
        *((uint16_t*) dest) = (uint16_t) s;
        return 0;
      }
      break;
    case TCode::UINT32:
      if ((s <= UINT32_MAX) & (s >= 0)) {
        *((uint32_t*) dest) = (uint32_t) s;
        return 0;
      }
      break;
    case TCode::UINT64:
      if (s >= 0) {
        *((uint64_t*) dest) = (uint64_t) s;
        return 0;
      }
      break;
    case TCode::BOOLEAN:    *((bool*) dest)   = (0 != s);        return 0;
    case TCode::DOUBLE:     *((double*) dest) = (s * 1.0D);      return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<int64_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_int(*((int64_t*) obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<int64_t>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// uint8_t
template <> void        C3PTypeConstraint<uint8_t>::to_string(void* obj, StringBuilder* out) {  out->concatf("%u", *((uint8_t*) obj));  }

template <> int8_t      C3PTypeConstraint<uint8_t>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  uint8_t* d = (uint8_t*) dest;
  switch (SRC_TYPE) {
    case TCode::INT8:
      if (*((int8_t*) src) >= 0) {
        *d = (uint8_t) *((int8_t*) src);
        return 0;
      }
      break;
    case TCode::INT16:
      if ((*((int16_t*) src) <= UINT8_MAX) & (*((int16_t*) src) >= 0)) {
        *d = (uint8_t) *((int16_t*) src);
        return 0;
      }
      break;
    case TCode::INT32:
      if ((*((int32_t*) src) <= UINT8_MAX) & (*((int32_t*) src) >= 0)) {
        *d = (uint8_t) *((int32_t*) src);
        return 0;
      }
      break;
    case TCode::INT64:
      if ((*((int64_t*) src) <= UINT8_MAX) & (*((int64_t*) src) >= 0)) {
        *d = (uint8_t) *((int64_t*) src);
        return 0;
      }
      break;
    case TCode::UINT8:      *d = (uint8_t) *((uint8_t*) src);    return 0;
    case TCode::UINT16:
      if (*((uint16_t*) src) <= UINT8_MAX) {
        *d = (uint8_t) *((uint16_t*) src);
        return 0;
      }
    case TCode::UINT32:
      if (*((uint32_t*) src) <= UINT8_MAX) {
        *d = (uint8_t) *((uint32_t*) src);
        return 0;
      }
      break;
    case TCode::UINT64:
      if (*((uint64_t*) src) <= UINT8_MAX) {
        *d = (uint8_t) *((uint64_t*) src);
        return 0;
      }
      break;
    case TCode::BOOLEAN:    *d = (*((bool*) src) ? 1 : 0);       return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<uint8_t>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  uint8_t s = *((uint8_t*) src);
  switch (DEST_TYPE) {
    case TCode::INT8:
      if (s <= INT8_MAX) {
        *((int8_t*) dest) = (int8_t) s;
        return 0;
      }
      break;
    case TCode::INT16:      *((int16_t*) dest) = (int16_t) s;    return 0;
    case TCode::INT32:      *((int32_t*) dest) = (int32_t) s;    return 0;
    case TCode::INT64:      *((int64_t*) dest) = (int64_t) s;    return 0;
    case TCode::UINT8:      *((uint8_t*) dest) = (uint8_t) s;    return 0;
    case TCode::UINT16:     *((uint16_t*) dest) = (uint16_t) s;  return 0;
    case TCode::UINT32:     *((uint32_t*) dest) = (uint32_t) s;  return 0;
    case TCode::UINT64:     *((uint64_t*) dest) = (uint64_t) s;  return 0;
    case TCode::BOOLEAN:    *((bool*) dest)   = (0 != s);        return 0;
    case TCode::FLOAT:      *((float*) dest)  = (s * 1.0F);      return 0;
    case TCode::DOUBLE:     *((double*) dest) = (s * 1.0D);      return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<uint8_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_int(*((uint8_t*) obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<uint8_t>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// uint16_t
template <> void        C3PTypeConstraint<uint16_t>::to_string(void* obj, StringBuilder* out) {  out->concatf("%u", *((uint16_t*) obj));  }

template <> int8_t      C3PTypeConstraint<uint16_t>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  uint16_t* d = (uint16_t*) dest;
  switch (SRC_TYPE) {
    case TCode::INT8:
      if (*((int8_t*) src) >= 0) {
        *d = (uint16_t) *((int8_t*) src);
        return 0;
      }
      break;
    case TCode::INT16:
      if (*((int16_t*) src) >= 0) {
        *d = (uint16_t) *((int16_t*) src);
        return 0;
      }
      break;
    case TCode::INT32:
      if ((*((int32_t*) src) <= UINT16_MAX) & (*((int32_t*) src) >= 0)) {
        *d = (uint16_t) *((int32_t*) src);
        return 0;
      }
      break;
    case TCode::INT64:
      if ((*((int64_t*) src) <= UINT16_MAX) & (*((int64_t*) src) >= 0)) {
        *d = (uint16_t) *((int64_t*) src);
        return 0;
      }
      break;
    case TCode::UINT8:      *d = (uint16_t) *((uint8_t*) src);    return 0;
    case TCode::UINT16:     *d = (uint16_t) *((uint16_t*) src);   return 0;
    case TCode::UINT32:
      if (*((uint32_t*) src) <= UINT16_MAX) {
        *d = (uint16_t) *((uint32_t*) src);
        return 0;
      }
      break;
    case TCode::UINT64:
      if (*((uint64_t*) src) <= UINT16_MAX) {
        *d = (uint16_t) *((uint64_t*) src);
        return 0;
      }
      break;
    case TCode::BOOLEAN:    *d = (*((bool*) src) ? 1 : 0);        return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<uint16_t>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  uint16_t s = *((uint16_t*) src);
  switch (DEST_TYPE) {
    case TCode::INT8:
      if (s <= INT8_MAX) {
        *((int8_t*) dest) = (int8_t) s;
        return 0;
      }
      break;
    case TCode::INT16:
      if (s <= INT16_MAX) {
        *((int16_t*) dest) = (int16_t) s;
        return 0;
      }
      break;
    case TCode::INT32:      *((int32_t*) dest) = (int32_t) s;    return 0;
    case TCode::INT64:      *((int64_t*) dest) = (int64_t) s;    return 0;
    case TCode::UINT8:
      if (s <= UINT8_MAX) {
        *((uint8_t*) dest) = (uint8_t) s;
        return 0;
      }
      break;
    case TCode::UINT16:     *((uint16_t*) dest) = (uint16_t) s;  return 0;
    case TCode::UINT32:     *((uint32_t*) dest) = (uint32_t) s;  return 0;
    case TCode::UINT64:     *((uint64_t*) dest) = (uint64_t) s;  return 0;
    case TCode::BOOLEAN:    *((bool*) dest)   = (0 != s);        return 0;
    case TCode::FLOAT:      *((float*) dest)  = (s * 1.0F);      return 0;
    case TCode::DOUBLE:     *((double*) dest) = (s * 1.0D);      return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<uint16_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_int(*((uint16_t*) obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<uint16_t>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// uint32_t
template <> void        C3PTypeConstraint<uint32_t>::to_string(void* obj, StringBuilder* out) {  out->concatf("%u", *((uint32_t*) obj));  }

template <> int8_t      C3PTypeConstraint<uint32_t>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  uint32_t* d = (uint32_t*) dest;
  switch (SRC_TYPE) {
    case TCode::INT8:
      if (*((int8_t*) src) >= 0) {
        *d = (uint32_t) *((int8_t*) src);
        return 0;
      }
      break;
    case TCode::INT16:
      if (*((int16_t*) src) >= 0) {
        *d = (uint32_t) *((int16_t*) src);
        return 0;
      }
      break;
    case TCode::INT32:
      if (*((int32_t*) src) >= 0) {
        *d = (uint32_t) *((int32_t*) src);
        return 0;
      }
      break;
    case TCode::INT64:
      if (*((int64_t*) src) <= UINT32_MAX) {
        *d = (uint32_t) *((int64_t*) src);
        return 0;
      }
      break;
    case TCode::UINT8:      *d = (uint32_t) *((uint8_t*) src);    return 0;
    case TCode::UINT16:     *d = (uint32_t) *((uint16_t*) src);   return 0;
    case TCode::UINT32:     *d = (uint32_t) *((uint32_t*) src);   return 0;
    case TCode::UINT64:
      if (*((uint64_t*) src) <= UINT32_MAX) {
        *d = (uint32_t) *((uint64_t*) src);
        return 0;
      }
      break;
    case TCode::BOOLEAN:    *d = (*((bool*) src) ? 1 : 0);        return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<uint32_t>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  uint32_t s = *((uint32_t*) src);
  switch (DEST_TYPE) {
    case TCode::INT8:
      if (s <= INT8_MAX) {
        *((int8_t*) dest) = (int8_t) s;
        return 0;
      }
      break;
    case TCode::INT16:
      if (s <= INT16_MAX) {
        *((int16_t*) dest) = (int16_t) s;
        return 0;
      }
      break;
    case TCode::INT32:
      if (s <= INT32_MAX) {
        *((int32_t*) dest) = (int32_t) s;
        return 0;
      }
      break;
    case TCode::INT64:      *((int64_t*) dest) = (int64_t) s;    return 0;
    case TCode::UINT8:
      if (s <= UINT8_MAX) {
        *((uint8_t*) dest) = (uint8_t) s;
        return 0;
      }
      break;
    case TCode::UINT16:
      if (s <= UINT16_MAX) {
        *((uint16_t*) dest) = (uint16_t) s;
        return 0;
      }
      break;
    case TCode::UINT32:     *((uint32_t*) dest) = (uint32_t) s;  return 0;
    case TCode::UINT64:     *((uint64_t*) dest) = (uint64_t) s;  return 0;
    case TCode::BOOLEAN:    *((bool*) dest)   = (0 != s);        return 0;
    case TCode::FLOAT:      *((float*) dest)  = (s * 1.0F);      return 0;
    case TCode::DOUBLE:     *((double*) dest) = (s * 1.0D);      return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<uint32_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_int(*((uint32_t*) obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<uint32_t>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// uint64_t
/// TODO: Large integers aren't printf()-able under newlib-nano. Add a
///   lib-check/conf step to help handle this case?
template <> void        C3PTypeConstraint<uint64_t>::to_string(void* obj, StringBuilder* out) {  out->concatf("%u", *((uint64_t*) obj));  }

template <> int8_t      C3PTypeConstraint<uint64_t>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  uint64_t* d = (uint64_t*) dest;
  switch (SRC_TYPE) {
    case TCode::INT8:
      if (*((int8_t*) src) >= 0) {
        *d = (uint64_t) *((int8_t*) src);
        return 0;
      }
      break;
    case TCode::INT16:
      if (*((int16_t*) src) >= 0) {
        *d = (uint64_t) *((int16_t*) src);
        return 0;
      }
      break;
    case TCode::INT32:
      if (*((int32_t*) src) >= 0) {
        *d = (uint64_t) *((int32_t*) src);
        return 0;
      }
      break;
    case TCode::INT64:
      if (*((int64_t*) src) >= 0) {
        *d = (uint64_t) *((int64_t*) src);
        return 0;
      }
      break;
    case TCode::UINT8:      *d = (uint64_t) *((uint8_t*) src);    return 0;
    case TCode::UINT16:     *d = (uint64_t) *((uint16_t*) src);   return 0;
    case TCode::UINT32:     *d = (uint64_t) *((uint32_t*) src);   return 0;
    case TCode::UINT64:     *d = (uint64_t) *((uint64_t*) src);   return 0;
    case TCode::BOOLEAN:    *d = (*((bool*) src) ? 1 : 0);        return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<uint64_t>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  uint64_t s = *((uint64_t*) src);
  switch (DEST_TYPE) {
    case TCode::INT8:
      if (s <= INT8_MAX) {
        *((int8_t*) dest) = (int8_t) s;
        return 0;
      }
      break;
    case TCode::INT16:
      if (s <= INT16_MAX) {
        *((int16_t*) dest) = (int16_t) s;
        return 0;
      }
      break;
    case TCode::INT32:
      if (s <= INT32_MAX) {
        *((int32_t*) dest) = (int32_t) s;
        return 0;
      }
      break;
    case TCode::INT64:
      if (s <= INT64_MAX) {
        *((int64_t*) dest) = (int64_t) s;
        return 0;
      }
      break;
    case TCode::UINT8:
      if (s <= UINT8_MAX) {
        *((uint8_t*) dest) = (uint8_t) s;
        return 0;
      }
      break;
    case TCode::UINT16:
      if (s <= UINT16_MAX) {
        *((uint16_t*) dest) = (uint16_t) s;
        return 0;
      }
      break;
    case TCode::UINT32:
      if (s <= UINT32_MAX) {
        *((uint32_t*) dest) = (uint32_t) s;
        return 0;
      }
      break;
    case TCode::UINT64:     *((uint64_t*) dest) = (uint64_t) s;    return 0;
    case TCode::BOOLEAN:    *((bool*) dest)     = (0 != s);        return 0;
    case TCode::DOUBLE:     *((double*) dest)   = (s * 1.0D);      return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<uint64_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_int(*((uint64_t*) obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<uint64_t>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// bool
template <> void        C3PTypeConstraint<bool>::to_string(void* obj, StringBuilder* out) {  out->concatf("%s", (*((bool*) obj) ? "true" : "false"));  }

template <> int8_t      C3PTypeConstraint<bool>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  bool* d = (bool*) dest;
  switch (SRC_TYPE) {
    case TCode::INT8:       *d = (0 != *((int8_t*) src));         return 0;
    case TCode::INT16:      *d = (0 != *((int16_t*) src));        return 0;
    case TCode::INT32:      *d = (0 != *((int32_t*) src));        return 0;
    case TCode::INT64:      *d = (0 != *((int64_t*) src));        return 0;
    case TCode::UINT8:      *d = (0 != *((uint8_t*) src));        return 0;
    case TCode::UINT16:     *d = (0 != *((uint16_t*) src));       return 0;
    case TCode::UINT32:     *d = (0 != *((uint32_t*) src));       return 0;
    case TCode::UINT64:     *d = (0 != *((uint64_t*) src));       return 0;
    case TCode::BOOLEAN:    *d = *((bool*) src);                  return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<bool>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  bool s = *((bool*) src);
  switch (DEST_TYPE) {
    case TCode::INT8:       *((int8_t*)   dest) = (s ? 1:0);     return 0;
    case TCode::INT16:      *((int16_t*)  dest) = (s ? 1:0);     return 0;
    case TCode::INT32:      *((int32_t*)  dest) = (s ? 1:0);     return 0;
    case TCode::INT64:      *((int64_t*)  dest) = (s ? 1:0);     return 0;
    case TCode::UINT8:      *((uint8_t*)  dest) = (s ? 1:0);     return 0;
    case TCode::UINT16:     *((uint16_t*) dest) = (s ? 1:0);     return 0;
    case TCode::UINT32:     *((uint32_t*) dest) = (s ? 1:0);     return 0;
    case TCode::UINT64:     *((uint64_t*) dest) = (s ? 1:0);     return 0;
    case TCode::BOOLEAN:    *((bool*)     dest) = s;             return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<bool>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_bool(*((bool*) obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<bool>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// float
/// To avoid inducing bugs related to alignment, many of these functions will
///   copy values byte-wise into their on-stack storage, where architectural
///   type-alignment boundaries can still be enforced by the compiler, despite
///   the type-punning.
template <> void        C3PTypeConstraint<float>::to_string(void* obj, StringBuilder* out) {
  out->concatf("%.4f", (double) _load_from_mem(obj));
}

template <> int8_t      C3PTypeConstraint<float>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  int8_t ret = -1;
  float d = 0.0f;
  switch (SRC_TYPE) {
    case TCode::INT8:       d = (1.0f * *((int8_t*) src));    ret = 0;  break;
    case TCode::INT16:      d = (1.0f * *((int16_t*) src));   ret = 0;  break;
    case TCode::INT32:      d = (1.0f * *((int32_t*) src));   ret = 0;  break;
    case TCode::UINT8:      d = (1.0f * *((uint8_t*) src));   ret = 0;  break;
    case TCode::UINT16:     d = (1.0f * *((uint16_t*) src));  ret = 0;  break;
    case TCode::UINT32:     d = (1.0f * *((uint32_t*) src));  ret = 0;  break;
    case TCode::FLOAT:      memcpy(dest, src, FIXED_LEN);  return 0;
    default:  break;
  }
  if (0 == ret) {
    _store_in_mem(dest, d);
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<float>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  // To avoid inducing bugs related to alignment, we copy the value byte-wise
  //   into our on-stack storage, where it's alignment can be controlled.
  float s = 0.0f;
  switch (DEST_TYPE) {
    case TCode::FLOAT:      memcpy(dest, src, FIXED_LEN);  return 0;
    case TCode::DOUBLE:
      {
        double s_d = (1.0d * _load_from_mem(src));
        memcpy(dest, (void*) &s_d, sizeOfType(TCode::DOUBLE));
      }
      return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<float>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_float(_load_from_mem(obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<float>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// double
/// To avoid inducing bugs related to alignment, many of these functions will
///   copy values byte-wise into their on-stack storage, where architectural
///   type-alignment boundaries can still be enforced by the compiler, despite
///   the type-punning.
template <> void        C3PTypeConstraint<double>::to_string(void* obj, StringBuilder* out) {
  out->concatf("%.6f", _load_from_mem(obj));
}

template <> int8_t      C3PTypeConstraint<double>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  int8_t ret = -1;
  double d = 0.0d;
  switch (SRC_TYPE) {
    case TCode::INT8:       d = (1.0d * *((int8_t*) src));      ret = 0;   break;
    case TCode::INT16:      d = (1.0d * *((int16_t*) src));     ret = 0;   break;
    case TCode::INT32:      d = (1.0d * *((int32_t*) src));     ret = 0;   break;
    case TCode::INT64:      d = (1.0d * *((int64_t*) src));     ret = 0;   break;
    case TCode::UINT8:      d = (1.0d * *((uint8_t*) src));     ret = 0;   break;
    case TCode::UINT16:     d = (1.0d * *((uint16_t*) src));    ret = 0;   break;
    case TCode::UINT32:     d = (1.0d * *((uint32_t*) src));    ret = 0;   break;
    case TCode::UINT64:     d = (1.0d * *((uint64_t*) src));    ret = 0;   break;
    case TCode::FLOAT:
      {
        float s = 0.0f;
        memcpy((void*) &s, src, sizeOfType(TCode::FLOAT));
        d = (1.0d * *((float*) src));
      }
      return 0;
    case TCode::DOUBLE:     memcpy(dest, src, FIXED_LEN);  return 0;
    default:  break;
  }
  if (0 == ret) {
    _store_in_mem(dest, d);
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<double>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    case TCode::DOUBLE:     memcpy(dest, src, FIXED_LEN);   return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<double>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_double(_load_from_mem(obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<double>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// char*
/// NOTE: length() always returns +1 for all c-style string types to account for
///   the storage overhead of a null-terminator.
template <> uint32_t    C3PTypeConstraint<char*>::length(void* obj) {  return (strlen((char*) obj) + 1);  }
template <> void        C3PTypeConstraint<char*>::to_string(void* obj, StringBuilder* out) {  out->concatf("%s", *((char**) obj));  }

template <> int8_t      C3PTypeConstraint<char*>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  char** d = (char**) dest;
  switch (SRC_TYPE) {
    case TCode::STR:     *d = *((char**) src);    return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<char*>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  char** s = (char**) src;
  switch (DEST_TYPE) {
    case TCode::STR:     *((char**) dest) = *s;   return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<char*>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat(*((const char**) obj));
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_string((const char*) obj);
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<char*>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// StringBuilder
template <> uint32_t    C3PTypeConstraint<StringBuilder>::length(void* obj) {  return ((StringBuilder*) obj)->length();  }
template <> void        C3PTypeConstraint<StringBuilder>::to_string(void* obj, StringBuilder* out) {  out->concat((StringBuilder*) obj);  }

template <> int8_t      C3PTypeConstraint<StringBuilder>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  StringBuilder* d = (StringBuilder*) dest;
  switch (SRC_TYPE) {
    //case TCode::STR:     d = *((char**) src);    return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<StringBuilder>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  StringBuilder* s = (StringBuilder*) src;
  switch (DEST_TYPE) {
    //case TCode::STR:     *((char**) dest) = *s;   return 0;
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<StringBuilder>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat(*((const char**) obj));
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        encoder.write_string((char*) ((StringBuilder*) obj)->string());
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<StringBuilder>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// C3PBinBinder
/// NOTE: This handler covers any binary data that is refered to by pointer
///   and length.
template <> uint32_t    C3PTypeConstraint<C3PBinBinder>::length(void* obj) {  return (((C3PBinBinder*) obj)->len);  }
template <> void        C3PTypeConstraint<C3PBinBinder>::to_string(void* obj, StringBuilder* out) {
  const uint32_t L_ENDER = ((C3PBinBinder*) obj)->len;
  if (L_ENDER > 0) {  StringBuilder::printBuffer(out, ((C3PBinBinder*) obj)->buf, L_ENDER);  }
  //_type_blind_to_string((C3PBinBinder*) obj, out);
}

template <> int8_t      C3PTypeConstraint<C3PBinBinder>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  if ((nullptr != src) & (nullptr != dest)) {
    switch (SRC_TYPE) {
      case TCode::BINARY:    memcpy(dest, src, sizeof(C3PBinBinder));  return 0;
      default:  break;
    }
  }
  return -1;
}


template <> int8_t      C3PTypeConstraint<C3PBinBinder>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  if ((nullptr != src) & (nullptr != dest)) {
    C3PBinBinder s = _load_from_mem(src);
    switch (DEST_TYPE) {
      case TCode::BINARY:    memcpy(dest, src, sizeof(C3PBinBinder));  return 0;
      default:  break;
    }
  }
  return -1;
}



////////////////////////////////////////////////////////////////////////////////
/// Vector3f64
template <> void        C3PTypeConstraint<Vector3f64>::to_string(void* obj, StringBuilder* out) {
  Vector3f64 v = _load_from_mem(obj);
  out->concatf("(%.6f, %.6f, %.6f)", (double)(v.x), (double)(v.y), (double)(v.z));
}

template <> int8_t      C3PTypeConstraint<Vector3f64>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    case TCode::VECT_3_DOUBLE:     _store_in_mem(dest, _load_from_mem(src));       return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<Vector3f64>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    case TCode::VECT_3_DOUBLE:     _store_in_mem(dest, _load_from_mem(src));       return 0;
    default:  break;
  }
  return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// Vector3u8
template <> void        C3PTypeConstraint<Vector3u8>::to_string(void* obj, StringBuilder* out) {
  Vector3u8 v = _load_from_mem(obj);
  out->concatf("(%u, %u, %u)", v.x, v.y, v.z);
}

template <> int8_t      C3PTypeConstraint<Vector3u8>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    case TCode::VECT_3_UINT8:     _store_in_mem(dest, _load_from_mem(src));       return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<Vector3u8>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    case TCode::VECT_3_UINT8:     _store_in_mem(dest, _load_from_mem(src));       return 0;
    default:  break;
  }
  return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// Vector3i8
template <> void        C3PTypeConstraint<Vector3i8>::to_string(void* obj, StringBuilder* out) {
  Vector3i8 v = _load_from_mem(obj);
  out->concatf("(%d, %d, %d)", v.x, v.y, v.z);
}

template <> int8_t      C3PTypeConstraint<Vector3i8>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    case TCode::VECT_3_INT8:     _store_in_mem(dest, _load_from_mem(src));       return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<Vector3i8>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    case TCode::VECT_3_INT8:     _store_in_mem(dest, _load_from_mem(src));       return 0;
    default:  break;
  }
  return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// Vector3f
template <> void        C3PTypeConstraint<Vector3f>::to_string(void* obj, StringBuilder* out) {
  Vector3f v = _load_from_mem(obj);
  out->concatf("(%.4f, %.4f, %.4f)", (double)(v.x), (double)(v.y), (double)(v.z));
}

template <> int8_t      C3PTypeConstraint<Vector3f>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    case TCode::VECT_3_FLOAT:     _store_in_mem(dest, _load_from_mem(src));       return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<Vector3f>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    case TCode::VECT_3_FLOAT:     _store_in_mem(dest, _load_from_mem(src));       return 0;
    default:  break;
  }
  return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// Vector3u32
template <> void        C3PTypeConstraint<Vector3u32>::to_string(void* obj, StringBuilder* out) {
  Vector3u32 v = _load_from_mem(obj);
  out->concatf("(%u, %u, %u)", v.x, v.y, v.z);
}

template <> int8_t      C3PTypeConstraint<Vector3u32>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    case TCode::VECT_3_UINT32:     _store_in_mem(dest, _load_from_mem(src));      return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<Vector3u32>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    case TCode::VECT_3_UINT32:     _store_in_mem(dest, _load_from_mem(src));      return 0;
    default:  break;
  }
  return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// Vector3i32
template <> void        C3PTypeConstraint<Vector3i32>::to_string(void* obj, StringBuilder* out) {
  Vector3i32 v = _load_from_mem(obj);
  out->concatf("(%d, %d, %d)", v.x, v.y, v.z);
}

template <> int8_t      C3PTypeConstraint<Vector3i32>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    case TCode::VECT_3_INT32:     _store_in_mem(dest, _load_from_mem(src));      return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<Vector3i32>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    case TCode::VECT_3_INT32:     _store_in_mem(dest, _load_from_mem(src));      return 0;
    default:  break;
  }
  return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// Vector3u16
template <> void        C3PTypeConstraint<Vector3u16>::to_string(void* obj, StringBuilder* out) {
  Vector3u16 v = _load_from_mem(obj);
  out->concatf("(%u, %u, %u)", v.x, v.y, v.z);
}

template <> int8_t      C3PTypeConstraint<Vector3u16>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    case TCode::VECT_3_UINT16:     _store_in_mem(dest, _load_from_mem(src));      return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<Vector3u16>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    case TCode::VECT_3_UINT16:     _store_in_mem(dest, _load_from_mem(src));      return 0;
    default:  break;
  }
  return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// Vector3i16
template <> void        C3PTypeConstraint<Vector3i16>::to_string(void* obj, StringBuilder* out) {
  Vector3i16 v = _load_from_mem(obj);
  out->concatf("(%d, %d, %d)", v.x, v.y, v.z);
}

template <> int8_t      C3PTypeConstraint<Vector3i16>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    case TCode::VECT_3_INT16:     _store_in_mem(dest, _load_from_mem(src));      return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<Vector3i16>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    case TCode::VECT_3_INT16:     _store_in_mem(dest, _load_from_mem(src));      return 0;
    default:  break;
  }
  return -1;
}


////////////////////////////////////////////////////////////////////////////////
/// Identity*
template <> uint32_t    C3PTypeConstraint<Identity*>::length(void* obj) {  return ((Identity*) obj)->length();  }
template <> void        C3PTypeConstraint<Identity*>::to_string(void* obj, StringBuilder* out) {  ((Identity*) obj)->toString(out);  }

template <> int8_t      C3PTypeConstraint<Identity*>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<Identity*>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<Identity*>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  Identity* ident = ((Identity*) obj);
  switch (FORMAT) {
    case TCode::BINARY:
      break;

    case TCode::CBOR:
      {
        uint16_t i_len = ident->length();
        uint8_t buf[i_len];
        if (ident->toBuffer(buf)) {
          cbor::output_stringbuilder output(out);
          cbor::encoder encoder(output);
          encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(TCODE));
          encoder.write_bytes(buf, i_len);
        }
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<Identity*>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// StopWatch*
template <> void        C3PTypeConstraint<StopWatch*>::to_string(void* obj, StringBuilder* out) {
  ((StopWatch*) obj)->serialize(out, TCode::STR);
}

template <> int         C3PTypeConstraint<StopWatch*>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  StopWatch* stopwatch = ((StopWatch*) obj);
  int ret = stopwatch->serialize(out, FORMAT);
  return ret;
}

template <> int8_t      C3PTypeConstraint<StopWatch*>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// KeyValuePair
template <> uint32_t    C3PTypeConstraint<KeyValuePair>::length(void* obj) {  return ((KeyValuePair*) obj)->memoryCost(true);  }
template <> void        C3PTypeConstraint<KeyValuePair>::to_string(void* obj, StringBuilder* out) {  ((KeyValuePair*) obj)->valToString(out);  }
//template <> void        C3PTypeConstraint<KeyValuePair>::to_string(void* obj, StringBuilder* out) {  ((KeyValuePair*) obj)->toString(out);  }

template <> int8_t      C3PTypeConstraint<KeyValuePair>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<KeyValuePair>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<KeyValuePair>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  KeyValuePair* subj = ((KeyValuePair*) obj);
  // NOTE: Hidden recursion.
  return ((0 == subj->serialize(out, FORMAT)) ? 0 : -1);
}

template <> int8_t      C3PTypeConstraint<KeyValuePair>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// Image
#if defined(CONFIG_C3P_IMG_SUPPORT)
template <> uint32_t    C3PTypeConstraint<Image>::length(void* obj) {  return ((Image*) obj)->bytesUsed();  }
template <> void        C3PTypeConstraint<Image>::to_string(void* obj, StringBuilder* out) {
  Image* img = (Image*) obj;
  out->concatf("Image (%ux%u %u bpp)", img->x(), img->y(), img->bitsPerPixel());
}

template <> int8_t      C3PTypeConstraint<Image>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  switch (SRC_TYPE) {
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<Image>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    default:  break;
  }
  return -1;
}

template <> int         C3PTypeConstraint<Image>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  Image* img = ((Image*) obj);

  switch (FORMAT) {
    case TCode::BINARY:
      break;

    case TCode::CBOR:
      {
        uint32_t sz_buf = img->bytesUsed();
        if (sz_buf > 0) {
          uint32_t nb_buf = 0;
          uint8_t intermediary[32] = {0, };
          if (0 == img->serializeWithoutBuffer(intermediary, &nb_buf)) {
            cbor::output_stringbuilder output(out);
            cbor::encoder encoder(output);
            encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(TCODE));
            encoder.write_bytes(intermediary, nb_buf);   // TODO: This might cause two discrete CBOR objects.
            //encoder.write_bytes(img->buffer(), sz_buf);
          }
        }
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<Image>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}
#endif   // CONFIG_C3P_IMG_SUPPORT