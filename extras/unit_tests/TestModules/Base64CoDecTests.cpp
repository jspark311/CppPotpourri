/*
File:   Base64CoDecTests.cpp
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


This program tests C3P_B64.
*/

#include "Pipes/BufferAccepter/Base64/C3P_B64.h"
#include "Pipes/BufferAccepter/TestFixtures/CoDecTestFixtures.h"

/*******************************************************************************
* Tests for base64 codec
*******************************************************************************/

/*
* A one-off struct to hold test cases for the base64 codec
*/
struct b64_test_case {
  const char* const test_description;
  const char* const encoded;
  const uint8_t*    decoded;
  const int         ascii_len;
  const int         binary_len;
};

const uint8_t test_bin_0[24]   = {0x42, 0x61, 0x73, 0x69, 0x63, 0x2b, 0x61, 0x73, 0x63, 0x69, 0x69, 0x5f, 0x69, 0x6e, 0x70, 0x75, 0x74, 0x2d, 0x73, 0x74, 0x72, 0x69, 0x6e, 0x67};
const char* const test_asc_0   = "QmFzaWMrYXNjaWlfaW5wdXQtc3RyaW5n";

const uint8_t test_bin_1[256]  = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, \
  0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, \
  0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, \
  0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, \
  0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, \
  0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F, 0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, \
  0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, \
  0x7D, 0x7E, 0x7F, 0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, \
  0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, \
  0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, \
  0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF, 0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, \
  0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF, 0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, \
  0xDC, 0xDD, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, \
  0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF};
const char* const test_asc_1   = "AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+P0BBQkNERUZHSElKS0xNTk9QUVJTVFVWV1hZWltcXV5fYGFiY2RlZmdoaWprbG1ub3BxcnN0dXZ3eHl6e3x9fn+AgYKDhIWGh4iJiouMjY6PkJGSk5SVlpeYmZqbnJ2en6ChoqOkpaanqKmqq6ytrq+wsbKztLW2t7i5uru8vb6/wMHCw8TFxsfIycrLzM3Oz9DR0tPU1dbX2Nna29zd3t/g4eLj5OXm5+jp6uvs7e7v8PHy8/T19vf4+fr7/P3+/w==";

b64_test_case b64_test_cases[] = {
  { "Basic ASCII text, no padding",
    test_asc_0,
    test_bin_0,
    strlen(test_asc_0), sizeof(test_bin_0)},
  { "Byte rainbow",
    test_asc_1,
    test_bin_1,
    strlen(test_asc_1), sizeof(test_bin_1)},
};



/*
*
*/
int b64_test_encoder() {
  printf("Running Base64Encoder tests...\n");
  const int REPLACEMENT_CASE_COUNT = (sizeof(b64_test_cases) / sizeof(b64_test_case));
  int  case_idx     = 0;
  bool test_failed  = false;
  while ((case_idx < REPLACEMENT_CASE_COUNT) & !test_failed) {
    printf("\tBeginning test case %d (%s)...\n", case_idx, b64_test_cases[case_idx].test_description);
    test_failed = true;
    BufAcceptTestSink test_sink;
    Base64Encoder encoder(&test_sink);
    BufAcceptTestSource test_source(&encoder);
    test_source.setProfiler(&test_sink.profiler);
    test_sink.bufferLimit(1000000);  // Ensure the whole buffer goes out at once.
    test_source.pushLimit(1000000);  // Ensure the whole buffer goes out at once.

    StringBuilder offering((uint8_t*) b64_test_cases[case_idx].decoded, b64_test_cases[case_idx].binary_len);
    StringBuilder check_string(b64_test_cases[case_idx].encoded);

    printf("\tPushing the buffer through the harness source indicates full claim... ");
    if (1 == test_source.pushBuffer(&offering)) {
      int polling_count = test_source.pollUntilStagnant();
      printf("Pass.\n\tTest harness moved at least one chunk... ");
      if (polling_count) {
        printf("Pass (ran %d times).\n\tSink received a result of the correct length (%d)... ", polling_count, b64_test_cases[case_idx].ascii_len);
        if (test_sink.take_log.length() == b64_test_cases[case_idx].ascii_len) {
          printf("Pass.\n\tThe sink received the correct content... ");
          if (1 == check_string.cmpBinString(test_sink.take_log.string(), test_sink.take_log.length())) {
            printf("Pass.\n\tTest case %d passes.\n", case_idx);
            test_failed = false;
          }
        }
      }
    }

    if (test_failed) {
      printf(" Fail.\n");
      StringBuilder log;
      log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
      offering.printDebug(&log);
      log.concat("\n");
      test_source.printDebug(&log);
      test_sink.printDebug(&log);
      printf("\n%s\n", (const char*) log.string());
    }
    case_idx++;
  }
  return (test_failed ? -1 : 0);
}


