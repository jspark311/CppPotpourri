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
#include "../C3PValue/C3PValue.h"
#include "../StringBuilder.h"
#include "../Identity/Identity.h"
#include "../StringBuilder.h"

/* CBOR support should probably be required to parse/pack. */
#if defined(CONFIG_C3P_CBOR)
  #include "../cbor-cpp/cbor.h"
#endif



/**
* This is the private structure with which we define types. It conveys the type
*   code, the type's size, and any special attributes it might have.
* These type definitions are used by library components that serialize for the
*   purposes of storage or communication with other versions of this library. So
*   we should strive to be consistent.
*/
typedef struct typecode_def_t {
  const TCode    type_code;     // This identifies the type to parsers/packers.
  const uint8_t  type_flags;    // Fixed metadata about a type, as this build implements it.
  const uint16_t fixed_len;     // If this type has a fixed length, it will be set here. 0 if no fixed length.
  const char* const  t_name;    // The name of the type.
} TypeCodeDef;


/*******************************************************************************
* Statics related to type support.
*******************************************************************************/
/**
* Static initializer for our type map that gives us runtime type information.
* If the type isn't here, we won't be able to handle it.
*/
static const TypeCodeDef static_type_codes[] = {
  {TCode::NONE,           (TCODE_FLAG_NON_EXPORTABLE),                               0,  "NONE"},
  {TCode::INT8,           (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          1,  "INT8"},
  {TCode::UINT8,          (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          1,  "UINT8"},
  {TCode::INT16,          (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          2,  "INT16"},
  {TCode::UINT16,         (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          2,  "UINT16"},
  {TCode::INT32,          (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          4,  "INT32"},
  {TCode::UINT32,         (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          4,  "UINT32"},
  {TCode::FLOAT,          (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          4,  "FLOAT"},
  {TCode::BOOLEAN,        (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          1,  "BOOL"},
  {TCode::UINT128,        (0),                                                       16, "UINT128"},
  {TCode::INT128,         (0),                                                       16, "INT128"},
  {TCode::UINT64,         (0),                                                       8,  "UINT64"},
  {TCode::INT64,          (0),                                                       8,  "INT64"},
  {TCode::DOUBLE,         (0),                                                       8,  "DOUBLE"},
  {TCode::VECT_3_UINT16,  (0),                                                       6,  "VEC3_UINT16"},
  {TCode::VECT_3_INT16,   (0),                                                       6,  "VEC3_INT16"},
  {TCode::VECT_3_FLOAT,   (0),                                                       12, "VEC3_FLOAT"},
  {TCode::VECT_3_INT8,    (0),                                                       3,  "VEC3_INT8"},
  {TCode::VECT_3_UINT8,   (0),                                                       3,  "VEC3_UINT8"},
  {TCode::VECT_3_INT16,   (0),                                                       6,  "VEC3_INT16"},
  {TCode::VECT_3_UINT16,  (0),                                                       6,  "VEC3_UINT16"},
  {TCode::VECT_3_INT32,   (0),                                                       12, "VEC3_INT32"},
  {TCode::VECT_3_UINT32,  (0),                                                       12, "VEC3_UINT32"},
  {TCode::IDENTITY,       (TCODE_FLAG_VARIABLE_LEN),                                 0,  "IDENTITY"},
  {TCode::KVP,            (TCODE_FLAG_VARIABLE_LEN),                                 0,  "KVP"},
  {TCode::STR,            (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_IS_NULL_DELIMITED),  0,  "STR"},
  {TCode::IMAGE,          (TCODE_FLAG_VARIABLE_LEN),                                 0,  "IMAGE"},
  {TCode::COLOR8,         (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          1,  "COLOR8"},
  {TCode::COLOR16,        (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          2,  "COLOR16"},
  {TCode::COLOR24,        (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          3,  "COLOR24"},
  {TCode::SI_UNIT,        (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_IS_NULL_DELIMITED),  0,  "SI_UNIT"},
  {TCode::BINARY,         (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "BINARY"},
  {TCode::BASE64,         (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "BASE64"},
  {TCode::JSON,           (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "JSON"},
  {TCode::CBOR,           (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "CBOR"},

  {TCode::STR_BUILDER,    (TCODE_FLAG_VARIABLE_LEN),                                 0,  "STR_BLDR"},
  {TCode::GEOLOCATION,    (TCODE_FLAG_VARIABLE_LEN),                                 0,  "GEOLOCATION"},
  {TCode::RESERVED,       (TCODE_FLAG_NON_EXPORTABLE),                               0,  "RESERVED"},
  //{TCode::VECT_2_FLOAT,   (0),                                                       0,  "VEC2_FLOAT"};
  //{TCode::VECT_2_DOUBLE,  (0),                                                       0,  "VEC2_DOUBLE"};
  //{TCode::VECT_2_INT8,    (0),                                                       0,  "VEC2_INT8"};
  //{TCode::VECT_2_UINT8,   (0),                                                       0,  "VEC2_UINT8"};
  //{TCode::VECT_2_INT16,   (0),                                                       0,  "VEC2_INT16"};
  //{TCode::VECT_2_UINT16,  (0),                                                       0,  "VEC2_UINT16"};
  //{TCode::VECT_2_INT32,   (0),                                                       0,  "VEC2_INT32"};
  //{TCode::VECT_2_UINT32,  (0),                                                       0,  "VEC2_UINT32"};
  //{TCode::VECT_3_DOUBLE,  (0),                                                       0,  "VEC3_DOUBLE"};
  //{TCode::VECT_4_FLOAT,   (0),                                                       16, "VEC4_FLOAT"},
  //{TCode::IPV4_ADDR,      (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          4,  "IPV4_ADDR"};
  //{TCode::URL,            (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_IS_NULL_DELIMITED),  1,  "URL"},
  //{TCode::AUDIO,          (TCODE_FLAG_VARIABLE_LEN),                                 0,  "AUDIO"},
};


/**
* Given a type code, find and return the entire TypeCodeDef.
*
* @param  TCode the type code being asked about.
* @return The desired TypeCodeDef, or nullptr on "not supported".
*/
static const TypeCodeDef* _get_type_def(const TCode tc) {
  uint8_t idx = 0;
  while (idx < (sizeof(static_type_codes) / sizeof(TypeCodeDef))) {
    // Search for the code...
    if (static_type_codes[idx].type_code == tc) {
      return &static_type_codes[idx];
    }
    idx++;
  }
  return nullptr;
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
  const TypeCodeDef* def = _get_type_def(tc);
  if (nullptr != def) {
    return def->t_name;
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
  const TypeCodeDef* def = _get_type_def(TC);
  if (nullptr != def) {
    return !(def->type_flags & TCODE_FLAG_VARIABLE_LEN);
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
const int typeIsPointerPunned(const TCode TC) {
  const TypeCodeDef* def = _get_type_def(TC);
  if (nullptr != def) {
    return (def->type_flags & TCODE_FLAG_VALUE_IS_PUNNED_PTR);
  }
  return false;
}

/**
* Given type code, find size in bytes. Returns 0 for variable-length
*   arguments, since this is their minimum size.
*
* @param  TCode the type code being asked about.
* @return The size of the type (in bytes), or -1 on failure.
*/
const int sizeOfType(const TCode TC) {
  const TypeCodeDef* def = _get_type_def(TC);
  if (nullptr != def) {
    return def->fixed_len;
  }
  return -1;
}



static C3PTypeConstraint<int8_t>      c3p_type_helper_int8;
static C3PTypeConstraint<int16_t>     c3p_type_helper_int16;
static C3PTypeConstraint<int32_t>     c3p_type_helper_int32;
static C3PTypeConstraint<int64_t>     c3p_type_helper_int64;
static C3PTypeConstraint<uint8_t>     c3p_type_helper_uint8;
static C3PTypeConstraint<uint16_t>    c3p_type_helper_uint16;
static C3PTypeConstraint<uint32_t>    c3p_type_helper_uint32;
static C3PTypeConstraint<uint64_t>    c3p_type_helper_uint64;
static C3PTypeConstraint<bool>        c3p_type_helper_bool;
static C3PTypeConstraint<float>       c3p_type_helper_float;
static C3PTypeConstraint<double>      c3p_type_helper_double;
static C3PTypeConstraint<char*>       c3p_type_helper_str;

static C3PTypeConstraint<Vector3u32>     c3p_type_helper_vect3_u32;
static C3PTypeConstraint<Vector3f>       c3p_type_helper_vect3_float;


/**
* Given type code, find the helper object.
*
* @param  TCode the type code being asked about.
* @return A pointer to the abstracted helper object.
*/
C3PType* getTypeHelper(const TCode TC) {
  //const TypeCodeDef* def = _get_type_def(TC);
  //if (nullptr != def) {
  //  return def->helper_obj;
  //}
  switch (TC) {
    case TCode::INT8:               return (C3PType*) &c3p_type_helper_int8;
    case TCode::INT16:              return (C3PType*) &c3p_type_helper_int16;
    case TCode::INT32:              return (C3PType*) &c3p_type_helper_int32;
    case TCode::UINT8:              return (C3PType*) &c3p_type_helper_uint8;
    case TCode::UINT16:             return (C3PType*) &c3p_type_helper_uint16;
    case TCode::UINT32:             return (C3PType*) &c3p_type_helper_uint32;
    case TCode::INT64:              return (C3PType*) &c3p_type_helper_int64;
    case TCode::INT128:             return nullptr;
    case TCode::UINT64:             return (C3PType*) &c3p_type_helper_uint64;
    case TCode::UINT128:            return nullptr;
    case TCode::BOOLEAN:            return (C3PType*) &c3p_type_helper_bool;
    case TCode::FLOAT:              return (C3PType*) &c3p_type_helper_float;
    case TCode::DOUBLE:             return (C3PType*) &c3p_type_helper_double;
    case TCode::BINARY:             return nullptr;
    case TCode::STR:                return (C3PType*) &c3p_type_helper_str;
    case TCode::VECT_2_FLOAT:       return nullptr;
    case TCode::VECT_2_DOUBLE:      return nullptr;
    case TCode::VECT_2_INT8:        return nullptr;
    case TCode::VECT_2_UINT8:       return nullptr;
    case TCode::VECT_2_INT16:       return nullptr;
    case TCode::VECT_2_UINT16:      return nullptr;
    case TCode::VECT_2_INT32:       return nullptr;
    case TCode::VECT_2_UINT32:      return nullptr;
    case TCode::VECT_3_FLOAT:       return (C3PType*) &c3p_type_helper_vect3_float;
    case TCode::VECT_3_DOUBLE:      return nullptr;
    case TCode::VECT_3_INT8:        return nullptr;
    case TCode::VECT_3_UINT8:       return nullptr;
    case TCode::VECT_3_INT16:       return nullptr;
    case TCode::VECT_3_UINT16:      return nullptr;
    case TCode::VECT_3_INT32:       return nullptr;
    case TCode::VECT_3_UINT32:      return (C3PType*) &c3p_type_helper_vect3_u32;
    case TCode::VECT_4_FLOAT:       return nullptr;
    case TCode::URL:                return nullptr;
    case TCode::JSON:               return nullptr;
    case TCode::CBOR:               return nullptr;
    case TCode::LATLON:             return nullptr;
    case TCode::COLOR8:             return nullptr;
    case TCode::COLOR16:            return nullptr;
    case TCode::COLOR24:            return nullptr;
    case TCode::SI_UNIT:            return nullptr;
    case TCode::BASE64:             return nullptr;
    case TCode::IPV4_ADDR:          return nullptr;
    case TCode::KVP:                return nullptr;
    case TCode::STR_BUILDER:        return nullptr;
    case TCode::IDENTITY:           return nullptr;
    case TCode::AUDIO:              return nullptr;
    case TCode::IMAGE:              return nullptr;
    case TCode::GEOLOCATION:        return nullptr;
    default:                        return nullptr;
  }
}



/*******************************************************************************
* C3PTypeConstraint
* The blocks below hold templated code that constrains our implemented types,
*   and manages each type in a place where it can be easilly disabled as a
*   type-wrapper candidate type.
*******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
/// int8_t
template <> const TCode C3PTypeConstraint<int8_t>::tcode() {            return TCode::INT8;  }
template <> uint32_t    C3PTypeConstraint<int8_t>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t      C3PTypeConstraint<int8_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
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
template <> const TCode C3PTypeConstraint<int16_t>::tcode() {            return TCode::INT16;  }
template <> uint32_t    C3PTypeConstraint<int16_t>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t      C3PTypeConstraint<int16_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
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
template <> const TCode C3PTypeConstraint<int32_t>::tcode() {            return TCode::INT32;  }
template <> uint32_t    C3PTypeConstraint<int32_t>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t      C3PTypeConstraint<int32_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
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
template <> const TCode C3PTypeConstraint<int64_t>::tcode() {            return TCode::INT64;  }
template <> uint32_t    C3PTypeConstraint<int64_t>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t      C3PTypeConstraint<int64_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
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
template <> const TCode C3PTypeConstraint<uint8_t>::tcode() {            return TCode::UINT8;  }
template <> uint32_t    C3PTypeConstraint<uint8_t>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t      C3PTypeConstraint<uint8_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
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
template <> const TCode C3PTypeConstraint<uint16_t>::tcode() {            return TCode::UINT16;  }
template <> uint32_t    C3PTypeConstraint<uint16_t>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t      C3PTypeConstraint<uint16_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
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
template <> const TCode C3PTypeConstraint<uint32_t>::tcode() {            return TCode::UINT32;  }
template <> uint32_t    C3PTypeConstraint<uint32_t>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t      C3PTypeConstraint<uint32_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
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
template <> const TCode C3PTypeConstraint<uint64_t>::tcode() {            return TCode::UINT64;  }
template <> uint32_t    C3PTypeConstraint<uint64_t>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t      C3PTypeConstraint<uint64_t>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
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
template <> const TCode C3PTypeConstraint<bool>::tcode() {            return TCode::BOOLEAN;  }
template <> uint32_t    C3PTypeConstraint<bool>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t      C3PTypeConstraint<bool>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
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
template <> const TCode C3PTypeConstraint<float>::tcode() {            return TCode::FLOAT;  }
template <> uint32_t    C3PTypeConstraint<float>::length(void* obj) {  return sizeOfType(tcode());  }
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
    case TCode::FLOAT:      memcpy(dest, src, sizeOfType(tcode()));  return 0;
    default:  break;
  }
  if (0 == ret) {
    memcpy(dest, (void*) &d, sizeOfType(tcode()));
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<float>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  // To avoid inducing bugs related to alignment, we copy the value byte-wise
  //   into our on-stack storage, where it's alignment can be controlled.
  float s = 0.0f;
  switch (DEST_TYPE) {
    case TCode::FLOAT:      memcpy(dest, src, sizeOfType(tcode()));  return 0;
    case TCode::DOUBLE:
      {
        memcpy((void*) &s, src, sizeOfType(tcode()));
        double s_d = (1.0d * s);
        memcpy(dest, (void*) &s_d, sizeOfType(TCode::DOUBLE));
      }
      return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<float>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        float s = 0.0f;
        memcpy((void*) &s, obj, sizeOfType(tcode()));
        encoder.write_float(s);
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
template <> const TCode C3PTypeConstraint<double>::tcode() {            return TCode::DOUBLE;  }
template <> uint32_t    C3PTypeConstraint<double>::length(void* obj) {  return sizeOfType(tcode());  }
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
    case TCode::DOUBLE:     memcpy(dest, src, sizeOfType(tcode()));  return 0;
    default:  break;
  }
  if (0 == ret) {
    _store_in_mem(dest, d);
  }
  return ret;
}

template <> int8_t      C3PTypeConstraint<double>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  switch (DEST_TYPE) {
    case TCode::DOUBLE:     memcpy(dest, src, sizeOfType(tcode()));   return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<double>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat((uint8_t*) obj, length());
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        double s = 0.0f;
        memcpy((void*) &s, obj, sizeOfType(tcode()));
        encoder.write_double(s);
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
template <> const TCode C3PTypeConstraint<char*>::tcode() {            return TCode::STR;  }
template <> uint32_t    C3PTypeConstraint<char*>::length(void* obj) {  return (strlen((char*) obj) + 1);  }
template <> void        C3PTypeConstraint<char*>::to_string(void* obj, StringBuilder* out) {  out->concatf("%s", (char*) obj);  }

template <> int8_t      C3PTypeConstraint<char*>::set_from(void* dest, const TCode SRC_TYPE, void* src) {
  char* d = (char*) dest;
  switch (SRC_TYPE) {
    //case TCode::STR:     *d = *((char**) src);    return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<char*>::get_as(void* src, const TCode DEST_TYPE, void* dest) {
  char* s = (char*) src;
  switch (DEST_TYPE) {
    //case TCode::STR:     *((char**) dest) = s;    return 0;
    default:  break;
  }
  return -1;
}

template <> int8_t      C3PTypeConstraint<char*>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
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
/// Vector3f
template <> const TCode C3PTypeConstraint<Vector3f>::tcode() {            return TCode::VECT_3_FLOAT;  }
template <> uint32_t    C3PTypeConstraint<Vector3f>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t C3PTypeConstraint<Vector3f>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat(*((const char**) obj));
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        // NOTE: This ought to work for any types retaining portability isn't important.
        // TODO: Gradually convert types out of this block. As much as possible should
        //   be portable. VECT_3_FLOAT ought to be an array of floats, for instance.
        encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(tcode()));
        encoder.write_bytes((uint8_t*) obj, length(obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t C3PTypeConstraint<Vector3f>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
/// Vector3u32
template <> const TCode C3PTypeConstraint<Vector3u32>::tcode() {            return TCode::VECT_3_UINT32;  }
template <> uint32_t    C3PTypeConstraint<Vector3u32>::length(void* obj) {  return sizeOfType(tcode());  }
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

template <> int8_t C3PTypeConstraint<Vector3u32>::serialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  switch (FORMAT) {
    case TCode::BINARY:
      //out->concat(*((const char**) obj));
      break;

    case TCode::CBOR:
      {
        cbor::output_stringbuilder output(out);
        cbor::encoder encoder(output);
        // NOTE: This ought to work for any types retaining portability isn't important.
        // TODO: Gradually convert types out of this block. As much as possible should
        //   be portable. VECT_3_FLOAT ought to be an array of floats, for instance.
        encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(tcode()));
        encoder.write_bytes((uint8_t*) obj, length(obj));
      }
      break;

    default:  break;
  }
  return ret;
}

template <> int8_t C3PTypeConstraint<Vector3u32>::deserialize(void* obj, StringBuilder* out, const TCode FORMAT) {
  int8_t ret = -1;
  return ret;
}
