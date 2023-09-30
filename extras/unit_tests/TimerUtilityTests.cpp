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

typedef void (*sleepFunction)(uint32_t);


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
        //sleepFunction = sleep_ms;
        break;
      case 1:
        test_type_string = (char*) "MicrosTimeout";
        timeout_period = (11804 + (randomUInt32() % 10000));
        timeout_obj = new MicrosTimeout(timeout_period);
        timeout_obj->reset();
        //sleepFunction = sleep_us;
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



int test_StopWatch() {
  stopwatch_1.markStart();
  int ret = 0;

  stopwatch_test.reset();
  // TODO

  stopwatch_1.markStop();
  printStopWatches();
  return ret;
}


void print_types_timer_utils() {
  printf("\tStopWatch                %u\t%u\n", sizeof(StopWatch),       alignof(StopWatch));
  printf("\tPeriodicTimeout          %u\t%u\n", sizeof(PeriodicTimeout), alignof(PeriodicTimeout));
  printf("\tMicrosTimeout            %u\t%u\n", sizeof(MicrosTimeout),   alignof(MicrosTimeout));
  printf("\tMillisTimeout            %u\t%u\n", sizeof(MillisTimeout),   alignof(MillisTimeout));
}


/*******************************************************************************
* The main functions.
*******************************************************************************/


int timer_utilities_main() {
  int ret = 1;   // Failure is the default result.
  const char* const MODULE_NAME = "Timer Utils";
  printf("===< %s >=======================================\n", MODULE_NAME);
  if (0 == test_PeriodicTimeout()) {
    if (0 == test_StopWatch()) {
      ret = 0;
    }
  }
  else printTestFailure(MODULE_NAME, "PeriodicTimeout");


  return ret;
}
