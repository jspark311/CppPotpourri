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
* Profiling
*******************************************************************************/
StopWatch stopwatch_term;
StopWatch stopwatch_remapper;
StopWatch stopwatch_fork;
StopWatch stopwatch_offset;
StopWatch stopwatch_integrator;
StopWatch stopwatch_diff;
StopWatch stopwatch_scaling;
StopWatch stopwatch_timeseries;


/*******************************************************************************
* 3-axis callbacks
* These, and the associated globals are used to track the behavior of
*   TripleAxisTerminalCallback. In practice, an application would use this
*   callback as a final stage in the pipeline to signal change notice and/or
*   note final output values from the pipeline.
*******************************************************************************/
Vector3f tap_last_unitless;
Vector3f tap_last_acc;
Vector3f tap_last_gyr;
Vector3f tap_last_mag;
Vector3f tap_last_euler;
Vector3f tap_last_bearing;

Vector3f tap_cb_dat_left;
Vector3f tap_cb_dat_right;
Vector3f tap_cb_err_left;
Vector3f tap_cb_err_right;
uint32_t tap_cb_seq_num_left   = 0;
uint32_t tap_cb_seq_num_right  = 0;
uint32_t tap_cb_updates_left   = 0;
uint32_t tap_cb_updates_right  = 0;
bool     tap_test_left_micros  = false;
bool     tap_test_right_micros = false;


FAST_FUNC int8_t callback_3axis_term_test(const SpatialSense S, Vector3f* dat, Vector3f* err, uint32_t seq_num) {
  Vector3f safe_err;
  StringBuilder log_line;
  log_line.concatf(
    "%s [seq %u]: (%.3f, %.3f, %.3f)",
    TripleAxisPipe::spatialSenseStr(S), seq_num,
    (double) dat->x, (double) dat->y, (double) dat->z
  );

  if (nullptr != err) {
    log_line.concatf(" +/-(%.3f, %.3f, %.3f)", (double) err->x, (double) err->y, (double) err->z);
  }
  else {
    log_line.concat(" (no error vector)");
  }
  c3p_log(LOG_LEV_INFO, "callback_3axis()", &log_line);

  switch (S) {
    case SpatialSense::ACC:        tap_last_acc.set(dat);       break;
    case SpatialSense::GYR:        tap_last_gyr.set(dat);       break;
    case SpatialSense::MAG:        tap_last_mag.set(dat);       break;
    case SpatialSense::EULER_ANG:  tap_last_euler.set(dat);     break;
    case SpatialSense::BEARING:    tap_last_bearing.set(dat);   break;
    default:                       tap_last_unitless.set(dat);  break;
  }
  return 0;
}


FAST_FUNC int8_t callback_3axis_fork_left(const SpatialSense S, Vector3f* dat, Vector3f* err, uint32_t seq_num) {
  tap_test_left_micros = micros();
  tap_cb_dat_left.set(dat);
  if (nullptr != err) {  tap_cb_err_left.set(err);  }
  else {  tap_cb_err_left.zero();  }
  tap_cb_seq_num_left = seq_num;
  tap_cb_updates_left++;
  return 0;
}


FAST_FUNC int8_t callback_3axis_fork_right(const SpatialSense S, Vector3f* dat, Vector3f* err, uint32_t seq_num) {
  tap_test_right_micros = micros();
  tap_cb_dat_right.set(dat);
  if (nullptr != err) {  tap_cb_err_right.set(err);  }
  else {  tap_cb_err_right.zero();  }
  tap_cb_seq_num_right = seq_num;
  tap_cb_updates_right++;
  return 0;
}


void tap_test_reset_callback_tracker() {
  tap_last_unitless.zero();
  tap_last_acc.zero();
  tap_last_gyr.zero();
  tap_last_mag.zero();
  tap_last_euler.zero();
  tap_last_bearing.zero();
  tap_cb_dat_left.zero();
  tap_cb_dat_right.zero();
  tap_cb_err_left.zero();
  tap_cb_err_right.zero();
  tap_cb_seq_num_left  = 0;
  tap_cb_seq_num_right = 0;
  tap_cb_updates_left  = 0;
  tap_cb_updates_right = 0;
  tap_test_left_micros  = false;
  tap_test_right_micros = false;
}

