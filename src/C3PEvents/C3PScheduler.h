/*
File:   C3PScheduler.h
Author: J. Ian Lindsay
Date:   2013.07.10

This class is meant to be an idle-time task scheduler for small microcontrollers. It
should be driven by a periodic interrupt of some sort, but it may also be effectively
used with a reliable polling scheme (at the possible cost of timing accuracy).


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

#ifndef __C3P_SCHEDULER_H
#define __C3P_SCHEDULER_H

#include <functional>
#include "../StringBuilder.h"
#include "../CppPotpourri.h"
#include "../FlagContainer.h"
#include "../StopWatch.h"
#include "../PriorityQueue.h"
#include "../ElementPool.h"
#include "../AbstractPlatform.h"
#include "../C3PLogger.h"


/*******************************************************************************
* This pure-virtual class represents a single schedule. The behavior of Schedule
*   execution should be confined to extending classes that intend on being
*   processed (possibly in an ISR stack frame) by the Scheduler singleton.
*
* Concurrency rules:
*   It was intended that the function invoked by a schedule be free to modify
*   the schedule directly during execution. However, a few constraints apply:
*   1) Do not sleep_ms() or sleep_us() within the scheduler's stack frame. Some
*      platforms might tolerate it within bounds. But you shouldn't be sleeping
*      in an async schedule regardless.
*   2) Do not write schedules with periods shorter than their own worst-case
*      execution times. Insertion into the Scheduler's execution queue is
*      idempotent until the schedule is serviced (double-adds are disallowed).
*
*
* NOTE: If (_recurrences == -1), then the schedule recurs for as long as
*   it remains enabled. If the value is zero, the schedule is disabled upon
*   successful execution. If the value is anything else, the schedule remains
*   enabled and this value is decremented.
*
* TODO: There is no implication of heap churn anywhere in this class (which may
*   be preferrable). But complicates things in other ways. This might end up
*   being ref-counted. But until then, Scheduler does no memory management.
*******************************************************************************/
class C3PSchedule {
  public:
    StopWatch  profiler;                  // If this schedule is being profiled, the ref will be here.

    virtual ~C3PSchedule() {};

    //inline int8_t     executeNow() {       return -1;   };
    inline bool         enabled() {                 return _enabled;      };
    inline void         enabled(bool x) {           _enabled = x;         };
    inline int32_t      recurrence() {              return _recurrences;  };
    inline void         recurrence(int32_t x) {     _recurrences = x;     };
    inline unsigned int period() {                  return _period;       };
    inline void         period(unsigned int x) {    _period = x;          };
    inline bool         executingNow() {            return _executing;    };
    inline unsigned int lastExec() {                return _last_exec;    };
    inline unsigned int nextExec() {                return _exec_at;      };
    inline const char*  handle() {                  return _handle;       };

    /* A valid schedule has a non-zero period. That is the only criteria. */
    inline bool valid() {    return (0 != _period);  };

    int8_t execute();
    void delay(unsigned int by_us); // Set the schedule's TTW to the given value this execution only.
    void delay();                // Reset the given schedule to its period and enable it.
    bool willRunAgain();         // Returns true if the indicated schedule will fire again.
    void printSchedule(StringBuilder*);


  protected:
    const char*  _handle;         // Handle for the task.

    C3PSchedule(const char* HANDLE, const uint32_t PERIOD, const int32_t RECURRENCES, const bool ENABLED) :
      _handle(HANDLE),
      _last_exec(0),
      _exec_at(0),
      _period(PERIOD),
      _recurrences(RECURRENCES),
      _enabled(ENABLED),
      _executing(false) {};

    /* Classes that wish to be considered as Schedules must implement these. */
    virtual int8_t _execute() =0;
    virtual void _print_schedule(StringBuilder*) =0;


  private:
    friend class C3PScheduler;    // Scheduler itself is allowed access to all members.
    unsigned int _last_exec;      // At what time was the last execution?
    unsigned int _exec_at;        // At what time will be the next execution?
    unsigned int _period;         // How often does this schedule execute?
    int32_t      _recurrences;    // How many times will execution occur? See notes.
    bool         _enabled;        // If true, this schedule will be processed.
    bool         _executing;      // If true, this schedule is presently executing.
    //bool       _wrap_control;   // If true, the next execution time will happen after a timer wrap.
    //bool       _run_in_isr;     // If true, this schedule will be processed in the ISR stack frame. Very dangerous.
    //bool       _autoclear;      // If true, this schedule will be removed after its last execution.
};


