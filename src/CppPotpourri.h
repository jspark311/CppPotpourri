/*
File:   CppPotpourri.h
Author: J. Ian Lindsay
Date:   2020.01.20

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

#include <inttypes.h>
#include <stdint.h>
#include "EnumeratedTypeCodes.h"

#ifndef __CPPPOTPOURRI_H__
#define __CPPPOTPOURRI_H__

class StringBuilder;  // Forward declaration for buffer interchange.


/*
* Using macros for these purposes can generate some hilarious bugs. Using
*   inlines gives us the benefit of strict type-checking at compile time, and
*   carries no costs.
*/
inline double   strict_max(double   a, double   b) {  return (a > b) ? a : b; };
inline float    strict_max(float    a, float    b) {  return (a > b) ? a : b; };
inline uint32_t strict_max(uint32_t a, uint32_t b) {  return (a > b) ? a : b; };
inline uint16_t strict_max(uint16_t a, uint16_t b) {  return (a > b) ? a : b; };
inline uint8_t  strict_max(uint8_t  a, uint8_t  b) {  return (a > b) ? a : b; };
inline int32_t  strict_max(int32_t  a, int32_t  b) {  return (a > b) ? a : b; };
inline int16_t  strict_max(int16_t  a, int16_t  b) {  return (a > b) ? a : b; };
inline int8_t   strict_max(int8_t   a, int8_t   b) {  return (a > b) ? a : b; };

inline double   strict_min(double   a, double   b) {  return (a < b) ? a : b; };
inline float    strict_min(float    a, float    b) {  return (a < b) ? a : b; };
inline uint32_t strict_min(uint32_t a, uint32_t b) {  return (a < b) ? a : b; };
inline uint16_t strict_min(uint16_t a, uint16_t b) {  return (a < b) ? a : b; };
inline uint8_t  strict_min(uint8_t  a, uint8_t  b) {  return (a < b) ? a : b; };
inline int32_t  strict_min(int32_t  a, int32_t  b) {  return (a < b) ? a : b; };
inline int16_t  strict_min(int16_t  a, int16_t  b) {  return (a < b) ? a : b; };
inline int8_t   strict_min(int8_t   a, int8_t   b) {  return (a < b) ? a : b; };

/*
* Type-strict value swap.
*/
inline void strict_swap(double*   a, double*   b) {  double   t = *a; *a = *b; *b = t;  };
inline void strict_swap(float*    a, float*    b) {  float    t = *a; *a = *b; *b = t;  };
inline void strict_swap(uint32_t* a, uint32_t* b) {  uint32_t t = *a; *a = *b; *b = t;  };
inline void strict_swap(uint16_t* a, uint16_t* b) {  uint16_t t = *a; *a = *b; *b = t;  };
inline void strict_swap(uint8_t*  a, uint8_t*  b) {  uint8_t  t = *a; *a = *b; *b = t;  };
inline void strict_swap(int32_t*  a, int32_t*  b) {  int32_t  t = *a; *a = *b; *b = t;  };
inline void strict_swap(int16_t*  a, int16_t*  b) {  int16_t  t = *a; *a = *b; *b = t;  };
inline void strict_swap(int8_t*   a, int8_t*   b) {  int8_t   t = *a; *a = *b; *b = t;  };

/*
* Given two values, gives the difference between them after accounting for wrap.
*/
inline double   wrap_accounted_delta(double   a, double   b) {   return (a > b) ? (a - b) : (b - a);   };
inline float    wrap_accounted_delta(float    a, float    b) {   return (a > b) ? (a - b) : (b - a);   };
inline uint64_t wrap_accounted_delta(uint64_t a, uint64_t b) {   return (a > b) ? (a - b) : (b - a);   };
inline uint32_t wrap_accounted_delta(uint32_t a, uint32_t b) {   return (a > b) ? (a - b) : (b - a);   };
inline uint16_t wrap_accounted_delta(uint16_t a, uint16_t b) {   return (a > b) ? (a - b) : (b - a);   };
inline uint8_t  wrap_accounted_delta(uint8_t  a, uint8_t  b) {   return (a > b) ? (a - b) : (b - a);   };
inline int32_t  wrap_accounted_delta(int32_t  a, int32_t  b) {   return (a > b) ? (a - b) : (b - a);   };
inline int16_t  wrap_accounted_delta(int16_t  a, int16_t  b) {   return (a > b) ? (a - b) : (b - a);   };
inline int8_t   wrap_accounted_delta(int8_t   a, int8_t   b) {   return (a > b) ? (a - b) : (b - a);   };

