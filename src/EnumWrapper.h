/**
File:   EnumWrapper.h
Author: J. Ian Lindsay
Date:   2023.02.11

Meditation: What is the difference (if any) between semantics and syntax?

This template is intended to extend the sematic assurances provided at
  compile-time (via enums) to the syntax of what actually happens in certain
  classes that use them.

This template is being undertaken as an experiment. Mostly for the benefit of
  templating finite state-machines.

NOTE: Use of this mechanism does NOT constrain the specific values of the enums,
  as assigned in their proper definitions. The list is null-terminated, and
  indicies are unimportant.


TODO: I think I can now use the compiler to force the definition of an INVALID
  enum for whatever enum is being represented by the template. This makes sense
  for cases where the enum space doesn't fully cover the space of the underlying
  type (IE, most enums). And it would also help with lookup-by-string.
*/

#include <stdlib.h>
#include "StringBuilder.h"

#ifndef __C3P_ENUM_WRAPPER
#define __C3P_ENUM_WRAPPER

#define ENUM_WRAPPER_FLAG_CATCHALL  0x01   // If set, the associated enum will be a catch-all.

/*
* A wrapper object to tie enums to their string representations.
* This is to save us the obnoxious task of re-writing this support code for all
*   exposed enums in the program. It should be entirely const so that builds can
*   confidently isolate it to flash.
*/
template <class T> class EnumDef {
  public:
    const T VAL;
    const uint8_t FLAGS;
    const char* const STR;

    EnumDef(const T EVAL, const char* const STR_REP, const uint32_t EFLAGS) : VAL(EVAL), FLAGS(EFLAGS), STR(STR_REP) {};
    EnumDef(const T EVAL, const char* const STR_REP) : VAL(EVAL), FLAGS(0), STR(STR_REP) {};
    EnumDef(const T EVAL) : VAL(EVAL), FLAGS(0), STR("<NO STR>") {};
};


/*
* A list of the above-defined objects. Like EnumDef<t>, instances of this template
*   ought to be able to be easilly relegated to flash.
*/
template <class T> class EnumDefList {
  public:
    EnumDefList(const EnumDef<T>* const DEFS, const uint32_t DEF_COUNT) : LIST_PTR(DEFS), COUNT(DEF_COUNT) {};

    /**
    * Is the supplied argument in the enum list? We have to ask, because the
    *   argument might be (often is) a cast integer from outside of the
    *   compiler's scope of semantic assurances.
    * Used to sanitize enums.
    *
    * @param The enum value to test.
    * @return true if so. False otherwise.
    */
    const bool enumValid(const T ENUM_TO_TEST) const {
      for (uint32_t i = 0; i < COUNT; i++) {
        if ((LIST_PTR + i)->VAL == ENUM_TO_TEST) return true;
      }
      return false;
    };


    /**
    * Used to print strings representing enums.
    * NOTE: Does not respect catch-all logic.
    * TODO: Should it stay that way?
    *
    * @param The enum value to test.
    * @return Always returns a valid string.
    */
    const char* const enumStr(const T ENUM) const {  // Also: const
      for (uint32_t i = 0; i < COUNT; i++) {
        if ((LIST_PTR + i)->VAL == ENUM) return (LIST_PTR + i)->STR;
      }
      return "<NO ENUM>";
    };


    /*
    * Find the enum represented by the given string.
    * If the entire enum set is exhausted without finding the search string but
    *   there an enum marked as a catch-all, the last catch-all defined in the
    *   enum list will be returned. In such a case, the `found` parameter will
    *   still be set to 0 to allow the caller to maintain semantic hygiene.
    */
    const T getEnumByStr(const char* NEEDLE, int8_t* found = nullptr) const {
      uint32_t catchall_idx = 0xFFFFFFFF;
      for (uint32_t i = 0; i < COUNT; i++) {
        if (0 == StringBuilder::strcasecmp(NEEDLE, (LIST_PTR + i)->STR)) {
          if (nullptr != found) {  *found = 1;  }
          return (LIST_PTR + i)->VAL;
        }
        else if ((LIST_PTR + i)->FLAGS & ENUM_WRAPPER_FLAG_CATCHALL) {
          catchall_idx = i;
        }
      }

      if (nullptr != found) {  *found = 0;  }

      if (0xFFFFFFFF != catchall_idx) {
        return (LIST_PTR + catchall_idx)->VAL;
      }
      else {
        // We can't do anything but return a defined enum. So we return the
        //   wrong one on purpose, and hope that the caller is observing the
        //   return-via-parameter.
        return (LIST_PTR + (COUNT-1))->VAL;
      }
    };


  private:
    const EnumDef<T>* const LIST_PTR;
    const uint32_t COUNT;
};

#endif  // __C3P_ENUM_WRAPPER
