/*
File:   C3PScheduler.cpp
Author: J. Ian Lindsay
Date:   2013.07.10

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

#include "C3PScheduler.h"

// Singleton instance
volatile static C3PScheduler* SCHEDULER_INSTANCE_PTR = nullptr;

FAST_FUNC C3PScheduler* C3PScheduler::getInstance() {
  if (nullptr == SCHEDULER_INSTANCE_PTR) {
    SCHEDULER_INSTANCE_PTR = new C3PScheduler(8);  // TODO: Arbitrary schedule limit.
  }
  return (C3PScheduler*) SCHEDULER_INSTANCE_PTR;
}


/*******************************************************************************
* C3PSchedule base class
*******************************************************************************/

FAST_FUNC int8_t C3PSchedule::execute() {
  _executing = true;
  const unsigned int NOW = micros();
  profiler.markStart();
  int8_t ret = _execute();
  if (_enabled) {
    if (0 < _recurrences) {
      _recurrences--;
    }
    if (0 == _recurrences) {
      _enabled = false;
    }
    _last_exec = NOW;
  }

  if (willRunAgain()) {
    _exec_at = (_period + NOW);
  }
  else {
    _exec_at = 0;
  }

  profiler.markStop();
  _executing = false;
  return ret;
}


FAST_FUNC void C3PSchedule::delay(unsigned int by_us) {
  if (!_executing) {
    if (!_enabled) {
      _exec_at = (unsigned int) micros();
      _enabled = true;
    }
    _exec_at += by_us;
  }
}


FAST_FUNC void C3PSchedule::delay() {
  if (!_executing) {
    _exec_at = ((unsigned int) micros() + _period);
    _enabled = true;
  }
}


FAST_FUNC bool C3PSchedule::willRunAgain() {
  bool ret = (_enabled & (0 < _recurrences));
  ret |= (_enabled & (-1 == _recurrences));
  return ret;
}


void C3PSchedule::printSchedule(StringBuilder* output) {
  _print_schedule(output);
  output->concatf("\tPeriod:          %u\n", _period);
  output->concat( "\trecurrences:     ");
  if (-1 == _recurrences) {   output->concat("forever\n");            }
  else {                      output->concatf("%d\n", _recurrences);  }

  if (willRunAgain()) {
    output->concatf("\tNext execution:  %u (%uus from now)\n", _exec_at, micros_until(_exec_at));
  }
  if (0 < profiler.executions()) {
    output->concatf("\tLast execution:  %u (%uus ago)\n", _last_exec, micros_since(_last_exec));
  }
  StopWatch::printDebugHeader(output);
  profiler.printDebug("execute()", output);
}


void C3PSchedule::printProfiler(StringBuilder* output) {
  profiler.printDebug(_handle, output);
}



/*******************************************************************************
* Kinds of Schedules
*******************************************************************************/

FAST_FUNC int8_t C3PScheduledPolling::_execute() {
  int8_t ret = 0;
  if (nullptr != _pollable_obj) {
    PollResult poll_ret = _pollable_obj->poll();
  }
  return ret;
}

void C3PScheduledPolling::_print_schedule(StringBuilder* output) {
  StringBuilder header_content(_handle);
  header_content.concat(" (ScheduledPolling)");
  StringBuilder::styleHeader2(output, (char*) header_content.string());
}



FAST_FUNC int8_t C3PScheduledLambda::_execute() {
  int8_t ret = 0;
  _fxn_lambda();
  return ret;
}

void C3PScheduledLambda::_print_schedule(StringBuilder* output) {
  StringBuilder header_content(_handle);
  header_content.concat(" (ScheduledLambda)");
  StringBuilder::styleHeader2(output, (char*) header_content.string());
}



FAST_FUNC int8_t C3PScheduledJitterProbe::_execute() {
  int8_t ret = 0;
  if (jitter.initialized()) {
    const unsigned int NOW = micros();
    uint32_t wrapped_jitter = (NOW - nextExec());
    if (nextExec() > NOW) {
      wrapped_jitter = (nextExec() - NOW);
    }
    jitter.feedSeries(wrapped_jitter);
  }
  else {
    jitter.init();
  }
  return ret;
}

void C3PScheduledJitterProbe::_print_schedule(StringBuilder* output) {
  StringBuilder header_content(_handle);
  header_content.concat(" (JitterProbe)");
  StringBuilder::styleHeader2(output, (char*) header_content.string());
}



/*******************************************************************************
* The Scheduler singleton
*******************************************************************************/

/*
* Destructor
*/
C3PScheduler::~C3PScheduler() {
  // TODO: Wipe the queues. Block until no executions are happening. Might not
  //   be important for the singleton pattern.
}


/**
* Adds a Schedule to our processing queue.
*
* @param sch The Schedule to add.
* @return 0 on success, or negative on error.
*/
int8_t C3PScheduler::addSchedule(C3PSchedule* sch) {
  int8_t ret = _active.insertIfAbsent(sch);
  if (0 == ret) {
    if (sch->enabled()) {
      sch->delay();  // Will mark enabled,
    }
  }
  return ret;
}


/*
* If the schedule to be removed is presently queued for service, it will be
*   removed if it is not executing. Otherwise, it will be allowed to resolve.
*/
int8_t C3PScheduler::removeSchedule(C3PSchedule* sch) {
  int8_t ret = -1;
  // TODO
  return ret;
}


bool C3PScheduler::containsSchedule(C3PSchedule* sched) {
  return _active.contains(sched);
}


/**
* This function should be called in the program's idle time, and will remove
*   schedules from the execution queue.
*/
FAST_FUNC void C3PScheduler::serviceSchedules() {
  if (_isr_count > 0) {
    profiler_deadband.markStop();
    profiler_service.markStart();
    // Service queue will refuse to process until the ISR has been pinged.
    C3PSchedule* current = _exec_queue.get();
    while (nullptr != current) {
      current->execute();
      current = _exec_queue.get();
    }
    profiler_service.markStop();
  }
}


/**
* This function should be called periodically from a timing source, and will add
*   schedules to the execution queue.
*/
FAST_FUNC void C3PScheduler::advanceScheduler() {
  for (uint32_t i = 0; i < _active.count(); i++) {
    C3PSchedule* current = _active.peek(i, false);
    if ((nullptr != current) && current->enabled()) {
      if ((ULONG_MAX >> 1) > micros_since(current->_exec_at)) {
        if (0 != _exec_queue.insertIfAbsent(current)) {
          // TODO: Anomaly tracking...
        }
      }
    }
  }
  profiler_deadband.markStart();
  _isr_count++;
}


void C3PScheduler::printDebug(StringBuilder* output) {
  StringBuilder::styleHeader1(output, "C3PScheduler");
  output->concatf("\tSchedule count:   %u\n", _active.count());
  output->concatf("\tLoops (SVC/ISR):  %u / %u\n\n", profiler_service.executions(), _isr_count);
  StopWatch::printDebugHeader(output);
  profiler_service.printDebug("Service", output);
  profiler_deadband.printDebug("Deadband", output);
  for (uint32_t i = 0; i < _active.count(); i++) {
    C3PSchedule* current = _active.peek(i, false);
    if (nullptr != current) {
      current->printProfiler(output);
    }
  }
}