/**
* Endian conversion fxns
*/
inline uint16_t endianSwap16(uint16_t x) {   return __builtin_bswap16(x);    };
inline uint32_t endianSwap32(uint32_t x) {   return __builtin_bswap32(x);    };
inline uint64_t endianSwap64(uint64_t x) {   return __builtin_bswap64(x);    };

/**
* High-level string operations.
* TODO: These feel like they are in the wrong place.
*/
void timestampToString(StringBuilder*, uint64_t);
uint64_t stringToTimestamp(const char*);
int randomArt(uint8_t* dgst_raw, unsigned int dgst_raw_len, const char* title, StringBuilder*);


/*
* Constants related to differentials in systems of time-keeping and other units.
*/
#define  LEAP_SECONDS_SINCE_EPOCH    27
#define  CELCIUS_KELVIN_REBASE       273.15f


/* Physical and mathematical constants. */
#define SPEED_OF_LIGHT         299792458  // Given in vacuum in m/s
#define PRESSURE_AT_SEA_LEVEL  101325.0f  // Given in Pascals
#define MEAN_RADIUS_OF_EARTH     6371009  // Given in meters (IUGG recommend value).


#ifndef PI
  #define PI 3.14159265358979323846264338327950288419716939937510
#endif


/*
* Enum for SI units.
* TODO: This will likely be reworked to allow natural unit derivations.
*/
enum class SIUnit : uint8_t {
  UNITLESS          = 0,
  /* SI base units */
  SECONDS           = 1,
  METERS            = 2,
  GRAMS             = 3,  // Kilograms breaks logical consistency. We use Grams.
  AMPERES           = 4,
  CELCIUS           = 5,  // Kelvin cleanly interconverts. We use Celcius.
  MOLES             = 6,
  CANDELAS          = 7,
  /* Derived units */
  HERTZ             = 8,
  RADIANS           = 9,
  STERADIANS        = 10,
  NEWTONS           = 11,
  PASCALS           = 12,
  JOULES            = 13,
  WATTS             = 14,
  COULOMBS          = 15,
  VOLTS             = 16,
  FARADS            = 17,
  OHMS              = 18,
  WEBERS            = 19,
  TESLAS            = 20,
  LUMENS            = 21,
  /* Units as related to time */
  METERS_PER_SECOND          = 128,   // Speed
  METERS_PER_SECOND_SQUARED  = 129,   // Acceleration
  METERS_PER_SECOND_CUBED    = 130,   // Impulse
  RADIANS_PER_SECOND         = 131,   // Angular velocity
  RADIANS_PER_SECOND_SQUARED = 132,   // Angular acceleration
  RADIANS_PER_SECOND_CUBED   = 133    // Angular impulse
};


/*******************************************************************************
* Interfaces and callback definitions in use throughout this library.
*******************************************************************************/
/* Shorthand for a pointer to a "void fxn(void)" */
typedef void  (*FxnPointer)();

/* Callbacks for drivers that provide extra GPI pins. */
typedef void (*PinCallback)(uint8_t pin, uint8_t level);


/* An interface class for accepting a buffer. */
class BufferAccepter {
  public:
    /**
    * A class would implement this class to provide an interface for accepting a
    *   formless buffer from another class.
    * NOTE: This idea was the fundamental idea behind Manuvr's BufferPipe class,
    *   which was not pure virtual, and carried far more implementation burden.
    *
    * @return -1 to reject buffer, 0 to accept without claiming, 1 to accept with claim.
    */
    virtual int8_t provideBuffer(StringBuilder* buf) =0;
};


/* An interface class for accepting a scalar value, with units. */
class ScalarAccepter {
  public:
    /**
    * A class would implement this class to provide an interface for accepting a
    *   value from outside tagged with a real-world unit and error bars.
    *
    * @return -1 to reject buffer, 0 to accept without claiming, 1 to accept with claim.
    */
    virtual int8_t provideScalar(SIUnit, double value, double error) =0;
};

#endif // __CPPPOTPOURRI_H__
