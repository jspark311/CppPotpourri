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

TODO: It would be nice to be able to wrap malloc here...

TODO: Until the build system migration to kconfig is complete,
  this is where we will (try to) itemize all the build options to
  make the future migration easier.

  __BUILD_HAS_THREADS
    __BUILD_HAS_PTHREADS
    __BUILD_HAS_FREERTOS
    __BUILD_HAS_ZEPHYR
    __BUILD_HAS_RIOTOS
    __BUILD_HAS_CONTIKI


  __BUILD_HAS_BASE64

  __HAS_CRYPT_WRAPPER
    __BUILD_HAS_ASYMMETRIC
    __BUILD_HAS_SYMMETRIC
    __BUILD_HAS_DIGEST

  __HAS_IDENT_CERT

*/

#ifndef BUILD_RATIONALIZER_METAHEADER
#define BUILD_RATIONALIZER_METAHEADER

/* To override the defaults, supply this at build time. */
#if defined(MANUVR_CONF_FILE)
  #include MANUVR_CONF_FILE
#else
  #include "ManuvrConf.h"
#endif

/* Cryptographic stuff... */
#include <Platform/Cryptographic/CryptOptUnifier.h>

#ifndef __MANUVR_OPTION_RATIONALIZER_H__
#define __MANUVR_OPTION_RATIONALIZER_H__


// This is the string that identifies this Manuvrable to other Manuvrables.
//   In MHB's case, this value will select the mEngine.
#ifndef FIRMWARE_NAME
  #error You need to name the firmware by providing FIRMWARE_NAME.
#endif

// Who made the hardware?
#ifndef MANUFACTURER_NAME
  #define MANUFACTURER_NAME   "Manuvr"
#endif

// How many random numbers should be cached? Must be > 0.
#ifndef PLATFORM_RNG_CARRY_CAPACITY
  #define PLATFORM_RNG_CARRY_CAPACITY 32
#endif

// How large a preallocation buffer should we keep?
#ifndef EVENT_MANAGER_PREALLOC_COUNT
  #define EVENT_MANAGER_PREALLOC_COUNT 8
#endif

#ifndef MAXIMUM_SEQUENTIAL_SKIPS
  #define MAXIMUM_SEQUENTIAL_SKIPS 20
#endif

// Debug support requires Console.
// NOTE: If your Makefile passes the MANUVR_DEBUG option, this will be enabled regardless.
#if defined(MANUVR_DEBUG) && !defined(MANUVR_CONSOLE_SUPPORT)
  #define MANUVR_CONSOLE_SUPPORT
#endif

// Use the build system to set default logging levels for modules.
#if defined(MANUVR_CONSOLE_SUPPORT)
  #if defined(MANUVR_DEBUG)
    #define DEFAULT_CLASS_VERBOSITY    6
  #else
    #define DEFAULT_CLASS_VERBOSITY    4
  #endif
#else
  #define DEFAULT_CLASS_VERBOSITY      0
#endif


/*
* Threading models...
* Threading choice exists independently of platform.
*/
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
#elif defined(__MANUVR_RIOTOS)
  //#pragma message "Building with RIOT support."
  #define __BUILD_HAS_THREADS
#elif defined(__MANUVR_CONTIKI)
  //#pragma message "Building with no threading support."
  #define MANUVR_PLATFORM_TIMER_PERIOD_MS 1
#endif

#if defined(__BUILD_HAS_THREADS)
  // If we have threads, set the latency of the idel state. This is a choice
  //   between power usage and event response latency.
  #ifndef CONFIG_C3P_IDLE_PERIOD_MS
    #define CONFIG_C3P_IDLE_PERIOD_MS 20
  #endif
#endif


// What is the granularity of our scheduler?
#ifndef MANUVR_PLATFORM_TIMER_PERIOD_MS
  #define MANUVR_PLATFORM_TIMER_PERIOD_MS 10
#endif

/* Encodings... */
// CBOR
#if defined(CONFIG_C3P_CBOR)
  #define __BUILD_HAS_CBOR
#endif

// JSON
#if defined(MANUVR_JSON)
  #define __BUILD_HAS_JSON
  // We do this to prevent the Makefile having to do it....
  #define HAVE_CONFIG_H
#endif

// Base64. The wrapper can only route a single implementation.
//   so we case-off exclusively, in order of preference. If we
//   are already sitting on cryptographic support, check those
//   libraries first.
#if defined(WITH_MBEDTLS) && defined(MBEDTLS_BASE64_C)
  #define __BUILD_HAS_BASE64
#elif defined (WITH_OPENSSL)

#endif



/* BufferPipe support... */

/* IP support... */
// LWIP
// Arduino
// *nix

/*
  Now that we've done all that work, we can provide some flags to the build
    system to give high-value assurances at compile-time such as....
    * Presence of cryptographic hardware.
    * Signature for firmware integrity.
    * Reliable feature maps at runtime.
    * Allowing modules written against Manuvr to easilly leverage supported
        capabilities, and providing an easy means to write fall-back code if
        a given option is not present at compile-time.
*/

/*
 Notions of Identity:
 If we have cryptographic wrappers, we can base these choices from those flags.
*/
#if defined(__BUILD_HAS_ASYMMETRIC)
  #define __HAS_IDENT_CERT        // We support X509 identity.
#endif   // __HAS_CRYPT_WRAPPER


/*
 PMICs:
*/
#if defined(CONFIG_C3P_BQ24155) || \
    defined(CONFIG_C3P_LTC294X)
  #define __HAS_BATTERY        // Safe to assume we have a battery.
#endif

/*
 Drivers that pull in GPIO message codes...
*/
#if defined(CONFIG_C3P_SX8634) || \
    defined(CONFIG_C3P_GPIO_ER)
  #ifndef __HAS_GPIO_MESSAGES
    #define __HAS_GPIO_MESSAGES
  #endif    // __HAS_GPIO_MESSAGES
#endif

/*
 Drivers that pull in button message codes...
*/
#if defined(CONFIG_C3P_SX8634)
  #ifndef __HAS_USER_INPUT_MESSAGES
    #define __HAS_USER_INPUT_MESSAGES
  #endif    // __HAS_USER_INPUT_MESSAGES
#endif


/* Framebuffer and display driver support... */
#if defined(CONFIG_C3P_SSD1331)
  #ifndef CONFIG_C3P_IMG_SUPPORT
    #define CONFIG_C3P_IMG_SUPPORT  // Framebuffers need this class.
  #endif    // CONFIG_C3P_IMG_SUPPORT
#endif   // CONFIG_C3P_SSD1331


#endif // BUILD_RATIONALIZER_METAHEADER
