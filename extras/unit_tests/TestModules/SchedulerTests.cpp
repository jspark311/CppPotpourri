/*
File:   SchedulerTests.cpp
Author: J. Ian Lindsay
Date:   2023.05.16

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


This program tests the Scheduler class. Since Scheduler is truely aware of the
  reports from micros(), we can't test its timing certainty in a given
  implementation (which might have a better notion of microseconds than a
  docker image). Nor can we actually verify that certain race-conditions are
  actually closed. That said, we test as much as we can.
*/

#include "TimerTools/C3PScheduler.h"
#include "AsyncSequencer.h"

/*******************************************************************************
* Scheduler globals
*******************************************************************************/
unsigned int  scheduler_period = 1000;
unsigned int  scheduler_slip   = 0;         //
C3PScheduler* scheduler        = nullptr;   // Pointer to constructed singleton.

// Result values used to close epistimological loops.
unsigned int  marker_sch_0     = 0;
unsigned int  marker_sch_1     = 0;


/*******************************************************************************
* Test Schedules
*******************************************************************************/
// This schedule runs 5 times exactly.
C3PScheduledLambda schedule_test_0(
  "test_0",
  5000, 5, true,
  []() {
    marker_sch_0++;
    return 0;
  }
);

// This schedule runs forever, and is use to check infinite recycle.
C3PScheduledLambda schedule_test_1(
  "test_1",
  250000, -1, true,
  []() {
    marker_sch_1++;
    return 0;
  }
);



/*******************************************************************************
* Scheduler test routines
*******************************************************************************/
/*
* Initialize the scheduler. Define and print the test schedules.
*/
int test_scheduler_init() {
  int ret = -1;
  scheduler = C3PScheduler::getInstance();
  if (nullptr != scheduler) {
    StringBuilder text_return;
    text_return.concat("schedule_test_0:\n");
    schedule_test_0.printSchedule(&text_return);
    text_return.concat("\nschedule_test_1:\n");
    schedule_test_1.printSchedule(&text_return);
    printf("\nAdding schedules...\n%s\n", text_return.string());
    int8_t add_ret = scheduler->addSchedule(&schedule_test_0);
    if (0 == add_ret) {
      add_ret = scheduler->addSchedule(&schedule_test_1);
      if (0 == add_ret) {
        if (scheduler->initialized()) {
          printf("Scheduler initialized.\n");
          ret = 0;
        }
        else printf("Failed to allocate queues.\n");
      }
      else printf("Failed to add schedule_test_1 (%d).\n", add_ret);
    }
    else printf("Failed to add schedule_test_0 (%d).\n", add_ret);

    if (0 != ret) {
      printf("Schedule addition failed.\n");
    }
  }
  else {
    printf("Singleton construction failed.\n");
  }
  return ret;
}


/*
* The initial conditions of the scheduler are critical to its proper operation.
*/
int test_scheduler_initial_conditions() {
  int ret = -1;
  StringBuilder text_return;
  //scheduler->printDebug(&text_return);
  // At this point, the scheduler should be constructed and schedules added, but
  //   advanceScheduler() hasn't been called yet. So serviceSchedules() should
  //   do nothing. Spam the service function and verify...
  scheduler->serviceSchedules();
  scheduler->serviceSchedules();
  scheduler->serviceSchedules();
  if (0 == scheduler->serviceLoops()) {
    // Calling advanceScheduler() once should be sufficient to allow the
    //   scheduler to begin operation. Verify...
    scheduler->advanceScheduler();
    scheduler->serviceSchedules();
    unsigned int svc_count = scheduler->serviceLoops();
    if (1 == svc_count) {
      // After the first serivce loop, any schedules that have come due since
      //   being added should have executed.
      ret = 0;
    }
    else {
      text_return.concatf("Service calls should be 1, but was in fact %u.\n", svc_count);
    }
  }
  else {
    text_return.concatf("It seems the scheduler is running service loops ahead of ISR.\n");
  }
  printf("%s\n", text_return.string());
  return ret;
}

