/*
File:   Compilers.h
Author: J. Ian Lindsay
Date:   2023.09.01

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


This file is intended to host build logic that pertains to specific compilers.
  This is the appropriate place to abstract pragmas, sections, and specific
  quirks for the sake of keeping the buildable code free from turmoil over such
  vagaries.

The library was originally written under GCC, but others have reported broad
  success (with minor adjustments) under both IAR and MSVC.
*/

#if !defined(C3P_COMPILERS_META_HEADER)
#define C3P_COMPILERS_META_HEADER

#include "Meta/Rationalizer.h"     // Include build options checking.

/*******************************************************************************
* Inclusions from the standard library that typically come packaged with the
*   toolchain. To the extent that these are not uniform across compilers and
*   environments, they should not be globally included from this file.
*******************************************************************************/
#include <inttypes.h>
#include <stdint.h>
#include <math.h>
#include <float.h>


/*******************************************************************************
* GCC
*******************************************************************************/

// TODO: #define underpinnings for hardware-assisted semaphores.
// TODO: __COUNTER__
// TODO: __INCLUDE_LEVEL__
#if defined(_C3P_COMPILER_IS_GCC)
  // The name of the compiler, as far as C3P is concerned.
  #define C3P_COMPILER_NAME ("gcc v" __GNUC__ "." __GNUC_MINOR__ "." __GNUC_PATCHLEVEL__)

  /*
  * Diagnostics, versioning, and errata handling.
  * NOTE: Intentionally left empty, for clarity of usage intent.
  */

  /*
  * Function Sections
  * Used to tag functions that should be placed in text sections that are fast,
  *   high-availability, secure, etc (if such sections are defined).
  * Ultimately, the observance (or not) of these attributes is up to the
  *   top-level linker. But even without being able to enforce the notion, it
  *   is useful for documentation of functions that the author has decided
  *   should be preferentially located in a specificly-constrained section.
  */
  #ifndef FAST_FUNC
    // Some functions are timing-critical, and should be preferentially located
    //   in a section with low execution latency.
    // NOTE: Speed is a concern orthogonal to that of ISR_FUNC.
    // NOTE: Section placement is a distinct attribute from optimizer level.
    #define FAST_FUNC __attribute__((section(".text.fastfunc")))
  #endif
  #ifndef ISR_FUNC
    // Functions intended to be run as interrupt service routines might need to
    //   be located in a section that isn't paged (is always avaialbe).
    // NOTE: Availability is a concern orthogonal to that of FASTFUNC.
    #define ISR_FUNC __attribute__((section(".text.isrfunc")))
  #endif
  #ifndef SECURE_FUNC
    // Some code might prefer to be run inside of memory that has some
    //   security assurances.
    #define SECURE_FUNC __attribute__((section(".text.secfunc")))
  #endif
  #ifndef WEAK_FUNC
    // Weak reference is tagged for readability.
    #define WEAK_FUNC __attribute__ ((weak))
  #endif


  /*
  * Optimizer attributes
  * https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html
  */
  #ifndef OPTIMIZE_DEBUG
    #define OPTIMIZE_DEBUG  __attribute__(( optimize ("Og") ))
  #endif
  #ifndef OPTIMIZE_SIZE
    #define OPTIMIZE_SIZE   __attribute__(( optimize ("Os") ))
  #endif
  #ifndef OPTIMIZE_SPEED
    #define OPTIMIZE_SPEED  __attribute__(( optimize ("O2") ))
  #endif
  #ifndef OPTIMIZE_FAST
    #define OPTIMIZE_FAST   __attribute__(( optimize ("O3") ))
  #endif
  #ifndef OPTIMIZE_HOT
    #define OPTIMIZE_HOT    __attribute__(( hot ))
  #endif
  #ifndef OPTIMIZE_COLD
    #define OPTIMIZE_COLD   __attribute__(( cold ))
  #endif

  /* Endian conversion wrappers */
  inline uint16_t endianSwap16(uint16_t x) {   return __builtin_bswap16(x);   };
  inline uint32_t endianSwap32(uint32_t x) {   return __builtin_bswap32(x);   };
  inline uint64_t endianSwap64(uint64_t x) {   return __builtin_bswap64(x);   };



/*******************************************************************************
* IAR
*******************************************************************************/
#elif defined(_C3P_COMPILER_IS_IAR)



/*******************************************************************************
* MSVC
*******************************************************************************/
#elif defined(_C3P_COMPILER_IS_MSVC)



#endif  // C3P_COMPILERS
#endif  // C3P_COMPILERS_META_HEADER
