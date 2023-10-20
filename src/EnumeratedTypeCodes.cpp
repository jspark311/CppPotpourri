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
#include "StringBuilder.h"


/*******************************************************************************
* Support functions for dealing with SIUnit codes.                             *
*******************************************************************************/
const char* const SIUnitToStr(const int8_t OOM, const bool sym) {
  switch (OOM) {
    case -15:  return (sym ? "f" : "femto");
    case -12:  return (sym ? "p" : "pico");
    case -9:   return (sym ? "n" : "nano");
    case -6:   return (sym ? "u" : "micro");
    case -3:   return (sym ? "m" : "milli");
    case -2:   return (sym ? "m" : "centi");
    case -1:   return (sym ? "m" : "deci");
    case 0:    return "";
    case 1:    return (sym ? "da" : "deca");
    case 2:    return (sym ? "h" : "hecto");
    case 3:    return (sym ? "k" : "kilo");
    case 6:    return (sym ? "M" : "mega");
    case 9:    return (sym ? "G" : "giga");
    case 12:   return (sym ? "T" : "tera");
    case 15:   return (sym ? "P" : "peta");
    default:   return "";
  }
}


const char* const SIUnitToStr(const SIUnit UC, const bool sym) {
  switch (UC) {
    case SIUnit::UNITLESS:                return (sym ? "" : "unitless");
    case SIUnit::SECONDS:                 return (sym ? "s" : "seconds");
    case SIUnit::METERS:                  return (sym ? "m" : "meters");
    case SIUnit::GRAMS:                   return (sym ? "g" : "grams");
    case SIUnit::AMPERES:                 return (sym ? "A" : "Amps");
    case SIUnit::KELVIN:                  return (sym ? "K" : "Kelvin");
    case SIUnit::MOLES:                   return (sym ? "mol" : "mol");
    case SIUnit::CANDELAS:                return (sym ? "cd" : "candela");
    case SIUnit::COUNTS:                  return (sym ? "" : "counts");
    case SIUnit::DEGREES:                 return (sym ? "deg" : "degrees");
    case SIUnit::RADIANS:                 return (sym ? "rad" : "radians");
    case SIUnit::STERADIANS:              return (sym ? "str" : "steradians");
    case SIUnit::PH:                      return (sym ? "pH" : "pH");
    case SIUnit::DECIBEL:                 return (sym ? "dB" : "dB");
    case SIUnit::GEES:                    return (sym ? "g's" : "g's");
    case SIUnit::COULOMBS:                return (sym ? "Q" : "Coulombs");
    case SIUnit::VOLTS:                   return (sym ? "V" : "Volts");
    case SIUnit::FARADS:                  return (sym ? "F" : "Farads");
    case SIUnit::OHMS:                    return (sym ? "Ohms" : "Ohms");
    case SIUnit::WEBERS:                  return (sym ? "Wb" : "Webers");
    case SIUnit::TESLAS:                  return (sym ? "T" : "Teslas");
    case SIUnit::LUMENS:                  return (sym ? "lm" : "lumens");
    case SIUnit::HERTZ:                   return (sym ? "Hz" : "Hertz");
    case SIUnit::NEWTONS:                 return (sym ? "N" : "Newtons");
    case SIUnit::PASCALS:                 return (sym ? "Pa" : "Pascals");
    case SIUnit::JOULES:                  return (sym ? "J" : "Joules");
    case SIUnit::WATTS:                   return (sym ? "W" : "Watts");
    case SIUnit::CELCIUS:                 return (sym ? "C" : "Celcius");
    case SIUnit::CONSTANT_PI:             return (sym ? "pi" : "pi");
    case SIUnit::CONSTANT_EULER:          return (sym ? "e" : "e");
    case SIUnit::CONSTANT_C:              return (sym ? "c" : "c");
    case SIUnit::CONSTANT_G:              return (sym ? "G" : "G");
    case SIUnit::OPERATOR_EXPONENT:       return "^";
    case SIUnit::OPERATOR_PLUS:           return "+";
    case SIUnit::OPERATOR_MINUS:          return "-";
    case SIUnit::OPERATOR_MULTIPLIED:     return "*";
    case SIUnit::OPERATOR_DIVIDED:        return "/";
    case SIUnit::OPERATOR_GROUP_LEFT:     return "(";
    case SIUnit::OPERATOR_GROUP_RIGHT:    return ")";
    default:   return "";   // Anything non-printable is an empty string.
  }
}


