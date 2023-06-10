/*
File:   SensorFilterTests.cpp
Author: J. Ian Lindsay
Date:   2023.06.09

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

#include "AbstractPlatform.h"
#include "SensorFilter.h"

/*******************************************************************************
* SensorFilter globals
*******************************************************************************/
#define TEST_FILTER_DEPTH      128

SensorFilter<uint32_t> filt_test_0_m(TEST_FILTER_DEPTH, FilteringStrategy::RAW);
SensorFilter<uint32_t> filt_test_0_0(TEST_FILTER_DEPTH, FilteringStrategy::RAW);
SensorFilter<uint32_t> filt_test_0_1(TEST_FILTER_DEPTH, FilteringStrategy::RAW);
SensorFilter<int32_t>  filt_test_1_m(TEST_FILTER_DEPTH, FilteringStrategy::RAW);
SensorFilter<int32_t>  filt_test_1_0(TEST_FILTER_DEPTH, FilteringStrategy::RAW);
SensorFilter<int32_t>  filt_test_1_1(TEST_FILTER_DEPTH, FilteringStrategy::RAW);
SensorFilter<float>    filt_test_2_m(TEST_FILTER_DEPTH, FilteringStrategy::RAW);
SensorFilter<float>    filt_test_2_0(TEST_FILTER_DEPTH, FilteringStrategy::RAW);
SensorFilter<float>    filt_test_2_1(TEST_FILTER_DEPTH, FilteringStrategy::RAW);

SensorFilter<float>    filt_stats_test_0(TEST_FILTER_DEPTH, FilteringStrategy::RAW);
SensorFilter<float>    filt_stats_test_1(TEST_FILTER_DEPTH, FilteringStrategy::RAW);

// These values are hand-verified test cases to check stats operation.
// TODO

/*******************************************************************************
* Scheduler test routines
*******************************************************************************/
/*
*
*/
int sensor_filter_init() {
  int ret = filt_stats_test_0.init();
  if (0 == ret) {     ret = filt_stats_test_1.init();    }
  if (0 == ret) {     ret = filt_test_0_m.init();        }
  if (0 == ret) {     ret = filt_test_0_0.init();        }
  if (0 == ret) {     ret = filt_test_0_1.init();        }
  if (0 == ret) {     ret = filt_test_1_m.init();        }
  if (0 == ret) {     ret = filt_test_1_0.init();        }
  if (0 == ret) {     ret = filt_test_1_1.init();        }
  if (0 == ret) {     ret = filt_test_2_m.init();        }
  if (0 == ret) {     ret = filt_test_2_0.init();        }
  if (0 == ret) {     ret = filt_test_2_1.init();        }
  if (0 != ret) {
    printf("SensorFilter::init() returns (%d).\n", ret);
  }
  return ret;
}


/*
*
*/
int sensor_filter_initial_conditions() {
  int ret = 0;
  // Build the master versions of the objects that we are going to use.
  for (int i = 0; i < TEST_FILTER_DEPTH; i++) {
    bool feed_failure = false;
    const int32_t TVAL_A = (int32_t) randomUInt32();
    const int32_t TVAL_B = (int32_t) randomUInt32();
    const float   TVAL_0 = (float) ((double) strict_max(TVAL_A, TVAL_B), (double) strict_min(TVAL_A, TVAL_B));
    feed_failure |= (0 > filt_test_0_m.feedFilter(randomUInt32()));
    feed_failure |= (0 > filt_test_1_m.feedFilter(randomUInt32()));
    feed_failure |= (0 > filt_test_2_m.feedFilter(TVAL_0));
    if (feed_failure) {
      printf("SensorFilter failed to feed at index %d.", i);
      return -1;
    }
  }

  if (0 == ret) {
    bool windows_not_full = false;
    windows_not_full |= !filt_test_0_m.windowFull();
    windows_not_full |= !filt_test_1_m.windowFull();
    windows_not_full |= !filt_test_2_m.windowFull();
    ret = (windows_not_full ? -2 : 0);
  }
  else printf("SensorFilter failed to set initial conditions.");

  return ret;
}


/*
*
*/
int sensor_filter_stats_tests() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int sensor_filter_rewindowing() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int sensor_filter_nominal_operation_0() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int sensor_filter_nominal_operation_1() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int sensor_filter_nominal_operation_2() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int sensor_filter_teardown() {
  int ret = 0;
  // TODO
  return ret;
}



/*******************************************************************************
* SensorFilter main function.
*******************************************************************************/
int sensor_filter_tests_main() {
  int ret = 1;   // Failure is the default result.
  if (0 == sensor_filter_init()) {
    if (0 == sensor_filter_initial_conditions()) {
      if (0 == sensor_filter_stats_tests()) {
        if (0 == sensor_filter_rewindowing()) {
          if (0 == sensor_filter_nominal_operation_0()) {
            if (0 == sensor_filter_nominal_operation_1()) {
              if (0 == sensor_filter_nominal_operation_2()) {
                if (0 == sensor_filter_teardown()) {
                  printf("**********************************\n");
                  printf("*  SensorFilter tests all pass   *\n");
                  printf("**********************************\n");
                  ret = 0;
                }
                else printTestFailure("SensorFilter failed teardown.");
              }
              else printTestFailure("SensorFilter failed nominal operations battery-2.");
            }
            else printTestFailure("SensorFilter failed nominal operations battery-1.");
          }
          else printTestFailure("SensorFilter failed nominal operations battery-0.");
        }
        else printTestFailure("SensorFilter failed re-windowing.");
      }
      else printTestFailure("SensorFilter failed stats test.");
    }
    else printTestFailure("SensorFilter failed to fill with test state.");
  }
  else printTestFailure("SensorFilter failed to initialize.");

  return ret;
}
