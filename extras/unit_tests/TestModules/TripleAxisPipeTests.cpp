/*
File:   TripleAxisPipeTests.cpp
Author: J. Ian Lindsay
Date:   2024.06.07

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


This program runs tests against the Vector3 pipeline contract, and the utility
  pipelienes that are included in C3P.
*/

#include "Vector3.h"
#include "Pipes/TripleAxisPipe/TripleAxisPipe.h"


/*******************************************************************************
* 3-axis callbacks
* We use the callback function to monitor for correct respoonse and content.
*******************************************************************************/
uint32_t snum_acc     = 0;
uint32_t snum_gyr     = 0;
uint32_t snum_mag     = 0;
uint32_t snum_euler   = 0;
uint32_t snum_bearing = 0;
Vector3f last_vec_acc;
Vector3f last_vec_gyr;
Vector3f last_vec_mag;
Vector3f last_vec_euler;
Vector3f last_vec_bearing;

StopWatch stopwatch_vec_acc;
StopWatch stopwatch_vec_gyr;
StopWatch stopwatch_vec_mag;
StopWatch stopwatch_vec_euler;
StopWatch stopwatch_vec_bearing;



FAST_FUNC int8_t callback_3axis_left(const SpatialSense S, Vector3f* dat, Vector3f* err, uint32_t seq_num) {
  c3p_log(LOG_LEV_INFO, "callback_3axis(%s, seq %u)", "(%.6f, %.6f, %.6f)", TripleAxisPipe::spatialSenseStr(S), seq_num, dat->x, dat->y, dat->z);
  switch (S) {
    case SpatialSense::ACC:        stopwatch_vec_acc.markStart();        last_vec_acc.set(dat);        snum_acc     = seq_num;    break;
    case SpatialSense::GYR:        stopwatch_vec_gyr.markStart();        last_vec_gyr.set(dat);        snum_gyr     = seq_num;    break;
    case SpatialSense::MAG:        stopwatch_vec_mag.markStart();        last_vec_mag.set(dat);        snum_mag     = seq_num;    break;
    case SpatialSense::EULER_ANG:  stopwatch_vec_euler.markStart();      last_vec_euler.set(dat);      snum_euler   = seq_num;    break;
    case SpatialSense::BEARING:    stopwatch_vec_bearing.markStart();    last_vec_bearing.set(dat);    snum_bearing = seq_num;    break;
    default:    break;
  }
  return 0;
}


FAST_FUNC int8_t callback_3axis_right(const SpatialSense S, Vector3f* dat, Vector3f* err, uint32_t seq_num) {
  c3p_log(LOG_LEV_INFO, "callback_3axis(%s, seq %u)", "(%.6f, %.6f, %.6f)", TripleAxisPipe::spatialSenseStr(S), seq_num, dat->x, dat->y, dat->z);
  switch (S) {
    case SpatialSense::ACC:        if (seq_num == snum_acc    ) {  stopwatch_vec_acc.markStop();      }    break;
    case SpatialSense::GYR:        if (seq_num == snum_gyr    ) {  stopwatch_vec_gyr.markStop();      }    break;
    case SpatialSense::MAG:        if (seq_num == snum_mag    ) {  stopwatch_vec_mag.markStop();      }    break;
    case SpatialSense::EULER_ANG:  if (seq_num == snum_euler  ) {  stopwatch_vec_euler.markStop();    }    break;
    case SpatialSense::BEARING:    if (seq_num == snum_bearing) {  stopwatch_vec_bearing.markStop();  }    break;
    default:    break;
  }
  return 0;
}


// The basic pipeline that we will use for testing.

TripleAxisTerminus     tap_term_left(SpatialSense::ACC, callback_3axis_left);
TripleAxisTerminus     tap_term_right(SpatialSense::ACC, callback_3axis_right);
TripleAxisFork         tap_profiling_fork(&tap_term_left, &tap_term_right);




/*******************************************************************************
* Test routines
*******************************************************************************/

/*
*/
int test_3ap_flags() {
  int ret = -1;
  return ret;
}


/*
*/
int test_3ap_relay() {
  int ret = -1;
  return ret;
}


