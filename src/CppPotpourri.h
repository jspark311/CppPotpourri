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

#ifndef __CPPPOTPOURRI_H__
#define __CPPPOTPOURRI_H__

#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include "Meta/Compilers.h"        // Include compiler support.
#include "EnumeratedTypeCodes.h"

class StringBuilder;  // Forward declaration for buffer interchange.


/*******************************************************************************
* Anti-macro functions
* Using macros for these purposes can generate some hilarious bugs. Using
*   inlines gives us the benefit of strict type-checking at compile time, and
*   carries no costs.
*******************************************************************************/

/* Return the maximum of two values. */
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

/* Return the minimum of two values. */
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

/* Type-strict value swap. */
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

/* Given two values (a and b), effectively returns abs(a-b). */
inline double   strict_abs_delta(double   a, double   b) {   return (a > b) ? (a - b) : (b - a);   };
inline float    strict_abs_delta(float    a, float    b) {   return (a > b) ? (a - b) : (b - a);   };
inline uint64_t strict_abs_delta(uint64_t a, uint64_t b) {   return (a > b) ? (a - b) : (b - a);   };
inline uint32_t strict_abs_delta(uint32_t a, uint32_t b) {   return (a > b) ? (a - b) : (b - a);   };
inline uint16_t strict_abs_delta(uint16_t a, uint16_t b) {   return (a > b) ? (a - b) : (b - a);   };
inline uint8_t  strict_abs_delta(uint8_t  a, uint8_t  b) {   return (a > b) ? (a - b) : (b - a);   };
inline int64_t  strict_abs_delta(int64_t  a, int64_t  b) {   return (a > b) ? (a - b) : (b - a);   };
inline int32_t  strict_abs_delta(int32_t  a, int32_t  b) {   return (a > b) ? (a - b) : (b - a);   };
inline int16_t  strict_abs_delta(int16_t  a, int16_t  b) {   return (a > b) ? (a - b) : (b - a);   };
inline int8_t   strict_abs_delta(int8_t   a, int8_t   b) {   return (a > b) ? (a - b) : (b - a);   };

/*
* Given a value and a range, gives the saturated result.
* TODO: If the compiler gives this, it will likely be ASM optimized using no
*   branches. Until I care enough, I'll use a double-nested ternary. Suffer.
*   Promote to either Compilers.h or Intrinsics.h, as appropriate.
*/
inline double   strict_range_bind(const double   VAL, const double   MIN, const double   MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline float    strict_range_bind(const float    VAL, const float    MIN, const float    MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline uint64_t strict_range_bind(const uint64_t VAL, const uint64_t MIN, const uint64_t MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline uint32_t strict_range_bind(const uint32_t VAL, const uint32_t MIN, const uint32_t MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline uint16_t strict_range_bind(const uint16_t VAL, const uint16_t MIN, const uint16_t MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline uint8_t  strict_range_bind(const uint8_t  VAL, const uint8_t  MIN, const uint8_t  MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline int64_t  strict_range_bind(const int64_t  VAL, const int64_t  MIN, const int64_t  MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline int32_t  strict_range_bind(const int32_t  VAL, const int32_t  MIN, const int32_t  MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline int16_t  strict_range_bind(const int16_t  VAL, const int16_t  MIN, const int16_t  MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };
inline int8_t   strict_range_bind(const int8_t   VAL, const int8_t   MIN, const int8_t   MAX) {   return strict_min(strict_max(VAL, MIN), MAX);   };

/**
* Given two values (NOW and THEN), returns the displacement of NOW from THEN.
* This function only makes sense if NOW and THEN are time-like. That is, if they
*   are taken to be locations on an unbounded half-dimensional finite number
*   line (such as an analog clockface). Although this function is most-commonly
*   used with time values, it is equally applicable to any displacement problem
*   that is time-like.
*
* If the second parameter is smaller than the first, a wrap will be assumed to
*   have happened between the mark and the comparison, and the return value will
*   be adjusted accordingly. No type-shifting is required, and a value of 0 will
*   be returned if the same number is given for both NOW and THEN.
*
* NOTE: This function generated controversy amongst a group of engineers until
*   it was noted that integer over/underflow is undefined behavior in C/C++. The
*   reasons for that are beyond scope here, but suffice it to say that we don't
*   want the compiler to take optimization options that would be available under
*   such conditions. Similarly to why we don't use a macro for this purpose.
* By using an extra conditional, two extra additions, and algebraically
*   superfluous parentheticals, we we are trying to disallow integer wrap, and
*   thus prevent the compiler from optimizing away our intention: to understand
*   the (very well-defined) wrapping behavior of hardware registers without that
*   behavior being defined by the language itself.
*
* @param NOW is the present position on the clockface.
* @param THEN is the position on the clockface against which we will compare.
* @return the positive-going displacement of NOW from THEN
*/
inline uint64_t delta_assume_wrap(const uint64_t NOW, const uint64_t THEN) {   return ((NOW >= THEN) ? (NOW - THEN) : (1 + (0xFFFFFFFFFFFFFFFF - (THEN - NOW))));  };
inline uint32_t delta_assume_wrap(const uint32_t NOW, const uint32_t THEN) {   return ((NOW >= THEN) ? (NOW - THEN) : (1 + (0xFFFFFFFF - (THEN - NOW))));          };
inline uint16_t delta_assume_wrap(const uint16_t NOW, const uint16_t THEN) {   return ((NOW >= THEN) ? (NOW - THEN) : (1 + (0xFFFF - (THEN - NOW))));              };
inline uint8_t  delta_assume_wrap(const uint8_t  NOW, const uint8_t  THEN) {   return ((NOW >= THEN) ? (NOW - THEN) : (1 + (0xFF - (THEN - NOW))));                };



/*******************************************************************************
* High-level string operations.
* TODO: These feel like they are in the wrong place.
*******************************************************************************/
void timestampToString(StringBuilder*, uint64_t);
uint64_t stringToTimestamp(const char*);
int randomArt(uint8_t* dgst_raw, unsigned int dgst_raw_len, const char* title, StringBuilder*);



/*******************************************************************************
* Interfaces and callback definitions in use throughout this library.
*******************************************************************************/
/* Shorthand for a pointer to a "void fxn(void)" */
typedef void  (*FxnPointer)();

/* Callbacks for drivers that provide extra GPI pins. */
typedef void (*PinCallback)(uint8_t pin, uint8_t level);



/*******************************************************************************
* For simplicity, many classes are writen in such a way as to benefit (or
*   require) periodic polling for them to update their own states. The more
*   complicated the class, the more likely it is to require this.
* To keep that complexity bounded, it is advised that such classes implement the
*   PollableObj interface to allow themselves to be recomposed into higher-level
*   logic without complicated APIs or "special treatment".
*******************************************************************************/
enum class PollingResult : int8_t {
  /*
  Code       Value   Semantics
  ----------------------------------------------------------------------------*/
  ERROR     = -1,    // Polling resulted in an internal class problem.
  NO_ACTION =  0,    // No action. No error.
  ACTION    =  1,    // Polling resulted in an evolution of class state.
  REPOLL    =  2     // Repoll immediately, subject to caller's descretion.
};

/**
* An interface class for simple state polling.
*/
class PollableObj {
  public:
    virtual PollingResult poll() =0;
};


#endif // __CPPPOTPOURRI_H__
