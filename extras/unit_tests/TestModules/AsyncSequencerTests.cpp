/*
File:   AsyncSequencerTests.cpp
Author: J. Ian Lindsay
Date:   2023.06.17

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


This program tests AsyncSequencer.
*/

#include "AsyncSequencer.h"


/*******************************************************************************
* We'll need some flags to keep things orderly...
*******************************************************************************/
#define ASYNC_SEQ_TEST_FLAG_00   0x00000001
#define ASYNC_SEQ_TEST_FLAG_01   0x00000002
#define ASYNC_SEQ_TEST_FLAG_02   0x00000004
#define ASYNC_SEQ_TEST_FLAG_03   0x00000008
#define ASYNC_SEQ_TEST_FLAG_04   0x00000010
#define ASYNC_SEQ_TEST_FLAG_05   0x00000020
#define ASYNC_SEQ_TEST_FLAG_06   0x00000040
#define ASYNC_SEQ_TEST_FLAG_07   0x00000080
#define ASYNC_SEQ_TEST_FLAG_08   0x00000100
#define ASYNC_SEQ_TEST_FLAG_09   0x00000200
#define ASYNC_SEQ_TEST_FLAG_10   0x00000400
#define ASYNC_SEQ_TEST_FLAG_11   0x00000800
#define ASYNC_SEQ_TEST_FLAG_12   0x00001000
#define ASYNC_SEQ_TEST_FLAG_13   0x00002000
#define ASYNC_SEQ_TEST_FLAG_14   0x00004000
#define ASYNC_SEQ_TEST_FLAG_XX   0x10000000   // This flag has no matching entry in the StepSequenceList.

// Full valid flag mask
#define ASYNC_SEQ_TEST_ALL_FLAGS ( \
  ASYNC_SEQ_TEST_FLAG_00 | ASYNC_SEQ_TEST_FLAG_01 | ASYNC_SEQ_TEST_FLAG_02 | \
  ASYNC_SEQ_TEST_FLAG_03 | ASYNC_SEQ_TEST_FLAG_04 | ASYNC_SEQ_TEST_FLAG_05 | \
  ASYNC_SEQ_TEST_FLAG_06 | ASYNC_SEQ_TEST_FLAG_07 | ASYNC_SEQ_TEST_FLAG_08 | \
  ASYNC_SEQ_TEST_FLAG_09 | ASYNC_SEQ_TEST_FLAG_10 | ASYNC_SEQ_TEST_FLAG_11 | \
  ASYNC_SEQ_TEST_FLAG_12 | ASYNC_SEQ_TEST_FLAG_13 | ASYNC_SEQ_TEST_FLAG_14)

// Full valid flag mask with no held deps.
#define ASYNC_SEQ_TEST_NO_HOLD_FLAGS ( \
  ASYNC_SEQ_TEST_FLAG_00 | ASYNC_SEQ_TEST_FLAG_01 | ASYNC_SEQ_TEST_FLAG_02 | \
  ASYNC_SEQ_TEST_FLAG_03)


// We hand-manipulate some globals in order to test that the result of the poll
//   and dispatch functions is being properly taken into account when evolving
//   state within the sequencer.
int8_t async_04_dispatch = 0;
int8_t async_04_poll     = 0;
int8_t async_09_dispatch = 0;
int8_t async_09_poll     = 0;
int8_t async_13_dispatch = 0;
int8_t async_13_poll     = 0;

// The number of times each of the corresponding functions was called.
int8_t async_04_d_count  = 0;
int8_t async_04_p_count  = 0;
int8_t async_09_d_count  = 0;
int8_t async_09_p_count  = 0;
int8_t async_13_d_count  = 0;
int8_t async_13_p_count  = 0;


/*******************************************************************************
* Now for lambdas and conditionals that form the logical basis of the sequence.
*******************************************************************************/