/*******************************************************************************
* Test routines
*******************************************************************************/
/*
*/
int test_3ap_terminal_callback() {
  printf("TripleAxisTerminalCallback...\n");
  Vector3f error_figure(0.0024f, 0.0024f, 0.0024f);
  Vector3f src_unitless = generate_random_vect3f();
  Vector3f src_acc      = generate_random_vect3f();
  Vector3f src_gyr      = generate_random_vect3f();
  Vector3f src_mag      = generate_random_vect3f();
  Vector3f src_euler    = generate_random_vect3f();
  Vector3f src_bearing  = generate_random_vect3f();
  int ret = -1;
  TripleAxisTerminalCallback terminal(callback_3axis_term_test);

  printf("\tpushVector() succeeds for all defined senses... ");
  bool push_succeeded = (0 == terminal.pushVector(SpatialSense::UNITLESS, &src_unitless, &error_figure, 0));
  push_succeeded = (0 == terminal.pushVector(SpatialSense::ACC,       &src_acc,     &error_figure, 1));
  push_succeeded = (0 == terminal.pushVector(SpatialSense::GYR,       &src_gyr,     &error_figure, 2));
  push_succeeded = (0 == terminal.pushVector(SpatialSense::MAG,       &src_mag,     &error_figure, 3));
  push_succeeded = (0 == terminal.pushVector(SpatialSense::EULER_ANG, &src_euler,   &error_figure, 4));
  push_succeeded = (0 == terminal.pushVector(SpatialSense::BEARING,   &src_bearing, &error_figure, 5));

  if (push_succeeded) {
    printf("Pass.\n\tVector values noted by the callback match expectation... ");
    bool compare_succeeded = (src_unitless == tap_last_unitless);
    compare_succeeded &= (src_acc == tap_last_acc);
    compare_succeeded &= (src_gyr == tap_last_gyr);
    compare_succeeded &= (src_mag == tap_last_mag);
    compare_succeeded &= (src_euler == tap_last_euler);
    compare_succeeded &= (src_bearing == tap_last_bearing);
    if (compare_succeeded) {
      ret = 0;
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
  }
  else {
    printf("PASS\n");
  }
  StringBuilder output;
  terminal.printPipe(&output, 1, LOG_LEV_DEBUG);
  printf("%s\n", (char*) output.string());
  return ret;
}



/*
*/
int test_3ap_storage() {
  printf("TripleAxisStorage...\n");
  int ret = -1;
  const uint32_t RND_SEQ_NUM = (uint32_t) millis();
  TripleAxisStorage terminal(SpatialSense::GYR);
  Vector3f src_val     = generate_random_vect3f();
  Vector3f err_val     = generate_random_vect3f();
  Vector3f trash_val   = generate_random_vect3f();
  printf("\tlastUpdate() and updateCount() both return zero for a fresh object... ");
  if ((0 == terminal.lastUpdate()) & (0 == terminal.updateCount())) {
    stopwatch_term.markStart();
    terminal.pushVector(SpatialSense::GYR, &src_val, &err_val, RND_SEQ_NUM);
    stopwatch_term.markStop();
    stopwatch_term.markStart();
    terminal.pushVector(SpatialSense::MAG, &trash_val, &trash_val, 1);
    stopwatch_term.markStop();
    stopwatch_term.markStart();
    terminal.pushVector(SpatialSense::EULER_ANG, &trash_val, &trash_val, 2);
    stopwatch_term.markStop();
    printf("Pass.\n\tThere was a single value update following a single valid pushVector() call... ");
    if (1 == terminal.updateCount()) {
      printf("Pass.\n\tThe sequence number in the terminal is correct... ");
      if (terminal.lastUpdate() == RND_SEQ_NUM) {
        printf("Pass.\n\tThe data held in the terminal is correct (%.3f, %.3f, %.3f)... ", (double) src_val.x, (double) src_val.y, (double) src_val.z);
        if (terminal.getData() == src_val) {
          printf("Pass.\n\tThe error held in the terminal is correct (%.3f, %.3f, %.3f)... ", (double) err_val.x, (double) err_val.y, (double) err_val.z);
          if (terminal.getError() == err_val) {
            printf("Pass.\n\treset() returns the class to its default state... ");
            terminal.reset();
            bool in_reset_state = (0 == terminal.updateCount());
            in_reset_state &= (0 == terminal.lastUpdate());
            in_reset_state &= (!terminal.haveError());
            in_reset_state &= (!terminal.dataFresh());
            in_reset_state &= terminal.getData().isZero();
            in_reset_state &= terminal.getError().isZero();
            if (in_reset_state) {
              printf("Pass.\n\tgetDataWithErr() return indicates fresh data... ");
              src_val = generate_random_vect3f();
              err_val = generate_random_vect3f();
              stopwatch_term.markStart();
              terminal.pushVector(SpatialSense::GYR, &src_val, &err_val, 1);
              stopwatch_term.markStop();
              Vector3<float> tmp_dat;
              Vector3<float> tmp_err;
              uint32_t seq = 0;
              if (1 == terminal.getDataWithErr(&tmp_dat, &tmp_err, &seq)) {
                printf("Pass.\n\tgetDataWithErr() return indicates stale data on second call... ");
                if (0 == terminal.getDataWithErr(&tmp_dat, &tmp_err, &seq)) {
                  printf("Pass.\n\tThe returned vector is correct (%.3f, %.3f, %.3f)... ", (double) src_val.x, (double) src_val.y, (double) src_val.z);
                  if (tmp_dat == src_val) {
                    printf("Pass.\n\tThe returned error is correct (%.3f, %.3f, %.3f)... ", (double) err_val.x, (double) err_val.y, (double) err_val.z);
                    if (tmp_err == err_val) {
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
    terminal.printPipe(&output, 1, LOG_LEV_DEBUG);
    printf("%s\n", (char*) output.string());
    ret = -1;
  }
  else {
    printf("PASS\n");
  }
  return ret;
}


/*
*/
int test_3ap_offset() {
  const uint32_t TEST_CYCLES = (107 + (randomUInt32() % 111));
  int ret = -1;
  TripleAxisStorage term(SpatialSense::UNITLESS);
  TripleAxisOffset  test_obj(&term);
  Vector3<float> src_val;
  Vector3<float> test_offset = generate_random_vect3f();
  test_obj.offsetVector(test_offset);
  printf("TripleAxisOffset...\n");
  printf("\tVectors can be pushed into the test object... ");
  if (0 == test_obj.pushVector(SpatialSense::UNITLESS, &src_val, nullptr, 0)) {
    printf("Pass.\n\tA vector arrived at the terminal... ");
    if (1 == term.updateCount()) {
      printf("Pass.\n\tThe produced vector equals the offset vector when (0, 0, 0) is passed in... ");
      if (term.getData() == test_offset) {
        printf("Pass.\n\tIssuing %u vectors as input... ", TEST_CYCLES);
        ret = 0;
        for (uint32_t i = 0; i < TEST_CYCLES; i++) {
          src_val = generate_random_vect3f();
          test_offset = generate_random_vect3f();
          test_obj.offsetVector(test_offset);
          stopwatch_offset.markStart();
          const bool PUSH_PASS = (0 == test_obj.pushVector(SpatialSense::UNITLESS, &src_val, nullptr, i));
          stopwatch_offset.markStop();
          Vector3<float> result = term.getData();
          Vector3<float> recomputed = (result - test_offset);
          // TODO: Promote into vector? Arbitrary epsilon.
          const bool COMPARE_PASS = (
            nearly_equal(src_val.x, recomputed.x, 0.00001) & \
            nearly_equal(src_val.y, recomputed.y, 0.00001) & \
            nearly_equal(src_val.z, recomputed.z, 0.00001)
          );
          if (!(COMPARE_PASS & PUSH_PASS)) {   ret--;  printf("%u ", i);  break;  }
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
  test_obj.printPipe(&output, 1, LOG_LEV_DEBUG);
  printf("%s\n", (char*) output.string());
  return ret;
}


/*
*/
int test_3ap_scaling() {
  const uint32_t TEST_CYCLES = (107 + (randomUInt32() % 111));
  int ret = -1;
  TripleAxisStorage term(SpatialSense::UNITLESS);
  TripleAxisScaling test_obj(&term);
  Vector3<float> error_figure(0.0024f, 0.0024f, 0.0024f);
  Vector3<float> src_val = generate_random_vect3f();
  printf("TripleAxisScaling...\n");
  printf("\tVectors can be pushed into the test object... ");
  if (0 == test_obj.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, 0)) {
    printf("Pass.\n\tA vector arrived at the terminal... ");
    if (1 == term.updateCount()) {
      printf("Pass.\n\tWithout setting a scaling parameter, the produced vector is normalized... ");
      if (nearly_equal(1.0f, term.getData().length(), 0.00001)) {
        // TODO: Ensure that the error vector was similarly scaled.
        printf("Pass.\n\tSetting a single-value scaling parameter results in a uniformly-scaled result (%u cycles)... ", TEST_CYCLES);
        ret = 0;
        for (uint32_t i = 0; i < TEST_CYCLES; i++) {
          float scale_float = generate_random_float();
          test_obj.scaling(scale_float);
          src_val = generate_random_vect3f();
          src_val.normalize();
          stopwatch_scaling.markStart();
          const bool PUSH_PASS = (0 == test_obj.pushVector(SpatialSense::UNITLESS, &src_val, nullptr, i));
          stopwatch_scaling.markStop();
          Vector3<float> result = term.getData();
          Vector3<float> recomputed = (result / scale_float);
          // TODO: Promote into vector? Arbitrary epsilon.
          const bool COMPARE_PASS = (
            nearly_equal(src_val.x, recomputed.x, 0.00001) & \
            nearly_equal(src_val.y, recomputed.y, 0.00001) & \
            nearly_equal(src_val.z, recomputed.z, 0.00001)
          );
          if (!(COMPARE_PASS & PUSH_PASS)) {   ret--;  printf("%u ", i);  break;  }
        }
        if (0 == ret) {
          printf("Pass.\n\tSetting a single-value scaling parameter results in a nonuniformly-scaled result (%u cycles)... ", TEST_CYCLES);
          for (uint32_t i = 0; i < TEST_CYCLES; i++) {
            Vector3<float> scale_vect = generate_random_vect3f();
            test_obj.scaling(scale_vect);
            src_val = generate_random_vect3f();
            src_val.normalize();
            stopwatch_scaling.markStart();
            const bool PUSH_PASS = (0 == test_obj.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, i));
            stopwatch_scaling.markStop();
            Vector3f result = term.getData();
            // TODO: Promote into vector? Arbitrary epsilon.
            const bool COMPARE_PASS = (
              nearly_equal(result.x, (src_val.x * scale_vect.x), 0.00001) & \
              nearly_equal(result.y, (src_val.y * scale_vect.y), 0.00001) & \
              nearly_equal(result.z, (src_val.z * scale_vect.z), 0.00001)
            );
            if (!(COMPARE_PASS & PUSH_PASS)) {   ret--;  printf("%u ", i);  break;  }
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
  test_obj.printPipe(&output, 1, LOG_LEV_DEBUG);
  printf("%s\n", (char*) output.string());
  return ret;
}



/*
*/
int test_3ap_fork() {
  printf("TripleAxisFork...\n");
  int ret = -1;
  // Callbacks are used to track timing.
  TripleAxisTerminalCallback cb_left(callback_3axis_fork_left);
  TripleAxisTerminalCallback cb_right(callback_3axis_fork_right);
  TripleAxisFork     fork(&cb_left, &cb_right);
  Vector3<float>     src_val;
  tap_test_reset_callback_tracker();    // Reset the globals used by the test.

  printf("\tVerifying that the fork processes left-first... ");
  uint32_t timer_deadlock = 1000000;
  while ((callback_3axis_fork_left >= callback_3axis_fork_right) && (timer_deadlock)) {
    src_val = generate_random_vect3f();
    stopwatch_fork.markStart();
    fork.pushVector(SpatialSense::UNITLESS, &src_val, nullptr, 1);
    stopwatch_fork.markStop();
    timer_deadlock--;
  }

  if (timer_deadlock > 0) {
    printf("Passed after %u iterations.\n", tap_cb_updates_left);
    printf("\tThe fork's left and right sides match (%.3f, %.3f, %.3f)... ", src_val.x, src_val.y, src_val.z);
    if (tap_cb_dat_left == src_val) {
      if (tap_cb_dat_right == src_val) {
        printf("Pass.\n\tupdateCount() matches on the left and right... ");
        if (tap_cb_updates_left == tap_cb_updates_right) {
          printf("PASS.\n");
          ret = 0;
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
    StringBuilder output;
    fork.printPipe(&output, 1, LOG_LEV_DEBUG);
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
  TripleAxisStorage term_noninv(SpatialSense::UNITLESS);
  TripleAxisStorage term_inv(SpatialSense::UNITLESS);
  TripleAxisRemapper remapper_noninv(&term_noninv);
  TripleAxisRemapper remapper_inv(&term_inv);
  TripleAxisFork     remapper(&remapper_noninv, &remapper_inv);
  // Generate random vectors, and send them into the fork, verify that the
  //   transform is being done correctly...

  printf("\tNo re-mapping... ");
  remapper_noninv.mapAfferent(AxisID::X, AxisID::Y, AxisID::Z, false, false, false);
  remapper_inv.mapAfferent(   AxisID::X, AxisID::Y, AxisID::Z, true, true, true);
  for (uint32_t i = 0; i < TEST_CYCLES; i++) {
    Vector3<float> src_val     = generate_random_vect3f();
    Vector3<float> src_val_inv(
      (src_val.x * -1.0f),
      (src_val.y * -1.0f),
      (src_val.z * -1.0f)
    );
    remapper.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, i);
    Vector3<float> result_noninv = term_noninv.getData();
    Vector3<float> result_inv    = term_inv.getData();
    if (term_noninv.getData() != src_val) {   ret--;  }
    if (term_inv.getData() != src_val_inv) {  ret--;  }
  }

  if (0 == ret) {
    printf("Pass\n\tRe-mapping to mute the afferent... ");
    term_noninv.reset();  term_inv.reset();
    remapper_noninv.mapAfferent(AxisID::NONE, AxisID::NONE, AxisID::NONE, false, false, false);
    remapper_inv.mapAfferent(   AxisID::NONE, AxisID::NONE, AxisID::NONE, true, true, true);
    for (uint32_t i = 0; i < TEST_CYCLES; i++) {
      Vector3<float> src_val = generate_random_vect3f();
      stopwatch_remapper.markStart();
      remapper.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, 1);
      stopwatch_remapper.markStop();
      if (term_noninv.getData() != ZERO_VECTOR) {   ret--;  }
      if (term_inv.getData() != ZERO_VECTOR) {      ret--;  }
    }
  }

  if (0 == ret) {
    printf("Pass\n\tRotating vector components (X to Y), (Y to Z), and (Z to X)... ");
    term_noninv.reset();  term_inv.reset();
    remapper_noninv.mapAfferent(AxisID::Y, AxisID::Z, AxisID::X, false, false, false);
    remapper_inv.mapAfferent(   AxisID::Y, AxisID::Z, AxisID::X, true, true, true);
    for (uint32_t i = 0; i < TEST_CYCLES; i++) {
      Vector3<float> src_val = generate_random_vect3f();
      stopwatch_remapper.markStart();
      remapper.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, 1);
      stopwatch_remapper.markStop();
      Vector3<float> result_noninv = term_noninv.getData();
      Vector3<float> result_inv    = term_inv.getData();
      if (src_val.x != result_noninv.y) {   ret--;  }
      if (src_val.y != result_noninv.z) {   ret--;  }
      if (src_val.z != result_noninv.x) {   ret--;  }
    }
  }


  if (0 == ret) {
    printf("Pass\n\tRotating vector components (X to Z), (Y to X), and (Z to Y)... ");
    term_noninv.reset();  term_inv.reset();
    remapper_noninv.mapAfferent(AxisID::Z, AxisID::X, AxisID::Y, false, false, false);
    remapper_inv.mapAfferent(   AxisID::Z, AxisID::X, AxisID::Y, true, true, true);
    for (uint32_t i = 0; i < TEST_CYCLES; i++) {
      Vector3<float> src_val = generate_random_vect3f();
      stopwatch_remapper.markStart();
      remapper.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, 1);
      stopwatch_remapper.markStop();
      Vector3<float> result_noninv = term_noninv.getData();
      Vector3<float> result_inv    = term_inv.getData();
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
  remapper.printPipe(&output, 1, LOG_LEV_DEBUG);
  printf("%s\n", (char*) output.string());
  return ret;
}



/*
*/
int test_3ap_sense_filter() {
  const uint32_t TEST_CYCLES = (107 + (randomUInt32() % 111));
  const SpatialSense SENSE_TO_FILTER_0 = SpatialSense::BEARING;
  const SpatialSense SENSE_TO_FILTER_1 = SpatialSense::MAG;
  int ret = -1;
  Vector3f error_figure(0.065f, 0.065f, 0.065f);
  Vector3f src_val;
  printf("TripleAxisSenseFilter (%u cycles)...\n", TEST_CYCLES);

  // This test is conducted by forking the vector stream, and attaching a filter
  //   to each side. Fork-left is whitelist and fork-right is blacklist.
  tap_test_reset_callback_tracker();    // Reset the globals used by the test.
  TripleAxisTerminalCallback cb_left(callback_3axis_fork_left);
  TripleAxisTerminalCallback cb_right(callback_3axis_fork_right);
  TripleAxisSenseFilter filt_match(SENSE_TO_FILTER_0, &cb_left);
  TripleAxisSenseFilter filt_nonmatch(SENSE_TO_FILTER_0, &cb_right);
  TripleAxisFork        fork(&filt_match, &filt_nonmatch);

  filt_match.forwardMatchedAfferents(true);        //
  filt_match.forwardMismatchedAfferents(false);    //
  filt_nonmatch.forwardMatchedAfferents(false);    //
  filt_nonmatch.forwardMismatchedAfferents(true);  //

  printf("\tpushVector() succeeds for %s... ", TripleAxisPipe::spatialSenseStr(SENSE_TO_FILTER_0));
  bool pushes_all_pass = true;
  for (uint32_t i = 0; i < TEST_CYCLES; i++) {
    src_val = generate_random_vect3f();
    pushes_all_pass &= (0 == fork.pushVector(SENSE_TO_FILTER_0, &src_val, &error_figure, i));
  }

  if (pushes_all_pass) {
    printf("Pass.\n\tThe correct number of vectors (%u) passed through filt_match... ", TEST_CYCLES);
    if (TEST_CYCLES == tap_cb_updates_left) {
      printf("Pass.\n\tThe correct number of vectors (0) passed through filt_nonmatch... ");
      if (0 == tap_cb_updates_right) {
        printf("Pass.\n\tpushVector() succeeds for %s... ", TripleAxisPipe::spatialSenseStr(SENSE_TO_FILTER_1));
        const uint32_t TEST_CYCLES_OVER_TWO = (TEST_CYCLES >> 1);
        for (uint32_t i = 0; i < TEST_CYCLES_OVER_TWO; i++) {
          src_val = generate_random_vect3f();
          pushes_all_pass &= (0 == fork.pushVector(SENSE_TO_FILTER_1, &src_val, &error_figure, i));
        }
        if (pushes_all_pass) {
          printf("Pass.\n\tThe number of vectors in filt_match is unchanged... ");
          if (TEST_CYCLES == tap_cb_updates_left) {
            printf("Pass.\n\tThe correct number of vectors (%u) passed through filt_nonmatch... ", TEST_CYCLES_OVER_TWO);
            if (TEST_CYCLES_OVER_TWO == tap_cb_updates_right) {
              printf("Pass.\n\tThe filter can be muted... ");
              filt_match.forwardMatchedAfferents(false);
              filt_match.forwardMismatchedAfferents(false);
              for (uint32_t i = 0; i < TEST_CYCLES; i++) {
                fork.pushVector(SENSE_TO_FILTER_0, &src_val, &error_figure, 0);
              }
              if (TEST_CYCLES == tap_cb_updates_left) {
                ret = 0;
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
  fork.printPipe(&output, 1, LOG_LEV_DEBUG);
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
  TripleAxisStorage term(SpatialSense::UNITLESS);
  TripleAxisStorage term_nonmatching(SpatialSense::GYR);
  TripleAxisFork     fork(&term, &term_nonmatching);
  TripleAxisTimeSeries timeseries(SpatialSense::UNITLESS, &fork, TEST_DEPTH);
  Vector3<float> src_val;

  // Fill all but the last slot.
  printf("\tNearly filling the timeseries via pushVector() succeeds... ");
  timeseries.forwardWhenFull(true);
  bool pushes_all_pass = true;
  for (uint32_t i = 0; i < (TEST_DEPTH-1); i++) {
    src_val = generate_random_vect3f();
    stopwatch_timeseries.markStart();
    pushes_all_pass &= (0 == timeseries.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, i));
    stopwatch_timeseries.markStop();
  }

  if (pushes_all_pass) {
    printf("Pass.\n\tThe timeseries has the correct sample count... ");
    if (timeseries.totalSamples() == (TEST_DEPTH-1)) {
      printf("Pass.\n\tPushing non-matching data fails (fwd_mismatches = false)... ");
      src_val = generate_random_vect3f();
      if (-1 == timeseries.pushVector(SpatialSense::GYR, &src_val, &error_figure, TEST_DEPTH)) {
        printf("Pass.\n\tPushing non-matching data passed (fwd_mismatches = true)... ");
        src_val = generate_random_vect3f();
        timeseries.forwardMismatchedAfferents(true);
        stopwatch_timeseries.markStart();
        if (0 == timeseries.pushVector(SpatialSense::GYR, &src_val, &error_figure, TEST_DEPTH)) {
          stopwatch_timeseries.markStop();
          printf("Pass.\n\tThe timeseries window remains unfilled... ");
          if (!timeseries.windowFull()) {
            printf("Pass.\n\tThe non-matching terminal object has a single sample... ");
            if (1 == term_nonmatching.updateCount()) {
              printf("Pass.\n\tThat sample's value matches what went in most-recently... ");
              if (term_nonmatching.getData() == src_val) {
                printf("Pass.\n\tThe matching terminal object has none... ");
                if (0 == term.updateCount()) {
                  printf("Pass.\n\tPushing the last value into the timeseries returns as expected... ");
                  src_val = generate_random_vect3f();
                  stopwatch_timeseries.markStart();
                  if (0 == timeseries.pushVector(SpatialSense::UNITLESS, &src_val, &error_figure, TEST_DEPTH)) {
                    stopwatch_timeseries.markStop();
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
  timeseries.printPipe(&output, 1, LOG_LEV_DEBUG);
  printf("%s\n", (char*) output.string());
  return ret;
}



/*
*/
int test_3ap_integrator() {
  Vector3<float> error_figure(0.001f, 0.001f, 0.001f);
  const uint32_t TEST_CYCLES = (107 + (randomUInt32() % 111));
  int ret = -1;
  TripleAxisStorage term(SpatialSense::ACC);
  TripleAxisIntegrator integrator(SpatialSense::ACC, &term);
  Vector3f src_val;

  printf("TripleAxisIntegrator (%u cycles)...\n", TEST_CYCLES);
  printf("\tAll calls to pushVector() succeed... ");

  bool pushes_all_pass = true;
  for (uint32_t i = 0; i < TEST_CYCLES; i++) {
    src_val = generate_random_vect3f();
    // To avoid also testing the error limits of the float type, we will limit
    //   the size of the input vector. This concern would normally be handled
    //   elsewhere.
    src_val.normalize();
    stopwatch_integrator.markStart();
    pushes_all_pass &= (0 == integrator.pushVector(SpatialSense::ACC, &src_val, &error_figure, i));
    stopwatch_integrator.markStop();
  }

  if (pushes_all_pass) {
    printf("Pass.\n\tThe integrator has the correct sample count... ");
    if (integrator.updateCount() == TEST_CYCLES) {
      ret = 0;
    }
  }

  if (0 != ret) {
    printf("Fail\n");
  }
  else {
    printf("PASS\n");
  }
  StringBuilder output;
  integrator.printPipe(&output, 1, LOG_LEV_DEBUG);
  printf("%s\n", (char*) output.string());
  return ret;
}



/*
*/
int test_3ap_differentiator() {
  Vector3<float> error_figure(0.085f, 0.085f, 0.085f);
  const uint32_t TEST_CYCLES = (107 + (randomUInt32() % 111));
  int ret = -1;
  TripleAxisDifferentiator test_obj(SpatialSense::ACC);
  printf("TripleAxisDifferentiator (%u cycles)...\n", TEST_CYCLES);

  printf("\tAll calls to pushVector() succeed... ");
  bool pushes_all_pass = true;   // Carefull...
  bool correct_result  = true;   // Carefull...
  bool error_scaled    = true;   // Carefull...
  bool first_push_dead = false;
  Vector3f most_recent_push;
  for (uint32_t i = 0; i < TEST_CYCLES; i++) {
    Vector3f src_val    = generate_random_vect3f();
    stopwatch_diff.markStart();
    pushes_all_pass &= (0 == test_obj.pushVector(SpatialSense::ACC, &src_val, &error_figure, i));
    stopwatch_diff.markStop();
    if (0 == i) {
      // Since two points are required to take a derivative, the first push
      //   should succeed, but not produce an efferent or leave an update trace.
      first_push_dead  = (test_obj.updateCount() == 0);
      first_push_dead &= !(test_obj.dataFresh());
    }
    else {
      Vector3f result_dat;
      Vector3f result_err;
      uint32_t seq;
      // After the second push, the class should begin updating. Check its values.
      correct_result &= (0 < test_obj.getDataWithErr(&result_dat, &result_err, &seq));
      correct_result &= ((src_val - most_recent_push) == result_dat);
      error_scaled   &= ((error_figure * 2) == result_err);
    }

    if (!(pushes_all_pass & correct_result & error_scaled & first_push_dead)) {  break;  }
    most_recent_push = src_val;
  }

  if (pushes_all_pass) {
    printf("Pass.\n\tThe first call to pushVector() did not produce an efferent... ");
    if (first_push_dead) {
      printf("Pass.\n\tSubsequent afferents produce correct results... ");
      if (correct_result) {
        printf("Pass.\n\tSubsequent afferents produce scaled error vectors... ");
        if (error_scaled) {
          printf("Pass.\n\tThe differentiator has the correct sample count... ");
          if (test_obj.updateCount() == (TEST_CYCLES-1)) {
            ret = 0;
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
  test_obj.printPipe(&output, 1, LOG_LEV_DEBUG);
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
#define CHKLST_3AP_TEST_TERMINAL_CB  0x00000001  // Tests the pipeline terminator class.
#define CHKLST_3AP_TEST_FORK         0x00000002  // The fork utility class.
#define CHKLST_3AP_TEST_CONV         0x00000004  // The axis reference converter.
#define CHKLST_3AP_TEST_SCALING      0x00000008  // Scaling classes.
#define CHKLST_3AP_TEST_OFFSET       0x00000010  // Offset class.
#define CHKLST_3AP_SENSE_FILTER      0x00000020  // Tests the SpatialSense filter.
#define CHKLST_3AP_TEST_STORAGE      0x00000040  // TripleAxisStorage
#define CHKLST_3AP_TEST_TIMESERIES   0x00000080  // Tests the 3AP time-series class.
#define CHKLST_3AP_TEST_INTEGRATOR   0x00000100  // The integrator class.
#define CHKLST_3AP_TEST_DIFF         0x00000200  // TripleAxisDifferentiator
#define CHKLST_3AP_TEST_DUMP_STATS   0x80000000  // Dumps profiler to test results.

#define CHKLST_3AP_TESTS_ALL ( \
  CHKLST_3AP_TEST_CONV | CHKLST_3AP_TEST_STORAGE | CHKLST_3AP_TEST_FORK | \
  CHKLST_3AP_TEST_OFFSET | CHKLST_3AP_TEST_SCALING | CHKLST_3AP_SENSE_FILTER | \
  CHKLST_3AP_TEST_INTEGRATOR | CHKLST_3AP_TEST_TIMESERIES | CHKLST_3AP_TEST_DIFF | \
  CHKLST_3AP_TEST_TERMINAL_CB | CHKLST_3AP_TEST_DUMP_STATS)


const StepSequenceList TOP_LEVEL_3AP_TEST_LIST[] = {
  { .FLAG         = CHKLST_3AP_TEST_TERMINAL_CB,
    .LABEL        = "TripleAxisTerminalCallback",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_terminal_callback()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_FORK,
    .LABEL        = "TripleAxisFork",
    .DEP_MASK     = (CHKLST_3AP_TEST_TERMINAL_CB),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_fork()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_SENSE_FILTER,
    .LABEL        = "TripleAxisSenseFilter",
    .DEP_MASK     = (CHKLST_3AP_TEST_FORK),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_sense_filter()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_STORAGE,
    .LABEL        = "TripleAxisStorage",
    .DEP_MASK     = (CHKLST_3AP_SENSE_FILTER),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_storage()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_OFFSET,
    .LABEL        = "TripleAxisOffset",
    .DEP_MASK     = (CHKLST_3AP_TEST_STORAGE),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_offset()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_SCALING,
    .LABEL        = "TripleAxisScaling",
    .DEP_MASK     = (CHKLST_3AP_TEST_STORAGE),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_scaling()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_CONV,
    .LABEL        = "TripleAxisRemapper",
    .DEP_MASK     = (CHKLST_3AP_TEST_STORAGE),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_axis_remapper()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_TIMESERIES,
    .LABEL        = "TripleAxisTimeSeries",
    .DEP_MASK     = (CHKLST_3AP_TEST_STORAGE),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_timeseries()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_INTEGRATOR,
    .LABEL        = "TripleAxisIntegrator",
    .DEP_MASK     = (CHKLST_3AP_TEST_STORAGE),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_integrator()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_DIFF,
    .LABEL        = "TripleAxisDifferentiator",
    .DEP_MASK     = (CHKLST_3AP_TEST_STORAGE),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_differentiator()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_3AP_TEST_ORIENTATION,
    .LABEL        = "TripleAxisOrientation",
    .DEP_MASK     = (CHKLST_3AP_TEST_FORK),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_orientation()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_DUMP_STATS,
    .LABEL        = "Dump stats",
    .DEP_MASK     = (CHKLST_3AP_TESTS_ALL & ~CHKLST_3AP_TEST_DUMP_STATS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() {
      StringBuilder output;
      StopWatch::printDebugHeader(&output);
      stopwatch_term.printDebug(      "term",       &output);
      stopwatch_remapper.printDebug(  "remapper",   &output);
      stopwatch_fork.printDebug(      "fork",       &output);
      stopwatch_offset.printDebug(    "offset",     &output);
      stopwatch_integrator.printDebug("integrator", &output);
      stopwatch_diff.printDebug("diff", &output);
      stopwatch_scaling.printDebug(   "scaling",    &output);
      stopwatch_timeseries.printDebug("timeseries", &output);
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
  printf("\tTripleAxisScaling         %u\t%u\n", sizeof(TripleAxisScaling),     alignof(TripleAxisScaling));
  printf("\tTripleAxisOffset          %u\t%u\n", sizeof(TripleAxisOffset),      alignof(TripleAxisOffset));
  printf("\tTripleAxisRemapper        %u\t%u\n", sizeof(TripleAxisRemapper),    alignof(TripleAxisRemapper));
  printf("\tTripleAxisSenseFilter     %u\t%u\n", sizeof(TripleAxisSenseFilter), alignof(TripleAxisSenseFilter));
  printf("\tTripleAxisTimeSeries      %u\t%u\n", sizeof(TripleAxisTimeSeries),  alignof(TripleAxisTimeSeries));
  printf("\tTripleAxisStorage         %u\t%u\n", sizeof(TripleAxisStorage),    alignof(TripleAxisStorage));
  printf("\tTripleAxisIntegrator      %u\t%u\n", sizeof(TripleAxisIntegrator),  alignof(TripleAxisIntegrator));
  printf("\tTripleAxisDifferentiator  %u\t%u\n", sizeof(TripleAxisDifferentiator),  alignof(TripleAxisDifferentiator));
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
