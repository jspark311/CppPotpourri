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

SIUnit UNIT_STR_HARD_MODE[] = { SIUnit::UNIT_GRAMMAR_MARKER,
  SIUnit::META_ORDER_OF_MAGNITUDE, (SIUnit) -6,
  SIUnit::SECONDS, SIUnit::UNITLESS
};
SIUnit UNIT_STR[] = { SIUnit::SECONDS, SIUnit::UNITLESS};

/*
* This helper function does a pedantic check to ensure that the given TimeSeries
*   is initialized and bears the proper freshly-initialized state:
* Window size is non-zero, and is initialized, but there are no samples within
*   it, and is not dirty().
*/
bool timeseries_helper_is_zeroed(TimeSeries<int16_t>* series) {
  if (series) {
    if (series->initialized()) {
      if ((series->windowSize() > 0) & (!series->windowFull())) {
        if ((0 == series->totalSamples()) & !series->dirty()) {
          const uint32_t TEST_SAMPLE_COUNT = series->windowSize();
          int16_t* val_ptr = series->memPtr();
          for (uint32_t i = 0; i < TEST_SAMPLE_COUNT; i++) {
            if (0 != *(val_ptr+i)) return false;
          }
          return true;
        }
      }
    }
  }
  return false;
}



/*******************************************************************************
* Test routines
*******************************************************************************/
/*
*
*/
int timeseries_init() {
  int ret = -1;
  printf("TimeSeries construction semantics...\n");
  printf("\tGlobally declared objects are created as expected... ");
  bool tcode_check_passes = (TCode::UINT32 == series_test_0_m.tcode());
  tcode_check_passes &= (TCode::INT32 == series_test_1_m.tcode());
  tcode_check_passes &= (TCode::FLOAT == series_test_2_m.tcode());

  if (tcode_check_passes) {
    printf("Pass.\n\tObjects initialize correctly... ");
    //timeseries_helper_is_zeroed(TimeSeries<int16_t>* series)
    ret = 0;
    if (0 == ret) {   ret -= series_test_0_m.init();   }
    if (0 == ret) {   ret -= series_test_0_0.init();   }
    if (0 == ret) {   ret -= series_test_0_1.init();   }
    if (0 == ret) {   ret -= series_test_1_m.init();   }
    if (0 == ret) {   ret -= series_test_1_0.init();   }
    if (0 == ret) {   ret -= series_test_1_1.init();   }
    if (0 == ret) {   ret -= series_test_2_m.init();   }
    if (0 == ret) {   ret -= series_test_2_0.init();   }
    if (0 == ret) {   ret -= series_test_2_1.init();   }
  }

  printf("%s.\n", ((0 != ret) ? "Fail" : "PASS"));
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
      printf("TimeSeries failed to series_test_0_m.feedSeries() at index %d.\n", i);
      return -1;
    }
    if ((0 > series_test_1_m.feedSeries(randomUInt32()))) {
      printf("TimeSeries failed to series_test_1_m.feedSeries() at index %d.\n", i);
      return -1;
    }
    if ((0 > series_test_2_m.feedSeries(generate_random_float()))) {
      printf("TimeSeries failed to series_test_2_m.feedSeries() at index %d.\n", i);
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
  else {
    printf("TimeSeries failed to set initial conditions.\n");
  }

  return ret;
}



// Substantially taken from StackOverflow. Thank you, Daniel Laügt.
// https://stackoverflow.com/questions/12278523/comparing-double-values-in-c
// Reworked for assignment vs equality assurance and removal of short-circuit,
//   and const usage.
bool nearly_equal(const double A, const double B, const int FACTOR_OF_EPSILON) {
  const double MIN_A = (A - (A - std::nextafter(A, std::numeric_limits<double>::lowest())) * FACTOR_OF_EPSILON);
  const double MAX_A = (A + (std::nextafter(A, std::numeric_limits<double>::max()) - A) * FACTOR_OF_EPSILON);
  return ((MIN_A <= B) & (MAX_A >= B));
}

// TODO: Using this mess for now until I tighten up my precision enough for the
//   good version to work. These known answers were arrived at
bool nearly_equal(const double A, const double B, const double PRECISION) {
  const double MIN_A = (A - PRECISION);
  const double MAX_A = (A + PRECISION);
  return ((MIN_A <= B) & (MAX_A >= B));
}


