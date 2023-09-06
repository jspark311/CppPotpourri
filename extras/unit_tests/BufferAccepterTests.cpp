/*
File:   BufferAccepterTests.cpp
Author: J. Ian Lindsay
Date:   2023.08.25

TODO: This test does not yet cover:
  * Line endings for RX/TX
  * Argument type parsing


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


This program tests StringBuilder, which is our preferred buffer abstraction.
This class makes extensive use of the heap, low-level memory assumptions, and is
  used as a premise for basically every program built on CppPotpourri. It should
  be extensively unit-tested.
*/

#include "BufferAccepter/TestFixtures/CoDecTestFixtures.h"


/*******************************************************************************
* Tests for isolated BufferAccepters in BufferAccepter.cpp
*******************************************************************************/
/*
* All tests for StringBuilderSink.
*/
int ba_sb_sink_test() {
  int ret = -1;
  printf("Running StringBuilderSink tests...\n");
  const uint32_t TEST_BUF_LEN    = (17 + (randomUInt32() % 15));
  const uint32_t SUB_CHUNK_LEN   = (TEST_BUF_LEN - (2 + (randomUInt32() % 7)));
  const uint32_t CAPTURE_MAX_LEN = ((TEST_BUF_LEN * 2) + SUB_CHUNK_LEN);
  StringBuilderSink sb_sink(CAPTURE_MAX_LEN);
  StringBuilder offering;
  printf("\tGenerating test string... ");
  generate_random_text_buffer(&offering, TEST_BUF_LEN);
  StringBuilder check_string(offering.string(), offering.length());
  printf("Done (%d bytes):  %s\n", TEST_BUF_LEN, (char*) check_string.string());

  printf("\tbufferAvailable() returns the size of CAPTURE_MAX_LEN (%d)... ", CAPTURE_MAX_LEN);
  if (CAPTURE_MAX_LEN == sb_sink.bufferAvailable()) {
    printf("Pass.\n\tPushing %d bytes to StringBuilderSink returns 1... ", TEST_BUF_LEN);
    if (1 == sb_sink.pushBuffer(&offering)) {
      const int LENGTH_CHECK_1 = (CAPTURE_MAX_LEN - TEST_BUF_LEN);
      printf("Pass.\n\tbufferAvailable() now reports (%d) bytes... ", LENGTH_CHECK_1);
      if (LENGTH_CHECK_1 == sb_sink.bufferAvailable()) {
        printf("Pass.\n\tThe pushed buffer left the source (strictly empty)... ");
        if (offering.isEmpty(true)) {
          printf("Pass.\n\tThe pushed buffer wound up in the sink... ");
          if (0 == StringBuilder::strcasecmp((char*) sb_sink.string(), (char*) check_string.string())) {
            printf("Pass.\n\tPushing %d bytes to StringBuilderSink for a second time returns 1... ", TEST_BUF_LEN);
            generate_random_text_buffer(&offering, TEST_BUF_LEN);
            if (1 == sb_sink.pushBuffer(&offering)) {
              const int LENGTH_CHECK_2 = (CAPTURE_MAX_LEN - (TEST_BUF_LEN * 2));
              printf("Pass.\n\tPushing the second buffer had the predicted results (%d bytes available)... ", LENGTH_CHECK_2);
              if (offering.isEmpty(true) && (LENGTH_CHECK_2 == sb_sink.bufferAvailable())) {
                printf("Pass.\n\tOver-capacity pushBuffer() returns 0... ");
                generate_random_text_buffer(&offering, TEST_BUF_LEN);
                if (0 == sb_sink.pushBuffer(&offering)) {
                  const int LENGTH_CHECK_3 = (TEST_BUF_LEN - SUB_CHUNK_LEN);
                  printf("Pass.\n\tThe source buffer still contains %d bytes following the incomplete claim... ", LENGTH_CHECK_3);
                  if (LENGTH_CHECK_3 == offering.length()) {
                    printf("Pass.\n\tbufferAvailable() returns 0 and length() returns (%d)... ", CAPTURE_MAX_LEN);
                    if ((0 == sb_sink.bufferAvailable()) & (CAPTURE_MAX_LEN == sb_sink.length())) {
                      sb_sink.clear();
                      offering.clear();
                      printf("Pass.\n\tAble to sink its full advertised length (%d bytes)... ", sb_sink.bufferAvailable());
                      generate_random_text_buffer(&offering, sb_sink.bufferAvailable());
                      if (1 == sb_sink.pushBuffer(&offering)) {
                        printf("Pass.\n\tbufferAvailable() returns 0... ");
                        if ((0 == sb_sink.bufferAvailable()) & (CAPTURE_MAX_LEN == sb_sink.length())) {
                          printf("Pass.\n\tStringBuilderSink passes tests.\n");
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
    printf(" Fail.\n");
    StringBuilder log;
    log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
    offering.printDebug(&log);
    log.concatf("\nRemaining sb_sink contents: (%u bytes)\n", sb_sink.length());
    sb_sink.printDebug(&log);
    printf("\n%s\n", (const char*) log.string());
  }
  return ret;
}



/* Tests for BufferAccepterFork that are one-sided. */
int ba_fork_single_test(bool is_right) {
  int ret = -1;
  const uint32_t SINK_LIMIT      = (171 + (randomUInt32() % 15));
  const uint32_t TEST_BUF_LEN    = (SINK_LIMIT / 3);
  printf("\tGenerating test string... ");
  StringBuilder offering;
  generate_random_text_buffer(&offering, TEST_BUF_LEN);
  StringBuilder check_string(offering.string(), offering.length());
  printf("Done (%d bytes).\n", TEST_BUF_LEN);
  BufferAccepterFork ba_fork;
  StringBuilderSink sb_sink(SINK_LIMIT);
  if (is_right) {
    printf("\tAssigned sb_sink to right-hand of fork...\n");
    ba_fork.leftHand(nullptr);
    ba_fork.rightHand(&sb_sink);
  }
  else {
    printf("\tAssigned sb_sink to left-hand of fork...\n");
    ba_fork.leftHand(&sb_sink);
    ba_fork.rightHand(nullptr);
  }
  printf("\tbufferAvailable() returns the limit of the only attached sink (%d)... ", SINK_LIMIT);
  if ((SINK_LIMIT == ba_fork.bufferAvailable()) && (sb_sink.bufferAvailable() == ba_fork.bufferAvailable())) {
    printf("Pass.\n\tPushing %d bytes to BufferAccepterFork returns 1... ", TEST_BUF_LEN);
    if (1 == ba_fork.pushBuffer(&offering)) {
      const int LENGTH_CHECK_1 = (SINK_LIMIT - TEST_BUF_LEN);
      printf("Pass.\n\tbufferAvailable() now reports (%d) bytes... ", LENGTH_CHECK_1);
      if ((LENGTH_CHECK_1 == ba_fork.bufferAvailable()) && (sb_sink.bufferAvailable() == ba_fork.bufferAvailable())) {
        printf("Pass.\n\tPushed buffer left source (strictly empty) and wound up in sink... ");
        if (offering.isEmpty(true) && (0 == StringBuilder::strcasecmp((char*) sb_sink.string(), (char*) check_string.string()))) {
          printf("Pass.\n\tPushing %d bytes to StringBuilderSink for a second time returns 1... ", TEST_BUF_LEN);
          generate_random_text_buffer(&offering, TEST_BUF_LEN);
          if (1 == ba_fork.pushBuffer(&offering)) {
            const int LENGTH_CHECK_2 = (SINK_LIMIT - (TEST_BUF_LEN * 2));
            printf("Pass.\n\tPushing the second buffer had the predicted results (%d bytes available)... ", LENGTH_CHECK_2);
            const int LENGTH_FREE_IN_SINK = ba_fork.bufferAvailable();
            if (offering.isEmpty(true) && (LENGTH_CHECK_2 == LENGTH_FREE_IN_SINK)) {
              const int OVERSTUFF_LENGTH = (LENGTH_FREE_IN_SINK + (4 + (randomUInt32() % 11)));
              printf("Pass.\n\tPushing a buffer of length %d into obj that only has %d free returns 0... ", OVERSTUFF_LENGTH, LENGTH_FREE_IN_SINK);
              generate_random_text_buffer(&offering, OVERSTUFF_LENGTH);
              if (0 == ba_fork.pushBuffer(&offering)) {
                const int LENGTH_CHECK_3 = (OVERSTUFF_LENGTH - LENGTH_FREE_IN_SINK);
                printf("Pass.\n\tThe source buffer still contains %d bytes following the incomplete claim... ", LENGTH_CHECK_3);
                if (LENGTH_CHECK_3 == offering.length()) {
                  printf("Pass.\n\tbufferAvailable() returns 0 and length() returns (%d)... ", SINK_LIMIT);
                  if ((0 == ba_fork.bufferAvailable()) & (SINK_LIMIT == sb_sink.length())) {
                    printf("Pass.\n\tBufferAccepterFork %s-handed tests pass.\n", (is_right ? "right":"left"));
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

  if (0 != ret) {
    printf(" Fail.\n");
    StringBuilder log;
    log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
    offering.printDebug(&log);
    log.concatf("\n%s-hand sink contents: (%u bytes)\n", (is_right ? "Right":"Left"), sb_sink.length());
    sb_sink.printDebug(&log);
    printf("\n%s\n", (const char*) log.string());
  }
  return ret;
}


/*
* All tests for BufferAccepterFork.
*/
int ba_fork_test() {
  int ret = -1;
  {
    printf("Running BufferAccepterFork trivial tests...\n");
    BufferAccepterFork ba_fork;
    StringBuilder offering("Some buffer to test with.");
    const uint8_t* MUTATION_CANARY_0    = offering.string();
    const int      INITIAL_OFFER_LENGTH = offering.length();
    printf("\tA fork with no efferants rejects buffers, and returns 0 for bufferAvailable()... ");
    if ((0 == ba_fork.bufferAvailable()) && (-1 == ba_fork.pushBuffer(&offering))) {
      printf("Pass.\n\tSource buffer is the sanme size (%d bytes) and unmutated... ", INITIAL_OFFER_LENGTH);
      if ((INITIAL_OFFER_LENGTH == offering.length()) && (MUTATION_CANARY_0 == offering.string())) {
        printf("Pass.\n");
        ret = 0;
      }
    }
    if (0 != ret) {
      printf(" Fail.\n");
      StringBuilder log;
      log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
      offering.printDebug(&log);
      printf("\n%s\n", (const char*) log.string());
    }
  }

  /* Left-hand only. */
  if (0 == ret) {
    printf("Running BufferAccepterFork left-handed tests...\n");
    ret = ba_fork_single_test(false);
  }

  /* Right-hand only. */
  if (0 == ret) {
    printf("Running BufferAccepterFork right-handed tests...\n");
    ret = ba_fork_single_test(true);
  }

  /* Both hands, with isometric sinks. */
  if (0 == ret) {
    ret = -1;
    printf("Running BufferAccepterFork isometric sink tests...\n");
    const uint32_t TEST_BUF_LEN    = (171 + (randomUInt32() % 15));
    const uint32_t LIMIT_LEFT      = TEST_BUF_LEN;
    const uint32_t LIMIT_RIGHT     = TEST_BUF_LEN;
    StringBuilderSink sink_left(LIMIT_LEFT);
    StringBuilderSink sink_right(LIMIT_RIGHT);
    BufferAccepterFork ba_fork(&sink_left, &sink_right);
    printf("\tGenerating test string... ");
    StringBuilder offering;
    generate_random_text_buffer(&offering, TEST_BUF_LEN);
    printf("Done (%d bytes),\n", TEST_BUF_LEN);

    printf("\tA fork with both efferants returns the minimum bufferAvailable() between them... left: ");
    const uint32_t PRELOAD_LEN_L = (3 + (randomUInt32() % 43));
    const uint32_t PRELOAD_LEN_R = (PRELOAD_LEN_L + 1 + (randomUInt32() % 10));
    const int      LEN_CHECK_L_0 = (TEST_BUF_LEN - PRELOAD_LEN_L);
    const int      LEN_CHECK_R_0 = (TEST_BUF_LEN - PRELOAD_LEN_R);
    StringBuilder garbage_prefill;
    generate_random_text_buffer(&garbage_prefill, PRELOAD_LEN_L);
    sink_left.concatHandoff(&garbage_prefill);
    if (LEN_CHECK_L_0 == ba_fork.bufferAvailable()) {
      printf("Pass, right: ");
      generate_random_text_buffer(&garbage_prefill, PRELOAD_LEN_R);
      sink_right.concatHandoff(&garbage_prefill);
      const int LEN_CHECK_FORK_0 = ba_fork.bufferAvailable();
      if (LEN_CHECK_R_0 == LEN_CHECK_FORK_0) {
        printf("Pass.\n\tPusing a full-length buffer will result in a partial claim... ");
        if (0 == ba_fork.pushBuffer(&offering)) {
          printf("Pass.\n\tA fork with both efferants will take as much as the most-laden half will allow... ");
          StringBuilderSink* least_laden = (PRELOAD_LEN_L > PRELOAD_LEN_R) ? &sink_right : &sink_left;
          StringBuilderSink* most_laden  = (PRELOAD_LEN_L > PRELOAD_LEN_R) ? &sink_left : &sink_right;
          const int  LENGTH_CHECKSUM_0 = (sink_left.bufferAvailable() + sink_right.bufferAvailable());
          const bool ONE_SINK_FILLED   = ((0 == sink_left.bufferAvailable()) | (0 == sink_right.bufferAvailable()));
          if (ONE_SINK_FILLED & (0 == ba_fork.bufferAvailable())) {
            printf("Pass.\n\tThe correct amount of unclaimed bytes were left in the source... ");
            const int  LENGTH_LEFT_IN_SRC_0 = offering.length();
            if (LENGTH_LEFT_IN_SRC_0 == strict_max(PRELOAD_LEN_R, PRELOAD_LEN_L)) {
              printf("Pass.\n\tThe least-laden half of the fork will still have buffer available... ");
              if (0 < least_laden->bufferAvailable()) {
                const int AVAILABLE_IN_LEAST_LADEN = strict_abs_delta(PRELOAD_LEN_L, PRELOAD_LEN_R);
                printf("Pass.\n\tThat amount will be the difference in initial lading (%d)... ", AVAILABLE_IN_LEAST_LADEN);
                if (AVAILABLE_IN_LEAST_LADEN == least_laden->bufferAvailable()) {
                  printf("Pass.\n\tAdditional calls to pushBuffer() result in rejection... ");
                  if (-1 == ba_fork.pushBuffer(&offering)) {
                    printf("Pass.\n\tClearing the filled sink allows another partial claim... ");
                    most_laden->clear();
                    if (0 == ba_fork.pushBuffer(&offering)) {
                      printf("Pass.\n\tThat partial claim filled the previously-unfilled half of the fork... ");
                      if (0 == least_laden->bufferAvailable()) {
                        printf("Pass.\n\tAdditional calls to pushBuffer() result in rejection... ");
                        if (-1 == ba_fork.pushBuffer(&offering)) {
                          printf("Pass.\n\tClearing the sinks causes bufferAvailable() to once again return (%d)... ", TEST_BUF_LEN);
                          sink_left.clear();
                          sink_right.clear();
                          if (TEST_BUF_LEN == ba_fork.bufferAvailable()) {
                            printf("Pass.\n\tPushing a full-length buffer results in a full claim... ");
                            offering.clear();
                            generate_random_text_buffer(&offering, TEST_BUF_LEN);
                            StringBuilder check_string(offering.string(), offering.length());
                            if (1 == ba_fork.pushBuffer(&offering)) {
                              printf("Pass.\n\tBoth halves of the fork are the same (correct) length... ");
                              if ((TEST_BUF_LEN == sink_left.length()) & (TEST_BUF_LEN == sink_right.length())) {
                                printf("Pass.\n\tBoth halves of the fork have different copies of the content... ");
                                char* str_ptr_l = (char*) sink_left.string();
                                char* str_ptr_r = (char*) sink_right.string();
                                if ((str_ptr_l != str_ptr_r) & (nullptr != str_ptr_l) & (nullptr != str_ptr_r)) {
                                  printf("Pass.\n\tLeft sink matches... ");
                                  if (1 == sink_left.cmpBinString(check_string.string(), check_string.length())) {
                                    printf("Pass.\n\tRight sink matches... ");
                                    if (1 == sink_right.cmpBinString(check_string.string(), check_string.length())) {
                                      printf("Pass.\n\tBufferAccepterFork passes all isometric sink tests.\n");
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
            }
          }
        }
      }
    }

    if (0 != ret) {
     printf(" Fail.\n");
     StringBuilder log;
     log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
     offering.printDebug(&log);
     log.concatf("\nRemaining sink_left contents: (%u bytes)\n", sink_left.length());
     sink_left.printDebug(&log);
     log.concatf("\nRemaining sink_right contents: (%u bytes)\n", sink_right.length());
     sink_right.printDebug(&log);
     printf("\n%s\n", (const char*) log.string());
    }
  }
  return ret;
}


/*******************************************************************************
* Test Harness
*
* As it is itself part of the interface we are testing, the test harness needs
*   to be tested. It the tester can't pass its own tests, no test it performs
*   will actually be testing the thing under test. Which is itself a test.
*******************************************************************************/

/*
* Test the sink side's expectation matching.
* This capability will be later used to evaluate the operation of a
*   BufferAccepter being gripped by the harness.
*/
int ba_harness_sink_expectation_tests() {
  int ret = -1;
  printf("Running BufAcceptTestSink expectation tests...\n");
  BufAcceptTestSink ba_test_sink;
  StringBuilder offering("Some string");
  offering.concat("to measure with");
  ba_test_sink.bufferLimit(128);   // We won't be pressing this... Much...

  printf("\tAdding initial structured test data... ");
  int STRUCTURE_CANARY_0 = offering.count();
  ba_test_sink.profiler.markStart();
  if (1 == ba_test_sink.pushBuffer(&offering)) {
    printf("Pass.\n\tNo expectations are tracked if none are given... ");
    if ((0 == ba_test_sink.expectationsMet()) & (0 == ba_test_sink.expectationsViolated())) {
      printf("Pass.\n\tpushBuffer() is appending to the take_log in a structure-preserving manner... ");
      int STRUCTURE_CANARY_1 = ba_test_sink.take_log.count();
      if ((0 < STRUCTURE_CANARY_1) & (STRUCTURE_CANARY_0 == STRUCTURE_CANARY_1)) {
        printf("Pass.\n\tAn expectation of length can be violated... ");
        offering.concat("garbage mock data");
        ba_test_sink.expectation(offering.length() + 4);
        ba_test_sink.profiler.markStart();
        ba_test_sink.pushBuffer(&offering);
        if ((0 == ba_test_sink.expectationsMet()) & (1 == ba_test_sink.expectationsViolated())) {
          printf("Pass.\n\tAn expectation of length can be met... ");
          offering.concat("garbage mock data");
          ba_test_sink.expectation(offering.length());
          ba_test_sink.profiler.markStart();
          ba_test_sink.pushBuffer(&offering);
          if ((1 == ba_test_sink.expectationsMet()) & (1 == ba_test_sink.expectationsViolated())) {
            printf("Pass.\n\tAn expectation of termination can be violated... ");
            ba_test_sink.expectation(0);
            ba_test_sink.expectation(LineTerm::CR);
            offering.concat("garbage mock data\r\n");
            ba_test_sink.profiler.markStart();
            ba_test_sink.pushBuffer(&offering);
            if ((1 == ba_test_sink.expectationsMet()) & (2 == ba_test_sink.expectationsViolated())) {
              printf("Pass.\n\tAn expectation of termination can be met... ");
              ba_test_sink.expectation(LineTerm::LF);
              offering.concat("garbage mock data\r\n");
              ba_test_sink.profiler.markStart();
              ba_test_sink.pushBuffer(&offering);
              if ((2 == ba_test_sink.expectationsMet()) & (2 == ba_test_sink.expectationsViolated())) {
                printf("Pass.\n");
                StringBuilder log;
                ba_test_sink.printDebug(&log);
                printf("\n\tFinal Sink state: \n%s\n\n", (const char*) log.string());
                printf("\treset() clears all expectaions and take_log... ");
                ba_test_sink.reset();
                bool reset_worked = (ba_test_sink.take_log.count() == 0);
                reset_worked &= (0 == ba_test_sink.expectationsMet());
                reset_worked &= (0 == ba_test_sink.expectationsViolated());
                if (reset_worked) {
                  printf("Pass.\n\tBufAcceptTestSink passes its expectation tests.\n");
                  ret = 0;
                }
              }
            }
          }
        }
      }
    }
  }

  if (0 != ret) {
    printf(" Fail.\n");
    StringBuilder log;
    ba_test_sink.printDebug(&log);
    printf("\n%s\n\n", (const char*) log.string());
  }
  return ret;
}


/*
* Test the sink side's trivial operation.
*/
int ba_harness_sink_trivial_tests() {
  int ret = -1;
  printf("Running BufAcceptTestSink trivial tests...\n");
  BufAcceptTestSink ba_test_sink;
  StringBuilder offering("Some string ");
  offering.concat("with structure ");
  offering.concat("for measuring.");
  const int   LEN_ORGINAL_OFFERING = offering.length();
  const char* STRUCTURE_CANARY_0   = offering.position(2);  // This will exist.
  // By default, the sink has no expectations to violate, and with no delcared
  //   buffer depth. It should reject any buffers we offer it.
  printf("\tOffer to a BufferAccepter that is full should be rejected... ");
  ba_test_sink.profiler.markStart();  // Ping the profiler. Normally the source would do this.
  if (-1 == ba_test_sink.pushBuffer(&offering)) {
    printf("Pass.\n\tBufAcceptTestSink marked a rejection as a result... ");
    if (1 == ba_test_sink.countRejections() && ba_test_sink.callCountsBalance()) {
      printf("Pass.\n\tpushBuffer() does not mutate a rejected offering... ");
      const char* STRUCTURE_CANARY_1 = offering.position(2);  // This should be the same.
      if ((LEN_ORGINAL_OFFERING == offering.length()) & (STRUCTURE_CANARY_1 == STRUCTURE_CANARY_0)) {
        // The trivial rejection case works. The sink can signal back-pressure,
        //   and it didn't eat any of the offering.
        // Open the gate, and try again.
        printf("Pass.\n\tpushBuffer() takes our full offering if it is able... ");
        ba_test_sink.bufferLimit(64);
        ba_test_sink.profiler.markStart();
        if (1 == ba_test_sink.pushBuffer(&offering)) {
          printf("Pass.\n\tBufAcceptTestSink marked a full claim as a result... ");
          if (1 == ba_test_sink.countFullClaims() && ba_test_sink.callCountsBalance()) {
            printf("Pass.\n\tpushBuffer() correctly adjusts the buffer following a full claim... ");
            if (0 == offering.length()) {
              printf("Pass.\n\tpushBuffer should reject on null-pointer... ");
              // Good. As long as our offering is empty, try our malformed and
              //   trivial argument cases. Incoming crash alert...
              ba_test_sink.profiler.markStart();
              if ((-1 == ba_test_sink.pushBuffer(nullptr)) && ba_test_sink.callCountsBalance()) {
                printf("Pass.\n\tpushBuffer() should report full claim of an empty offering... ");
                ba_test_sink.profiler.markStart();
                if ((1 == ba_test_sink.pushBuffer(&offering)) && ba_test_sink.callCountsBalance()) {
                  // Good. Now test partial claim by trying to over-stuff a single
                  //   call. Four times the declared buffer limit ought to do it...
                  // The resulting StringBuilder will be nearly a worst-case for
                  //   efficiency. But that is part of the point... BufferAccepter
                  //   should manage it.
                  printf("Pass.\n\tpushBuffer() should only be able to take some of an offering and report a partial claim... ");
                  for (int i = 0; i < ba_test_sink.bufferLimit(); i++) {
                    uint32_t longword_to_add = randomUInt32();
                    offering.concat((uint8_t*) &longword_to_add, sizeof(uint32_t));
                  }
                  const int LEN_PARTIAL_OFFERING_0 = offering.length();
                  ba_test_sink.profiler.markStart();
                  if (0 == ba_test_sink.pushBuffer(&offering)) {
                    printf("Pass.\n\tBufAcceptTestSink marked a partial claim as a result... ");
                    if (1 == ba_test_sink.countPartialClaims() && ba_test_sink.callCountsBalance()) {
                      printf("Pass.\n\tpushBuffer() adjusts the buffer after its partial take... ");
                      const int LEN_PARTIAL_OFFERING_1 = offering.length();
                      if (LEN_PARTIAL_OFFERING_0 > LEN_PARTIAL_OFFERING_1) {
                        printf("Pass.\n\tpushBuffer() adjusted by the correct amount... ");
                        if (LEN_PARTIAL_OFFERING_1 == (LEN_PARTIAL_OFFERING_0 - ba_test_sink.bufferLimit())) {
                          printf("Pass.\n\treset() works... ");
                          ba_test_sink.reset();
                          bool reset_worked = ba_test_sink.callCountsBalance();
                          reset_worked &= (0 == ba_test_sink.bufferLimit());
                          reset_worked &= (0 == ba_test_sink.callCount());
                          reset_worked &= (0 == ba_test_sink.countRejections());
                          reset_worked &= (0 == ba_test_sink.countPartialClaims());
                          reset_worked &= (0 == ba_test_sink.countFullClaims());
                          reset_worked &= (0 == ba_test_sink.expectationsMet());
                          reset_worked &= (0 == ba_test_sink.expectationsViolated());
                          if (reset_worked) {
                            printf("Pass.\n\tBufAcceptTestSink passes its trivial tests.\n");
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
  }

  if (0 != ret) {
    printf(" Fail.\n");
    StringBuilder log;
    log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
    offering.printDebug(&log);
    log.concat("\n");
    ba_test_sink.printDebug(&log);
    printf("\n%s\n\n", (const char*) log.string());
  }
  return ret;
}


/*
* Test the source side's trivial operation.
*/
int ba_harness_source_trivial_tests() {
  int ret = -1;
  printf("Running BufAcceptTestSource trivial tests...\n");
  printf("\tGenerating test string... ");
  const uint32_t TEST_BUF_LEN    = (129 + (randomUInt32() % 32));
  const uint32_t CAPTURE_MAX_LEN = (TEST_BUF_LEN + 16);
  const uint32_t PUSH_LEN_LIMIT  = (16 + (randomUInt32() % 8));
  StringBuilder offering;
  generate_random_text_buffer(&offering, TEST_BUF_LEN);
  StringBuilder check_string(offering.string(), offering.length());
  printf("Done (%d bytes):\n\t%s\n", TEST_BUF_LEN, (char*) check_string.string());
  BufAcceptTestSource ba_test_source;
  StringBuilderSink sb_sink(CAPTURE_MAX_LEN);

  printf("\tbufferAvailable() with no efferant returns 0... ");
  if (0 == ba_test_source.bufferAvailable()) {
    printf("Pass.\n\tPush to BufAcceptTestSource with no efferant returns -1... ");
    if ((-1 == ba_test_source.pushBuffer(&offering)) && (TEST_BUF_LEN == offering.length())) {
      printf("Pass.\n\tConnecting to an efferant BufferAccepter... ");
      ba_test_source.setEfferant(&sb_sink);
      printf("Done.\n\tBufAcceptTestSource->bufferAvailable() passes through to efferant... ");
      if (CAPTURE_MAX_LEN == ba_test_source.bufferAvailable()) {
        printf("Pass.\n\tPush to BufAcceptTestSource with efferant returns 1... ");
        if ((1 == ba_test_source.pushBuffer(&offering)) && (0 == offering.length())) {
          printf("Pass.\n\tbacklogLength() is equal to the length of the just-pushed buffer... ");
          if (TEST_BUF_LEN == ba_test_source.backlogLength()) {
            printf("Pass.\n\tpoll() still returns zero... ");
            if (0 == ba_test_source.poll()) {
              printf("Pass.\n\tpoll() returns 1 after setting pushLimit(%d)... ", PUSH_LEN_LIMIT);
              ba_test_source.pushLimit(PUSH_LEN_LIMIT);
              if (1 == ba_test_source.poll()) {
                printf("Pass.\n\tbacklogLength() is equal to the size of the pushed buffer minus the chunk size... ");
                if ((TEST_BUF_LEN - PUSH_LEN_LIMIT) == ba_test_source.backlogLength()) {
                  printf("Pass.\n\tpoll() eventually returns 0 again... ");
                  int poll_count_before_stagnation = ba_test_source.pollUntilStagnant();
                  printf("Done (%d iterations)\n", poll_count_before_stagnation);
                  printf("\tbacklogLength() is equal to 0... ");
                  if (0 == ba_test_source.backlogLength()) {
                    printf("Pass.\n\tThe content of the buffer sink equals what we originally pushed... ");
                    if (0 == StringBuilder::strcasecmp((char*) sb_sink.string(), (char*) check_string.string())) {
                      printf("Pass.\n\tFinal object states... ");
                      StringBuilder log;
                      log.concatf("\nsb_sink contents: (%u bytes)\n%s\n", sb_sink.length(), (char*) sb_sink.string());
                      sb_sink.printDebug(&log);
                      log.concat("\n");
                      ba_test_source.printDebug(&log);
                      printf("\n%s\n", (const char*) log.string());
                      printf("\treset() works... ");
                      ba_test_source.reset();
                      bool reset_worked = ba_test_source.callCountsBalance();
                      reset_worked &= (0 == ba_test_source.pushLimit());
                      reset_worked &= (0 == ba_test_source.callCount());
                      reset_worked &= (0 == ba_test_source.countRejections());
                      reset_worked &= (0 == ba_test_source.countPartialClaims());
                      reset_worked &= (0 == ba_test_source.countFullClaims());
                      if (reset_worked) {
                        printf("Pass.\n\tBufAcceptTestSource passes its trivial tests.\n\n");
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

  if (0 != ret) {
    printf(" Fail.\n");
    StringBuilder log;
    log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
    offering.printDebug(&log);
    log.concat("\n");
    ba_test_source.printDebug(&log);
    printf("\n%s\n\n", (const char*) log.string());
  }
  return ret;
}


/*
* All tests for the sink side of the harness.
*/
int ba_harness_sink_tests() {
  int ret = -1;
  if (0 == ba_harness_sink_trivial_tests()) {
    if (0 == ba_harness_sink_expectation_tests()) {
      ret = 0;
    }
    else printf("BufAcceptTestSink failed its expectation tests.\n");
  }
  else printf("BufAcceptTestSink failed its trivial tests.\n");
  return ret;
}


/*
* All tests for the source side of the harness.
*/
int ba_harness_source_tests() {
  int ret = -1;
  if (0 == ba_sb_sink_test()) {
    if (0 == ba_fork_test()) {
      if (0 == ba_harness_source_trivial_tests()) {
        ret = 0;
      }
      else printf("BufAcceptTestSource failed its tests.\n");
    }
    else printf("BufferAccepterFork failed its tests.\n");
  }
  else printf("StringBuilderSink failed its tests.\n");
  return ret;
}


/*
* All tests for the harness.
*/
int ba_harness_test() {
  int ret = -1;
  if ((0 == ba_harness_sink_tests()) && (0 == ba_harness_source_tests())) {
    // Each half of our test harness looks good. Let's hook them together
    //   directly, and see if they still play nice. This is really a test of the
    //   BufferAccepter interface contract, and not the harness, which will
    //   never be used this way again.
    BufAcceptTestSource ba_test_source;
    BufAcceptTestSink ba_test_sink;
    ba_test_source.setEfferant(&ba_test_sink);
    ba_test_source.setProfiler(&ba_test_sink.profiler);

    //
    ba_test_source.pushLimit(11);
    ba_test_sink.bufferLimit(17);

    const uint32_t TEST_BUF_LEN    = (61 + (randomUInt32() % 80));
    StringBuilder offering;
    generate_random_text_buffer(&offering, TEST_BUF_LEN);

    ba_test_source.pushBuffer(&offering);
    int poll_counter = ba_test_source.pollUntilStagnant();
    printf("\tpoll() was called %d times to accomplish the request...", poll_counter);

    StringBuilder log;
    log.concatf("\nRemaining offering contents: (%u bytes)\n", offering.length());
    log.concat("\n");
    ba_test_sink.printDebug(&log);
    ba_test_source.printDebug(&log);
    printf("\n%s\n\n", (const char*) log.string());

    ret = 0;
  }
  return ret;
}


void print_types_buffer_accepter() {
  printf("\tStringBuilderSink     %u\t%u\n", sizeof(StringBuilderSink),   alignof(StringBuilderSink));
  printf("\tBufferAccepterFork    %u\t%u\n", sizeof(BufferAccepterFork),  alignof(BufferAccepterFork));
  printf("\tBufAcceptTestSource   %u\t%u\n", sizeof(BufAcceptTestSource), alignof(BufAcceptTestSource));
  printf("\tBufAcceptTestSink     %u\t%u\n", sizeof(BufAcceptTestSink),   alignof(BufAcceptTestSink));
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int buffer_accepter_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "BufferAccepter";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == ba_harness_test()) {
    ret = 0;
  }
  else printTestFailure(MODULE_NAME, "BufferAccepter doesn't have a reliable test harness.");

  return ret;
}
