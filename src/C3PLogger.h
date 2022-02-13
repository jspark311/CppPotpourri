/**
File:   C3PLogger.h
Author: J. Ian Lindsay
Date:   2021.10.16

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


Personal note:
  I've taught many people over the years. When a guy who is software-enabled asks
  me how to get started in hardware, I tell him to pick up a cheap Arduino and
  implement a push button from scratch. And it sounds pointless and easy, until
  you try to do it for the first time. Because buttons are surprisingly
  difficult. Contact bounce, state-tracking, long-press or not, distinguishing
  events as spurious or intentionally repeated, etc...

  When a someone who is hardware-enabled asks me how to get started in software,
  I might start telling them to write a reusable system logger from scratch.
  Because it sounds pointless and easy, until you try to do it for the fifth
  time. At which point, the smartest of us should have given up trying. So
  before I start in on my 6th attempt, I'm going to lay out my general values
  and concerns before I begin work.

  The dangers here are complexity and weight. In that order. The logger has to
  be present and operable under all conditions where log might be generated,
  and it can't rely on any other data structures in CppPotpourri which might
  generate logs.

  The logging system in ManuvrOS had a _very_ basic API (too basic), but far too
  much implementation complexity (logging was the province of the Kernel). This
  last choice caused a tremendous maintenance burden in the Kernel class, as it
  was required to be included in every class that might potentially generate
  logs. Don't do that sort of thing again. If anything, this class might should
  be a singleton of its own, apart from even platform. That said...

  Logging is fundamentally a platform choice, since platform support is
  ultimately required to print a character to a screen, file, socket, whatever.
  So this class should remain an interface (at minimum), or a pure-virtual (in
  the heaviest case), with the final implementation being given in
  ManuvrPlatform, with the rest of the platform-specific implementations of
  AbstractPlatform, I2CAdapter, et al. At that point, the platform can make
  choices about which modes of output/caching/policy will be available to the
  program.

  The API to the Logger ought to support log severity, source tags, and
  build-time definitions for which levels of logging should be included in the
  binary. As before, we adopt the SYSLOG severity conventions. Because those
  work really well.
                                           ---J. Ian Lindsay 2021.10.16 20:59:15


The Logger abstraction strategy is running on ESP-IDF (platform wrapper case).
  The design values above were taken as canon. It looks like it will work for
  all cases. Time will tell. Considering this task complete.
                                           ---J. Ian Lindsay 2022.02.13 02:58:37
*/

#include "StringBuilder.h"

#ifndef __CPPPOTPOURRI_LOGGER_H__
#define __CPPPOTPOURRI_LOGGER_H__

// This is the maximum length of a log tag. Tags
//   exceeding this length will be truncated.
#define LOG_TAG_MAX_LEN             24

// These flags only apply to the optional C3PLogger class.
#define LOGGER_FLAG_PRINT_LEVEL   0x01  // Should output include the severity?
#define LOGGER_FLAG_PRINT_TIME    0x02  // Should output include the timestamp?
#define LOGGER_FLAG_PRINT_TAG     0x04  // Should output include the tag?

/*
* This is a special-purpose text-handling class. It accepts log and renders it
*   into a string suitable for being fed to a serial port, socket, file, etc.
* This class is completely optional, and is only useful for sophisticated output
*   under conditions that don't automatically provide for it (such as stdio, or
*   a serial port).
* Platforms that have built-in faculties for logging should not use this class,
*   but instead implement the c3p_log() functions in such a way as to wrap their
*   existing APIs.
*/
class C3PLogger {
  public:
    C3PLogger(const uint8_t F = 0) : _flags(F) {};
    ~C3PLogger() {};

    // On builds that use this class, these two functions will be called by
    //   their c3p_log() counterparts.
    int8_t print(uint8_t severity, const char* tag, const char* fmt, ...);
    int8_t print(uint8_t severity, const char* tag, StringBuilder*);

    void fetchLog(StringBuilder*);

    inline void    setSink(BufferAccepter* x) {    _sink = x;      };
    inline void    verbosity(uint8_t x) {    _verb_limit = x;      };
    inline uint8_t verbosity() {             return _verb_limit;   };

    inline bool    printSeverity() {         return _class_flag(LOGGER_FLAG_PRINT_LEVEL);    };
    inline bool    printTime() {             return _class_flag(LOGGER_FLAG_PRINT_TIME);     };
    inline bool    printTag() {              return _class_flag(LOGGER_FLAG_PRINT_TAG);      };
    inline void    printSeverity(bool x) {   _class_set_flag(LOGGER_FLAG_PRINT_LEVEL, x);    };
    inline void    printTime(bool x) {       _class_set_flag(LOGGER_FLAG_PRINT_TIME, x);     };
    inline void    printTag(bool x) {        _class_set_flag(LOGGER_FLAG_PRINT_TAG, x);      };


  private:
    StringBuilder  _log;
    uint8_t        _tag_ident  = 0;
    BufferAccepter* _sink      = nullptr;
    uint8_t        _flags      = 0;
    uint8_t        _verb_limit = LOG_LEV_DEBUG;   // Used as a global limit.

    void _store_or_forward(StringBuilder*);

    inline bool _class_flag(uint8_t f) {   return ((_flags & f) == f);   };
    inline void _class_clear_flag(uint8_t _flag) {  _flags &= ~_flag;    };
    inline void _class_set_flag(uint8_t _flag) {    _flags |= _flag;     };
    inline void _class_set_flag(bool nu, uint8_t _flag) {
      _flags = (nu) ? (_flags | _flag) : (_flags & ~_flag);
    };
};

#endif // __CPPPOTPOURRI_LOGGER_H__
