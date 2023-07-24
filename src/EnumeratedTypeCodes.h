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


The TCode enum and surrounding functions should not be needed outside of a few
  special-cases in parser/packer code. Most of it is abstracted away by type
  polymorphism in classes that need to distinguish types. Notably: KeyValuePair.

This is also the correct place to store constants commonly used in programs.

TODO: Might-should adopt some IANA standard code-spaces here? Is there a
  painless way to get better inter-op? Dig...
*/

#include <inttypes.h>
#include <stddef.h>

#ifndef __ENUMERATED_TYPE_CODES_H__
#define __ENUMERATED_TYPE_CODES_H__

class StringBuilder;


/*
* Constants related to differentials in systems of time-keeping and other units.
*/
#define  LEAP_SECONDS_SINCE_EPOCH    27        // Last checked: 2023.07.22
#define  CELCIUS_KELVIN_REBASE       273.15f


/* Physical and mathematical constants. */
#define SPEED_OF_LIGHT         299792458  // Given in vacuum in m/s
#define PRESSURE_AT_SEA_LEVEL  101325.0f  // Given in Pascals
#define MEAN_RADIUS_OF_EARTH     6371009  // Given in meters (IUGG recommend value).

/* Ensure that we have a definition of pi. */
#ifndef PI
  #define PI 3.14159265358979323846264338327950288419716939937510
#endif



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
#define TCODE_FLAG_HAS_DESTRUCTOR      0x10  // This type is allocated with new().
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
  //INVALID       = 0xFF     // A code denoting TCode invalidity.
};


/* Quick inlines to facilitate moving into and out of serialization. */
inline uint8_t TcodeToInt(const TCode code) {   return (const uint8_t) code; };
inline TCode IntToTcode(const uint8_t code) {   return (const TCode) code;   };

const char* const typecodeToStr(const TCode);
const bool typeIsFixedLength(const TCode);
const int typeIsPointerPunned(const TCode);
const int sizeOfType(const TCode);


/*******************************************************************************
* An abstract typeless data container class. This is used to support type      *
*   abstraction of our internal types, and cuts down on templating elsewhere.  *
*******************************************************************************/
// TODO: This needs to eat all of the type polymorphism in KeyValuePair.
class C3PValue {
  public:
    const TCode TCODE;

    C3PValue(const TCode TC) : TCODE(TC) {};
    ~C3PValue() {};

    int8_t serialize(StringBuilder*, TCode);
    int8_t deserialize(StringBuilder*, TCode);


  private:
};


/*******************************************************************************
* Support functions for dealing with SI unit codes (AKA: UCodes)               *
*******************************************************************************/

/*
* Enum for SI units.
*
* NOTE: This is being reworked to allow natural unit derivations. Only code
*   against base units, and the listed derived units.
* TODO: Take the most useful stuff and leave codespace for the rest:
*   https://en.wikipedia.org/wiki/List_of_dimensionless_quantities
*/
enum class SIUnit : uint8_t {
  UNITLESS          = 0x00,  // This is also used as terminator for multibyte unit strings.

  /* SI base units */
  SECONDS           = 0x01,
  METERS            = 0x02,
  GRAMS             = 0x03,  // Kilograms breaks logical consistency. We use Grams.
  AMPERES           = 0x04,
  KELVIN            = 0x05,
  MOLES             = 0x06,
  CANDELAS          = 0x07,

  /* Scalar (dimensionless) units */
  COUNTS            = 0x08,   // How many of something.
  DEGREES           = 0x09,
  RADIANS           = 0x0A,
  STERADIANS        = 0x0B,
  PH                = 0x0C,
  DECIBEL           = 0x0D,
  GEES              = 0x0E,  // How many multiples of Earth-gravity.

  /* Derived units. Buildable with grammar, but try to cover the bases. */
  COULOMBS          = 0x40,
  VOLTS             = 0x41,
  FARADS            = 0x42,
  OHMS              = 0x43,
  WEBERS            = 0x44,
  TESLAS            = 0x45,
  LUMENS            = 0x46,
  HERTZ             = 0x47,  // Counts/second
  NEWTONS           = 0x48,
  PASCALS           = 0x49,
  JOULES            = 0x4A,
  WATTS             = 0x4B,
  CELCIUS           = 0x4C,

  /* Constants and ratios */
  CONSTANT_PI       = 0xC0,  //
  CONSTANT_EULER    = 0xC1,  //
  CONSTANT_C        = 0xC2,  // Speed of light in a vacuum.
  CONSTANT_G        = 0xC3,  // Universal Gravitational Constant

  /*
  * Operator and meta UCodes for internal unit grammar support.
  *
  * Notes on specific UCodes:
  * ----------------------------------------------------------------------------
  * UNIT_GRAMMAR_MARKER:
  *   If you need a unit that is not covered by the predefined units above, or
  *   you want to handle units with more nuance, you can construct custom units
  *   by making this the first UCode in a string of UCodes terminated by
  *   UNITLESS (null-terminated).
  *
  * META_DIMENSIONALITY:
  *   This is used to distinguish a vector quantity from a list of scalars. If
  *   omitted in a unit string, it is implied to be 1. It is optional in that
  *   case.
  */
  META_ORDER_OF_MAGNITUDE    = 0xF0, // Takes 1 arg (int8): The OoM of the quantity.
  META_DIMENSIONALITY        = 0xF1, // Takes 1 arg (uint8): The dimentionality of the quantity.
  META_EXTENDED_CONSTANT     = 0xF2, // Takes 1 arg (uint8): An extended code for a defined constant.
  META_LITERAL_TCODE         = 0xF3, // Takes 2 args (TCode, value): A literal numeric value of the specified type.
  META_RESERVED_1            = 0xF4, // Reserved code.
  META_RESERVED_2            = 0xF5, // Reserved code.
  META_RESERVED_3            = 0xF6, // Reserved code.
  UNIT_GRAMMAR_MARKER        = 0xF7, // Denotes a multibyte string containing unit grammar.
  OPERATOR_EXPONENT          = 0xF8, // Takes 1 arg (int8): The exponent value.
  OPERATOR_PLUS              = 0xF9, //
  OPERATOR_MINUS             = 0xFA, //
  OPERATOR_MULTIPLIED        = 0xFB, //
  OPERATOR_DIVIDED           = 0xFC, //
  OPERATOR_GROUP_LEFT        = 0xFD, // "("
  OPERATOR_GROUP_RIGHT       = 0xFE, // ")"
  INVALID                    = 0xFF  // An invalid catch-all enum.
};


/* Quick inlines to facilitate moving into and out of serialization. */
inline uint8_t SIUnitToInt(const SIUnit code) {  return (const uint8_t) code; };
inline SIUnit IntToSIUnit(const uint8_t code) {  return (const SIUnit) code;  };

const char* const metricPrefixStr(const int8_t OOM, const bool sym = false);
const char* const SIUnitToStr(const SIUnit, const bool sym = false);
void SIUnitToStr(const SIUnit*, StringBuilder*, const bool sym);

#endif // __ENUMERATED_TYPE_CODES_H__
