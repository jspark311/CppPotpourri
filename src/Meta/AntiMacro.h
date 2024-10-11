/*
File:   AntiMacro.h
Author: J. Ian Lindsay
Date:   2024.10.08

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


Using macros for these purposes can generate some hilarious bugs. Using
  inlines gives us the benefit of strict type-checking at compile time, and
  carries no costs.

NOTE: It is not an oversight that cross-type functions are not given. IE, there
  is nothing like strict_max(uint8_t, int16_t). Appropriate casting is being
  forced onto the calling code on purpose.

TODO: Either the compiler or the platform might provide better implementations
  for these things. Once discovered and implemented, these function defs may be
  promoted to either Compilers.h or Intrinsics.h, as appropriate.
*/

#ifndef __C3P_ANTIMACRO_H__
#define __C3P_ANTIMACRO_H__

#include "Compilers.h"

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

/*
* Given a value and a range, gives the saturated result.
* TODO: If the compiler gives this, it will likely be ASM optimized using no
*   branches. Until I care enough, I'll use a double-nested ternary. Suffer.
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


/*******************************************************************************
* Numeric approximation functions
* Float and Double are not Real. They are discrete representations of an
*   infinitely-divisible range in two dimentions (left and right of the decimal
*   point). General arithmetic using them is thus very tricky. These functions
*   are meant to ease some of the burden of treating them with due care, and
*   were informed by Christer Ericson's most excellent work on this point:
*     https://realtimecollisiondetection.net/blog/?p=89
*
* TODO: These feel like they might be in the wrong place. Either of the
*   following two conditions would prove this true:
*   1) The use of std::math functions proves to be non-portable, in which case
*      they should be moved to either Compilers.h or Intrinsics.h.
*   2) The burdern of inlining them is unacceptable, in which case, they should
*      be moved out of the Meta directory entirely.
*******************************************************************************/
// Using Ericson's terminology, these functions assume that absolute error will
//   be the same value as reletive error.
inline bool nearly_equal(const float A, const float B, const float PRECISION) {
  return (fabsf(A - B) <= (PRECISION * fmaxf(1.0f, fmaxf(fabsf(A), fabsf(B)))));
};

inline bool nearly_equal(const double A, const double B, const double PRECISION) {
  return (fabs(A - B) <= (PRECISION * fmax(1.0d, fmax(fabs(A), fabs(B)))));
};


#endif // __C3P_ANTIMACRO_H__
