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


This is a special-purpose text-handling class. It accepts log and renders it
  into a string suitable for being fed to a serial port, socket, file, etc.
  Using it is completely optional, and is only useful for sophisticated output
  of logs under conditions that don't automatically provide for it. This is the
  case for virtualy all bare-metal embedded programs.
Espressif and Linux environments typically have better options for this, but it
  could still be used in those contexts to isolate C3P logs into a specific UART
  or filestream.

Platforms that have built-in faculties for logging should probably just
  implement the c3p_log() functions in such a way as to wrap their existing
  APIs. See associated notes in AbstractPlatform.h.
*/

#include "Meta/Rationalizer.h"
#include "StringBuilder.h"
#include "Pipes/BufferAccepter/BufferAccepter.h"

#ifndef __C3P_LOGGER_H
#define __C3P_LOGGER_H

// This is the maximum length of a log tag. Tags
//   exceeding this length will be truncated.
#ifndef LOG_TAG_MAX_LEN
  #define LOG_TAG_MAX_LEN             24
#endif

// These flags only apply to the optional C3PLogger class.
#define LOGGER_FLAG_PRINT_LEVEL   0x01  // Should output include the severity?
#define LOGGER_FLAG_PRINT_TIME    0x02  // Should output include the timestamp?
#define LOGGER_FLAG_PRINT_TAG     0x04  // Should output include the tag?

// Forward declaration of string conversion fxn.
const char* c3p_log_severity_string(const uint8_t severity);

/*
* Optional class for emulating a logging faculty on platforms that
*   don't otherwise support it.
*/
class C3PLogger {
  public:
    C3PLogger(const uint8_t F = 0, BufferAccepter* SINK = nullptr)
      : _sink(SINK), _tag_ident(1), _flags(F), _verb_limit(LOG_LEV_DEBUG) {};
    ~C3PLogger() {};

    // On builds that use this class, this function will be called by c3p_log().
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
    StringBuilder   _log;          // Local buffer.
    BufferAccepter* _sink;         // Optional sink for formatted text.
    uint8_t         _tag_ident;    // Padding scalar to keep output aligned.
    uint8_t         _flags;        // Options that control output.
    uint8_t         _verb_limit;   // Used as a global limit.

    void _store_or_forward(StringBuilder*);

    inline bool _class_flag(uint8_t f) {   return ((_flags & f) == f);   };
    inline void _class_clear_flag(uint8_t _flag) {  _flags &= ~_flag;    };
    inline void _class_set_flag(uint8_t _flag) {    _flags |= _flag;     };
    inline void _class_set_flag(bool nu, uint8_t _flag) {
      _flags = (nu) ? (_flags | _flag) : (_flags & ~_flag);
    };
};

#endif  // __C3P_LOGGER_H
