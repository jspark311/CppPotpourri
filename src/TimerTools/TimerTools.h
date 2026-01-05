/*
File:   TimerTools.h
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

#ifndef __C3P_TIMER_TOOLS_H__
#define __C3P_TIMER_TOOLS_H__

#include <inttypes.h>
#include <stdint.h>
#include "../AbstractPlatform.h"
#include "../C3PValue/C3PType.h"
class StringBuilder;


/*******************************************************************************
* StopWatch: A class to benchmark periodic events.
*******************************************************************************/
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
    bool  addRuntime(const unsigned long START_TIME, const unsigned long STOP_TIME);
    void  reset();
    void printDebug(const char*, StringBuilder*);

    static void printDebugHeader(StringBuilder*);

  private:
    friend int    C3PTypeConstraint<StopWatch*>::serialize(void*, StringBuilder*, const TCode);
    friend int8_t C3PTypeConstraint<StopWatch*>::construct(void*, KeyValuePair*);

    uint32_t _tag;      // A slot for arbitrary application data.
    uint32_t _start_micros;
    uint32_t _run_time_last;
    uint32_t _run_time_best;
    uint32_t _run_time_worst;
    uint32_t _run_time_average;
    uint32_t _run_time_total;
    uint32_t _executions;
};


/*******************************************************************************
* PeriodicTimeout: A class to rate-limit periodic events.
* It is generalized, and must be extended by one of the two specific classes
*   that implement the timer logic over either millis() or micros().
*******************************************************************************/
class PeriodicTimeout {
  public:
    virtual ~PeriodicTimeout() {};      // Featureless destructor.

    inline void trigger() {                _mark = (_now() - (_period+1));  };
    inline void reset() {                  _mark = _now();                  };
    inline void reset(unsigned long p) {   _mark = _now(); _period = p;     };
    inline void period(unsigned long p) {  _period = p;                     };
    inline bool enabled() {                return (0 < _period);            };
    inline unsigned long period() {        return _period;                  };
    inline unsigned long remaining() {
      return (expired() ? 0 : _until(_mark + _period));
    };
    inline bool     expired() {
      return ((0 == _period) || (_period <= _since(_mark)));
    };

  protected:
    unsigned long _period;
    unsigned long _mark;

    PeriodicTimeout(unsigned long p) : _period(p), _mark(0) {};
    virtual unsigned long _now() =0;
    virtual unsigned long _until(unsigned long) =0;
    virtual unsigned long _since(unsigned long) =0;
};


/* A class to rate-limit periodic events at the millisecond scale. */
class MillisTimeout : public PeriodicTimeout {
  public:
    MillisTimeout(unsigned long p = 0) : PeriodicTimeout(p) {};

  protected:
    unsigned long _now() {                      return millis();            };
    unsigned long _until(unsigned long mark) {  return millis_until(mark);  };
    unsigned long _since(unsigned long mark) {  return millis_since(mark);  };
};


/* A class to rate-limit periodic events at the microsecond scale. */
class MicrosTimeout : public PeriodicTimeout {
  public:
    MicrosTimeout(unsigned long p = 0) : PeriodicTimeout(p) {};

  protected:
    unsigned long _now() {                      return micros();            };
    unsigned long _until(unsigned long mark) {  return micros_until(mark);  };
    unsigned long _since(unsigned long mark) {  return micros_since(mark);  };
};


#endif // __C3P_TIMER_TOOLS_H__
