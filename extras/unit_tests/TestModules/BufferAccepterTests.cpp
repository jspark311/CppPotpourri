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

*/

#include "Pipes/BufferAccepter/TestFixtures/CoDecTestFixtures.h"
#include "Pipes/BufferAccepter/Base64/C3P_B64.h"


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
  if (CAPTURE_MAX_LEN == (uint32_t) sb_sink.bufferAvailable()) {
    printf("Pass.\n\tPushing %d bytes to StringBuilderSink returns 1... ", TEST_BUF_LEN);
    if (1 == sb_sink.pushBuffer(&offering)) {
      const int LENGTH_CHECK_1 = (CAPTURE_MAX_LEN - TEST_BUF_LEN);
      printf("Pass.\n\tbufferAvailable() now reports (%d) bytes... ", LENGTH_CHECK_1);
      if (LENGTH_CHECK_1 == (uint32_t) sb_sink.bufferAvailable()) {
        printf("Pass.\n\tThe pushed buffer left the source (strictly empty)... ");
        if (offering.isEmpty(true)) {
          printf("Pass.\n\tThe pushed buffer wound up in the sink... ");
          if (0 == StringBuilder::strcasecmp((char*) sb_sink.string(), (char*) check_string.string())) {
            printf("Pass.\n\tPushing %d bytes to StringBuilderSink for a second time returns 1... ", TEST_BUF_LEN);
            generate_random_text_buffer(&offering, TEST_BUF_LEN);
            if (1 == sb_sink.pushBuffer(&offering)) {
              const int LENGTH_CHECK_2 = (CAPTURE_MAX_LEN - (TEST_BUF_LEN * 2));
              printf("Pass.\n\tPushing the second buffer had the predicted results (%d bytes available)... ", LENGTH_CHECK_2);
              if (offering.isEmpty(true) && (LENGTH_CHECK_2 == (uint32_t) sb_sink.bufferAvailable())) {
                printf("Pass.\n\tOver-capacity pushBuffer() returns 0... ");
                generate_random_text_buffer(&offering, TEST_BUF_LEN);
                if (0 == sb_sink.pushBuffer(&offering)) {
                  const int LENGTH_CHECK_3 = (TEST_BUF_LEN - SUB_CHUNK_LEN);
                  printf("Pass.\n\tThe source buffer still contains %d bytes following the incomplete claim... ", LENGTH_CHECK_3);
                  if (LENGTH_CHECK_3 == offering.length()) {
                    printf("Pass.\n\tbufferAvailable() returns 0 and length() returns (%d)... ", CAPTURE_MAX_LEN);
                    if ((0 == sb_sink.bufferAvailable()) & (CAPTURE_MAX_LEN == (uint32_t) sb_sink.length())) {
                      sb_sink.clear();
                      offering.clear();
                      printf("Pass.\n\tAble to sink its full advertised length (%d bytes)... ", sb_sink.bufferAvailable());
                      generate_random_text_buffer(&offering, sb_sink.bufferAvailable());
                      if (1 == sb_sink.pushBuffer(&offering)) {
                        printf("Pass.\n\tbufferAvailable() returns 0... ");
                        if ((0 == sb_sink.bufferAvailable()) & (CAPTURE_MAX_LEN == (uint32_t) sb_sink.length())) {
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
  if ((SINK_LIMIT == (uint32_t) ba_fork.bufferAvailable()) && (sb_sink.bufferAvailable() == ba_fork.bufferAvailable())) {
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
    const int PRELOAD_LEN_L = (int) (3 + (randomUInt32() % 43));
    const int PRELOAD_LEN_R = (int) (PRELOAD_LEN_L + 1 + (randomUInt32() % 10));
    const int LEN_CHECK_L_0 = (TEST_BUF_LEN - PRELOAD_LEN_L);
    const int LEN_CHECK_R_0 = (TEST_BUF_LEN - PRELOAD_LEN_R);
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
                              if ((TEST_BUF_LEN == (uint32_t) sink_left.length()) & (TEST_BUF_LEN == (uint32_t) sink_right.length())) {
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
    if ((-1 == ba_test_source.pushBuffer(&offering)) && (TEST_BUF_LEN == (uint32_t) offering.length())) {
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


int c3p_b64_test_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "C3P_B64";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == b64_test_encoder()) {
    if (0 == b64_test_decoder()) {
      if (0 == b64_test_loopback()) {
        ret = 0;
      }
    }
  }

  return ret;
}



/*******************************************************************************
* BufferAccepter test plan
*******************************************************************************/
#define CHKLST_BA_TEST_FIXTURES        0x00000001  // The test fixtures are sane.
#define CHKLST_BA_TEST_CODEC_BASE64    0x00000002  // Base64Encoder, Base64Decoder

#define CHKLST_BA_TESTS_ALL ( \
  CHKLST_BA_TEST_FIXTURES | CHKLST_BA_TEST_CODEC_BASE64 \
)


const StepSequenceList TOP_LEVEL_BA_TEST_LIST[] = {
  { .FLAG         = CHKLST_BA_TEST_FIXTURES,
    .LABEL        = "Test fixtures",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == ba_harness_test()) ? 1:-1);  }
  },

  // By now, we'll be able to test some of our top-level abstractions that deal
  //   with the outside world. It can be said that the true purpose of the unit
  //   tests is to have confidence in the things being tested below. Not only
  //   because their dep complexities are the highest in the library, but
  //   also because these pieces are exposed to input from the outside world
  //   (which is always in a state of anarchy).
  // Test our Base64 implementation...
  { .FLAG         = CHKLST_BA_TEST_CODEC_BASE64,
    .LABEL        = "Base64 CoDec",
    .DEP_MASK     = (CHKLST_BA_TEST_FIXTURES),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_b64_test_main()) ? 1:-1);  }
  },
};

AsyncSequencer bufferaccepter_test_plan(TOP_LEVEL_BA_TEST_LIST, (sizeof(TOP_LEVEL_BA_TEST_LIST) / sizeof(TOP_LEVEL_BA_TEST_LIST[0])));



/*******************************************************************************
* The main function.
*******************************************************************************/

void print_types_buffer_accepter() {
  printf("\tStringBuilderSink     %u\t%u\n", sizeof(StringBuilderSink),   alignof(StringBuilderSink));
  printf("\tBufferAccepterFork    %u\t%u\n", sizeof(BufferAccepterFork),  alignof(BufferAccepterFork));
  printf("\tBufAcceptTestSource   %u\t%u\n", sizeof(BufAcceptTestSource), alignof(BufAcceptTestSource));
  printf("\tBufAcceptTestSink     %u\t%u\n", sizeof(BufAcceptTestSink),   alignof(BufAcceptTestSink));
  printf("\tBase64Encoder         %u\t%u\n", sizeof(Base64Encoder),  alignof(Base64Encoder));
  printf("\tBase64Decoder         %u\t%u\n", sizeof(Base64Decoder),  alignof(Base64Decoder));
}


int buffer_accepter_main() {
  const char* const MODULE_NAME = "BufferAccepter";
  printf("===< %s >=======================================\n", MODULE_NAME);

  bufferaccepter_test_plan.requestSteps(CHKLST_BA_TESTS_ALL);
  while (!bufferaccepter_test_plan.request_completed() && (0 == bufferaccepter_test_plan.failed_steps(false))) {
    bufferaccepter_test_plan.poll();
  }
  int ret = (bufferaccepter_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  bufferaccepter_test_plan.printDebug(&report_output, "BufferAccepter test report");
  printf("%s\n", (char*) report_output.string());
  return ret;
}
