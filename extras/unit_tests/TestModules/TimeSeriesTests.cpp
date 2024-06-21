/*
File:   TimeSeriesTests.cpp
Author: J. Ian Lindsay
Date:   2024.01.27

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
#include "TimeSeries/TimeSeries.h"

/*******************************************************************************
* TimeSeries globals
*******************************************************************************/
#define TEST_FILTER_DEPTH      128

TimeSeries<uint32_t> series_test_0_m(TEST_FILTER_DEPTH);
TimeSeries<uint32_t> series_test_0_0(TEST_FILTER_DEPTH);
TimeSeries<uint32_t> series_test_0_1(TEST_FILTER_DEPTH);
TimeSeries<int32_t>  series_test_1_m(TEST_FILTER_DEPTH);
TimeSeries<int32_t>  series_test_1_0(TEST_FILTER_DEPTH);
TimeSeries<int32_t>  series_test_1_1(TEST_FILTER_DEPTH);
TimeSeries<float>    series_test_2_m(TEST_FILTER_DEPTH);
TimeSeries<float>    series_test_2_0(TEST_FILTER_DEPTH);
TimeSeries<float>    series_test_2_1(TEST_FILTER_DEPTH);

// These values are knwon-answer test cases to check stats operation.
TimeSeries<int32_t> series_stats_test_0(TEST_FILTER_DEPTH);
TimeSeries<int32_t> series_stats_test_1(TEST_FILTER_DEPTH);

const int32_t KAT_SERIES_0_MIN   = -126000;
const int32_t KAT_SERIES_0_MAX   = 127000;
const double  KAT_SERIES_0_MEAN  = 0.0;
const double  KAT_SERIES_0_STDEV = 0.0;
const double  KAT_SERIES_0_SNR   = 0.0;

const int32_t KAT_SERIES_1_MIN   = 0;
const int32_t KAT_SERIES_1_MAX   = 0;
const double  KAT_SERIES_1_MEAN  = 0.0;
const double  KAT_SERIES_1_STDEV = 0.0;
const double  KAT_SERIES_1_SNR   = 0.0;


/*******************************************************************************
* Scheduler test routines
*******************************************************************************/
/*
*
*/
int timeseries_init() {
  int ret = 0;

  if (0 == ret) {   ret = series_stats_test_0.name((char*) "stats_0");   }
  if (0 == ret) {   ret = series_stats_test_1.name((char*) "stats_1");   }

  if (0 == ret) {   ret = series_stats_test_0.init();    }
  if (0 == ret) {   ret = series_stats_test_1.init();    }
  if (0 == ret) {   ret = series_test_0_m.init();        }
  if (0 == ret) {   ret = series_test_0_0.init();        }
  if (0 == ret) {   ret = series_test_0_1.init();        }
  if (0 == ret) {   ret = series_test_1_m.init();        }
  if (0 == ret) {   ret = series_test_1_0.init();        }
  if (0 == ret) {   ret = series_test_1_1.init();        }
  if (0 == ret) {   ret = series_test_2_m.init();        }
  if (0 == ret) {   ret = series_test_2_0.init();        }
  if (0 == ret) {   ret = series_test_2_1.init();        }
  if (0 != ret) {
    printf("TimeSeries::init() returns (%d).\n", ret);
  }
  return ret;
}


