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
  success (with minor adjustments) under both IAR and MSVSC.
*/

#if !defined(C3P_COMPILERS_META_HEADER)
#define C3P_COMPILERS_META_HEADER

#include "Meta/Rationalizer.h"     // Include build options checking.

/*******************************************************************************
* Compiler-specific glue
*******************************************************************************/

/* Endian conversion wrappers */
inline uint16_t endianSwap16(uint16_t x) {   return __builtin_bswap16(x);    };
inline uint32_t endianSwap32(uint32_t x) {   return __builtin_bswap32(x);    };
inline uint64_t endianSwap64(uint64_t x) {   return __builtin_bswap64(x);    };


/*******************************************************************************
* Sections
*******************************************************************************/

//TODO: #define RAMFUNC

//TODO: #define ISR

//TODO: #define underpinnings for hardware-assisted semaphores.



#endif  // C3P_COMPILERS_META_HEADER
