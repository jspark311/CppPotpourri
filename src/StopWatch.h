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



  /*
  * A class to rate-limit periodic events. It is generalized, and must be
  *   extended by one of the two specific classes that implement the timer logic 
  *   over either millis() or micros().
  */
  class PeriodicTimeout {
    public:
      ~PeriodicTimeout() {};      // Featureless destructor.

      inline void     reset() {                 _mark = _now();               };
      inline void     reset(unsigned int p) {   _mark = _now(); _period = p;  };
      inline void     period(unsigned int p) {  _period = p;                  };
      inline unsigned int period() {            return _period;               };
      inline unsigned int remaining() {
        return (expired() ? 0 : _until(_mark));
      };
      inline bool     expired() {
        return ((0 == _period) || (_period <= _since(_mark)));
      };

    protected:
      unsigned int _period;
      unsigned int _mark;

      PeriodicTimeout(unsigned int p) : _period(p), _mark(0) {};
      virtual unsigned int _now() =0;   // TODO: unspec uint width.
      virtual unsigned int _until(unsigned int) =0;   // TODO: unspec uint width.
      virtual unsigned int _since(unsigned int) =0;   // TODO: unspec uint width.
  };


  /* A class to rate-limit periodic events. */
  class MillisTimeout : public PeriodicTimeout {
    public:
      MillisTimeout(unsigned int p = 0) : PeriodicTimeout(p) {};

    protected:
      unsigned int _now() {                     return millis();             };
      unsigned int _until(unsigned int mark) {  return millis_until(mark);   };
      unsigned int _since(unsigned int mark) {  return millis_since(mark);   };
  };


  /* A class to rate-limit periodic events. */
  class MicrosTimeout : public PeriodicTimeout {
    public:
      MicrosTimeout(unsigned int p = 0) : PeriodicTimeout(p) {};

    protected:
      unsigned int _now() {                     return micros();             };
      unsigned int _until(unsigned int mark) {  return micros_until(mark);   };
      unsigned int _since(unsigned int mark) {  return micros_since(mark);   };
  };


#endif // __STOP_WATCH_H__