const StepSequenceList ASYNC_SEQ_SELF_DIAGNOSTIC[] = {
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_00,
    .LABEL        = "FLAG_00",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_01,
    .LABEL        = "FLAG_01",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_00),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_02,
    .LABEL        = "FLAG_02",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_00),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_03,
    .LABEL        = "FLAG_03",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_00 | ASYNC_SEQ_TEST_FLAG_02),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_04,
    .LABEL        = "FLAG_04",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { async_04_d_count++;  return async_04_dispatch;  },
    .POLL_FXN     = []() { async_04_p_count++;  return async_04_poll;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_05,
    .LABEL        = "FLAG_05",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_04 | ASYNC_SEQ_TEST_FLAG_03),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_06,
    .LABEL        = "FLAG_06",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_03),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_07,
    .LABEL        = "FLAG_07",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_08,
    .LABEL        = "FLAG_08",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_06 | ASYNC_SEQ_TEST_FLAG_07),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_09,
    .LABEL        = "FLAG_09",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_06 | ASYNC_SEQ_TEST_FLAG_00),
    .DISPATCH_FXN = []() { async_09_d_count++;  return async_09_dispatch;  },
    .POLL_FXN     = []() { async_09_p_count++;  return async_09_poll;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_10,
    .LABEL        = "FLAG_10",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_08),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_11,
    .LABEL        = "FLAG_11",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_08),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_12,
    .LABEL        = "FLAG_12",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_10 | ASYNC_SEQ_TEST_FLAG_11),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_13,
    .LABEL        = "FLAG_13",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { async_13_d_count++;  return async_13_dispatch;  },
    .POLL_FXN     = []() { async_13_p_count++;  return async_13_poll;  }
  },
  { .FLAG         = ASYNC_SEQ_TEST_FLAG_14,
    .LABEL        = "FLAG_14",
    .DEP_MASK     = (ASYNC_SEQ_TEST_FLAG_13 | ASYNC_SEQ_TEST_FLAG_09 | ASYNC_SEQ_TEST_FLAG_05),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
};

// And with that, we can declare an operational checklist.
const uint32_t REAL_STEP_COUNT = (sizeof(ASYNC_SEQ_SELF_DIAGNOSTIC) / sizeof(ASYNC_SEQ_SELF_DIAGNOSTIC[0]));
AsyncSequencer async_seq_unit_tests(ASYNC_SEQ_SELF_DIAGNOSTIC, REAL_STEP_COUNT);



/*******************************************************************************
* Support functions re-used in the test.
*******************************************************************************/

/* Dumps output from the object under test. */
void async_seq_dump_to_printf() {
  StringBuilder output;
  async_seq_unit_tests.printDebug(&output);
  printf("%s\n", (char*) output.string());
}

/*
* Returns the number of state transitions before state ceases to evolve, or -1
*   on failure.
*/
int async_seq_run_until_stagnant() {
  int ret = 0;
  int ret_local = async_seq_unit_tests.poll();
  while (ret_local > 0) {
    if (-1 == ret_local) {
      return -1;
    }
    ret += ret_local;
    ret_local = async_seq_unit_tests.poll();
  }
  return ret;
}

