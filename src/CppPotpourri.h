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
inline uint64_t strict_max(uint64_t a, uint64_t b) {  return (a > b) ? a : b; };
inline uint32_t strict_max(uint32_t a, uint32_t b) {  return (a > b) ? a : b; };
inline uint16_t strict_max(uint16_t a, uint16_t b) {  return (a > b) ? a : b; };
inline uint8_t  strict_max(uint8_t  a, uint8_t  b) {  return (a > b) ? a : b; };
inline int64_t  strict_max(int64_t  a, int64_t  b) {  return (a > b) ? a : b; };
inline int32_t  strict_max(int32_t  a, int32_t  b) {  return (a > b) ? a : b; };
inline int16_t  strict_max(int16_t  a, int16_t  b) {  return (a > b) ? a : b; };
inline int8_t   strict_max(int8_t   a, int8_t   b) {  return (a > b) ? a : b; };

inline double   strict_min(double   a, double   b) {  return (a < b) ? a : b; };
inline float    strict_min(float    a, float    b) {  return (a < b) ? a : b; };
inline uint64_t strict_min(uint64_t a, uint64_t b) {  return (a < b) ? a : b; };
inline uint32_t strict_min(uint32_t a, uint32_t b) {  return (a < b) ? a : b; };
inline uint16_t strict_min(uint16_t a, uint16_t b) {  return (a < b) ? a : b; };
inline uint8_t  strict_min(uint8_t  a, uint8_t  b) {  return (a < b) ? a : b; };
inline int64_t  strict_min(int64_t  a, int64_t  b) {  return (a < b) ? a : b; };
inline int32_t  strict_min(int32_t  a, int32_t  b) {  return (a < b) ? a : b; };
inline int16_t  strict_min(int16_t  a, int16_t  b) {  return (a < b) ? a : b; };
inline int8_t   strict_min(int8_t   a, int8_t   b) {  return (a < b) ? a : b; };

/*
* Type-strict value swap.
*/
inline void strict_swap(double*   a, double*   b) {  double   t = *a; *a = *b; *b = t;  };
inline void strict_swap(float*    a, float*    b) {  float    t = *a; *a = *b; *b = t;  };
inline void strict_swap(uint64_t* a, uint64_t* b) {  uint64_t t = *a; *a = *b; *b = t;  };
inline void strict_swap(uint32_t* a, uint32_t* b) {  uint32_t t = *a; *a = *b; *b = t;  };
inline void strict_swap(uint16_t* a, uint16_t* b) {  uint16_t t = *a; *a = *b; *b = t;  };
inline void strict_swap(uint8_t*  a, uint8_t*  b) {  uint8_t  t = *a; *a = *b; *b = t;  };
inline void strict_swap(int64_t*  a, int64_t*  b) {  int64_t  t = *a; *a = *b; *b = t;  };
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
inline int64_t  wrap_accounted_delta(int64_t  a, int64_t  b) {   return (a > b) ? (a - b) : (b - a);   };
inline int32_t  wrap_accounted_delta(int32_t  a, int32_t  b) {   return (a > b) ? (a - b) : (b - a);   };
inline int16_t  wrap_accounted_delta(int16_t  a, int16_t  b) {   return (a > b) ? (a - b) : (b - a);   };
inline int8_t   wrap_accounted_delta(int8_t   a, int8_t   b) {   return (a > b) ? (a - b) : (b - a);   };

/*
* Given a value and a range, gives the saturated result.
* TODO: If the compiler gives this, it will likely be ASM optimized using no
*   branches. Until I care enough, I'll use a double-nested ternary. Suffer.
*/
inline double   range_bind(const double   VAL, const double   MIN, const double   MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline float    range_bind(const float    VAL, const float    MIN, const float    MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline uint64_t range_bind(const uint64_t VAL, const uint64_t MIN, const uint64_t MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline uint32_t range_bind(const uint32_t VAL, const uint32_t MIN, const uint32_t MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline uint16_t range_bind(const uint16_t VAL, const uint16_t MIN, const uint16_t MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline uint8_t  range_bind(const uint8_t  VAL, const uint8_t  MIN, const uint8_t  MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline int64_t  range_bind(const int64_t  VAL, const int64_t  MIN, const int64_t  MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline int32_t  range_bind(const int32_t  VAL, const int32_t  MIN, const int32_t  MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline int16_t  range_bind(const int16_t  VAL, const int16_t  MIN, const int16_t  MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline int8_t   range_bind(const int8_t   VAL, const int8_t   MIN, const int8_t   MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };

/**
* Endian conversion wrappers.
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



/*******************************************************************************
* Interfaces and callback definitions in use throughout this library.
*******************************************************************************/
/*
* Shorthand for a pointer to a "void fxn(void)"
* No contracts. It is just used to make code read better.
*/
typedef void  (*FxnPointer)();

/*
* Callbacks for drivers that provide extra GPI pins.
*/
typedef void (*PinCallback)(uint8_t pin, uint8_t level);


/**
* An interface class for accepting a buffer.
*/
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
    virtual int8_t provideBuffer(StringBuilder*) =0;
};


/**
* An interface class for accepting a scalar value, with units and error.
*/
class ScalarAccepter {
  public:
    /**
    * A class would implement this class to provide an interface for accepting a
    *   value from outside tagged with a real-world unit and error bars.
    *
    * @return -1 to reject value, 0 to accept.
    */
    virtual int8_t provideScalar(SIUnit, double value, double error) =0;
};


/**
* For simplicity, many classes are writen in such a way as to benefit (or
*   require) periodic polling for them to update their own states. The more
*   complicated the class, the more likely it is to require this.
* To keep that complexity bounded, it is advised that such classes implement the
*   PollableObj interface to allow themselves to be recomposed into higher-level
*   logic without complicated APIs or "special treatment".
*/
enum class PollingResult : int8_t {
  /*
  Code       Value   Semantics
  ----------------------------------------------------------------------------*/
  ERROR     = -1,    // Polling resulted in an internal class problem.
  NO_ACTION =  0,    // No action. No error.
  ACTION    =  1,    // Polling resulted in an evolution of class state.
  REPOLL    =  2     // Repoll immediately, subject to caller's descretion.
};

class PollableObj {
  public:
    virtual PollingResult poll() =0;
};


#endif // __CPPPOTPOURRI_H__
