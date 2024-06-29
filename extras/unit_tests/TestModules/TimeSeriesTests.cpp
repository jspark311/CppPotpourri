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

void dump_timeseries(TimeSeriesBase*);


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

SIUnit UNIT_STR[] = { SIUnit::SECONDS, SIUnit::UNITLESS};

/*******************************************************************************
* Scheduler test routines
*******************************************************************************/
/*
*
*/
int timeseries_init() {
  int ret = 0;
  //SIUnit UNIT_STR[] = { SIUnit::UNIT_GRAMMAR_MARKER,
  //  SIUnit::META_ORDER_OF_MAGNITUDE, (SIUnit) -6,
  //  SIUnit::SECONDS, SIUnit::UNITLESS
  //};

  if (0 == ret) {   ret = series_stats_test_0.name((char*) "stats_0");   }
  if (0 == ret) {   ret = series_stats_test_1.name((char*) "stats_1");   }
  if (0 == ret) {   ret = series_stats_test_0.units(UNIT_STR);   }
  if (0 == ret) {   ret = series_stats_test_1.units(UNIT_STR);   }

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
* This tests the class under its most-likely conditions: one-by-one addition of
*   new data as it arrives from a fairly slow source.
*/
int timeseries_nominal_operation_0() {
  const uint32_t TEST_SAMPLE_COUNT = (91 + (randomUInt32() % 23));
  int ret = -1;
  printf("Testing normal operation (sequential) with a sample count of %u...\n", TEST_SAMPLE_COUNT);
  printf("\tCreating test object... ");

  uint32_t input_values[TEST_SAMPLE_COUNT];
  uint32_t stored_values[TEST_SAMPLE_COUNT];
  TimeSeries<uint32_t> series_0(TEST_SAMPLE_COUNT);
  series_0.name((char*) "series_0");
  series_0.units(UNIT_STR);
  series_0.init();
  for (uint32_t i = 0; i < TEST_SAMPLE_COUNT; i++) {
    input_values[i]  = randomUInt32();
    stored_values[i] = 0;
  }
  if (0 == series_0.init()) {
    printf("Pass.\n\tAdding half of the samples... ");
    const uint32_t PARTIAL_WINDOW_COUNT = (TEST_SAMPLE_COUNT >> 1);
    uint32_t true_sample_count = 0;
    ret = 0;
    while ((0 == ret) & (true_sample_count < PARTIAL_WINDOW_COUNT)) {
      if (0 > series_0.feedSeries(input_values[true_sample_count])) {
        ret = -1;
      }
      else {
        true_sample_count++;
      }
    }
    if (0 == ret) {
      ret--;
      printf("Pass.\n\tSeries indicates the correct sample count... ");
      const bool COUNT_MATCH_0 = (series_0.totalSamples() == PARTIAL_WINDOW_COUNT);
      const bool COUNT_MATCH_1 = (PARTIAL_WINDOW_COUNT == true_sample_count);
      if (COUNT_MATCH_0 & COUNT_MATCH_1) {
        printf("Pass.\n\tSeries indicates dirty... ");
        if (series_0.dirty()) {
          printf("Pass.\n\tCalling markClean() clears the dirty condition... ");
          series_0.markClean();
          if (!series_0.dirty()) {
            printf("Pass.\n\tSeries does not indicate a full window... ");
            if (!series_0.windowFull()) {
              const uint32_t REMAINING_WINDOW_COUNT = (TEST_SAMPLE_COUNT - PARTIAL_WINDOW_COUNT);
              printf("Pass.\n\tAdding the remaining %u samples exactly fills the window... ", REMAINING_WINDOW_COUNT);
              ret = 0;
              while ((0 == ret) & (true_sample_count < TEST_SAMPLE_COUNT)) {
                if (0 > series_0.feedSeries(input_values[true_sample_count])) {
                  ret = -1;
                }
                else {
                  true_sample_count++;
                }
                if (series_0.windowFull() & (true_sample_count < TEST_SAMPLE_COUNT)) {
                  ret = -1;
                }
              }
              if (0 == ret) {
                ret--;
                printf("Pass.\n\tThe window is full, and the series is dirty... ");
                const bool COUNT_MATCH_2 = (series_0.totalSamples() == TEST_SAMPLE_COUNT);
                if (COUNT_MATCH_2 & series_0.windowFull() & series_0.dirty()) {
                  printf("Pass.\n\tThe data can be read back in bulk... ");
                  // These out to come out in proper order.
                  if (0 == series_0.copyValues(stored_values, TEST_SAMPLE_COUNT)) {
                    printf("Pass.\n\tThe series is no longer dirty... ");
                    if (!series_0.dirty()) {
                      printf("Pass.\n\tThe data is properly recorded... ");
                      ret = 0;  // The last test. If everything matches, the group passes.
                      for (uint32_t i = 0; i < TEST_SAMPLE_COUNT; i++) {
                        if (input_values[i] != stored_values[i]) {
                          ret = -1;
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
  }

  printf("%s.\n", ((0 != ret) ? "Fail" : "PASS"));
  if (0 != ret) {
    dump_timeseries(&series_0);
    for (uint32_t i = 0; i < TEST_SAMPLE_COUNT; i++) {
      printf("%5u ", input_values[i]);
      if ((i & 0x07) == 7) printf("\n");
    }
    printf("\n\n");
    for (uint32_t i = 0; i < TEST_SAMPLE_COUNT; i++) {
      printf("%5u ", stored_values[i]);
      if ((i & 0x07) == 7) printf("\n");
    }
    printf("\n");
  }
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
* Test cases for foreseeable API abuse.
*/
int timeseries_test_abuse() {
  int ret = 0;
  // TODO
  return ret;
}


/*
* Test the transfer of an entire package of timeseries data all at once.
*/
int timeseries_test_parse_pack() {
  int ret = -1;
  // Serialize the source.
  StringBuilder serialized;
  if (0 == series_stats_test_0.serialize(&serialized, TCode::CBOR)) {
    // StringBuilder txt_output;
    // serialized.printDebug(&txt_output);
    // printf("%s\n", txt_output.string());
    // Deserialize into the target.
    //TimeSeries<int32_t> filt_copy_test(0);
    //if (0 == filt_copy_test.deserialize(&serialized, TCode::CBOR)) {
    //  ret = 0;
    //}
    // TODO: test for equality.
  }
  ret = 0;  // TODO: Wrong
  return ret;
}


/*
*
*/
int timeseries_data_sharing() {
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
  printf("\tTimeSeries3<uint8_t>   %u\t%u\n", sizeof(TimeSeries3<uint8_t>), alignof(TimeSeries3<uint8_t>));
  printf("\tTimeSeries3<int32_t>   %u\t%u\n", sizeof(TimeSeries3<int32_t>), alignof(TimeSeries3<int32_t>));
  printf("\tTimeSeries3<float>     %u\t%u\n", sizeof(TimeSeries3<float>),   alignof(TimeSeries3<float>));
  printf("\tTimeSeries3<double>    %u\t%u\n", sizeof(TimeSeries3<double>),  alignof(TimeSeries3<double>));
}


/*******************************************************************************
* Test plan
*******************************************************************************/
#define CHKLST_TIMESERIES_TEST_CONSTRUCTION   0x00000001  //
#define CHKLST_TIMESERIES_TEST_INITIAL_COND   0x00000002  //
#define CHKLST_TIMESERIES_TEST_STATS          0x00000004  //
#define CHKLST_TIMESERIES_TEST_REWINDOWING    0x00000008  //
#define CHKLST_TIMESERIES_TEST_NORMAL_OP_0    0x00000010  //
#define CHKLST_TIMESERIES_TEST_NORMAL_OP_1    0x00000020  //
#define CHKLST_TIMESERIES_TEST_ABUSE          0x00000040  //
#define CHKLST_TIMESERIES_TEST_PARSE_PACK     0x00000080  //
#define CHKLST_TIMESERIES_TEST_SHARING        0x00000100  //
#define CHKLST_TIMESERIES_TEST_DESTRUCTION    0x80000000  //

#define CHKLST_TIMESERIES_TESTS_ALL ( \
  CHKLST_TIMESERIES_TEST_CONSTRUCTION | CHKLST_TIMESERIES_TEST_INITIAL_COND | \
  CHKLST_TIMESERIES_TEST_STATS | CHKLST_TIMESERIES_TEST_REWINDOWING | \
  CHKLST_TIMESERIES_TEST_NORMAL_OP_0 | CHKLST_TIMESERIES_TEST_NORMAL_OP_1 | \
  CHKLST_TIMESERIES_TEST_ABUSE | CHKLST_TIMESERIES_TEST_PARSE_PACK | \
  CHKLST_TIMESERIES_TEST_SHARING | CHKLST_TIMESERIES_TEST_DESTRUCTION)


const StepSequenceList TIMESERIES_TEST_LIST[] = {
  { .FLAG         = CHKLST_TIMESERIES_TEST_CONSTRUCTION,
    .LABEL        = "Construction",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timeseries_init()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMESERIES_TEST_INITIAL_COND,
    .LABEL        = "Initial conditions",
    .DEP_MASK     = (CHKLST_TIMESERIES_TEST_CONSTRUCTION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timeseries_initial_conditions()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMESERIES_TEST_STATS,
    .LABEL        = "Stats calculation",
    .DEP_MASK     = (CHKLST_TIMESERIES_TEST_NORMAL_OP_0 | CHKLST_TIMESERIES_TEST_NORMAL_OP_1),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timeseries_stats_tests()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMESERIES_TEST_REWINDOWING,
    .LABEL        = "Re-windowing",
    .DEP_MASK     = (CHKLST_TIMESERIES_TEST_INITIAL_COND),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timeseries_rewindowing()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMESERIES_TEST_NORMAL_OP_0,
    .LABEL        = "Normal operation (Sequential)",
    .DEP_MASK     = (CHKLST_TIMESERIES_TEST_INITIAL_COND),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timeseries_nominal_operation_0()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMESERIES_TEST_NORMAL_OP_1,
    .LABEL        = "Normal operation (Bulk)",
    .DEP_MASK     = (CHKLST_TIMESERIES_TEST_INITIAL_COND),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timeseries_nominal_operation_1()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMESERIES_TEST_ABUSE,
    .LABEL        = "Normal operation (Abuse)",
    .DEP_MASK     = (CHKLST_TIMESERIES_TEST_NORMAL_OP_0 | CHKLST_TIMESERIES_TEST_NORMAL_OP_1),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timeseries_test_abuse()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMESERIES_TEST_PARSE_PACK,
    .LABEL        = "Parsing and packing",
    .DEP_MASK     = (CHKLST_TIMESERIES_TEST_ABUSE),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timeseries_test_parse_pack()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMESERIES_TEST_SHARING,
    .LABEL        = "Data sharing",
    .DEP_MASK     = (CHKLST_TIMESERIES_TEST_PARSE_PACK),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timeseries_data_sharing()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_TIMESERIES_TEST_DESTRUCTION,
    .LABEL        = "Destruction",
    .DEP_MASK     = (CHKLST_TIMESERIES_TEST_SHARING),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == timeseries_teardown()) ? 1:-1);  }
  },
};

AsyncSequencer tseries_test_plan(TIMESERIES_TEST_LIST, (sizeof(TIMESERIES_TEST_LIST) / sizeof(TIMESERIES_TEST_LIST[0])));



/*******************************************************************************
* The main function
*******************************************************************************/

int timeseries_tests_main() {
  const char* const MODULE_NAME = "TimeSeries";
  printf("===< %s >=======================================\n", MODULE_NAME);

  tseries_test_plan.requestSteps(CHKLST_TIMESERIES_TESTS_ALL);
  tseries_test_plan.requestSteps(0);
  while (!tseries_test_plan.request_completed() && (0 == tseries_test_plan.failed_steps(false))) {
    tseries_test_plan.poll();
  }
  int ret = (tseries_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  tseries_test_plan.printDebug(&report_output, "TimeSeries test report");
  printf("%s\n", (char*) report_output.string());

  return ret;
}