/*
* Tests the statistical functions using a handful of KATs.
* This test needs to be phrased as a known-answer test to avoid comparison
*   against a "golden implementation" reproduced in this testing program.
*/
int timeseries_stats_tests() {
  const uint32_t TEST_SAMPLE_COUNT   = 1500;
  const double   TEST_PRECISION      = 0.0002D;
  const int32_t  TEST_EPSILON_FACTOR = (int32_t) (TEST_PRECISION / std::numeric_limits<double>::epsilon());
  printf("Statistical KATs with a sample count of %u, and an epsilon factor of %.d required for success...\n", TEST_SAMPLE_COUNT, TEST_EPSILON_FACTOR);

  int ret = -1;
  float osc_val = 153.0;

  const double  EXPECTED_DBL_MIN  = 102.442193159035;
  const double  EXPECTED_DBL_MAX  = 153000;
  const double  EXPECTED_DBL_MEDN = 206.415273504598;
  const double  EXPECTED_DBL_MEAN = 804.898759643693;
  const double  EXPECTED_DBL_RMS  = 5065.69080921953;
  const double  EXPECTED_DBL_STDV = 5001.33595765524;
  const double  EXPECTED_DBL_SNR  = 0.025900637819809;

  const int32_t EXPECTED_INT_MIN  = 102;
  const int32_t EXPECTED_INT_MAX  = 153000;
  const int32_t EXPECTED_INT_MEDN = 206;
  const double  EXPECTED_INT_MEAN = 804.402;
  const double  EXPECTED_INT_RMS  = 5065.62458083897;
  const double  EXPECTED_INT_STDV = 5001.34879971353;
  const double  EXPECTED_INT_SNR  = 0.025868544627461;

  TimeSeries<double>  series_dbl(TEST_SAMPLE_COUNT);
  TimeSeries<int32_t> series_int(TEST_SAMPLE_COUNT);
  series_dbl.name((char*) "state double");
  series_dbl.init();
  series_int.name((char*) "state int32");
  series_int.init();

  // Generate the test curve, and fill the series...
  for (uint32_t i = 0; i < TEST_SAMPLE_COUNT; i++) {
    const double TEST_CURVE = (double) (((osc_val/(i+1)) + (sin(i/13.0D)/350.0D)) * 1000);
    series_dbl.feedSeries(TEST_CURVE);
    series_int.feedSeries((int32_t) TEST_CURVE);
    //printf("%5d \t %.12f\n", (int32_t) TEST_CURVE, TEST_CURVE);
  }

  const double  RESULT_DBL_MIN  = series_dbl.minValue();
  const double  RESULT_DBL_MAX  = series_dbl.maxValue();
  const double  RESULT_DBL_MEAN = series_dbl.mean();
  const double  RESULT_DBL_MEDN = series_dbl.median();
  const double  RESULT_DBL_RMS  = series_dbl.rms();
  const double  RESULT_DBL_STDV = series_dbl.stdev();
  const double  RESULT_DBL_SNR  = series_dbl.snr();

  const int32_t RESULT_INT_MIN  = series_int.minValue();
  const int32_t RESULT_INT_MAX  = series_int.maxValue();
  const double  RESULT_INT_MEAN = series_int.mean();
  const int32_t RESULT_INT_MEDN = series_int.median();
  const double  RESULT_INT_RMS  = series_int.rms();
  const double  RESULT_INT_STDV = series_int.stdev();
  const double  RESULT_INT_SNR  = series_int.snr();

  printf("\tTesting with type DOUBLE...\n");
  printf("\t\tminValue() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_MIN);
  if (nearly_equal(EXPECTED_DBL_MIN, RESULT_DBL_MIN, TEST_PRECISION)) {
    printf("Pass.\n\t\tmaxValue() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_MAX);
    if (nearly_equal(EXPECTED_DBL_MAX, RESULT_DBL_MAX, TEST_PRECISION)) {
      printf("Pass.\n\t\tmean() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_MEAN);
      if (nearly_equal(EXPECTED_DBL_MEAN, RESULT_DBL_MEAN, TEST_PRECISION)) {
        printf("Pass.\n\t\tmedian() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_MEDN);
        if (nearly_equal(EXPECTED_DBL_MEDN, RESULT_DBL_MEDN, TEST_PRECISION)) {
          printf("Pass.\n\t\trms() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_RMS);
          if (nearly_equal(EXPECTED_DBL_RMS, RESULT_DBL_RMS, TEST_PRECISION)) {
            printf("Pass.\n\t\tstdev() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_STDV);
            if (nearly_equal(EXPECTED_DBL_STDV, RESULT_DBL_STDV, TEST_PRECISION)) {
              printf("Pass.\n\t\tsnr() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_SNR);
              if (nearly_equal(EXPECTED_DBL_SNR, RESULT_DBL_SNR, TEST_PRECISION)) {
                ret = 0;
              }
            }
          }
        }
      }
    }
  }

  if (0 == ret) {
    ret = -1;
    printf("PASS\n");
    printf("\tTesting with type INT32...\n");
    printf("\t\tminValue() matches within expected value (%d)... ", EXPECTED_INT_MIN);
    if (EXPECTED_INT_MIN == RESULT_INT_MIN) {
      printf("Pass.\n\t\tmaxValue() matches expected value (%d)... ", EXPECTED_INT_MAX);
      if (EXPECTED_INT_MAX == RESULT_INT_MAX) {
        printf("Pass.\n\t\tmean() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_INT_MEAN);
        if (nearly_equal(EXPECTED_INT_MEAN, RESULT_INT_MEAN, TEST_PRECISION)) {
          printf("Pass.\n\t\tmedian() matches expected value (%d)... ", EXPECTED_INT_MEDN);
          if (EXPECTED_INT_MEDN == RESULT_INT_MEDN) {
            printf("Pass.\n\t\trms() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_INT_RMS);
            if (nearly_equal(EXPECTED_INT_RMS, RESULT_INT_RMS, TEST_PRECISION)) {
              printf("Pass.\n\t\tstdev() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_INT_STDV);
              if (nearly_equal(EXPECTED_INT_STDV, RESULT_INT_STDV, TEST_PRECISION)) {
                printf("Pass.\n\t\tsnr() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_INT_SNR);
                if (nearly_equal(EXPECTED_INT_SNR, RESULT_INT_SNR, TEST_PRECISION)) {
                  ret = 0;
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
    dump_timeseries(&series_dbl);
    dump_timeseries(&series_int);
  }
  return ret;
}



/*
* Re-windowing is the act of changing the sample capacity of the TimeSeries.
* Doing this will cause all existing class state as it pertains to samples being
*   reset. Samples zero'd, marked clean, 0 == totalSamples(), etc...
*/
int timeseries_rewindowing() {
  const uint32_t TEST_SAMPLE_COUNT_0 = (91 + (randomUInt32() % 23));
  const uint32_t TEST_SAMPLE_COUNT_1 = (TEST_SAMPLE_COUNT_0 + 15 + (randomUInt32() % 31));
  bool stat_passes = false;
  bool dyn_passes  = false;
  printf("Testing the ability to reallocate windows (%u --> %u)...\n", TEST_SAMPLE_COUNT_0, TEST_SAMPLE_COUNT_1);
  printf("\tGenerating test objects... ");

  // Self-allocating objects will be able to change window size.
  // Objects created with explicit memory pools will not be able to change their
  //   window size.
  int16_t static_series_mem[TEST_SAMPLE_COUNT_0];
  TimeSeries<int16_t> series_static(static_series_mem, TEST_SAMPLE_COUNT_0);
  TimeSeries<int16_t> series_dynamic(TEST_SAMPLE_COUNT_0);
  series_static.name("static-mem");
  series_dynamic.name("dynamic-mem");

  if ((0 == series_static.init()) & (0 == series_dynamic.init())) {
    printf("Pass.\n\tWindows are both full... ");
    const int16_t* MEM_PTR_DYN_0 = series_dynamic.memPtr();
    // Fill the series with index values,
    for (uint32_t i = 0; i < TEST_SAMPLE_COUNT_0; i++) {
      const int16_t KNOWN_VALUE = (int16_t) i;
      series_static.feedSeries(KNOWN_VALUE);
      series_dynamic.feedSeries(KNOWN_VALUE);
    }

    if (series_static.windowFull() & series_dynamic.windowFull()) {
      printf("Pass.\n\twindowSize(%u) succeeds for both static and dynamic (no change to pool size)... ", TEST_SAMPLE_COUNT_0);
      if (((0 == series_static.windowSize(TEST_SAMPLE_COUNT_0)) & (0 == series_dynamic.windowSize(TEST_SAMPLE_COUNT_0)))) {
        printf("Pass.\n\tBoth objects are zeroed... ");
        if (timeseries_helper_is_zeroed(&series_static) & timeseries_helper_is_zeroed(&series_dynamic)) {
          // Re-fill the series...
          for (uint32_t i = 0; i < TEST_SAMPLE_COUNT_0; i++) {
            const int16_t KNOWN_VALUE = (int16_t) i;
            series_static.feedSeries(KNOWN_VALUE);
            series_dynamic.feedSeries(KNOWN_VALUE);
          }
          printf("Pass.\n\twindowSize(%u) fails for static-mem... ", TEST_SAMPLE_COUNT_1);
          if (0 != series_static.windowSize(TEST_SAMPLE_COUNT_1)) {
            printf("Pass.\n\t\twindowSize() returns the old value (%u)... ", TEST_SAMPLE_COUNT_0);
            if (TEST_SAMPLE_COUNT_0 == series_static.windowSize()) {
              printf("Pass.\n\t\tThe value returned by memPtr() is the same as before... ");
              if (series_static.memPtr() == static_series_mem) {
                printf("Pass.\n\t\tThe sample pool has not been wiped... ");
                if (!timeseries_helper_is_zeroed(&series_static)) {
                  stat_passes = true;
                  printf("Pass.\n\twindowSize(%u) succeeds for dynamic-mem... ", TEST_SAMPLE_COUNT_1);
                  if (0 == series_dynamic.windowSize(TEST_SAMPLE_COUNT_1)) {
                    printf("Pass.\n\t\twindowSize() returns the new value (%u)... ", TEST_SAMPLE_COUNT_1);
                    if (TEST_SAMPLE_COUNT_1 == series_dynamic.windowSize()) {
                      printf("Pass.\n\t\tThe value returned by memPtr() is different for dynamic... ");
                      if (series_dynamic.memPtr() != MEM_PTR_DYN_0) {
                        printf("Pass.\n\t\tThe sample pool has been wiped... ");
                        if (timeseries_helper_is_zeroed(&series_dynamic)) {
                          printf("Pass.\n\twindowSize(0) succeeds for dynamic-mem... ");
                          if (0 == series_dynamic.windowSize(0)) {
                            printf("Pass.\n\tThe dynamic-mem series is no longer initialized... ");
                            if (!series_dynamic.initialized()) {
                              dyn_passes = true;
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
    }
  }

  int ret = ((stat_passes & dyn_passes) ? 0 : -1);
  printf("%s.\n", ((0 != ret) ? "Fail" : "PASS"));
  if (!stat_passes) {  dump_timeseries(&series_static);   printf("\n");  }
  if (!dyn_passes) {   dump_timeseries(&series_dynamic);  printf("\n");  }
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
  const uint32_t TEST_SAMPLE_COUNT = (91 + (randomUInt32() % 23));
  int ret = -1;
  printf("Testing normal operation (bulk) with a sample count of %u...\n", TEST_SAMPLE_COUNT);
  printf("\tGenerating test objects... ");

  int16_t stored_mem[TEST_SAMPLE_COUNT];
  TimeSeries<int16_t> series_0(TEST_SAMPLE_COUNT);
  series_0.name("bulk-test");
  if (0 == series_0.init()) {
    int16_t* MEM_PTR = series_0.memPtr();
    // Fill the series with index values via direct manipulation of memory. This
    //   might happen in a real DMA-based use-case, or it might be to avoid the
    //   overhead associated with looping discretely over several values. In any
    //   case, calling feedSeries() with no args should mark the window as full,
    //   and increment totalSamples() by windowSize().
    for (int16_t i = 0; i < (int16_t) TEST_SAMPLE_COUNT; i++) {
      *(MEM_PTR + i) = i;
    }

    printf("Pass.\n\tindexIsWhichSample(x) returns 0 for all input... ");
    bool bogus_index_relationship = false;
    for (int16_t i = 0; i < (int16_t) (TEST_SAMPLE_COUNT+10); i++) {
      bogus_index_relationship |= (0 != series_0.indexIsWhichSample(i));
    }
    if (!bogus_index_relationship) {
      printf("Pass.\n\tfeedSeries() returns 1... ");
      if (1 == series_0.feedSeries()) {
        printf("Pass.\n\twindowFull() returns true... ");
        if (series_0.windowFull()) {
          const uint32_t COPY_LENGTH = (5 + (randomUInt32() % 19));
          const uint32_t COPY_START  = (11 + (randomUInt32() % 43));
          printf("Pass.\n\tcopyValueRange(%u, %u, true) succeeds... ", COPY_LENGTH, COPY_START);
          if (0 == series_0.copyValueRange(stored_mem, COPY_LENGTH, COPY_START, true)) {
            printf("Pass.\n\tcopyValueRange(%u, %u, true) produced the expected pattern in the target buffer... ", COPY_LENGTH, COPY_START);
            bool compare_failed = false;
            for (int16_t i = 0; i < (int16_t) COPY_LENGTH; i++) {
              // The value written was the index, and thus should match.
              compare_failed |= (stored_mem[i] != *(MEM_PTR + COPY_START + i));
              compare_failed |= (stored_mem[i] != (COPY_START + i));
            }
            if (!compare_failed) {
              printf("Pass.\n\tindexIsWhichSample(x) returns parity for all input when (totalSamples() == windowSize())... ");
              for (int16_t i = 0; i < (int16_t) TEST_SAMPLE_COUNT; i++) {
                bogus_index_relationship |= (i != series_0.indexIsWhichSample(i));
              }
              if (!bogus_index_relationship) {
                const uint32_t ADDED_SAMPLE_COUNT = ((TEST_SAMPLE_COUNT << 2) + (randomUInt32() % 137));
                const uint32_t EXPECTED_TOTAL_COUNT = (TEST_SAMPLE_COUNT + ADDED_SAMPLE_COUNT);
                printf("Pass.\n\tAdding %u additional samples produces the expected outcome from totalSamples() (%u)... ", ADDED_SAMPLE_COUNT, EXPECTED_TOTAL_COUNT);
                for (int16_t i = 0; i < (int16_t) ADDED_SAMPLE_COUNT; i++) {
                  series_0.feedSeries(i + TEST_SAMPLE_COUNT);
                }
                if (EXPECTED_TOTAL_COUNT == series_0.totalSamples()) {
                  printf("Pass.\n\tindexIsWhichSample(x) returns expected results for all input... ");
                  for (int16_t i = 0; i < (int16_t) TEST_SAMPLE_COUNT; i++) {
                    bogus_index_relationship |= (i != (series_0.indexIsWhichSample(i) % TEST_SAMPLE_COUNT));
                  }
                  if (!bogus_index_relationship) {
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
  printf("%s.\n", ((0 != ret) ? "Fail" : "PASS"));
  if (0 != ret) {
    dump_timeseries(&series_0);
    for (uint32_t i = 0; i < TEST_SAMPLE_COUNT; i++) {
      printf("%5d ", stored_mem[i]);
      if ((i & 0x07) == 7) printf("\n");
    }
  }
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
  const uint32_t TEST_SAMPLE_COUNT = (91 + (randomUInt32() % 23));
  printf("Testing Parsing and packing with a sample count of %u...\n", TEST_SAMPLE_COUNT);
  printf("\tGenerating test objects... ");
  bool pack_passes    = false;
  bool parse_passes   = false;
  bool compare_passes = false;

  StringBuilder serialized_txt;
  StringBuilder serialized_cbor;

  TimeSeries<int16_t> series_0(TEST_SAMPLE_COUNT);
  series_0.name("source");
  series_0.units(UNIT_STR);

  if (0 == series_0.init()) {
    // Fill the series with index values via direct manipulation of memory.
    int16_t* MEM_PTR = series_0.memPtr();
    for (int16_t i = 0; i < (int16_t) TEST_SAMPLE_COUNT; i++) {  *(MEM_PTR + i) = i;  }
    if (1 == series_0.feedSeries()) {
      // Serialize the source into a few different formats.
      printf("Pass.\n\tSerialize to text... ");
      series_0.printSeries(&serialized_txt);
      { //if (0 == series_0.serialize(&serialized_txt, TCode::STR)) {
        printf("Pass.\n\tSerialize to CBOR... ");
        if (0 == series_0.serialize(&serialized_cbor, TCode::CBOR)) {
          printf("Pass.\n\tSerializing the original TimeSeries did not mark it as clean... ");
          if (series_0.dirty()) {
            pack_passes = true;
          }
        }
      }
    }
  }

  TimeSeries<int16_t>* series_1 = nullptr;
  if (pack_passes) {
    // Deserialize into the target for machine-readable serializations.
    printf("Pass.\n\tDeserializing CBOR back into an object... ");
    //if (0 == series_1.deserialize(&serialized, TCode::CBOR)) {
    C3PValue* series_c3pval = C3PValue::deserialize(&serialized_cbor, TCode::CBOR);
    if (nullptr != series_c3pval) {
      printf("Pass.\n\tThe result is truly a TimeSeries... ");
      TimeSeriesBase* ts_base = nullptr;
      if (0 == series_c3pval->get_as(&ts_base)) {
        printf("Pass.\n\tThe TimeSeries has the expected TCode (%s == %s)... ", typecodeToStr(ts_base->tcode()), typecodeToStr(series_0.tcode()));
        if (ts_base->tcode() == series_0.tcode()) {
          series_1 = (TimeSeries<int16_t>*) ts_base;
          parse_passes = true;
        }
      }
    }
  }

  if (parse_passes) {
    // Compare the objects. They should have final content that is exactly equal.
    printf("Pass.\n\tThe new TimeSeries has the same metadata as the original... ");
    compare_passes  = (series_0.initialized()  == series_1->initialized());
    compare_passes &= (series_0.windowSize()   == series_1->windowSize());
    compare_passes &= (series_0.windowFull()   == series_1->windowFull());
    compare_passes &= (series_0.totalSamples() == series_1->totalSamples());
    compare_passes &= (series_0.lastIndex()    == series_1->lastIndex());
    compare_passes &= (0 == StringBuilder::strcasecmp(series_0.name(),  series_1->name()));
    compare_passes &= (0 == StringBuilder::strcasecmp((const char*) series_0.units(), (const char*) series_1->units()));

    if (compare_passes) {
      int16_t stored_mem_0[TEST_SAMPLE_COUNT];
      int16_t stored_mem_1[TEST_SAMPLE_COUNT];
      printf("Pass.\n\tThe original TimeSeries reads back in bulk... ");
      if (0 == series_0.copyValueRange(stored_mem_0, TEST_SAMPLE_COUNT, 0, true)) {
        printf("Pass.\n\tThe new TimeSeries is dirty()... ");
        if (series_1->dirty()) {
          printf("Pass.\n\tThe new TimeSeries reads back in bulk... ");
          if (0 == series_1->copyValueRange(stored_mem_1, TEST_SAMPLE_COUNT, 0, true)) {
            printf("Pass.\n\tThe samples in the new TimeSeries match those of the original... ");
            for (int16_t i = 0; i < (int16_t) TEST_SAMPLE_COUNT; i++) {
              //printf("%d\t%d\t%d\n", i, stored_mem_0[i], stored_mem_1[i]);
              compare_passes &= (stored_mem_0[i] == i);  // Source data is correct.
              compare_passes &= (stored_mem_1[i] == i);  // Copied data is correct.
            }
          }
        }
      }
    }
  }

  int8_t ret = (pack_passes & parse_passes & compare_passes) ? 0 : -1;
  printf("%s.\n", ((0 != ret) ? "Fail" : "PASS"));
  StringBuilder final_output("\nSerializer outputs:\n---------------------------\nTEXT:\n---------------------------\n");
  final_output.concatHandoff(&serialized_txt);
  final_output.concat("\nUnconsumed CBOR:\n---------------------------\n");
  serialized_cbor.printDebug(&final_output);
  printf("%s\n", (char*) final_output.string());

  if (nullptr != series_1) {
    dump_timeseries(series_1);
    delete series_1;
  }
  return ret;
}


/*
* This tests the partial-update uses of the parser and packer. The goal is to
*   keep the objects in sync in spite of having parsing split up into multiple
*   steps.
*/
int timeseries_data_sharing() {
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

#define CHKLST_TIMESERIES_TESTS_ALL ( \
  CHKLST_TIMESERIES_TEST_CONSTRUCTION | CHKLST_TIMESERIES_TEST_INITIAL_COND | \
  CHKLST_TIMESERIES_TEST_STATS | CHKLST_TIMESERIES_TEST_REWINDOWING | \
  CHKLST_TIMESERIES_TEST_NORMAL_OP_0 | CHKLST_TIMESERIES_TEST_NORMAL_OP_1 | \
  CHKLST_TIMESERIES_TEST_ABUSE | CHKLST_TIMESERIES_TEST_PARSE_PACK | \
  CHKLST_TIMESERIES_TEST_SHARING)


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
