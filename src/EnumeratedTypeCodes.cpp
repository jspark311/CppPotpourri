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
*/

// TODO: Might-should adopt some IANA standard code-spaces here? Is there a
//   painless way to get better inter-op? Dig...

#include "EnumeratedTypeCodes.h"


/*******************************************************************************
* Support functions for dealing with type codes.                               *
*******************************************************************************/

const char* const typecodeToStr(TCode tc) {
  switch (tc) {
    case TCode::NONE:          return "NONE";
    case TCode::INT8:          return "INT8";
    case TCode::INT16:         return "INT16";
    case TCode::INT32:         return "INT32";
    case TCode::UINT8:         return "UINT8";
    case TCode::UINT16:        return "UINT16";
    case TCode::UINT32:        return "UINT32";
    case TCode::INT64:         return "INT64";
    case TCode::INT128:        return "INT128";
    case TCode::UINT64:        return "UINT64";
    case TCode::UINT128:       return "UINT128";
    case TCode::BOOLEAN:       return "BOOLEAN";
    case TCode::FLOAT:         return "FLOAT";
    case TCode::DOUBLE:        return "DOUBLE";
    case TCode::BINARY:        return "BINARY";
    case TCode::STR:           return "STR";
    case TCode::VECT_2_FLOAT:  return "VECT_2_FLOAT";
    case TCode::VECT_2_DOUBLE: return "VECT_2_DOUBLE";
    case TCode::VECT_2_INT8:   return "VECT_2_INT8";
    case TCode::VECT_2_UINT8:  return "VECT_2_UINT8";
    case TCode::VECT_2_INT16:  return "VECT_2_INT16";
    case TCode::VECT_2_UINT16: return "VECT_2_UINT16";
    case TCode::VECT_2_INT32:  return "VECT_2_INT32";
    case TCode::VECT_2_UINT32: return "VECT_2_UINT32";
    case TCode::VECT_3_FLOAT:  return "VECT_3_FLOAT";
    case TCode::VECT_3_DOUBLE: return "VECT_3_DOUBLE";
    case TCode::VECT_3_INT8:   return "VECT_3_INT8";
    case TCode::VECT_3_UINT8:  return "VECT_3_UINT8";
    case TCode::VECT_3_INT16:  return "VECT_3_INT16";
    case TCode::VECT_3_UINT16: return "VECT_3_UINT16";
    case TCode::VECT_3_INT32:  return "VECT_3_INT32";
    case TCode::VECT_3_UINT32: return "VECT_3_UINT32";
    case TCode::VECT_4_FLOAT:  return "VECT_4_FLOAT";
    case TCode::URL:           return "URL";
    case TCode::JSON:          return "JSON";
    case TCode::CBOR:          return "CBOR";
    case TCode::LATLON:        return "LATLON";
    case TCode::COLOR8:        return "COLOR8";
    case TCode::COLOR16:       return "COLOR16";
    case TCode::COLOR24:       return "COLOR24";
    case TCode::SI_UNIT:       return "SI_UNIT";
    case TCode::BASE64:        return "BASE64";
    case TCode::IPV4_ADDR:     return "IPV4_ADDR";
    case TCode::KVP:           return "KVP";
    case TCode::STR_BUILDER:   return "STR_BUILDER";
    case TCode::IDENTITY:      return "IDENTITY";
    case TCode::AUDIO:         return "AUDIO";
    case TCode::IMAGE:         return "IMAGE";
    case TCode::RESERVED:      return "RESERVED";
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
  switch (TC) {
    case TCode::NONE:           // Reserved. Denotes end-of-list.
    case TCode::INT8:           // 8-bit integer
    case TCode::INT16:          // 16-bit integer
    case TCode::INT32:          // 32-bit integer
    case TCode::UINT8:          // Unsigned 8-bit integer
    case TCode::UINT16:         // Unsigned 16-bit integer
    case TCode::UINT32:         // Unsigned 32-bit integer
    case TCode::INT64:          // 64-bit integer
    case TCode::INT128:         // 128-bit integer
    case TCode::UINT64:         // Unsigned 64-bit integer
    case TCode::UINT128:        // Unsigned 128-bit integer
    case TCode::BOOLEAN:        // A boolean
    case TCode::FLOAT:          // A float
    case TCode::DOUBLE:         // A double
    case TCode::VECT_2_FLOAT:   // A vector of floats in 2-space
    case TCode::VECT_2_DOUBLE:  // A vector of floats in 2-space
    case TCode::VECT_2_INT8:    // A vector of signed integers in 2-space
    case TCode::VECT_2_UINT8:   // A vector of unsigned integers in 2-space
    case TCode::VECT_2_INT16:   // A vector of signed integers in 2-space
    case TCode::VECT_2_UINT16:  // A vector of unsigned integers in 2-space
    case TCode::VECT_2_INT32:   // A vector of signed integers in 2-space
    case TCode::VECT_2_UINT32:  // A vector of unsigned integers in 2-space
    case TCode::VECT_3_FLOAT:   // A vector of floats in 3-space
    case TCode::VECT_3_DOUBLE:  // A vector of floats in 3-space
    case TCode::VECT_3_INT8:    // A vector of signed integers in 3-space
    case TCode::VECT_3_UINT8:   // A vector of unsigned integers in 3-space
    case TCode::VECT_3_INT16:   // A vector of signed integers in 3-space
    case TCode::VECT_3_UINT16:  // A vector of unsigned integers in 3-space
    case TCode::VECT_3_INT32:   // A vector of signed integers in 3-space
    case TCode::VECT_3_UINT32:  // A vector of unsigned integers in 3-space
    case TCode::VECT_4_FLOAT:   // A vector of floats in 4-space
    case TCode::LATLON:         // An alias of VECT_2_DOUBLE that indicates a point on a sphere.
    case TCode::COLOR8:         // Alias of UINT8. 8-bit color data
    case TCode::COLOR16:        // Alias of UINT16. 16-bit color data
    case TCode::COLOR24:        // Alias of UINT32. 24-bit color data
    case TCode::SI_UNIT:        // Alias of UINT8. An SIUnit enum value.
    case TCode::IPV4_ADDR:      // Alias of UINT32. An IP address, in network byte-order.
      return true;

    case TCode::URL:            // An alias of string that carries the semantic 'URL'.
    case TCode::JSON:           // A JSON object. Semantic layer on STR.
    case TCode::CBOR:           // A CBOR object. Semantic layer on BINARY.
    case TCode::KVP:            // A pointer to a KeyValuePair
    case TCode::STR_BUILDER:    // A pointer to a StringBuilder
    case TCode::IDENTITY:       // A pointer to an Identity class
    case TCode::AUDIO:          // A pointer to an audio stream
    case TCode::IMAGE:          // A pointer to an image class
    case TCode::BINARY:         // A collection of bytes
    case TCode::STR:            // A null-terminated string
    case TCode::RESERVED:       // Reserved for custom extension.
    default:
      return false;
  }
}