/*******************************************************************************
* Kinds of Schedules
*******************************************************************************/
/**
* A concrete type of Schedule that calls the given object's poll() function.
* This is mostly for fitting simpler classes into a Schedule-driven application.
*/
class C3PScheduledPolling : public C3PSchedule {
  public:
    C3PScheduledPolling(const char* HANDLE, const uint32_t PERIOD, const int32_t RECURRENCES, const bool ENABLED, PollableObj* obj) :
      C3PSchedule(HANDLE, PERIOD, RECURRENCES, ENABLED), _pollable_obj(obj) {};

    virtual ~C3PScheduledPolling() {};


  protected:
    PollableObj* _pollable_obj;          // Pointer to the class to be polled.

    /* Obligate overrides from C3PSchedule... */
    int8_t _execute();
    void _print_schedule(StringBuilder*);
};


/**
* A concrete type of Schedule that calls the given lamba function on execution.
*/
class C3PScheduledLambda : public C3PSchedule {
  public:
    C3PScheduledLambda(const char* HANDLE, const uint32_t PERIOD, const int32_t RECURRENCES, const bool ENABLED, std::function<int8_t(void)> lam) :
      C3PSchedule(HANDLE, PERIOD, RECURRENCES, ENABLED), _fxn_lambda(lam) {};

    virtual ~C3PScheduledLambda() {};


  protected:
    std::function<int8_t(void)> _fxn_lambda;  // A lambda to be called.

    /* Obligate overrides from C3PSchedule... */
    int8_t _execute();
    void _print_schedule(StringBuilder*);
};


/**
* A concrete type of Schedule that shows the given Event to the given Observer.
*/
//class C3PScheduledEvent : public C3PSchedule {
//  public:
//    C3PScheduledEvent(const uint32_t PERIOD, const int32_t RECURANCES, const bool ENABLED, C3PEvent* evnt, C3PObserver* obs) :
//      C3PSchedule(PERIOD, RECURANCES, ENABLED), _event(evnt), _observer(obs) {};
//
//    virtual ~C3PScheduledEvent() {};
//
//
//  protected:
//    C3PEvent*    _event;
//    C3PObserver* _observer;
//
//    /* Obligate overrides from C3PSchedule... */
//    int8_t _execute();
//    void _print_schedule(StringBuilder*);
//};



/*******************************************************************************
* The Scheduler singleton
*
* NOTE: All times are given as microseconds, and are 32-bit. That makes the
*   maximum useful schedule period a bit more than 71.5 minutes.
*******************************************************************************/
class C3PScheduler {
  public:
    StopWatch  profiler_service;   // Number of calls to serviceScheduledEvents() that actually ran a schedule.
    StopWatch  profiler_deadband;  // Used to make inferences about jitter.

    ~C3PScheduler();  // Destructor

    int8_t addSchedule(C3PSchedule*);
    int8_t removeSchedule(C3PSchedule*);
    bool   containsSchedule(C3PSchedule*);
    inline C3PSchedule* getScheduleByIndex(unsigned int idx) {   return _active.get(idx, false);   };
    inline unsigned int scheduleCount() {    return _active.count();    };

    void serviceSchedules();              // Execute any schedules that have come due.
    void advanceScheduler();              // Push all enabled schedules forward by one tick.

    void printDebug(StringBuilder*);

    inline uint32_t serviceLoops() {      return profiler_service.executions();    };
    inline bool initialized() {
      return (_active.allocated() & _exec_queue.allocated());
    };

    static C3PScheduler* getInstance();   // Enforcement of singleton pattern.


  private:
    // TODO: No memory management strategy forces some waste in RingBuffer...
    RingBuffer<C3PSchedule*> _active;
    RingBuffer<C3PSchedule*> _exec_queue;
    //RingBuffer<C3PSchedule*> _inactive;
    uint32_t _isr_count;
    uint32_t _advance_overruns;

    /* Constructor */
    C3PScheduler(const uint32_t MAX_SCHEDULE_COUNT) :
      _active(MAX_SCHEDULE_COUNT),
      //_inactive(MAX_SCHEDULE_COUNT),
      _exec_queue(MAX_SCHEDULE_COUNT),
      _isr_count(0)
    {};
};

#endif  // __C3P_SCHEDULER_H
