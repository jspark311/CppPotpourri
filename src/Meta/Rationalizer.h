/*
File:   Rationalizer.h
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


This is a header file for error-checking combinations of options at compile-time
  and providing defines that isolate downstream code from potential mistakes or
  oversights in the options. This file should be included immediately after any
  build-global user configuration files.
The only inclusion in this file should be headers with similar tasks.


C3P wants the following capabilities, which will probably implicate this file.
--------------------------------------------------------------------------------
  TODO: It would be nice to be able to wrap malloc here...
  TODO: Signature for firmware integrity (with cryptographic support).
  TODO: Reliable, unified feature maps at runtime.
*/

#ifndef BUILD_RATIONALIZER_META_HEADER
#define BUILD_RATIONALIZER_META_HEADER

/* Some programs might prefer to configure C3P via header file. */
#if defined(C3P_CONF_FILE)
  #include C3P_CONF_FILE
#endif

/*******************************************************************************
* Cryptographic options and concerns are handled in their own file.
*******************************************************************************/
#include "../CryptoBurrito/CryptOptUnifier.h"

/*******************************************************************************
* Supported encoding for parser/packers.
* These tend to multiply build size costs, and most programs only need
*   one (if any). Because their call-chains are often opaque to the linker, we
*   require them to be enabled this way.
*******************************************************************************/
// CBOR
#if defined(CONFIG_C3P_CBOR)
  #define __BUILD_HAS_CBOR
#endif

// JSON
#if defined(CONFIG_C3P_JSON)
  #define __BUILD_HAS_JSON
#endif

// Base64, which we wrap if we are already sitting on cryptographic support and
//   is present. This prevents code replication, and the implementation used in
//   conjunction with cryptographic support is almost certainly harder. If no
//   B64 support was available elsewhere, and the program requests it, we will
//   lean on the local C3P implementation.
#if defined(CONFIG_C3P_BASE64)
  #define __BUILD_HAS_BASE64
  #if defined(CONFIG_C3P_WITH_MBEDTLS) && defined(MBEDTLS_BASE64_C)
    #define __BUILD_HAS_BASE64_VIA_MBEDTLS
  #elif defined (CONFIG_C3P_WITH_OPENSSL)
    //#define __BUILD_HAS_BASE64_VIA_OPENSSL
  #endif
#endif  // CONFIG_C3P_BASE64


/*******************************************************************************
* Supported type support for C3PValue.
* These options govern which high-level types can be handled by C3PValue. These
*   choices will impact support for parsing/packing specific types.
* These tend to multiply build size costs, and most programs don't need to parse
*   or pack all of the high-level types that they might support internally.
*   Because their call-chains are often opaque to the linker, we allow them to
*   be disabled this way, following the enablement of their respective types.
*******************************************************************************/
// TODO: Image
// TODO: EnumWrapper
// TODO:


/*******************************************************************************
* Assumptions about platform properties.
* NOTE: This abstraction strategy relies on the platform being built with
*   the same options as C3P.
*******************************************************************************/
// How many random numbers should be cached? Must be > 0.
#ifndef PLATFORM_RNG_CARRY_CAPACITY
  #define PLATFORM_RNG_CARRY_CAPACITY 32
#endif

// Thread models. Threading choice exists independently of platform.
#if defined(__MANUVR_LINUX) || defined(__MANUVR_APPLE)
  //#pragma message "Building with pthread support."
  #define __BUILD_HAS_THREADS
  #define __BUILD_HAS_PTHREADS
#elif defined(MANUVR_PF_FREERTOS) || defined(__MANUVR_ESP32)
  //#pragma message "Building with freeRTOS support."
  #define __BUILD_HAS_THREADS
  #define __BUILD_HAS_FREERTOS
#elif defined(__MANUVR_ZEPHYR)
  //#pragma message "Building with Zephyr support."
  #define __BUILD_HAS_THREADS
#elif defined(__MANUVR_YOCTO)
  //#pragma message "Building with Yocto support."
  // This is basically just linux for all we care.
  #define __BUILD_HAS_THREADS
  #define __BUILD_HAS_PTHREADS
#elif defined(__MANUVR_RIOTOS)
  //#pragma message "Building with RIOT support."
  // RIOT is a light-weight tickless multitasking environment.
  #define __BUILD_HAS_THREADS
#elif defined(__MANUVR_CONTIKI)
  //#pragma message "Building with no threading support."
  // Contiki is a threadless cooperative multitasking environment.
  #define MANUVR_PLATFORM_TIMER_PERIOD_MS 1
#endif

#if defined(__BUILD_HAS_THREADS)
  // If we have threads, set the latency of the idle state. This is a choice
  //   between power usage and event response latency. Any program built on
  //   a threading model needs to define this, or the default value of 10ms
  //   will be used.
  // Local modules are free to NOT use this value for any threads they create,
  //   but modules that specify thread idle thresholds too-tightly will drain
  //   power and CPU time faster than necessary. So modules that specify thier
  //   own threading idle times should still consult this value, and ensure that
  //   their own choices are greater than this number.
  // The default value of 20 is fairly easy to meet on $10 linux systems that
  //   are built carefully. But it might be too aggressive for a high-end MCU
  //   that is doing lots of work.
  #ifndef CONFIG_C3P_IDLE_PERIOD_MS
    #define CONFIG_C3P_IDLE_PERIOD_MS 20
  #endif
#endif


// What is the granularity of our system timer?
#ifndef CONFIG_C3P_TIMER_PERIOD_MS
  #define CONFIG_C3P_TIMER_PERIOD_MS 10
#endif



/*******************************************************************************
* Notions of Identity
* If we have cryptographic wrappers, we can base these choices from those flags.
*******************************************************************************/
#if defined(__BUILD_HAS_ASYMMETRIC)
  #define __HAS_IDENT_CERT        // We support X509 identity.
#endif   // __HAS_CRYPT_WRAPPER


/*******************************************************************************
* Feature map
*******************************************************************************/

// The presence of a PMIC implies we have a battery.
#if defined(CONFIG_C3P_BQ24155) || \
    defined(CONFIG_C3P_LTC294X)
  #define __HAS_BATTERY
#endif

// Drivers that pull in GPIO message codes.
#if defined(CONFIG_C3P_SX8634) || \
    defined(CONFIG_C3P_GPIO_ER)
  #ifndef __HAS_GPIO_MESSAGES
    #define __HAS_GPIO_MESSAGES
  #endif    // __HAS_GPIO_MESSAGES
#endif

// Drivers that pull in button message codes...
#if defined(CONFIG_C3P_SX8634)
  #ifndef __HAS_USER_INPUT_MESSAGES
    #define __HAS_USER_INPUT_MESSAGES
  #endif    // __HAS_USER_INPUT_MESSAGES
#endif

// Framebuffer and display driver support...
#if defined(CONFIG_C3P_SSD1331)
  #ifndef CONFIG_C3P_IMG_SUPPORT
    #define CONFIG_C3P_IMG_SUPPORT  // Framebuffers need this class.
  #endif    // CONFIG_C3P_IMG_SUPPORT
#endif   // CONFIG_C3P_SSD1331


#endif  // BUILD_RATIONALIZER_META_HEADER
