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


/*******************************************************************************
* Globals
*******************************************************************************/

StopWatch stopwatch_0;
StopWatch stopwatch_1;
StopWatch stopwatch_2;
StopWatch stopwatch_3;
StopWatch stopwatch_99;


void printStopWatches() {
  StringBuilder out;
  StopWatch::printDebugHeader(&out);
  stopwatch_0.printDebug("UUID", &out);
  stopwatch_1.printDebug("Vector", &out);
  stopwatch_99.printDebug("UNUSED", &out);
  printf("%s\n\n", (const char*) out.string());
}



int test_PeriodicTimeout() {
  int test_state = 0;
  while ((test_state >= 0) & (test_state < 2)) {
    char* test_type_string = (char*) "";
    PeriodicTimeout* timeout_obj = nullptr;
    unsigned int timeout_period = 0;

    switch (test_state++) {
      case 0:
        test_type_string = (char*) "MillisTimeout";
        timeout_period = (20 + (randomUInt32() % 80));
        timeout_obj = new MillisTimeout(timeout_period);
        break;
      case 1:
        test_type_string = (char*) "MicrosTimeout";
        timeout_period = (11804 + (randomUInt32() % 10000));
        timeout_obj = new MillisTimeout(timeout_period);
        break;
      default:  return -1;
    }
    printf("Testing PeriodicTimeout (%s)...\n", test_type_string);
  }
  return ((2 == test_state) ? 0 : -1);
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
  stopwatch_0.markStart();
  if (0 == test_PeriodicTimeout()) {
    stopwatch_0.markStop();
    ret = 0;
  }
  else printTestFailure(MODULE_NAME, "PeriodicTimeout");

  printStopWatches();
  return ret;
}
