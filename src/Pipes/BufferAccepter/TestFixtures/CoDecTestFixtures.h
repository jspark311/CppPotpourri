/*
File:   CoDecTestFixtures.h
Author: J. Ian Lindsay
Date:   2023.08.25

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


Test fixtures for CoDecs. Only programs concerned with unit testing need to
  include this file.
*/

#include "../BufferAccepter.h"
#include "../../TimerTools.h"

#ifndef __C3P_CODEC_TEST_FIXTURES_H__
#define __C3P_CODEC_TEST_FIXTURES_H__

/*
* Class to analyze BufferAccepter intake behaviors.
* This should be connected to the input side of a BufferAccepter under-test.
*/
class BufAcceptTestSource : public BufferCoDec {
  public:
    BufAcceptTestSource(BufferAccepter* eff = nullptr) : BufferCoDec(eff) {};
    ~BufAcceptTestSource() {};

    /* Implementation of BufferAccepter. This is how we accept input. */
    int8_t pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    /* Implementation of test harness. */
    inline void setProfiler(StopWatch* x) {          _profiler = x;  };

    void printDebug(StringBuilder*);
    bool efferantViolatesContract();
    int8_t poll();
    int8_t pollUntilStagnant();
    void reset();
    bool callCountsBalance();
    inline void     pushLimit(int32_t x) {     _fake_buffer_limit = x;        };
    inline int32_t  pushLimit() {              return _fake_buffer_limit;     };
    inline uint32_t callCount() {              return _call_count;            };
    inline uint32_t countRejections() {        return _pb_call_count_rej;     };
    inline uint32_t countPartialClaims() {     return _pb_call_count_partial; };
    inline uint32_t countFullClaims() {        return _pb_call_count_full;    };
    inline int32_t  backlogLength() {          return _backlog.length();      };

    // TODO: It would be nice to have a fxn to generate random printable strings of a
    //   shape approximating something human readable. Maybe another for ASCII unsafe
    //   buffer data.
    //int8_t generateBuffer();
    //int8_t generateSentence();


  private:
    StringBuilder   _backlog;  // Data is only sent via this object, in metered bursts.
    StopWatch*      _profiler = nullptr;
    int32_t  _fake_buffer_limit     = 0;   // Implies reject all offered buffers.
    uint32_t _pb_call_count_rej     = 0;   // Count of times pushBuffer() returned -1.
    uint32_t _pb_call_count_partial = 0;   // Count of times pushBuffer() returned 0.
    uint32_t _pb_call_count_full    = 0;   // Count of times pushBuffer() returned 1.

    uint32_t _false_rejections      = 0;   // Count of times pushBuffer() returned -1 but didn't take nothing.
    uint32_t _false_partial_claims  = 0;   // Count of times pushBuffer() returned 0 but didn't take anything.
    uint32_t _false_full_claims     = 0;   // Count of times pushBuffer() returned 1 but didn't take everything.
    uint32_t _call_count            = 0;   // Count of times pushBuffer() was called.
};



/*
* Class to analyze BufferAccepter output behaviors.
* This should be connected to the output side of a BufferAccepter under-test.
*
* NOTE: The take_log is extra-contractual. For contractual purposes, this class
*   does no true buffering. It discards whatever it receives after noticing a
*   few things about it.
*
* NOTE: The take_log is structure-preserving with respect to buffer scatter.
*   Thus, its count() might not reflect the same value as callCount() if a
*   badly-fragged buffer came in. Also, to retain this diagnostic information,
*   take_log should not be mutated from outside. Be careful...
*/
class BufAcceptTestSink : public BufferAccepter {
  public:
    StringBuilder take_log;   // Collected list of offered buffers that were taken.
    StopWatch profiler;       // The end-point of the test harness contains the profiler.

    /* Implementation of BufferAccepter. This is how we accept input. */
    int8_t pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    /* Implementation of test harness. */
    void printDebug(StringBuilder*);
    void reset();
    bool callCountsBalance();
    inline void     bufferLimit(int32_t x) {   _fake_buffer_limit = x;        };
    inline int32_t  bufferLimit() {            return _fake_buffer_limit;     };
    inline uint32_t callCount() {              return profiler.executions();  };
    inline uint32_t countRejections() {        return _pb_call_count_rej;     };
    inline uint32_t countPartialClaims() {     return _pb_call_count_partial; };
    inline uint32_t countFullClaims() {        return _pb_call_count_full;    };
    inline uint32_t expectationsMet() {        return _expectations_met;      };
    inline uint32_t expectationsViolated() {   return _expectations_violated; };
    inline uint32_t expectedLength() {         return _expected_length;       };
    inline LineTerm expectedTerminator() {     return _expected_terminator;   };
    inline void     expectation(uint32_t x) {  _expected_length = x;          };
    inline void     expectation(LineTerm x) {  _expected_terminator = x;      };


  private:
    int32_t  _fake_buffer_limit     = 0;   // Implies reject all offered buffers.
    uint32_t _pb_call_count_rej     = 0;   // Count of times pushBuffer() returned -1.
    uint32_t _pb_call_count_partial = 0;   // Count of times pushBuffer() returned 0.
    uint32_t _pb_call_count_full    = 0;   // Count of times pushBuffer() returned 1.
    uint32_t _expectations_met      = 0;   // How many times did an offered buffer meet expectations?
    uint32_t _expectations_violated = 0;   // How many times did an offered buffer violate expectations?
    uint32_t _expected_length       = 0;                   // Implies no expectaion.
    LineTerm _expected_terminator   = LineTerm::ZEROBYTE;  // Implies no expectaion.

    bool _does_terminator_match();
};

#endif  // __C3P_CODEC_TEST_FIXTURES_H__