void SIUnitToStr(const SIUnit* UC_STR, StringBuilder* output, const bool sym) {
  if (nullptr != UC_STR) {
    SIUnit* cur_ptr = (SIUnit*) UC_STR;
    if (SIUnit::UNIT_GRAMMAR_MARKER == *cur_ptr) {
      // This is going to be a multibyte operation...
      int8_t oom = 0;   // Implied base order-of-magnitude.
      cur_ptr++;
      while (SIUnit::UNITLESS != *cur_ptr) {
        const SIUnit CURRENT_UCODE = *cur_ptr++;
        switch (CURRENT_UCODE) {
          case SIUnit::META_ORDER_OF_MAGNITUDE:
            oom = (int8_t) *cur_ptr++;
            break;
          case SIUnit::META_DIMENSIONALITY:
            break;
          case SIUnit::META_LITERAL_TCODE:
            {  // TODO: Assumes an int8 (wrongly, eventually).
              int8_t literal_int = (int8_t) *cur_ptr++;
              if (-1 == literal_int) {
                output->concat('-');
              }
              else if (1 != literal_int) {
                output->concatf("%d", literal_int);
              }
            }
            break;

          default:
            output->concat(SIUnitToStr(CURRENT_UCODE, sym));
            break;
        }
      }

      // Modify the unit with the SI prefix, if called for.
      output->prepend(SIUnitToStr(oom, sym));
    }
    else {
      output->concat(SIUnitToStr(*cur_ptr, sym));
    }
  }
}


/*******************************************************************************
* Support functions for dealing with line terminators.                         *
*******************************************************************************/

/**
* This is the private structure to hold line-terminator data.
*/
//typedef struct typecode_def_t {
//  const LineTerm    code;     // This identifies the type to parsers/packers.
//  const uint8_t     length;   // Fixed metadata about a type, as this build implements it.
//  const char* const name;     // The name of the type.
//  const char* const literal;  // The name of the type.
//} LineTermDef;
//
//
//static const LineTermDef static_lf_data[] = {
//  {LineTermDef::ZEROBYTE, 0,   "\0",   "ZEROBYTE"},
//  {LineTermDef::CR,       1,   "\r",   "CR"},
//  {LineTermDef::LF,       1,   "\n",   "LF"},
//  {LineTermDef::CRLF,     2,   "\r\n", "CRLF"},
//  {LineTermDef::INVALID,  0,   "",     "INVALID"}
//};


const char* const lineTerminatorNameStr(const LineTerm lt) {
  switch (lt) {
    case LineTerm::ZEROBYTE:  return "ZEROBYTE";
    case LineTerm::CR:        return "CR";
    case LineTerm::LF:        return "LF";
    case LineTerm::CRLF:      return "CRLF";
    default:  break;
  }
  return "";
}


const char* const lineTerminatorLiteralStr(const LineTerm lt) {
  switch (lt) {
    case LineTerm::ZEROBYTE:  return "\0";
    case LineTerm::CR:        return "\r";
    case LineTerm::LF:        return "\n";
    case LineTerm::CRLF:      return "\r\n";
    default:  break;
  }
  return "";
}


const uint8_t lineTerminatorLength(const LineTerm lt) {
  switch (lt) {
    case LineTerm::ZEROBYTE:
    case LineTerm::CR:
    case LineTerm::LF:        return 1;
    case LineTerm::CRLF:      return 2;
    default:  break;
  }
  return 0;
}
