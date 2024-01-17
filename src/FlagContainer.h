/*
File:   FlagContainer.h
Author: J. Ian Lindsay
Date:   2021.05.31

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


This set of classes aggregates many boolean flags (defined elsewhere) into a
  single class member that composes more cleanly, versus having this same code
  repeated in all classes that need flags.
*/

#include <inttypes.h>
#include <stdint.h>

#ifndef __FLAG_CONTAINERS_H__
#define __FLAG_CONTAINERS_H__

/* Takes a single byte of memory. */
class FlagContainer8 {
  public:
    uint8_t raw;   // The raw value of all flags.
    FlagContainer8(const uint8_t RESET_VALUE = 0) : raw(RESET_VALUE) {};

    /* Flag manipulation inlines */
    inline bool all_set(uint8_t _flag) {  return (_flag == (raw & _flag)); };
    inline bool value(uint8_t _flag) {    return (raw & _flag); };
    inline void flip(uint8_t _flag) {     raw ^= _flag;         };
    inline void clear(uint8_t _flag) {    raw &= ~_flag;        };
    inline void set(uint8_t _flag) {      raw |= _flag;         };
    inline void set(uint8_t _flag, bool x) {
      if (x)  raw |= _flag;
      else    raw &= ~_flag;
    };
};


/* Takes two bytes of memory. */
class FlagContainer16 {
  public:
    uint16_t raw;   // The raw value of all flags.
    FlagContainer16(const uint16_t RESET_VALUE = 0) : raw(RESET_VALUE) {};

    /* Flag manipulation inlines */
    inline bool all_set(uint16_t _flag) {  return (_flag == (raw & _flag)); };
    inline bool value(uint16_t _flag) {    return (raw & _flag); };
    inline void flip(uint16_t _flag) {     raw ^= _flag;         };
    inline void clear(uint16_t _flag) {    raw &= ~_flag;        };
    inline void set(uint16_t _flag) {      raw |= _flag;         };
    inline void set(uint16_t _flag, bool x) {
      if (x)  raw |= _flag;
      else    raw &= ~_flag;
    };
};


/* Takes four bytes of memory. */
class FlagContainer32 {
  public:
    uint32_t raw;   // The raw value of all flags.
    FlagContainer32(const uint32_t RESET_VALUE = 0) : raw(RESET_VALUE) {};

    /* Flag manipulation inlines */
    inline bool all_set(uint32_t _flag) {  return (_flag == (raw & _flag)); };
    inline bool value(uint32_t _flag) {    return (raw & _flag); };
    inline void flip(uint32_t _flag) {     raw ^= _flag;         };
    inline void clear(uint32_t _flag) {    raw &= ~_flag;        };
    inline void set(uint32_t _flag) {      raw |= _flag;         };
    inline void set(uint32_t _flag, bool x) {
      if (x)  raw |= _flag;
      else    raw &= ~_flag;
    };
};


/* Takes eight bytes of memory. */
class FlagContainer64 {
  public:
    uint64_t raw;   // The raw value of all flags.
    FlagContainer64(const uint64_t RESET_VALUE = 0) : raw(RESET_VALUE) {};

    /* Flag manipulation inlines */
    inline bool all_set(uint64_t _flag) {  return (_flag == (raw & _flag)); };
    inline bool value(uint64_t _flag) {    return (raw & _flag); };
    inline void flip(uint64_t _flag) {     raw ^= _flag;         };
    inline void clear(uint64_t _flag) {    raw &= ~_flag;        };
    inline void set(uint64_t _flag) {      raw |= _flag;         };
    inline void set(uint64_t _flag, bool x) {
      if (x)  raw |= _flag;
      else    raw &= ~_flag;
    };
};

#endif    // __FLAG_CONTAINERS_H__
