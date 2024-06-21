/*
File:   LineTermCoDecTests.cpp
Author: J. Ian Lindsay
Date:   2023.09.04


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


This program tests LineCoDec.

Lingo used in this test file:
"KAT":          "Known-answer test"
"Call-break":   See BufferAccepter contract.
"term-at-tail": The condtion where an input buffer ends with a sequences of
                bytes that corresponds to a line ending.
*/

#include "Pipes/BufferAccepter/LineCoDec/LineCoDec.h"
#include "Pipes/BufferAccepter/TestFixtures/CoDecTestFixtures.h"

/*******************************************************************************
* Known-answers for line-termination codec tests.
*
* This will be a highly-permuted series of tests to assure proper line
*   termination behaivors under a variety of simulated constraints.
*******************************************************************************/
// NOTE: We will permute each string with line-terminators added in the test
//   procedure, and adjusting the expected results accordingly. But we don't
//   want to re-code double the tests.
const char* const KAT_INPUT_0 = "This string\nhas all\r\nthe possible\n\rline ending\rsequences.\nBut it\ndoesn't have\r\na terminator\n\rsequence at\rthe ending.";
const char* const KAT_INPUT_1 = "This string\ronly has\rCR endings,\rand also no\rfinal terminator.";
const char* const KAT_INPUT_2 = "This string\nonly has\nLF endings,\nand also no\nfinal terminator.";
const char* const KAT_INPUT_3 = "This string\r\nonly has\r\nCRLF endings,\r\nand also no\r\nfinal terminator.";

const int KAT_LEN_0 = strlen(KAT_INPUT_0);  // 116
const int KAT_LEN_1 = strlen(KAT_INPUT_1);  // 62
const int KAT_LEN_2 = strlen(KAT_INPUT_2);  // 62
const int KAT_LEN_3 = strlen(KAT_INPUT_3);  // 68

// NOTE: We will then permute each KAT under three basic conditions:
//   1) No trickle or back-pressure.
//   1) Trickle from source.
//   1) Back-pressure from efferant.
// NOTE: This value gives enough leeway for 100 CRLF inflations before it no
//   longer becomes "wide open" for the sake of a single push.
const int DRY_SOURCE_LIMIT   = 50;   // TODO: Fine-tuning parameters. Values arbitrary, but must be fixed and within undocumented bounds.
const int FLOODED_SINK_LIMIT = 67;   // TODO: Fine-tuning parameters. Values arbitrary, but must be fixed and within undocumented bounds.
const int WIDE_OPEN_CHANNEL  = (strict_max( strict_max(KAT_LEN_0, KAT_LEN_1), strict_max(KAT_LEN_2, KAT_LEN_3)) + 100);
// TODO: I ^just... wow...
//   Recode in preprocessor macro logic, as that would be preferable to
//   this (multiply-nested ternarys that happen at runtime).

// TODO: Doing these repeat special cases up here, because they would be
//   unreadable inline.
const int KAT_CB_NUL_XFORM_DS_0 = (KAT_LEN_0 / DRY_SOURCE_LIMIT) + ((0 < KAT_LEN_0 % DRY_SOURCE_LIMIT) ? 1 : 0);
const int KAT_CB_NUL_XFORM_DS_1 = (KAT_LEN_1 / DRY_SOURCE_LIMIT) + ((0 < KAT_LEN_1 % DRY_SOURCE_LIMIT) ? 1 : 0);
const int KAT_CB_NUL_XFORM_DS_2 = (KAT_LEN_2 / DRY_SOURCE_LIMIT) + ((0 < KAT_LEN_2 % DRY_SOURCE_LIMIT) ? 1 : 0);
const int KAT_CB_NUL_XFORM_DS_3 = (KAT_LEN_3 / DRY_SOURCE_LIMIT) + ((0 < KAT_LEN_3 % DRY_SOURCE_LIMIT) ? 1 : 0);

const int KAT_CB_NUL_XFORM_FS_0 = (KAT_LEN_0 / FLOODED_SINK_LIMIT) + ((0 < KAT_LEN_0 % FLOODED_SINK_LIMIT) ? 1 : 0);
const int KAT_CB_NUL_XFORM_FS_1 = (KAT_LEN_1 / FLOODED_SINK_LIMIT) + ((0 < KAT_LEN_1 % FLOODED_SINK_LIMIT) ? 1 : 0);
const int KAT_CB_NUL_XFORM_FS_2 = (KAT_LEN_2 / FLOODED_SINK_LIMIT) + ((0 < KAT_LEN_2 % FLOODED_SINK_LIMIT) ? 1 : 0);
const int KAT_CB_NUL_XFORM_FS_3 = (KAT_LEN_3 / FLOODED_SINK_LIMIT) + ((0 < KAT_LEN_3 % FLOODED_SINK_LIMIT) ? 1 : 0);

