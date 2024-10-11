/*
File:   CppPotpourri.h
Author: J. Ian Lindsay
Date:   2020.01.20

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
*/

#ifndef __CPPPOTPOURRI_H__
#define __CPPPOTPOURRI_H__

#include "Meta/Compilers.h"        // Include compiler support.
#include "Meta/AntiMacro.h"        // Library-wide anti-macros.
#include "EnumeratedTypeCodes.h"
#include "C3PRandom/C3PRandom.h"

class StringBuilder;  // Forward declaration for buffer interchange.



/*******************************************************************************
* High-level string operations.
* TODO: These feel like they are in the wrong place.
*******************************************************************************/
void timestampToString(StringBuilder*, uint64_t);
uint64_t stringToTimestamp(const char*);
int randomArt(uint8_t* dgst_raw, unsigned int dgst_raw_len, const char* title, StringBuilder*);



/*******************************************************************************
* Interfaces and callback definitions in use throughout this library.
*******************************************************************************/
/* Shorthand for a pointer to a "void fxn(void)" */
typedef void  (*FxnPointer)();

/* Callbacks for drivers that provide extra GPI pins. */
typedef void (*PinCallback)(uint8_t pin, uint8_t level);



/*******************************************************************************
* Asynchronous polling interface
*
* For simplicity, many classes are writen in such a way as to benefit (or
*   require) periodic polling for them to update their own states. The more
*   complicated the class, the more likely it is to require this.
* To keep that complexity bounded, it is advised that such classes implement the
*   C3PPollable interface to allow themselves to be recomposed into higher-level
*   logic without complicated APIs or "special treatment".
*******************************************************************************/
enum class PollResult : int8_t {
  /*
  Code       Value   Semantics
  ----------------------------------------------------------------------------*/
  ERROR     = -1,    // Polling resulted in an internal class problem.
  NO_ACTION =  0,    // No action. No error.
  ACTION    =  1,    // Polling resulted in an evolution of class state.
  REPOLL    =  2     // Repoll immediately, subject to caller's descretion.
};

/**
* An interface class for simple state polling.
*/
class C3PPollable {
  public:
    virtual PollResult poll() =0;
};


/*******************************************************************************
* This class is intended to be a compositional element that implements
*   reference-counting. This might be employed for garbage collectors,
*   wake-locking, or generally any purpose where a class should be notified
*   when nothing is depending on it.
* Maximum reference count is somewhat less than 16-bit to give some headroom for
*   concurrency.
*******************************************************************************/
class C3PRefCounter {
  public:
    // Constructor.
    C3PRefCounter(const uint16_t STARTING_COUNT = 0) :
      _count_locked(false), _ref_count(STARTING_COUNT) {};

    ~C3PRefCounter() {}; // Trivial destructor.

    /**
    * Releases a reference. Release happens at the dependent edge of the
    *   semaphore to prevent the count hitting zero in a concurrency race.
    *
    * @return true if the reference count was 0 at the end of the call.
    */
    bool refRelease() {
      while (_count_locked) {};
      _count_locked = true;
      if (0 < _ref_count) _ref_count--;
      _count_locked = false;
      return (0 == _ref_count);
    };

    /**
    * Take a reference.
    *
    * @return true if the call was noted.
    */
    bool refTake() {
      while (_count_locked) {};
      if (64000 <= _ref_count) return false;
      _count_locked = true;
      _ref_count++;
      _count_locked = false;
      return true;
    };

    inline uint16_t refCount() {    return _ref_count;    };

    static constexpr uint16_t MAXIMUM_REFS = 64000;


  protected:
    bool      _count_locked;
    uint16_t  _ref_count;
};

#endif // __CPPPOTPOURRI_H__