/*
*
*/
int b64_test_decoder() {
  int ret = -1;
  printf("Running Base64Decoder tests...\n");
  const int REPLACEMENT_CASE_COUNT = (sizeof(b64_test_cases) / sizeof(b64_test_case));
  int  case_idx     = 0;
  bool test_failed  = false;
  while ((case_idx < REPLACEMENT_CASE_COUNT) & !test_failed) {
    printf("\tBeginning test case %d (%s)...\n", case_idx, b64_test_cases[case_idx].test_description);
    test_failed = true;
    BufAcceptTestSink test_sink;
    Base64Decoder decoder(&test_sink);
    BufAcceptTestSource test_source(&decoder);
    test_source.setProfiler(&test_sink.profiler);
    test_sink.bufferLimit(1000000);  // Ensure the whole buffer goes out at once.
    test_source.pushLimit(1000000);  // Ensure the whole buffer goes out at once.

    StringBuilder check_string((uint8_t*) b64_test_cases[case_idx].decoded, b64_test_cases[case_idx].binary_len);
    StringBuilder offering(b64_test_cases[case_idx].encoded);

    printf("\tPushing the buffer through the harness source indicates full claim... ");
    if (1 == test_source.pushBuffer(&offering)) {
      int polling_count = test_source.pollUntilStagnant();
      printf("Pass.\n\tTest harness moved at least one chunk... ");
      if (polling_count) {
        printf("Pass (ran %d times).\n\tSink received a result of the correct length (%d)... ", polling_count, b64_test_cases[case_idx].binary_len);
        if (test_sink.take_log.length() == b64_test_cases[case_idx].binary_len) {
          printf("Pass.\n\tThe sink received the correct content... ");
          if (1 == check_string.cmpBinString(test_sink.take_log.string(), test_sink.take_log.length())) {
            printf("Pass.\n\tTest case %d passes.\n", case_idx);
            test_failed = false;
          }
        }
      }
    }

    if (test_failed) {
      printf(" Fail.\n");
      StringBuilder log;
      log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
      offering.printDebug(&log);
      log.concat("\n");
      test_source.printDebug(&log);
      test_sink.printDebug(&log);
      printf("\n%s\n", (const char*) log.string());
    }
    case_idx++;
  }
  return (test_failed ? -1 : 0);
}


/*
*
*/
int b64_test_loopback() {
  int ret = -1;
  const int TEST_ITERATIONS = 32;
  printf("Running Base64 loopback tests (%d iterations)...\n", TEST_ITERATIONS);
  int loop_count = 0;
  bool test_failed  = false;
  while ((loop_count < TEST_ITERATIONS) & !test_failed) {
    const uint32_t TEST_BUF_LEN    = (1033 + (randomUInt32() % 907));
    printf("\tIteration %d (%d bytes of input)...\n", loop_count, TEST_BUF_LEN);
    test_failed = true;

    BufAcceptTestSink test_sink;
    Base64Decoder decoder(&test_sink);
    Base64Encoder encoder(&decoder);
    BufAcceptTestSource test_source(&encoder);
    test_source.setProfiler(&test_sink.profiler);
    test_sink.bufferLimit(1000000);  // Ensure the whole buffer goes out at once.
    test_source.pushLimit(1000000);  // Ensure the whole buffer goes out at once.

    StringBuilder offering;
    generate_random_text_buffer(&offering, TEST_BUF_LEN);
    StringBuilder check_string(offering.string(), offering.length());

    printf("\tPushing the buffer through the harness source indicates full claim... ");
    if (1 == test_source.pushBuffer(&offering)) {
      int polling_count = test_source.pollUntilStagnant();
      printf("Pass.\n\t\tTest harness moved at least one chunk... ");
      if (polling_count) {
        printf("Pass (ran %d times).\n\t\tSink received a result of the correct length (%d)... ", polling_count, TEST_BUF_LEN);
        if (test_sink.take_log.length() == TEST_BUF_LEN) {
          printf("Pass.\n\t\tThe sink received the correct content... ");
          if (1 == check_string.cmpBinString(test_sink.take_log.string(), test_sink.take_log.length())) {
            printf("Pass.\n\t\tIteration %d passes.\n", loop_count);
            test_failed = false;
          }
        }
      }
    }

    if (test_failed) {
      printf(" Fail.\n");
      StringBuilder log;
      log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
      offering.printDebug(&log);
      log.concat("\n");
      test_source.printDebug(&log);
      test_sink.printDebug(&log);
      printf("\n%s\n", (const char*) log.string());
    }
    loop_count++;
  }
  return (test_failed ? -1 : 0);
}


/* Type-size printout. */
void print_types_c3p_b64() {
  printf("\tBase64Encoder         %u\t%u\n", sizeof(Base64Encoder),  alignof(Base64Encoder));
  printf("\tBase64Decoder         %u\t%u\n", sizeof(Base64Decoder),  alignof(Base64Decoder));
}



/*******************************************************************************
* The main function.
*******************************************************************************/
int c3p_b64_test_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "C3P_B64";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == b64_test_encoder()) {
    if (0 == b64_test_decoder()) {
      if (0 == b64_test_loopback()) {
        ret = 0;
      }
      else printTestFailure(MODULE_NAME, "Decoder can't understand encoder's output.");
    }
    else printTestFailure(MODULE_NAME, "Decoder fails tests.");
  }
  else printTestFailure(MODULE_NAME, "Encoder fails tests");

  return ret;
}