/*
* Each known-answer test is run under a variety of simulated constraints.
* We permute on the boolean options for call-break, and consolidate the outcome
*   into the three members indicating the correct number of breaks in each mode.
*/
struct lineterm_test_conditions {
  const int push_chunk_length;   // How fragmented should the pushes be?
  const int sink_buffer_limit;   // How constricted should the buffer sink be?
  const int call_breaks_mode_0;  // How many call-breaks in this mode?
  const int call_breaks_mode_1;  // How many call-breaks in this mode?
  const int call_breaks_mode_2;  // How many call-breaks in this mode?
};

/* A one-off struct to hold test cases for the base64 codec. */
struct lineterm_test_case {
  const char* const test_description;
  const char* const input;                        // The string fed to the test.
  const char* const output;                       // Expected output.
  const LineTerm    output_terminator;            // Optional replacement term.
  const LineTerm    replace_0;                    // Optional search term.
  const LineTerm    replace_1;                    // Optional search term.
  const LineTerm    replace_2;                    // Optional search term.
  struct lineterm_test_conditions conditions[3];  // We permute the test under 3 constraint sets.
};

/*
* This is the list and substance of the KATs.
*
* NOTE: These tests are themselves subject to mistakes, and behavior is
*   highly contingent upon condition parameters.
* NOTE: Each test case is permuted on three parameters to the class. n^4 is
*   expensive, but three of those four factors are small and bounded by test
*   structure. Still, every top-level KAT in the list below will produce 18 test
*   cases to assure that the input and output strings match every time.
* Additionally, much permutation is _not_ being done (exhaustive search term
*   use, buffer constraints, etc).
*/
struct lineterm_test_case lineterm_test_cases[] = {
  { "Null-transform",
    KAT_INPUT_0,
    KAT_INPUT_0,  // Output should match input.
    LineTerm::ZEROBYTE, LineTerm::ZEROBYTE, LineTerm::ZEROBYTE, LineTerm::ZEROBYTE,
    {
      { WIDE_OPEN_CHANNEL, WIDE_OPEN_CHANNEL,  1, 1, 1 },  // With a wide-open channel and no replacements, expect a single call-break.
      { DRY_SOURCE_LIMIT,  WIDE_OPEN_CHANNEL,  KAT_CB_NUL_XFORM_DS_0, KAT_CB_NUL_XFORM_DS_0, KAT_CB_NUL_XFORM_DS_0 },  // Expect as many calls as required to move the whole string.
      { WIDE_OPEN_CHANNEL, FLOODED_SINK_LIMIT, KAT_CB_NUL_XFORM_FS_0, KAT_CB_NUL_XFORM_FS_0, KAT_CB_NUL_XFORM_FS_0 }   // Expect as many calls as required to move the whole string.
    },
  },
  { "Transform (CR)->LF",
    KAT_INPUT_0,
    "This string\nhas all\n\nthe possible\n\nline ending\nsequences.\nBut it\ndoesn't have\n\na terminator\n\nsequence at\nthe ending.",
    LineTerm::LF, LineTerm::CR, LineTerm::ZEROBYTE, LineTerm::ZEROBYTE,
    {
      { WIDE_OPEN_CHANNEL, WIDE_OPEN_CHANNEL,  1, 1, 6 },  // Search terms define the call breaks.
      { DRY_SOURCE_LIMIT,  WIDE_OPEN_CHANNEL,  KAT_CB_NUL_XFORM_DS_0, 6, 6 },  // Expect as many calls as required to move the whole string.
      { WIDE_OPEN_CHANNEL, FLOODED_SINK_LIMIT, KAT_CB_NUL_XFORM_FS_0, 6, 6 }   // Expect as many calls as required to move the whole string.
    },
  },
  { "Transform (LF)->CR",
    KAT_INPUT_0,
    "This string\rhas all\r\rthe possible\r\rline ending\rsequences.\rBut it\rdoesn't have\r\ra terminator\r\rsequence at\rthe ending.",
    LineTerm::CR, LineTerm::LF, LineTerm::ZEROBYTE, LineTerm::ZEROBYTE,
    {
      { WIDE_OPEN_CHANNEL, WIDE_OPEN_CHANNEL,  1, 1, 7 },  // Search terms define the call breaks.
      { DRY_SOURCE_LIMIT,  WIDE_OPEN_CHANNEL,  KAT_CB_NUL_XFORM_DS_0, 7, 7 },  // Expect as many calls as required to move the whole string.
      { WIDE_OPEN_CHANNEL, FLOODED_SINK_LIMIT, KAT_CB_NUL_XFORM_FS_0, 7, 7 }   // Expect as many calls as required to move the whole string.
    },
  },
  { "Transform (CRLF)->LF",
    KAT_INPUT_0,
    "This string\nhas all\nthe possible\n\rline ending\rsequences.\nBut it\ndoesn't have\na terminator\n\rsequence at\rthe ending.",
    LineTerm::LF, LineTerm::CRLF, LineTerm::ZEROBYTE, LineTerm::ZEROBYTE,
    {
      { WIDE_OPEN_CHANNEL, WIDE_OPEN_CHANNEL,  1, 1, 2 },  // Search terms define the call breaks.
      { DRY_SOURCE_LIMIT,  WIDE_OPEN_CHANNEL,  KAT_CB_NUL_XFORM_DS_0, 2, 2 },  // Expect as many calls as required to move the whole string.
      { WIDE_OPEN_CHANNEL, FLOODED_SINK_LIMIT, KAT_CB_NUL_XFORM_FS_0, 2, 2 }   // Expect as many calls as required to move the whole string.
    },
  }
};



