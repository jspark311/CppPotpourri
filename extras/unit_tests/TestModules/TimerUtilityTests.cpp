/*
File:   TimerUtilityTests.cpp
Author: J. Ian Lindsay
Date:   2016.09.20

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


This program runs tests against our timer-related utilities.
*/

#include "TimerTools/C3PTrace.h"

/*******************************************************************************
* Globals
*******************************************************************************/

StopWatch stopwatch_0;
StopWatch stopwatch_1;
StopWatch stopwatch_test;


void printStopWatches() {
  StringBuilder out;
  StopWatch::printDebugHeader(&out);
  stopwatch_0.printDebug("PeriodicTimeout", &out);
  stopwatch_1.printDebug("StopWatch", &out);
  stopwatch_test.printDebug("StopWatch", &out);
  printf("%s\n\n", (const char*) out.string());
}


/*
* PeriodicTimeout is an interface class to the system timers via millis() and
*   micros(). The relevant calls are wrapped into the two child classes.
*/
int test_PeriodicTimeout() {
  stopwatch_0.markStart();
  int test_state = 0;
  while ((test_state >= 0) & (test_state < 2)) {
    bool local_pass = false;
    char* test_type_string = (char*) "";
    PeriodicTimeout* timeout_obj = nullptr;
    unsigned long timeout_period = 0;

    switch (test_state) {
      case 0:
        test_type_string = (char*) "MillisTimeout";
        timeout_period = (20 + (randomUInt32() % 80));
        timeout_obj = new MillisTimeout(timeout_period);
        break;
      case 1:
        test_type_string = (char*) "MicrosTimeout";
        timeout_period = (11804 + (randomUInt32() % 10000));
        timeout_obj = new MicrosTimeout(timeout_period);
        timeout_obj->reset();
        break;
      default:  return -1;
    }
    printf("Testing PeriodicTimeout (%s)...\n", test_type_string);
    printf("\tThe constructor parameter (%lu) was recorded as the period... ", timeout_period);
    if (timeout_obj->period() == timeout_period) {
      printf("Pass.\n\texpired() should return false... ");
      if (!timeout_obj->expired()) {
        printf("Pass.\n\tenabled() should return true... ");
        if (timeout_obj->enabled()) {
          unsigned long time_remaining = timeout_obj->remaining();
          printf("Pass.\n\tremaining() should be less-than or equal-to period (%lu <= %lu)... ", time_remaining, timeout_obj->period());
          if (time_remaining <= timeout_obj->period()) {
            printf("Pass.\n\tsleeping to pass the time... ");
            switch (test_state) {
              case 0:  sleep_ms(timeout_period + 1);    break;
              case 1:  sleep_us(timeout_period + 1);    break;
            }
            time_remaining = timeout_obj->remaining();
            printf("Done.\n\tremaining() should be zero (0 == %lu)... ", time_remaining);
            if (0 == time_remaining) {
              printf("Pass.\n\texpired() should now return true... ");
              if (timeout_obj->expired()) {
                printf("Pass.\n\treset() works... ");
                timeout_obj->reset();
                if (timeout_obj->period() >= timeout_obj->remaining()) {
                  if (!timeout_obj->expired()) {
                    printf("Pass.\n\treset(0) results in a disabled timer... ");
                    timeout_obj->reset(0);
                    if (!timeout_obj->enabled()) {
                      printf("Pass.\n\tA disabled timer reads as expired... ");
                      if (timeout_obj->expired()) {
                        printf("Pass.\n\tremaining() should return zero for a disabled timer... ");
                        if (0 == timeout_obj->remaining()) {
                          printf("Pass.\n\t%s passes all tests.\n", test_type_string);
                          local_pass = true;
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
    delete timeout_obj;
    timeout_obj = nullptr;

    if (!local_pass) {  test_state = -1;  }  // Abort the variant loop if the test didn't pass.
    else {              test_state++;     }
  }
  stopwatch_0.markStop();

  if (2 != test_state) {
    printf("Fail.\n");
    return -1;
  }
  return 0;
}



/*
* StopWatch is used to profile a single code pathway.
*/
int test_StopWatch() {
  stopwatch_1.markStart();
  int ret = 0;

  stopwatch_test.reset();
  // TODO

  stopwatch_1.markStop();
  printStopWatches();
  return ret;
}



/*
* C3PTrace is used to build timing profiles within live programs.
*/
int test_c3ptrace_basics() {
  int ret = 0;
  return ret;
}


void print_types_timer_utils() {
  printf("\tStopWatch                %u\t%u\n", sizeof(StopWatch),       alignof(StopWatch));
  printf("\tC3PTrace                 %u\t%u\n", sizeof(C3PTrace),        alignof(C3PTrace));
  printf("\tTracePath                %u\t%u\n", sizeof(TracePath),       alignof(TracePath));
  printf("\tTracePoint               %u\t%u\n", sizeof(TracePoint),      alignof(TracePoint));
  printf("\tPeriodicTimeout          %u\t%u\n", sizeof(PeriodicTimeout), alignof(PeriodicTimeout));
  printf("\tMicrosTimeout            %u\t%u\n", sizeof(MicrosTimeout),   alignof(MicrosTimeout));
  printf("\tMillisTimeout            %u\t%u\n", sizeof(MillisTimeout),   alignof(MillisTimeout));
}


/*******************************************************************************
* Test plan
*******************************************************************************/
#define CHKLST_TIMER_UTIL_TEST_TIMEOUT      0x00000001  //
#define CHKLST_TIMER_UTIL_TEST_STOPWATCH    0x00000002  //
#define CHKLST_TIMER_UTIL_TEST_TRACE_BASIC  0x00000004  //

#define CHKLST_TIMER_UTIL_TESTS_ALL ( \
  CHKLST_TIMER_UTIL_TEST_TIMEOUT | CHKLST_TIMER_UTIL_TEST_STOPWATCH | \
  CHKLST_TIMER_UTIL_TEST_TRACE_BASIC)


const StepSequenceList TOP_LEVEL_TIMER_UTIL_TEST_LIST[] = {
  { .FLAG         = CHKLST_TIMER_UTIL_TEST_TIMEOUT,
    .LABEL        = "PeriodicTimeout",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_PeriodicTimeout()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMER_UTIL_TEST_STOPWATCH,
    .LABEL        = "StopWatch",
    .DEP_MASK     = (CHKLST_TIMER_UTIL_TEST_TIMEOUT),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_StopWatch()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMER_UTIL_TEST_TRACE_BASIC,
    .LABEL        = "C3PTrace",
    .DEP_MASK     = (CHKLST_TIMER_UTIL_TEST_STOPWATCH),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_c3ptrace_basics()) ? 1:-1);  }
  },
};

AsyncSequencer timer_util_test_plan(TOP_LEVEL_TIMER_UTIL_TEST_LIST, (sizeof(TOP_LEVEL_TIMER_UTIL_TEST_LIST) / sizeof(TOP_LEVEL_TIMER_UTIL_TEST_LIST[0])));



/*******************************************************************************
* The main function
*******************************************************************************/

int timer_utilities_main() {
  const char* const MODULE_NAME = "Timer Utils";
  printf("===< %s >=======================================\n", MODULE_NAME);

  timer_util_test_plan.requestSteps(CHKLST_TIMER_UTIL_TESTS_ALL);
  while (!timer_util_test_plan.request_completed() && (0 == timer_util_test_plan.failed_steps(false))) {
    timer_util_test_plan.poll();
  }
  int ret = (timer_util_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  timer_util_test_plan.printDebug(&report_output, "Timer Utils test report");
  printf("%s\n", (char*) report_output.string());

  return ret;
}
