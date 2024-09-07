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

This file is home to some simple enumerated types that aren't complex enough to
  be fractioned off into a separate file.

This is also the correct place to store constants and enumerated types commonly
  used in programs.
*/

#ifndef __ENUMERATED_TYPE_CODES_H__
#define __ENUMERATED_TYPE_CODES_H__

#include <inttypes.h>
#include <stddef.h>
#include "C3PValue/C3PType.h"

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
#define COFACTOR_RADIAN_TO_DEGREE  (180/PI)


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
  CONSTANT_PI       = 0xC0,  // pi
  CONSTANT_EULER    = 0xC1,  // e
  CONSTANT_C        = 0xC2,  // Speed of light in a vacuum.
  CONSTANT_G        = 0xC3,  // Universal Gravitational Constant.

  /*
  * Reduction of common idioms. Unit definitions should prefer to use one of
  *   these single-byte idioms, as opposed to spelling them out with
  *   more-general codes.
  * IE: These two unit strings both express acceleration, but the former is
  *   preferable to reduce load and bugs.
  *   {METERS, PER_SECOND_SQUARE, 0}
  *   {METERS, OPERATOR_DIVIDED, OPERATOR_GROUP_LEFT, SECONDS, OPERATOR_EXPONENT, 2, OPERATOR_GROUP_RIGHT, 0}
  *
  * TODO: Not sure if it is a good idea to do this, but it has clear advantages.
  *   Doing it until it either works out, or collapses.
  */
  PER_SECOND        = 0xE0,  // Rate
  PER_SECOND_SQUARE = 0xE1,  // Acceleration
  PER_SECOND_CUBE   = 0xE2,  // Jerk

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


/*
* Line-termination identifiers.
* NOTE: LF ("\n") is the firmware's internal standard for string representation.
* NOTE: The specific values of this enum must not be >8, since they are used
*   for bitmask generation. Does anyone need to support 8 different kinds of
*   line endings?
*/
enum class LineTerm : uint8_t {
  ZEROBYTE = 0x00,
  CR       = 0x01,
  LF       = 0x02,
  CRLF     = 0x03,
  INVALID  = 0x08  // Maximum valid value.
};

const char* const lineTerminatorNameStr(const LineTerm);
const char* const lineTerminatorLiteralStr(const LineTerm);
const uint8_t lineTerminatorLength(const LineTerm);

#endif // __ENUMERATED_TYPE_CODES_H__
