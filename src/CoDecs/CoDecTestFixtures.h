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

#include "CoDec.h"
#include "../StopWatch.h"


#ifndef __C3P_CODEC_TEST_FIXTURES_H__
#define __C3P_CODEC_TEST_FIXTURES_H__

/*
* Class to generate random printable strings of a shape approximating something
* Human readable.
*/
class BufAcceptTestSource {
  public:
    BufAcceptTestSource() {};
    ~BufAcceptTestSource() {};

    int8_t generateSentence();
    inline void setEfferant(BufferAccepter* x) {   _efferant = x;  };


  private:
    BufferAccepter* _efferant = nullptr;
};


/*
* Class to analyze BufferAccepter behaviors.
*
* NOTE:
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
    int8_t provideBuffer(StringBuilder*);
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
    uint32_t _pb_call_count_rej     = 0;   // Count of times provideBuffer() returned -1.
    uint32_t _pb_call_count_partial = 0;   // Count of times provideBuffer() returned 0.
    uint32_t _pb_call_count_full    = 0;   // Count of times provideBuffer() returned 1.
    uint32_t _expectations_met      = 0;   // How many times did an offered buffer meet expectations?
    uint32_t _expectations_violated = 0;   // How many times did an offered buffer violate expectations?
    uint32_t _expected_length       = 0;                   // Implies no expectaion.
    LineTerm _expected_terminator   = LineTerm::ZEROBYTE;  // Implies no expectaion.

    bool _does_terminator_match();
};

#endif  // __C3P_CODEC_TEST_FIXTURES_H__