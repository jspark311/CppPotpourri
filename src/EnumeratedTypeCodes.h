/*
File:   EnumeratedTypeCodes.h
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

#include <inttypes.h>
#include <stddef.h>

#ifndef __ENUMERATED_TYPE_CODES_H__
#define __ENUMERATED_TYPE_CODES_H__


/* A list of parameter types that are handled by the input parser. */
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
  URL           = 0x60,    // An alias of string that carries the semantic 'URL'.
  JSON          = 0x61,    // A JSON object. Semantic layer on STR.
  CBOR          = 0x62,    // A CBOR object. Semantic layer on BINARY.
  LATLON        = 0x63,    // An alias of VECT_2_DOUBLE that indicates a point on a sphere.
  COLOR8        = 0x64,    // Alias of UINT8. 8-bit color data
  COLOR16       = 0x65,    // Alias of UINT16. 16-bit color data
  COLOR24       = 0x66,    // Alias of UINT32. 24-bit color data

  /* Pointers to internal class instances */
  STR_BUILDER   = 0xFB,    // A pointer to a StringBuilder
  IDENTITY      = 0xFC,    // A pointer to an Identity class
  AUDIO         = 0xFD,    // A pointer to an audio stream
  IMAGE         = 0xFE,    // A pointer to an image class
  RESERVED      = 0xFF     // Reserved for custom extension.
};


/*******************************************************************************
* Support functions for dealing with type codes.                               *
*******************************************************************************/

/* Quick inlines to facilitate moving into and out of serialization. */
inline uint8_t TcodeToInt(const TCode code) {   return (uint8_t) code; };
inline TCode IntToTcode(const uint8_t code) {   return (TCode) code;   };


#endif // __ENUMERATED_TYPE_CODES_H__