/* Reset the sequencer back to its reset state, and verify. */
int async_seq_impose_initial_state() {
  int ret = -1;
  printf("Testing resetSequencer() and initial state... ");
  async_seq_unit_tests.resetSequencer();
  printf("\tThere should be no steps running... ");
  if (!async_seq_unit_tests.steps_running()) {
    printf("Pass.\n\trequest_fulfilled() should return true at this point... ");
    ret--;
    if (async_seq_unit_tests.request_fulfilled()) {
      async_04_dispatch = 0;
      async_04_poll     = 0;
      async_09_dispatch = 0;
      async_09_poll     = 0;
      async_13_dispatch = 0;
      async_13_poll     = 0;
      async_04_d_count  = 0;
      async_04_p_count  = 0;
      async_09_d_count  = 0;
      async_09_p_count  = 0;
      async_13_d_count  = 0;
      async_13_p_count  = 0;
      printf("Pass.\n");
      ret = 0;
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
  }
  return ret;
}


/* Reset the sequencer back to its reset state, and verify. */
int async_seq_impose_initial_state_via_reset_steps() {
  int ret = -1;
  printf("Testing resetSteps() and initial state... ");
  async_seq_unit_tests.resetSteps(ASYNC_SEQ_TEST_ALL_FLAGS);
  printf("\tThere should be no steps running... ");
  if (!async_seq_unit_tests.steps_running()) {
    printf("Pass.\n\trequest_fulfilled() should return true at this point... ");
    ret--;
    if (async_seq_unit_tests.request_fulfilled()) {
      async_04_dispatch = 0;
      async_04_poll     = 0;
      async_09_dispatch = 0;
      async_09_poll     = 0;
      async_13_dispatch = 0;
      async_13_poll     = 0;
      async_04_d_count  = 0;
      async_04_p_count  = 0;
      async_09_d_count  = 0;
      async_09_p_count  = 0;
      async_13_d_count  = 0;
      async_13_p_count  = 0;
      printf("Pass.\n");
      ret = 0;
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
  }
  return ret;
}


/*******************************************************************************
* AsyncSequencer test routines
*******************************************************************************/


int async_seq_test_simple_advancement() {
  int ret = -1;
  int ret_local = 0;
  async_seq_unit_tests.requestSteps(ASYNC_SEQ_TEST_NO_HOLD_FLAGS);
  ret_local = async_seq_run_until_stagnant();
  ret_local += async_seq_run_until_stagnant();
  if (0 <= ret_local) {
    printf("Sequence mask 0x%08x polled to stagnation after %d state transitions.\n", ASYNC_SEQ_TEST_NO_HOLD_FLAGS, ret_local);
    ret--;
    if (async_seq_unit_tests.request_completed()) {
      ret--;
      if (async_seq_unit_tests.request_fulfilled()) {
        // Next, we'll add in some dependent states and gate their passing for
        //   the sake of making inferences about operation.
        // FLAG_05 depends on FLAG_04 (gated), and FLAG_03 (already passed).
        async_seq_unit_tests.requestSteps(ASYNC_SEQ_TEST_FLAG_05);
        // Nothing should happen when we poll, be cause FLAG_04 should be implicitly requested,
        //   and is returning 0 in its dispatch function. But we should still see
        //   it run if we poll.
        ret_local = async_seq_run_until_stagnant();
        if (1 == ret_local) {
          if (1 == async_04_d_count) {
            if (!async_seq_unit_tests.request_completed()) {
              if (!async_seq_unit_tests.request_fulfilled()) {
                if (!async_seq_unit_tests.steps_running()) {
                  if (async_seq_unit_tests.all_steps_have_passed(ASYNC_SEQ_TEST_NO_HOLD_FLAGS)) {
                    // Allow FLAG_04 to dispatch. It's conversion to running status will count toward our
                    //   return value during polling, but will not yet resolve, since poll() will still return 0.
                    async_04_dispatch = 1;
                    ret_local = async_seq_unit_tests.poll();
                    if (2 == ret_local) {
                      if ((2 == async_04_d_count) && (1 == async_04_p_count)) {
                        // Once again, verify the basics. Except that now we
                        //   expect steps to be running.
                        bool basis_sound = !async_seq_unit_tests.request_completed();
                        basis_sound &= !async_seq_unit_tests.request_fulfilled();
                        basis_sound &= async_seq_unit_tests.steps_running();
                        if (basis_sound) {
                          // Finally, release FLAG_04's poll() fxn, and collect
                          //   and count the debris.
                          async_04_poll = 1;
                          ret_local = async_seq_run_until_stagnant();
                          if (5 == ret_local) {
                            if ((2 == async_04_d_count) && (2 == async_04_p_count)) {
                              bool final_state_chk = async_seq_unit_tests.request_completed();
                              final_state_chk &= async_seq_unit_tests.request_fulfilled();
                              final_state_chk &= !async_seq_unit_tests.steps_running();
                              if (final_state_chk) {
                                if (!async_seq_unit_tests.all_steps_have_passed(ASYNC_SEQ_TEST_FLAG_06)) {
                                  ret = 0;
                                }
                                else printf("FLAG_06 was over-eager. Should not have run, but did.\n");
                              }
                              else printf("Final state report is not as expected.\n");
                            }
                            else printf("Incorrect async_04_d/p_counts: %d  %d.\n", async_04_d_count, async_04_p_count);
                          }
                          else printf("async_seq_run_until_stagnant() was expected to return 5 the third time, returned %d instead.\n", ret_local);
                        }
                        else printf("State reporting basis is not sound.\n");
                      }
                      else printf("Incorrect async_04_d/p_counts: %d  %d.\n", async_04_d_count, async_04_p_count);
                    }
                    else printf("async_seq_run_until_stagnant() was expected to return 2 the second time, returned %d instead.\n", ret_local);
                  }
                  else printf("all_steps_have_passed(ASYNC_SEQ_TEST_NO_HOLD_FLAGS) should be true at this point.\n");
                }
                else printf("steps_running() should not return true yet.\n");
              }
              else printf("request_fulfilled() should not return true yet.\n");
            }
            else printf("request_completed(ASYNC_SEQ_TEST_FLAG_05) should not return true yet.\n");
          }
          else printf("async_04_d_count should be 1, but we found %d.\n", async_04_d_count);
        }
        else printf("async_seq_run_until_stagnant() was expected to return 1 the first time, returned %d instead.\n", ret_local);
      }
      else printf("Simple request should have marked the state as fulfilled, but did not.\n");
    }
    else printf("Non-held sequences should have completed, but did not.\n");
  }
  else printf("There should be no errors in sequencer polling.\n");

  if (0 != ret) {  async_seq_dump_to_printf();  }

  return ret;
}


/* This is substantially the same test as above, but with failures. */
int async_seq_test_simple_failures() {
  int ret = -1;
  int ret_local = 0;
  printf("Testing checklist failure handling... ");
  printf("\tResetting checklist... ");
  if (0 == async_seq_impose_initial_state()) {
    printf("Pass.\n\tThe checklist fails... ");
    async_04_dispatch = 1;   // FLAG_04 will dispatch, but fail to poll.
    async_04_poll     = -1;
    async_09_dispatch = -1;  // FLAG_09 will fail to dispatch, but will poll successfully.
    async_09_poll     = 1;
    async_13_dispatch = -1;  // FLAG_13 will fail to either dispatch or poll.
    async_13_poll     = -1;
    // FLAG_14 ultimately has all manipulated steps as dependencies. So if any
    //   of these values is some value other than 1 (success), or 0 (defer), the
    //   sequence will fail to complete.
    async_seq_unit_tests.requestSteps(ASYNC_SEQ_TEST_FLAG_14);
    async_seq_run_until_stagnant();
    const uint32_t FAILED_0 = async_seq_unit_tests.failed_steps(true);
    if (0 != FAILED_0) {
      printf("Pass.\n\tAll steps that should have failed did so... ");
      const uint32_t FAIL_BOAT_0 = (ASYNC_SEQ_TEST_FLAG_04 | ASYNC_SEQ_TEST_FLAG_09 | ASYNC_SEQ_TEST_FLAG_13);
      if (FAILED_0 == FAIL_BOAT_0) {
        printf("Pass.\n\tSteps failed at the expected places... \n");
        const bool PEDANTIC_FAIL_CHECK_04 = ((1 == async_04_d_count) & (1 == async_04_p_count));
        const bool PEDANTIC_FAIL_CHECK_09 = ((1 == async_09_d_count) & (0 == async_09_p_count));
        const bool PEDANTIC_FAIL_CHECK_13 = ((1 == async_13_d_count) & (0 == async_13_p_count));
        printf("\t\tFLAG_04 passed DISPATCH, and therefore POLL'd... %s\n", (PEDANTIC_FAIL_CHECK_04 ? "Pass":"Fail"));
        printf("\t\tFLAG_09 failed DISPATCH, and therefore did not POLL... %s\n", (PEDANTIC_FAIL_CHECK_09 ? "Pass":"Fail"));
        printf("\t\tFLAG_13 failed DISPATCH, and therefore did not POLL... %s\n", (PEDANTIC_FAIL_CHECK_13 ? "Pass":"Fail"));
        if (PEDANTIC_FAIL_CHECK_04 & PEDANTIC_FAIL_CHECK_09 & PEDANTIC_FAIL_CHECK_13) {
          printf("Pass.\n\tResetting the failed steps marks them as having not been run... ");
          async_seq_unit_tests.resetSteps(FAIL_BOAT_0);
          bool pedantic_reset_check = !async_seq_unit_tests.all_steps_have_run(ASYNC_SEQ_TEST_FLAG_04);
          pedantic_reset_check &= !async_seq_unit_tests.all_steps_have_run(ASYNC_SEQ_TEST_FLAG_09);
          pedantic_reset_check &= !async_seq_unit_tests.all_steps_have_run(ASYNC_SEQ_TEST_FLAG_13);
          if (pedantic_reset_check) {
            printf("Pass.\n\tChecklist succeeds this time... ");
            async_04_poll     = 1;  // FLAG_04 will now succeed.
            async_09_dispatch = 1;  // FLAG_09 will now succeed.
            async_13_dispatch = 1;  // FLAG_13 will now succeed.
            async_13_poll     = 1;
            async_seq_unit_tests.requestSteps(FAIL_BOAT_0);
            async_seq_run_until_stagnant();
            if (async_seq_unit_tests.request_fulfilled()) {
              printf("PASS.\n");
              ret = 0;
            }
          }
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
    async_seq_dump_to_printf();
  }

  return ret;
}




int async_seq_test_full_execution() {
  int ret = -1;
  int ret_local = 0;
  if (0 == async_seq_impose_initial_state_via_reset_steps()) {
    async_04_dispatch = 1;
    async_04_poll     = 1;
    async_09_dispatch = 1;
    async_09_poll     = 1;
    async_13_dispatch = 1;
    async_13_poll     = 1;
    async_seq_unit_tests.requestSteps(ASYNC_SEQ_TEST_ALL_FLAGS);
    ret_local += async_seq_run_until_stagnant();
    if (0 <= ret_local) {
      printf("Sequence mask 0x%08x polled to stagnation after %d state transitions.\n", ASYNC_SEQ_TEST_ALL_FLAGS, ret_local);
      bool final_state_chk = async_seq_unit_tests.request_completed();
      final_state_chk &= async_seq_unit_tests.request_fulfilled();
      final_state_chk &= !async_seq_unit_tests.steps_running();
      if (final_state_chk) {
        if (async_seq_unit_tests.all_steps_have_passed()) {
          bool final_count_chk = (1 == async_04_d_count);
          final_count_chk &= (1 == async_04_p_count);
          final_count_chk &= (1 == async_09_d_count);
          final_count_chk &= (1 == async_09_p_count);
          final_count_chk &= (1 == async_13_d_count);
          final_count_chk &= (1 == async_13_p_count);
          if (final_count_chk) {
            ret = 0;
          }
          else printf("final_count_chk fails Some dispatch/poll fxns did not run exactly once.\n");
        }
        else printf("Not all sequence steps report back as passed.\n");
      }
      else printf("Final state report is not as expected.\n");
    }
    else printf("Failed to run the entire set of valid sequences.\n");
  }
  else printf("Failed to impose the initial state prior to test.\n");

  return ret;
}


int async_seq_test_key_listing() {
  int ret = -1;
  const uint32_t STEP_COUNT = async_seq_unit_tests.stepCount();
  StringBuilder tmp_sb;
  printf("Testing stepList() and stepCount()... ");
  printf("\tstepCount() returns (%u)... ", REAL_STEP_COUNT);
  if (REAL_STEP_COUNT == STEP_COUNT) {
    const uint32_t SL_COUNT = async_seq_unit_tests.stepList(&tmp_sb);
    printf("Pass.\n\tstepList() should also return (%u)... ", REAL_STEP_COUNT);
    if (REAL_STEP_COUNT == SL_COUNT) {
      printf("Pass.\n\tThe StringBuilder written by stepList() should have a matching count()... ");
      if (REAL_STEP_COUNT == tmp_sb.count()) {
        printf("Pass.\n");
        ret = 0;
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
    async_seq_dump_to_printf();
  }
  return ret;
}



int async_seq_test_explicit_set() {
  int ret = -1;
  const uint32_t SET_REQ      = randomUInt32();
  const uint32_t SET_RUNABLE  = randomUInt32();
  const uint32_t SET_RUNNING  = randomUInt32();
  const uint32_t SET_COMPLETE = randomUInt32();
  const uint32_t SET_PASSED   = randomUInt32();
  uint32_t get_req      = 0;
  uint32_t get_runable  = 0;
  uint32_t get_running  = 0;
  uint32_t get_complete = 0;
  uint32_t get_passed   = 0;

  printf("Testing setState() and getState()... ");
  printf("\tgetState() returns all zeroes immediately after reset... ");
  async_seq_unit_tests.resetSequencer();
  async_seq_unit_tests.getState(&get_req, &get_runable, &get_running, &get_complete, &get_passed);
  if ((0 == get_req) & (0 == get_runable) & (0 == get_running) & (0 == get_complete) & (0 == get_passed)) {
    printf("Pass.\n\tsetState(%u, %u, %u, %u, %u) imparts the proper values... ", get_req, get_runable, get_running, get_complete, get_passed);
    async_seq_unit_tests.setState(SET_REQ, SET_RUNABLE, SET_RUNNING, SET_COMPLETE, SET_PASSED);
    async_seq_unit_tests.getState(&get_req, &get_runable, &get_running, &get_complete, &get_passed);
    if ((SET_REQ == get_req) & (SET_RUNABLE == get_runable) & (SET_RUNNING == get_running) & (SET_COMPLETE == get_complete) & (SET_PASSED == get_passed)) {
      printf("Pass.\n");
      ret = 0;
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
    async_seq_dump_to_printf();
  }
  return ret;
}


int async_seq_test_abuse() {
  int ret = -1;
  ret = 0; // TODO: Test failures induced by programmer mistakes.
  int ret_local = 0;
  return ret;
}


void print_types_async_sequencer() {
  printf("\tAsyncSequencer        %u\t%u\n", sizeof(AsyncSequencer),    alignof(AsyncSequencer));
  printf("\tStepSequenceList      %u\t%u\n", sizeof(StepSequenceList),  alignof(StepSequenceList));
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int async_seq_test_main() {
  int ret = 1;   // Failure is the default result.
  const char* const MODULE_NAME = "AsyncSequencer";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == async_seq_impose_initial_state()) {
    if (0 == async_seq_test_simple_advancement()) {
      if (0 == async_seq_test_simple_failures()) {
        if (0 == async_seq_test_full_execution()) {
          if (0 == async_seq_test_key_listing()) {
            if (0 == async_seq_test_explicit_set()) {
              if (0 == async_seq_test_abuse()) {
                ret = 0;
              }
            }
          }
        }
      }
    }
  }

  return ret;
}