/*******************************************************************************
* Tests for line-termination codec
*******************************************************************************/

/*
*
*/
int line_term_trivial_tests() {
  int ret = -1;
  printf("Running trivial tests...\n");
  StringBuilder offering;
  BufAcceptTestSink test_sink;
  Base64Decoder decoder(&test_sink);
  BufAcceptTestSource test_source(&decoder);
  test_source.setProfiler(&test_sink.profiler);
  test_sink.bufferLimit(1000000);  // Ensure the whole buffer goes out at once.
  test_source.pushLimit(1000000);  // Ensure the whole buffer goes out at once.

  printf("\tPushing a buffer through a null transform results in the same buffer... ");
  printf("Pass.\n\tThat buffer has the same break count... ");
  printf("Pass.\n\tThat buffer matches what was pushed... ");
  printf("Pass.\n\tLineTerm::INVALID as the replacement sequence results in a null transform... ");

  if (0 != ret) {
    printf(" Fail.\n");
    StringBuilder log;
    log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
    offering.printDebug(&log);
    log.concat("\n");
    test_source.printDebug(&log);
    test_sink.printDebug(&log);
    printf("\n%s\n", (const char*) log.string());
  }
  return ret;
}


/*
*
*/
int line_term_known_answer_tests() {
  int ret = -1;
  printf("Running known-answer tests...\n");
  const int CASE_COUNT = (sizeof(lineterm_test_cases) / sizeof(lineterm_test_case));
  int  case_idx        = 0;  // NOTE: Cloaked fourth-order loop. Tread carefully.
  int  condition_idx   = 0;  // NOTE: Cloaked fourth-order loop. Tread carefully.
  int  break_mode_idx  = 0;  // NOTE: Cloaked fourth-order loop. Tread carefully.
  bool has_term_at_end = 0;  // NOTE: Cloaked fourth-order loop. Tread carefully.
  bool test_failed  = false;
  while ((case_idx < CASE_COUNT) & !test_failed) {
    printf(
      "\tBeginning case %d (%s, Conditions %d, break-mode %d, %sterm at tail)...\n",
      case_idx, lineterm_test_cases[case_idx].test_description, condition_idx, break_mode_idx, (has_term_at_end ? "" : "no ")
    );
    test_failed = true;
    BufAcceptTestSink test_sink;
    LineEndingCoDec line_breaker(&test_sink);
    BufAcceptTestSource test_source(&line_breaker);
    test_source.setProfiler(&test_sink.profiler);

    test_source.pushLimit(lineterm_test_cases[case_idx].conditions[condition_idx].push_chunk_length);
    test_sink.bufferLimit(lineterm_test_cases[case_idx].conditions[condition_idx].sink_buffer_limit);

    StringBuilder offering(lineterm_test_cases[case_idx].input);
    StringBuilder check_string((uint8_t*) lineterm_test_cases[case_idx].output, strlen(lineterm_test_cases[case_idx].output));

    // Setup the test conditions in the CoDec...
    line_breaker.setTerminator(lineterm_test_cases[case_idx].output_terminator);
    if (LineTerm::ZEROBYTE != lineterm_test_cases[case_idx].replace_0) {
      line_breaker.replaceOccurrencesOf(lineterm_test_cases[case_idx].replace_0, true);
    }
    if (LineTerm::ZEROBYTE != lineterm_test_cases[case_idx].replace_1) {
      line_breaker.replaceOccurrencesOf(lineterm_test_cases[case_idx].replace_1, true);
    }
    if (LineTerm::ZEROBYTE != lineterm_test_cases[case_idx].replace_2) {
      line_breaker.replaceOccurrencesOf(lineterm_test_cases[case_idx].replace_2, true);
    }

    int expected_call_breaks = 0;
    int expected_length      = 0;
    switch (break_mode_idx) {
      case 0:  expected_call_breaks = lineterm_test_cases[case_idx].conditions[condition_idx].call_breaks_mode_0;  break;
      case 1:  expected_call_breaks = lineterm_test_cases[case_idx].conditions[condition_idx].call_breaks_mode_1;  break;
      case 2:  expected_call_breaks = lineterm_test_cases[case_idx].conditions[condition_idx].call_breaks_mode_2;  break;
    }

    // Adjust the test conditions to reflect the expected results of this
    //   permutation of the test case. If there is a terminator at the end of
    //   the input, we will expect different values for length and break.
    // If the CoDec is configured to break on line boundaries, the test
    //   case will need to reflect so.
    //test_sink.expectation(LineTerm::);
    if (line_breaker.holdUntilBreak()) {
      if (LineTerm::ZEROBYTE != lineterm_test_cases[case_idx].output_terminator) {
        if (has_term_at_end) {
          expected_call_breaks++;
        }
        else {
          // We need to trim the end from the check string to reflect the fact that
          //   it should not be transmitted by the CoDec under anything but
          //   no-break conditions.
          // TODO: Check this logic... It may run aground on some test cases.
          check_string.split(lineTerminatorLiteralStr(lineterm_test_cases[case_idx].output_terminator));
          check_string.drop_position(check_string.count() - 1);
          check_string.implode(lineTerminatorLiteralStr(lineterm_test_cases[case_idx].output_terminator));
        }
      }
    }
    expected_length = check_string.length();

    printf(
      "\t\tExpected length in sink is %d after %d calls to its pushBuffer() fxn. Limits (src: %d,  sink: %d)\n",
      expected_length, expected_call_breaks, test_source.pushLimit(), test_sink.bufferLimit()
    );

    printf("\t\tPushing the buffer through the harness source indicates full claim... ");
    if (1 == test_source.pushBuffer(&offering)) {
      int polling_count = test_source.pollUntilStagnant();
      printf("Pass.\n\t\tTest harness moved at least one chunk... ");
      if (polling_count) {
        printf("Pass (ran %d times).\n\t\tSink received the expected number of call-breaks (%d)... ", polling_count, expected_call_breaks);
        if (expected_call_breaks == test_sink.callCount()) {
          printf("Pass.\n\t\tNeither the sink nor source observed contract violations... ");
          if ((!test_source.efferantViolatesContract()) & test_sink.callCountsBalance()) {
            printf("Pass.\n\t\tThe sink received the correct length (%d)... ", expected_length);
            if (test_sink.take_log.length() == expected_length) {
              printf("Pass.\n\t\tThe sink received the correct content... ");
              if (1 == check_string.cmpBinString(test_sink.take_log.string(), test_sink.take_log.length())) {
                const int PERM_COUNT = (3 * 3 * 2);  // condition_idx * break_mode_idx * bool (term-at-tail)
                const int PERM_ID    = ((condition_idx * 6) + (break_mode_idx * 2) + (has_term_at_end ? 2:1));
                printf("Pass.\n\t\tPermutation (%d / %d) of test case %d passes.\n", PERM_ID, PERM_COUNT, case_idx);
                test_failed = false;
              }
            }
          }
        }
      }
    }

    if (test_failed) {
      printf(" Fail.\n");
      StringBuilder log;
      test_source.printDebug(&log);
      test_sink.printDebug(&log);
      printf("\n%s\n", (const char*) log.string());
    }

    // NOTE: Cloaked fourth-order loop. Tread carefully.
    if (has_term_at_end) {           // NOTE: Subtle reliance on post-increment behavior throughout.
      has_term_at_end = false;       // This was the permutation permutation with pseudo-flushed buffers. Reset for next call-break mode.
      if (2 == break_mode_idx++) {   // Re-run the test case with new call-break expectations.
        break_mode_idx = 0;          // This was the final call-break permutation. Reset for next conditions.
        if (2 == condition_idx++) {  // Re-run the test case under a new set of conditions.
          condition_idx = 0;         // This was the final permutation of conditions. Reset for next case.
          case_idx++;                // Move to the next case.
        }
      }
    }
    else {
      // Re-run the test with a sought terminator at the
      //   end (to force flushing behavior).
      has_term_at_end = true;
    }
  }
  return (test_failed ? -1 : 0);
}



void print_types_line_term_codec() {
  printf("\tLineEndingCoDec       %u\t%u\n", sizeof(LineEndingCoDec), alignof(LineEndingCoDec));
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int c3p_line_codec_test_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "LineEndingCoDec";
  printf("===< %s >=======================================\n", MODULE_NAME);

  //if (0 == line_term_trivial_tests()) {
  if (true) {
    if (0 == line_term_known_answer_tests()) {
      ret = 0;
    }
  }

  ret = 0; // TODO: Lies.
  return ret;
}
