/**
File:   EnumWrapper.h
Author: J. Ian Lindsay
Date:   2023.02.11


This template is intended to extend the sematic assurances provided at
  compile-time (via enums) into assurances about actual run-time
  behavior of classes that use those enums in a way that can't be directly
  validated at build time. That is: an enum sanitizer.

Pervasive use of const is a requirement for not only RAM savings (as a minor
  side-effect), but primarilly for the purpose of extending the compiler's
  immutability assurances all the way into the flash chip during runtime (for
  embedded builds).

It does this mainly by providing a string anchor to the compiler's notion of the
  value of the enum itself (expressed as the enum, and not whatever underlying
  integral type is used to store it). But individual enums can be configured to
  act as a defined "fallback" value in the event that the sanitizer fails to
  validate a up-cast integer. In practice, this usually happens in cases where
  raw values of integral type are imported from outside sources at runtime, and
  then improperly construed as enum-controlled data by simple casting.

For this purpose, "Outside sources" might be a prior version our own program
  that wrote a value of an enum out to a file as a down-cast integer, and is now
  trying to unpack that file into a live object following changes to the enum's
  specific values.

EnumDef also provides an optional opaque context byte for use by whatever
  software is defining the enum.

NOTE: Use of this mechanism does NOT constrain the specific values of the enums,
  as assigned in their proper definitions. Indicies within the list are
  unimportant.

NOTE: Because we have an assurance of immutability, one of the standard pillars
  of OO-style encapsulation is meaningless. We don't need to guard object state
  against mutation when everything is immutable. Thus, all class members are
  public with C const naming conventions, and there are no trivial accessor
  methods.

TODO: I think I can now use the compiler to force the definition of an INVALID
  enum for whatever enum is being represented by the template. This makes sense
  for cases where the enum space doesn't fully cover the space of the underlying
  type (IE, most enums). And it would also help with lookup-by-string.


--------------------------------------------------------------------------------
Meditation: What is the difference (if any) between semantics and syntax?

Axiom: Naming a thing that does not exist will (by the act of naming it) bring
         the thing named into existance.
That is: A label is sufficient (but not necessary) grounds for ontogenesis.

To the extent that we can bias the evolution of the ontology of a thing by
  naming it, naming a thing is a form of control over the thing named.

In a computer, we always face ontological limits to our named things when we
  first attempt to create one in a finite memory space, or try to consider them
  in finite amounts of time. So we tie a few pieces of information to our names
  for the sake of facilitating the future evolution of things that we name with
  these classes.
--------------------------------------------------------------------------------
*/

#include <stdlib.h>
#include "StringBuilder.h"

#ifndef __C3P_ENUM_WRAPPER
#define __C3P_ENUM_WRAPPER

/* Flags pertaining to a given enum */
#define ENUM_WRAPPER_FLAG_CATCHALL  0x01   // If set, the associated enum will be a catch-all.

/*
* A wrapper object to tie enums to their string representations and an optional
*   context byte.
* This is to save us the obnoxious task of re-writing this support code for all
*   exposed enums in the program. It should be entirely const so that builds can
*   confidently isolate it to flash.
*/
template <class T> class EnumDef {
  public:
    const T VAL;
    const uint8_t FLAGS;
    const uint8_t CONTEXT;
    const char* const STR;

    EnumDef(const T EVAL, const char* const STR_REP, const uint8_t EFLAGS = 0, const uint8_t CNTXT = 0)
      : VAL(EVAL), FLAGS(EFLAGS), CONTEXT(CNTXT), STR(STR_REP) {};
};


/*
* A list of the above-defined objects. Like EnumDef<t>, instances of this template
*   ought to be able to be easilly relegated to flash.
*/
template <class T> class EnumDefList {
  public:
    const EnumDef<T>* const LIST_PTR;
    const uint32_t COUNT;
    const char* const LIST_NAME;

    /**
    * Constructor
    */
    EnumDefList(
      const EnumDef<T>* const DEFS,    // A pointer to the first item in the list.
      const uint32_t DEF_COUNT,        // The size of the list must be explicit.
      const char* const LNAME = ""     // An optional name for this list?
    ) : LIST_PTR(DEFS), COUNT(DEF_COUNT), LIST_NAME(LNAME) {};

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
    * @param The enum for which to fetch the definition string.
    * @return Always returns a valid string.
    */
    const char* const enumStr(const T ENUM) const {  // Also: const
      for (uint32_t i = 0; i < COUNT; i++) {
        if ((LIST_PTR + i)->VAL == ENUM) return (LIST_PTR + i)->STR;
      }
      return "<NO ENUM>";
    };


    /**
    * Used to retrieve the extra context byte for a given enum.
    * NOTE: Does not respect catch-all logic. A failed look-up will return 0.
    *
    * @param The enum for which to fetch the context byte.
    * @return The requested context byte, or 0 on failed lookup.
    */
    const uint8_t enumExtra(const T ENUM) const {
      for (uint32_t i = 0; i < COUNT; i++) {
        if ((LIST_PTR + i)->VAL == ENUM) return (LIST_PTR + i)->CONTEXT;
      }
      return "<NO ENUM>";
    };


    /**
    * Used to print strings representing enums.
    * NOTE: Does not respect catch-all logic.
    *
    * @param The enum for which to fetch the definition.
    * @return The definition container for the matching enum (if found), or nullptr.
    */
    const EnumDef<T>* enumDef(const T ENUM) const {
      for (uint32_t i = 0; i < COUNT; i++) {
        if ((LIST_PTR + i)->VAL == ENUM) return (LIST_PTR + i);
      }
      return nullptr;
    };


    /**
    * Find the enum represented by the given string.
    * If the entire enum set is exhausted without finding the search string but
    *   there exists an enum marked as a catch-all, the last catch-all defined
    *   in the enum list will be returned. In such a case, the `found` parameter
    *   will still be set to 0 to allow the caller to maintain semantic hygiene.
    *
    * TODO: Consolidate fetch function as a private member with a parameter
    *   that selects catch-all observation.
    *
    * @param NEEDLE is definition string for the enum we want.
    * @return The definition container for the matching enum (if found),
    *           or that of the catch-all (if one is defined), or nullptr.
    */
    const EnumDef<T>* getEnumDefByStr(const char* NEEDLE) const {
      uint32_t catchall_idx = 0xFFFFFFFF;
      for (uint32_t i = 0; i < COUNT; i++) {
        if (0 == StringBuilder::strcasecmp(NEEDLE, (LIST_PTR + i)->STR)) {
          return (LIST_PTR + i);
        }
        else if ((LIST_PTR + i)->FLAGS & ENUM_WRAPPER_FLAG_CATCHALL) {
          catchall_idx = i;
        }
      }

      return ((0xFFFFFFFF != catchall_idx) ? (LIST_PTR + catchall_idx) : nullptr);
    };


    /**
    * Find the enum represented by the given string.
    * If the entire enum set is exhausted without finding the search string but
    *   there exists an enum marked as a catch-all, the last catch-all defined
    *   in the enum list will be returned. In such a case, the `found` parameter
    *   will still be set to 0 to allow the caller to maintain semantic hygiene.
    *
    * @param NEEDLE is definition string for the enum we want.
    * @param found is a pass-by-reference return code for the look-up.
    * @return The matching enum (if found), or the catch-all (if one is
    *           defined), or the last enum in the list.
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
};

#endif  // __C3P_ENUM_WRAPPER