/*
* Ensure that the schedules actually ran as many times as the profiler claims,
*   and as was ordered.
*/
int test_scheduler_run_count_checks() {
  int ret = -1;
  if (schedule_test_0.profiler.executions() == marker_sch_0) {    // Schedule's profiler agrees with our result?
    if (schedule_test_1.profiler.executions() == marker_sch_1) {  // Schedule's profiler agrees with our result?
      if ((0 < marker_sch_0) & (0 < marker_sch_1)) {              // Both schedules ran at least once.
        ret = 0;
      }
    }
  }
  StringBuilder text_return("Schedule count check ");
  if (0 == ret) {
    text_return.concat("passes.\n");
    text_return.concatf("marker_sch_0:     %5u\n", marker_sch_0);
    text_return.concatf("marker_sch_1:     %5u\n", marker_sch_1);
  }
  else {
    text_return.concat("failure.\n");
    text_return.concatf(
      "marker_sch_0:     %5d\t%5u\t%5u\n",
      schedule_test_0.recurrence(),
      marker_sch_0,
      schedule_test_0.profiler.executions()
    );
    text_return.concatf(
      "marker_sch_1:     %5d\t%5u\t%5u\n",
      schedule_test_1.recurrence(),
      marker_sch_1,
      schedule_test_1.profiler.executions()
    );
  }
  scheduler->printDebug(&text_return);
  printf("%s\n", text_return.string());
  return ret;
}


/*
* Drive the scheduler forward for the defined number of microseconds.
*/
int test_scheduler_spin_1_to_1(const unsigned int us_to_spin) {
  unsigned int entry_time = micros();
  while (us_to_spin > (micros() - entry_time)) {
    sleep_us(scheduler_period);
    scheduler->advanceScheduler();
    scheduler->serviceSchedules();
  }

  unsigned int local_slip = strict_abs_delta((uint64_t) micros_since(entry_time), (uint64_t) us_to_spin);
  scheduler_slip = strict_max(scheduler_slip, local_slip);
  StringBuilder text_return;
  text_return.concatf("Local timing slip: %u\n", local_slip);
  printf("%s\n", text_return.string());
  return test_scheduler_run_count_checks();
}

/*
* Drive the scheduler forward for the defined number of microseconds, but there
*   is not a 1-to-1 relationship between calls to advance and service functions.
*/
int test_scheduler_spin_n_to_1(const unsigned int us_to_spin) {
  int ret = 0;
  unsigned int entry_time = micros();
  while (us_to_spin > (micros() - entry_time)) {
    sleep_us(scheduler_period);
    if (randomUInt32() & 1) {
      scheduler->advanceScheduler();
    }
    scheduler->serviceSchedules();
  }
  unsigned int local_slip = strict_abs_delta((uint64_t) micros_since(entry_time), (uint64_t) us_to_spin);
  scheduler_slip = strict_max(scheduler_slip, local_slip);
  StringBuilder text_return;
  text_return.concatf("Local timing slip: %u\n", local_slip);
  printf("%s\n", text_return.string());
  return test_scheduler_run_count_checks();
}


int test_scheduler_schedule_removal() {
  int ret = 0;
  // TODO
  return ret;
}


void print_types_scheduler() {
  printf("\tC3PScheduler          %u\t%u\n", sizeof(C3PScheduler), alignof(C3PScheduler));
  printf("\tC3PScheduledLambda    %u\t%u\n", sizeof(C3PScheduledLambda),   alignof(C3PScheduledLambda));
}



/*******************************************************************************
* Scheduler main function.
*******************************************************************************/
int scheduler_tests_main() {
  int ret = 1;   // Failure is the default result.
  const char* const MODULE_NAME = "C3PScheduler";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == test_scheduler_init()) {
    if (0 == test_scheduler_initial_conditions()) {
      if (0 == test_scheduler_spin_1_to_1(3000000)) {
        if (0 == test_scheduler_spin_n_to_1(3000000)) {
          if (0 == test_scheduler_schedule_removal()) {
            printf("**********************************\n");
            printf("*  C3PScheduler tests all pass   *\n");
            printf("**********************************\n");
            ret = 0;
          }
          else printTestFailure(MODULE_NAME, "Schedule removal.");
        }
        else printTestFailure(MODULE_NAME, "Schedules did not execute as expected (n-to-1).");
      }
      else printTestFailure(MODULE_NAME, "Schedules did not execute as expected (1-to-1).");
    }
    else printTestFailure(MODULE_NAME, "Scheduler initial conditions failed to evolve into working state.");
  }
  else printTestFailure(MODULE_NAME, "Scheduler failed to initialize.");

  return ret;
}
