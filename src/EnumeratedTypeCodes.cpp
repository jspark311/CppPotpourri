/*
File:   EnumeratedTypeCodes.cpp
Author: J. Ian Lindsay
Date:   2014.03.10

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


TODO: Might-should adopt some IANA standard code-spaces here? Is there a
  painless way to get better inter-op? Dig...
*/

#include "EnumeratedTypeCodes.h"

/**
* This is the private structure with which we define types. It conveys the type
*   code, the type's size, and any special attributes it might have.
* These type definitions are used by library components that serialize for the
*   purposes of storage or communication with other versions of this library. So
*   we should strive to be consistent.
*/
typedef struct typecode_def_t {
  const TCode    type_code;   // This identifies the type to parsers/packers.
  const uint8_t  type_flags;  // Fixed metadata about a type, as this build implements it.
  const uint16_t fixed_len;   // If this type has a fixed length, it will be set here. 0 if no fixed length.
  const char* const t_name;   // The name of the type.
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
  {TCode::IDENTITY,       (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_HAS_DESTRUCTOR),     0,  "IDENTITY"},
  {TCode::KVP,            (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_HAS_DESTRUCTOR),     0,  "KVP"},
  {TCode::STR,            (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_IS_NULL_DELIMITED),  0,  "STR"},
  {TCode::IMAGE,          (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_HAS_DESTRUCTOR),     0,  "IMAGE"},
  {TCode::COLOR8,         (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          1,  "COLOR8"},
  {TCode::COLOR16,        (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          2,  "COLOR16"},
  {TCode::COLOR24,        (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          3,  "COLOR24"},
  {TCode::BINARY,         (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "BINARY"},
  {TCode::BASE64,         (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "BASE64"},
  {TCode::CBOR,           (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "CBOR"},
  {TCode::STR_BUILDER,    (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_HAS_DESTRUCTOR),     0,  "STR_BLDR"},
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
  //{TCode::LATLON,         (0),                                                       16, "LATLON"};
  //{TCode::SI_UNIT,        (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          1,  "SI_UNIT"};
  //{TCode::IPV4_ADDR,      (TCODE_FLAG_VALUE_IS_PUNNED_PTR),                          4,  "IPV4_ADDR"};
  //{TCode::URL,            (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_IS_NULL_DELIMITED),  1,  "URL"},
  //{TCode::AUDIO,          (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_HAS_DESTRUCTOR),     0,  "AUDIO"},
  //{TCode::JSON,           (TCODE_FLAG_VARIABLE_LEN | TCODE_FLAG_LEGAL_FOR_ENCODING), 0,  "JSON"},
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
