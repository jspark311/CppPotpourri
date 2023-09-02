/*
File:   StopWatch.h
Author: J. Ian Lindsay
Date:   2015.12.01

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

#include <inttypes.h>
#include <stdint.h>
#include "AbstractPlatform.h"

#ifndef __STOP_WATCH_H__
  #define __STOP_WATCH_H__

  class StringBuilder;

  /* A class to benchmark periodic events. */
  class StopWatch {
    public:
      StopWatch(uint32_t tag = 0);   // Constructor. Assigns tag value. Calls reset().
      ~StopWatch() {};           // Featureless destructor.

      inline uint32_t tag() {          return _tag;                };
      inline uint32_t bestTime() {     return _run_time_best;      };
      inline uint32_t lastTime() {     return _run_time_last;      };
      inline uint32_t worstTime() {    return _run_time_worst;     };
      inline uint32_t meanTime() {     return _run_time_average;   };
      inline uint32_t totalTime() {    return _run_time_total;     };
      inline uint32_t executions() {   return _executions;         };
      inline void     markStart() {    _start_micros = (uint32_t) micros();   };
      bool  markStop();
      void  reset();
      void printDebug(const char*, StringBuilder*);

      static void printDebugHeader(StringBuilder*);


    private:
      uint32_t _tag;      // A slot for arbitrary application data.
      uint32_t _start_micros;
      uint32_t _run_time_last;
      uint32_t _run_time_best;
      uint32_t _run_time_worst;
      uint32_t _run_time_average;
      uint32_t _run_time_total;
      uint32_t _executions;
  };



  /* A class to rate-limit periodic events. */
  class PeriodicTimeout {
    public:
      PeriodicTimeout(uint32_t p = 0) : _period(p), _mark(0) {};
      ~PeriodicTimeout() {};      // Featureless destructor.

      inline void     reset() {             _mark = (uint32_t) millis();               };
      inline void     reset(uint32_t p) {   _mark = (uint32_t) millis(); _period = p;  };
      inline void     period(uint32_t p) {  _period = p;     };
      inline uint32_t period() {            return _period;  };
      inline uint32_t remaining() {
        return (expired() ? 0 : millis_until(_mark));
      };
      inline bool     expired() {
        return ((0 == _period) || (_period <= millis_since(_mark)));
      };


    private:
      uint32_t _period;
      uint32_t _mark;
  };


#endif // __STOP_WATCH_H__
