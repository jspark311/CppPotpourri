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

C3PScheduler* C3PScheduler::getInstance() {
  if (nullptr == SCHEDULER_INSTANCE_PTR) {
    SCHEDULER_INSTANCE_PTR = new C3PScheduler(8);  // TODO: Arbitrary schedule limit.
  }
  return (C3PScheduler*) SCHEDULER_INSTANCE_PTR;
}


/*******************************************************************************
* C3PSchedule base class
*******************************************************************************/

int8_t C3PSchedule::execute() {
  _executing = true;
  profiler.markStart();
  int8_t ret = _execute();
  if (_enabled & (0 < _recurrances)) {
    _recurrances--;
  }
  else {
    _enabled = false;
    _exec_at = 0;
  }
  profiler.markStop();
  _executing = false;
  return ret;
}


void C3PSchedule::delay(uint32_t by_us) {
  if (!_executing) {
    if (!_enabled) {
      _exec_at = (uint32_t) micros();
      _enabled = true;
    }
    _exec_at += by_us;
  }
}


void C3PSchedule::delay() {
  if (!_executing) {
    _exec_at = ((uint32_t) micros() + _period);
    _enabled = true;
  }
}


bool C3PSchedule::willRunAgain() {
  bool ret = (_enabled & (0 < _recurrances));
  ret |= (_enabled & (-1 == _recurrances));
  return ret;
}


void C3PSchedule::printSchedule(StringBuilder* output) {
  _print_schedule(output);
  output->concatf("\tPeriod:          %u\n", _period);
  output->concat( "\tRecurrances:     ");
  if (-1 == _recurrances) {   output->concat("forever\n");            }
  else {                      output->concatf("%d\n", _recurrances);  }

  if (willRunAgain()) {
    output->concatf("\tNext execution:  %u (%uus from now)\n", _exec_at, wrap_accounted_delta(_exec_at, (uint32_t) micros()));
  }
  if (0 > profiler.executions()) {
    output->concatf("\tLast execution:  %u (%uus ago)\n", _last_exec, wrap_accounted_delta(micros(), _last_exec));
  }
  StopWatch::printDebugHeader(output);
  profiler.printDebug("execute()", output);
}



/*******************************************************************************
* Kinds of Schedules
*******************************************************************************/

int8_t C3PScheduledPolling::_execute() {
  int8_t ret = 0;
  _pollable_obj->poll();
  return ret;
}

void C3PScheduledPolling::_print_schedule(StringBuilder* output) {
  StringBuilder::styleHeader1(output, "ScheduledPolling");
}



int8_t C3PScheduledLambda::_execute() {
  int8_t ret = 0;
  _fxn_lambda();
  return ret;
}

void C3PScheduledLambda::_print_schedule(StringBuilder* output) {
  StringBuilder::styleHeader1(output, "ScheduledLambda");
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


int8_t C3PScheduler::removeSchedule(C3PSchedule* sch) {
  int8_t ret = -1;
  // TODO
  return ret;
}


void C3PScheduler::serviceSchedules() {
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


void C3PScheduler::advanceScheduler() {
  for (uint32_t i = 0; i < _active.count(); i++) {
    C3PSchedule* current = _active.get(i, false);
    if ((nullptr != current) && current->enabled()) {

      if (current->_exec_at <= micros()) {   // TODO: Wrap handling.
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
}