/*
*/
int test_3ap_terminus() {
  printf("TripleAxisTerminus...\n");
  int ret = -1;
  TripleAxisTerminus terminal(SpatialSense::GYR);
  Vector3<float>     src_val     = generate_random_vect3f();
  Vector3<float>     err_val     = generate_random_vect3f();
  Vector3<float>     trash_val   = generate_random_vect3f();
  const uint32_t     IN_THE_PAST = (uint32_t) millis();
  printf("\tlastUpdate() and updateCount() both return zero for a fresh object... ");
  if ((0 == terminal.lastUpdate()) & (0 == terminal.updateCount())) {
    terminal.pushVector(SpatialSense::GYR, &src_val, &err_val, 1);
    terminal.pushVector(SpatialSense::MAG, &trash_val, &trash_val, 1);
    terminal.pushVector(SpatialSense::EULER_ANG, &trash_val, &trash_val, 1);
    printf("Pass.\n\tThere was a single value update following a single valid pushVector() call... ");
    if (1 == terminal.updateCount()) {
      printf("Pass.\n\tThe timestamp in the terminal is plausibly correct... ");
      if (terminal.lastUpdate() >= IN_THE_PAST) {
        printf("Pass.\n\tThe data held in the terminal is correct (%.3f, %.3f, %.3f)... ", src_val.x, src_val.y, src_val.z);
        if (*(terminal.getData()) == src_val) {
          printf("Pass.\n\tThe error held in the terminal is correct (%.3f, %.3f, %.3f)... ", err_val.x, err_val.y, err_val.z);
          if (*(terminal.getError()) == err_val) {
            printf("Pass.\n\treset() returns the class to its default state... ");
            terminal.reset();
            bool in_reset_state = (0 == terminal.updateCount());
            in_reset_state &= (0 == terminal.lastUpdate());
            in_reset_state &= (!terminal.haveError());
            in_reset_state &= (!terminal.dataFresh());
            in_reset_state &= terminal.getData()->isZero();
            in_reset_state &= terminal.getError()->isZero();
            if (in_reset_state) {
              printf("Pass.\n\tgetDataWithErr() return indicates fresh data... ");
              src_val = generate_random_vect3f();
              err_val = generate_random_vect3f();
              terminal.pushVector(SpatialSense::GYR, &src_val, &err_val, 1);
              Vector3<float> tmp_dat;
              Vector3<float> tmp_err;
              uint32_t seq = 0;
              if (1 == terminal.getDataWithErr(&tmp_dat, &tmp_err, &seq)) {
                printf("Pass.\n\tgetDataWithErr() return indicates stale data on second call... ");
                if (0 == terminal.getDataWithErr(&tmp_dat, &tmp_err, &seq)) {
                  printf("Pass.\n\tThe returned vector is correct (%.3f, %.3f, %.3f)... ", src_val.x, src_val.y, src_val.z);
                  if (tmp_dat == src_val) {
                    printf("Pass.\n\tThe returned error is correct (%.3f, %.3f, %.3f)... ", err_val.x, err_val.y, err_val.z);
                    if (tmp_err == err_val) {
                      printf("PASS.\n");
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

  if (0 != ret) {
    printf("Fail.\n");
    StringBuilder output;
    terminal.printPipe(&output, 0, LOG_LEV_DEBUG);
    printf("%s\n", (char*) output.string());
    ret = -1;
  }
  return ret;
}


/*
*/
int test_3ap_fork() {
  printf("TripleAxisFork...\n");
  int ret = -1;
  TripleAxisTerminus term_left(SpatialSense::UNITLESS);
  TripleAxisTerminus term_right(SpatialSense::UNITLESS);
  TripleAxisFork     fork(&term_left, &term_right);
  Vector3<float>     src_val;

  printf("\tVerifying that the fork processes left-first... ");
  uint32_t timer_deadlock = 1000000;
  while ((term_left.lastUpdate() >= term_right.lastUpdate()) && (timer_deadlock)) {
    src_val = generate_random_vect3f();
    fork.pushVector(SpatialSense::UNITLESS, &src_val, nullptr, 1);
    timer_deadlock--;
  }

  if (timer_deadlock > 0) {
    printf("Passed after %u iterations.\n", term_left.updateCount());
    printf("\tThe fork's left and right sides match (%.3f, %.3f, %.3f)... ", src_val.x, src_val.y, src_val.z);
    if (*(term_left.getData()) == src_val) {
      if (*(term_right.getData()) == src_val) {
        printf("Pass.\n\tupdateCount() matches on the left and right... ");
        if (term_left.updateCount() == term_right.updateCount()) {
          printf("PASS.\n");
          ret = 0;
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
    StringBuilder output;
    fork.printPipe(&output, 0, LOG_LEV_DEBUG);
    printf("%s\n", (char*) output.string());
    ret = -1;
  }
  return ret;
}


/*
* Ensure that the axis re-mapper works.
*/
int test_3ap_axis_remapper() {
  const Vector3<float> ZERO_VECTOR(0.0f, 0.0f, 0.0f);
  const uint32_t TEST_CYCLES = (7 + (randomUInt32() % 11));
  Vector3<float> error_figure(0.15f, 0.15f, 0.15f);
  printf("TripleAxisRemapper (%u iterations)...\n", TEST_CYCLES);
  int ret = 0;
  TripleAxisTerminus term_noninv(SpatialSense::UNITLESS);
  TripleAxisTerminus term_inv(SpatialSense::UNITLESS);
  TripleAxisRemapper remapper_noninv(&term_noninv);
  TripleAxisRemapper remapper_inv(&term_inv);
  TripleAxisFork     remapper(&remapper_noninv, &remapper_inv);
  // Generate random vectors, and send them into the fork, verify that the
  //   transform is being done correctly...

  printf("\tNo re-mapping... ");
  remapper_noninv.mapAffernt(AxisID::X, AxisID::Y, AxisID::Z, false, false, false);
  remapper_inv.mapAffernt(   AxisID::X, AxisID::Y, AxisID::Z, true, true, true);
  for (uint32_t i = 0; i < TEST_CYCLES; i++) {
    Vector3<float> src_val     = generate_random_vect3f();
    Vector3<float> src_val_inv(
      (src_val.x * -1.0f),
      (src_val.y * -1.0f),
      (src_val.z * -1.0f)
    );
    remapper.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, i);
    Vector3<float> result_noninv = *(term_noninv.getData());
    Vector3<float> result_inv    = *(term_inv.getData());
    if (*(term_noninv.getData()) != src_val) {   ret--;  }
    if (*(term_inv.getData()) != src_val_inv) {  ret--;  }
  }

  if (0 == ret) {
    printf("Pass\n\tRe-mapping to mute the afferant... ");
    term_noninv.reset();  term_inv.reset();
    remapper_noninv.mapAffernt(AxisID::NONE, AxisID::NONE, AxisID::NONE, false, false, false);
    remapper_inv.mapAffernt(   AxisID::NONE, AxisID::NONE, AxisID::NONE, true, true, true);
    for (uint32_t i = 0; i < TEST_CYCLES; i++) {
      Vector3<float> src_val = generate_random_vect3f();
      remapper.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, 1);
      if (*(term_noninv.getData()) != ZERO_VECTOR) {   ret--;  }
      if (*(term_inv.getData()) != ZERO_VECTOR) {      ret--;  }
    }
  }

  if (0 == ret) {
    printf("Pass\n\tRotating vector components (X to Y), (Y to Z), and (Z to X)... ");
    term_noninv.reset();  term_inv.reset();
    remapper_noninv.mapAffernt(AxisID::Y, AxisID::Z, AxisID::X, false, false, false);
    remapper_inv.mapAffernt(   AxisID::Y, AxisID::Z, AxisID::X, true, true, true);
    for (uint32_t i = 0; i < TEST_CYCLES; i++) {
      Vector3<float> src_val = generate_random_vect3f();
      remapper.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, 1);
      Vector3<float> result_noninv = *(term_noninv.getData());
      Vector3<float> result_inv    = *(term_inv.getData());
      if (src_val.x != result_noninv.y) {   ret--;  }
      if (src_val.y != result_noninv.z) {   ret--;  }
      if (src_val.z != result_noninv.x) {   ret--;  }
    }
  }


  if (0 == ret) {
    printf("Pass\n\tRotating vector components (X to Z), (Y to X), and (Z to Y)... ");
    term_noninv.reset();  term_inv.reset();
    remapper_noninv.mapAffernt(AxisID::Z, AxisID::X, AxisID::Y, false, false, false);
    remapper_inv.mapAffernt(   AxisID::Z, AxisID::X, AxisID::Y, true, true, true);
    for (uint32_t i = 0; i < TEST_CYCLES; i++) {
      Vector3<float> src_val = generate_random_vect3f();
      remapper.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, 1);
      Vector3<float> result_noninv = *(term_noninv.getData());
      Vector3<float> result_inv    = *(term_inv.getData());
      if (src_val.x != result_noninv.z) {   ret--;  }
      if (src_val.y != result_noninv.x) {   ret--;  }
      if (src_val.z != result_noninv.y) {   ret--;  }
    }
  }


  if (0 != ret) {
    printf("Fail\n");
  }
  else {
    printf("PASS\n");
  }
  StringBuilder output;
  remapper.printPipe(&output, 0, LOG_LEV_DEBUG);
  printf("%s\n", (char*) output.string());
  return ret;
}


/*
*
*/
int test_3ap_timeseries() {
  Vector3<float> error_figure(0.15f, 0.15f, 0.15f);
  const uint32_t TEST_DEPTH = (107 + (randomUInt32() % 111));
  int ret = -1;
  printf("TripleAxisTimeSeries (depth of %u)...\n", TEST_DEPTH);
  TripleAxisTerminus term(SpatialSense::UNITLESS);
  TripleAxisTerminus term_nonmatching(SpatialSense::GYR);
  TripleAxisFork     fork(&term, &term_nonmatching);
  TripleAxisTimeSeries timeseries(SpatialSense::UNITLESS, &fork, TEST_DEPTH);
  Vector3<float> src_val;

  // Fill all but the last slot.
  printf("\tNearly filling the timeseries via pushVector() succeeds... ");
  timeseries.forwardWhenFull(true);
  bool pushes_all_pass = true;
  for (uint32_t i = 0; i < (TEST_DEPTH-1); i++) {
    src_val = generate_random_vect3f();
    pushes_all_pass &= (0 == timeseries.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, i));
  }

  if (pushes_all_pass) {
    printf("Pass.\n\tThe timeseries has the correct sample count... ");
    if (timeseries.totalSamples() == (TEST_DEPTH-1)) {
      printf("Pass.\n\tPushing non-matching data fails (fwd_mismatches = false)... ");
      src_val = generate_random_vect3f();
      if (-1 == timeseries.pushVector(SpatialSense::GYR, &src_val, &error_figure, TEST_DEPTH)) {
        printf("Pass.\n\tPushing non-matching data passed (fwd_mismatches = true)... ");
        src_val = generate_random_vect3f();
        timeseries.forwardMismatchedAfferants(true);
        if (0 == timeseries.pushVector(SpatialSense::GYR, &src_val, &error_figure, TEST_DEPTH)) {
          printf("Pass.\n\tThe timeseries window remains unfilled... ");
          if (!timeseries.windowFull()) {
            printf("Pass.\n\tThe non-matching terminal object has a single sample... ");
            if (1 == term_nonmatching.updateCount()) {
              printf("Pass.\n\tThat sample's value matches what went in most-recently... ");
              if (*(term_nonmatching.getData()) == src_val) {
                printf("Pass.\n\tThe matching terminal object has none... ");
                if (0 == term.updateCount()) {
                  printf("Pass.\n\tPushing the last value into the timeseries returns as expected... ");
                  src_val = generate_random_vect3f();
                  if (0 == timeseries.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, TEST_DEPTH)) {
                    printf("Pass.\n\tThe timeseries window is now filled... ");
                    if (timeseries.windowFull()) {
                      printf("Pass.\n\tThe matching terminal object has a single sample... ");
                      if (1 == term.updateCount()) {
                        printf("Pass.\n\tgetDataWithErr() return indicates fresh data... ");
                        Vector3f tmp_dat;
                        Vector3f tmp_err;
                        if (0 >= timeseries.getDataWithErr(&tmp_dat, &tmp_err)) {
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
    }
  }

  if (0 != ret) {
    printf("Fail\n");
  }
  else {
    printf("PASS\n");
  }
  StringBuilder output;
  timeseries.printPipe(&output, 0, LOG_LEV_DEBUG);
  printf("%s\n", (char*) output.string());
  return ret;
}



/*
*/
int test_3ap_orientation() {
  int ret = -1;
  return ret;
}



/*******************************************************************************
* Test plan
*******************************************************************************/
#define CHKLST_3AP_TEST_WITH_FLAGS   0x00000001  // Ensures that TripleAxisPipeWithFlags behaves correctly.
#define CHKLST_3AP_TEST_RELAY        0x00000002  // Tests the fork utility class.
//#define CHKLST_3AP_TEST_  0x00000004  //
//#define CHKLST_3AP_TEST_  0x00000008  //
//#define CHKLST_3AP_TEST_  0x00000010  //
//#define CHKLST_3AP_TEST_  0x00000020  //
//#define CHKLST_3AP_TEST_  0x00000040  //
//#define CHKLST_3AP_TEST_  0x00000080  //
#define CHKLST_3AP_TEST_FORK          0x00000100  // The fork utility class.
#define CHKLST_3AP_TEST_CONV          0x00000200  // The axis reference converter.
#define CHKLST_3AP_TEST_TERMINUS      0x00000400  // Tests the pipelien terminator class.
#define CHKLST_3AP_TEST_TIMESERIES    0x00000800  // Tests the 3AP time-series class.
#define CHKLST_3AP_TEST_ORIENTATION   0x00001000  // Tests the orientation filter.

#define CHKLST_3AP_TEST_DUMP_STATS    0x80000000  // Dumps profiler to test results.


#define CHKLST_3AP_TESTS_ALL ( \
  CHKLST_3AP_TEST_CONV | CHKLST_3AP_TEST_TERMINUS | \
  CHKLST_3AP_TEST_TIMESERIES | \
  CHKLST_3AP_TEST_DUMP_STATS | CHKLST_3AP_TEST_FORK)
//  CHKLST_3AP_TEST_WITH_FLAGS | CHKLST_3AP_TEST_RELAY | \
//  CHKLST_3AP_TEST_TIMESERIES | CHKLST_3AP_TEST_ORIENTATION | \


const StepSequenceList TOP_LEVEL_3AP_TEST_LIST[] = {
  { .FLAG         = CHKLST_3AP_TEST_TERMINUS,
    .LABEL        = "TripleAxisTerminus",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_terminus()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_FORK,
    .LABEL        = "TripleAxisFork",
    .DEP_MASK     = (CHKLST_3AP_TEST_TERMINUS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_fork()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_CONV,
    .LABEL        = "TripleAxisRemapper",
    .DEP_MASK     = (CHKLST_3AP_TEST_FORK),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_axis_remapper()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_TIMESERIES,
    .LABEL        = "TripleAxisTimeSeries",
    .DEP_MASK     = (CHKLST_3AP_TEST_TERMINUS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_timeseries()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_WITH_FLAGS,
    .LABEL        = "TripleAxisPipeWithFlags",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_flags()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_RELAY,
    .LABEL        = "Relay flag handling",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_relay()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_ORIENTATION,
    .LABEL        = "TripleAxisOrientation",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_orientation()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_DUMP_STATS,
    .LABEL        = "Dump stats",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() {
      StringBuilder output;
      tap_profiling_fork.printPipe(&output, 0, LOG_LEV_DEBUG);
      StopWatch::printDebugHeader(&output);
      stopwatch_vec_acc.printDebug("acc",         &output);
      stopwatch_vec_gyr.printDebug("gyr",         &output);
      stopwatch_vec_mag.printDebug("mag",         &output);
      stopwatch_vec_euler.printDebug("euler",     &output);
      stopwatch_vec_bearing.printDebug("bearing", &output);
      printf("%s\n", (char*) output.string());
      return 1;
    }
  },
};

AsyncSequencer tap_test_plan(TOP_LEVEL_3AP_TEST_LIST, (sizeof(TOP_LEVEL_3AP_TEST_LIST) / sizeof(TOP_LEVEL_3AP_TEST_LIST[0])));



/*******************************************************************************
* The main function
*******************************************************************************/
void print_types_3ap() {
  printf("\tTripleAxisFork            %u\t%u\n", sizeof(TripleAxisFork),        alignof(TripleAxisFork));
  printf("\tTripleAxisRemapper        %u\t%u\n", sizeof(TripleAxisRemapper),    alignof(TripleAxisRemapper));
  printf("\tTripleAxisTerminus        %u\t%u\n", sizeof(TripleAxisTerminus),    alignof(TripleAxisTerminus));
  printf("\tTripleAxisTimeSeries      %u\t%u\n", sizeof(TripleAxisTimeSeries),  alignof(TripleAxisTimeSeries));
  printf("\tTripleAxisOrientation     %u\t%u\n", sizeof(TripleAxisOrientation), alignof(TripleAxisOrientation));
}


int tripleaxispipe_tests_main() {
  const char* const MODULE_NAME = "TripleAxisPipe";
  printf("===< %s >=======================================\n", MODULE_NAME);

  tap_test_plan.requestSteps(CHKLST_3AP_TESTS_ALL);
  while (!tap_test_plan.request_completed() && (0 == tap_test_plan.failed_steps(false))) {
    tap_test_plan.poll();
  }
  int ret = (tap_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  tap_test_plan.printDebug(&report_output, "TripleAxisPipe test report");
  printf("%s\n", (char*) report_output.string());

  return ret;
}