/*
*
*/
int timeseries_initial_conditions() {
  int ret = 0;
  // Build the master versions of the objects that we are going to use.
  printf("TimeSeries setting up initial conditions...\n");
  for (int i = 0; i < TEST_FILTER_DEPTH; i++) {
    if ((0 > series_test_0_m.feedSeries(randomUInt32()))) {
      printf("TimeSeries failed to series_test_0_m.feedSeries() at index %d.", i);
      return -1;
    }
    if ((0 > series_test_1_m.feedSeries(randomUInt32()))) {
      printf("TimeSeries failed to series_test_1_m.feedSeries() at index %d.", i);
      return -1;
    }
    if ((0 > series_test_2_m.feedSeries(generate_random_float()))) {
      printf("TimeSeries failed to series_test_2_m.feedSeries() at index %d.", i);
      return -1;
    }

    bool feed_failure = false;
    // For the stats filters, we build a test pattern and send it through a
    //   filter configured for each mode we care to test.
    const int32_t TVAL_C = (int32_t) (1000*i*((i%2)?1:-1));
    feed_failure |= (0 > series_stats_test_0.feedSeries(TVAL_C));
    feed_failure |= (0 > series_stats_test_1.feedSeries(TVAL_C));

    if (feed_failure) {
      printf("TimeSeries failed to feed stats timeseries at index %d.", i);
      return -1;
    }
  }

  if (0 == ret) {
    bool windows_not_full = false;
    printf("\tAll test objects have full windows...\n");
    windows_not_full |= !series_test_0_m.windowFull();
    windows_not_full |= !series_test_1_m.windowFull();
    windows_not_full |= !series_test_2_m.windowFull();
    ret = (windows_not_full ? -2 : 0);
  }
  else printf("TimeSeries failed to set initial conditions.");

  return ret;
}


/*
* TODO
*/
int timeseries_stats_tests() {
  int ret = -1;
  StringBuilder output;
  series_stats_test_0.printSeries(&output);  // This should force stats calculation.
  series_stats_test_1.printSeries(&output);  // This should force stats calculation.
  printf("%s\n", output.string());
  if (series_stats_test_0.minValue() == KAT_SERIES_0_MIN) {
    if (series_stats_test_0.maxValue() == KAT_SERIES_0_MAX) {
      ret = 0;
    }
  }
  return ret;
}


/*
*
*/
int timeseries_rewindowing() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int timeseries_nominal_operation_0() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int timeseries_nominal_operation_1() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int timeseries_nominal_operation_2() {
  int ret = 0;
  // TODO
  return ret;
}


/*
* Test the transfer of an entire package of timeseries data all at once.
*/
int timeseries_data_sharing_0() {
  int ret = -1;
  // Serialize the source.
  StringBuilder serialized;
  if (0 == series_stats_test_0.serialize(&serialized, TCode::CBOR)) {
    // StringBuilder txt_output;
    // serialized.printDebug(&txt_output);
    // printf("%s\n", txt_output.string());
    // Deserialize into the target.
    TimeSeries<int32_t> filt_copy_test(0);
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
int timeseries_data_sharing_1() {
  int ret = 0;
  // TODO
  return ret;
}


/*
*
*/
int timeseries_teardown() {
  int ret = 0;
  // TODO
  return ret;
}




void print_types_timeseries() {
  printf("\tTimeSeries<uint8_t>    %u\t%u\n", sizeof(TimeSeries<uint8_t>), alignof(TimeSeries<uint8_t>));
  printf("\tTimeSeries<int32_t>    %u\t%u\n", sizeof(TimeSeries<int32_t>), alignof(TimeSeries<int32_t>));
  printf("\tTimeSeries<float>      %u\t%u\n", sizeof(TimeSeries<float>),   alignof(TimeSeries<float>));
  printf("\tTimeSeries<double>     %u\t%u\n", sizeof(TimeSeries<double>),  alignof(TimeSeries<double>));
}



/*******************************************************************************
* TimeSeries main function.
*******************************************************************************/
int timeseries_tests_main() {
  int ret = 1;   // Failure is the default result.
  const char* const MODULE_NAME = "TimeSeries";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == timeseries_init()) {
    if (0 == timeseries_initial_conditions()) {
      if (0 == timeseries_stats_tests()) {
        if (0 == timeseries_rewindowing()) {
          if (0 == timeseries_nominal_operation_0()) {
            if (0 == timeseries_nominal_operation_1()) {
              if (0 == timeseries_nominal_operation_2()) {
                if (0 == timeseries_data_sharing_0()) {
                  if (0 == timeseries_data_sharing_1()) {
                    if (0 == timeseries_teardown()) {
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
