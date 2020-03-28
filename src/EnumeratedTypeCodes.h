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
  NONE          = 0x00,    // Reserved. Denotes end-of-list.
  INT           = 0x01,    // A signed integer
  UINT          = 0x02,    // An unsigned integer
  BOOLEAN       = 0x03,    // A boolean
  FLOAT         = 0x04,    // A float or double
  STR           = 0x05,    // A null-terminated string
  BINARY        = 0x06,    // A collection of bytes, usually given as a hex string.
  VECT_4_FLOAT  = 0x07,    // A vector of floats in 4-space
  VECT_3_FLOAT  = 0x08,    // A vector of floats in 3-space
  VECT_3_INT    = 0x09,    // A vector of signed integers in 3-space
  VECT_3_UINT   = 0x0A,    // A vector of unsigned integers in 3-space
  JSON          = 0x0B,    // A JSON object. Semantic layer on STR.
  CBOR          = 0x0C,    // A CBOR object. Semantic layer on BINARY.
  URL           = 0x0D,    // An alias of string that carries the semantic 'URL'.
  COLOR16       = 0x0E,    // 16-bit color data. Color is represented as RGB (565).
  COLOR24       = 0x0F,    // 24-bit color data. Color is represented as RGB (888).
  RESERVED      = 0xFF     // Reserved for custom extension.
};


// This might work, but it doesn't allow us to derive other units from enum code alone.
// Would rather avoid lots of executable logic for accomplishing conversions.
// We are enabling this experiment. Don't get too used to it, because it will
//   almost certainly change.
enum class UnitCode : uint8_t {
  /* SI base units */
  SECONDS       = 0x00,    //
  METERS        = 0x01,    //
  GRAMS         = 0x02,    // SI base unit is Kg. But makes logic harder. Flout convention.
  AMPERES       = 0x03,
  KELVIN        = 0x04,
  MOL           = 0x05,
  CANDELA       = 0x06
};


// Shorthand for a pointer to a "void fxn(void)"
typedef void  (*FxnPointer)();

#endif // __ENUMERATED_TYPE_CODES_H__
