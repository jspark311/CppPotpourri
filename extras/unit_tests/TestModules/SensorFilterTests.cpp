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

// These values are knwon-answer test cases to check stats operation.
SensorFilter<int32_t> filt_stats_test_0(TEST_FILTER_DEPTH, FilteringStrategy::RAW);
SensorFilter<int32_t> filt_stats_test_1(TEST_FILTER_DEPTH, FilteringStrategy::MOVING_AVG);
const int32_t KAT_FILT_0_MIN   = -126000;
const int32_t KAT_FILT_0_MAX   = 127000;
const double  KAT_FILT_0_MEAN  = 0.0;
const double  KAT_FILT_0_STDEV = 0.0;
const double  KAT_FILT_0_SNR   = 0.0;

const int32_t KAT_FILT_1_MIN   = 0;
const int32_t KAT_FILT_1_MAX   = 0;
const double  KAT_FILT_1_MEAN  = 0.0;
const double  KAT_FILT_1_STDEV = 0.0;
const double  KAT_FILT_1_SNR   = 0.0;

/*******************************************************************************
* Scheduler test routines
*******************************************************************************/
/*
*
*/
int sensor_filter_init() {
  int ret = 0;

  if (0 == ret) {   ret = filt_stats_test_0.name((char*) "stats_0");   }
  if (0 == ret) {   ret = filt_stats_test_1.name((char*) "stats_1");   }

  if (0 == ret) {   ret = filt_stats_test_0.init();    }
  if (0 == ret) {   ret = filt_stats_test_1.init();    }
  if (0 == ret) {   ret = filt_test_0_m.init();        }
  if (0 == ret) {   ret = filt_test_0_0.init();        }
  if (0 == ret) {   ret = filt_test_0_1.init();        }
  if (0 == ret) {   ret = filt_test_1_m.init();        }
  if (0 == ret) {   ret = filt_test_1_0.init();        }
  if (0 == ret) {   ret = filt_test_1_1.init();        }
  if (0 == ret) {   ret = filt_test_2_m.init();        }
  if (0 == ret) {   ret = filt_test_2_0.init();        }
  if (0 == ret) {   ret = filt_test_2_1.init();        }
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

    // For the stats filters, we build a test pattern and send it through a
    //   filter configured for each mode we care to test.
    const int32_t TVAL_C = (int32_t) (1000*i*((i%2)?1:-1));
    feed_failure |= (0 > filt_stats_test_0.feedFilter(TVAL_C));
    feed_failure |= (0 > filt_stats_test_1.feedFilter(TVAL_C));

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
* TODO
*/
int sensor_filter_stats_tests() {
  int ret = -1;
  StringBuilder output;
  filt_stats_test_0.printFilter(&output);  // This should force stats calculation.
  filt_stats_test_1.printFilter(&output);  // This should force stats calculation.
  printf("%s\n", output.string());
  if (filt_stats_test_0.minValue() == KAT_FILT_0_MIN) {
    if (filt_stats_test_0.maxValue() == KAT_FILT_0_MAX) {
      ret = 0;
    }
  }
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
* Test the transfer of an entire package of timeseries data all at once.
*/
int sensor_filter_data_sharing_0() {
  int ret = -1;
  // Serialize the source.
  StringBuilder serialized;
  if (0 == filt_stats_test_0.serialize(&serialized, TCode::CBOR)) {
    // StringBuilder txt_output;
    // serialized.printDebug(&txt_output);
    // printf("%s\n", txt_output.string());
    // Deserialize into the target.
    SensorFilter<int32_t> filt_copy_test(0, FilteringStrategy::RAW);
    if (0 == filt_copy_test.deserialize(&serialized, TCode::CBOR)) {
      ret = 0;
    }
    // TODO: test for equality.
  }
  ret = 0;  // TODO: Wrong
  return ret;
}


/*
*
*/
int sensor_filter_data_sharing_1() {
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




void print_types_sensorfilter() {
  printf("\tSensorFilter<uint8_t>    %u\t%u\n", sizeof(SensorFilter<uint8_t>), alignof(SensorFilter<uint8_t>));
  printf("\tSensorFilter<int32_t>    %u\t%u\n", sizeof(SensorFilter<int32_t>), alignof(SensorFilter<int32_t>));
  printf("\tSensorFilter<float>      %u\t%u\n", sizeof(SensorFilter<float>),   alignof(SensorFilter<float>));
  printf("\tSensorFilter<double>     %u\t%u\n", sizeof(SensorFilter<double>),  alignof(SensorFilter<double>));
}



/*******************************************************************************
* SensorFilter main function.
*******************************************************************************/
int sensor_filter_tests_main() {
  int ret = 1;   // Failure is the default result.
  const char* const MODULE_NAME = "SensorFilter";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == sensor_filter_init()) {
    if (0 == sensor_filter_initial_conditions()) {
      if (0 == sensor_filter_stats_tests()) {
        if (0 == sensor_filter_rewindowing()) {
          if (0 == sensor_filter_nominal_operation_0()) {
            if (0 == sensor_filter_nominal_operation_1()) {
              if (0 == sensor_filter_nominal_operation_2()) {
                if (0 == sensor_filter_data_sharing_0()) {
                  if (0 == sensor_filter_data_sharing_1()) {
                    if (0 == sensor_filter_teardown()) {
                      ret = 0;
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

  return ret;
}
