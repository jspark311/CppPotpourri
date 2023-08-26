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

#include "CoDecs/CoDecTestFixtures.h"


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
  if (1 == ba_test_sink.provideBuffer(&offering)) {
    printf("Pass.\n\tNo expectations are tracked if none are given... ");
    if ((0 == ba_test_sink.expectationsMet()) & (0 == ba_test_sink.expectationsViolated())) {
      printf("Pass.\n\tprovideBuffer() is appending to the take_log in a structure-preserving manner... ");
      int STRUCTURE_CANARY_1 = ba_test_sink.take_log.count();
      if ((0 < STRUCTURE_CANARY_1) & (STRUCTURE_CANARY_0 == STRUCTURE_CANARY_1)) {
        printf("Pass.\n\tAn expectation of length can be violated... ");
        offering.concat("garbage mock data");
        ba_test_sink.expectation(offering.length() + 4);
        ba_test_sink.profiler.markStart();
        ba_test_sink.provideBuffer(&offering);
        if ((0 == ba_test_sink.expectationsMet()) & (1 == ba_test_sink.expectationsViolated())) {
          printf("Pass.\n\tAn expectation of length can be met... ");
          offering.concat("garbage mock data");
          ba_test_sink.expectation(offering.length());
          ba_test_sink.profiler.markStart();
          ba_test_sink.provideBuffer(&offering);
          if ((1 == ba_test_sink.expectationsMet()) & (1 == ba_test_sink.expectationsViolated())) {
            printf("Pass.\n\tAn expectation of termination can be violated... ");
            ba_test_sink.expectation(0);
            ba_test_sink.expectation(LineTerm::CR);
            offering.concat("garbage mock data\r\n");
            ba_test_sink.profiler.markStart();
            ba_test_sink.provideBuffer(&offering);
            if ((1 == ba_test_sink.expectationsMet()) & (2 == ba_test_sink.expectationsViolated())) {
              printf("Pass.\n\tAn expectation of termination can be met... ");
              ba_test_sink.expectation(LineTerm::LF);
              offering.concat("garbage mock data\r\n");
              ba_test_sink.profiler.markStart();
              ba_test_sink.provideBuffer(&offering);
              if ((2 == ba_test_sink.expectationsMet()) & (2 == ba_test_sink.expectationsViolated())) {
                printf("Pass.\n\treset() clears all expectaions and take_log... ");
                ba_test_sink.reset();
                bool reset_worked = (ba_test_sink.take_log.count() == 0);
                reset_worked &= (0 == ba_test_sink.expectationsMet());
                reset_worked &= (0 == ba_test_sink.expectationsViolated());
                if (reset_worked) {
                  printf("Pass.\n\tBufAcceptTestSink passes its expectation tests.\n\n");
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
  if (-1 == ba_test_sink.provideBuffer(&offering)) {
    printf("Pass.\n\tBufAcceptTestSink marked a rejection as a result... ");
    if (1 == ba_test_sink.countRejections() && ba_test_sink.callCountsBalance()) {
      printf("Pass.\n\tprovideBuffer() does not mutate a rejected offering... ");
      const char* STRUCTURE_CANARY_1 = offering.position(2);  // This should be the same.
      if ((LEN_ORGINAL_OFFERING == offering.length()) & (STRUCTURE_CANARY_1 == STRUCTURE_CANARY_0)) {
        // The trivial rejection case works. The sink can signal back-pressure,
        //   and it didn't eat any of the offering.
        // Open the gate, and try again.
        printf("Pass.\n\tprovideBuffer() takes our full offering if it is able... ");
        ba_test_sink.bufferLimit(64);
        ba_test_sink.profiler.markStart();
        if (1 == ba_test_sink.provideBuffer(&offering)) {
          printf("Pass.\n\tBufAcceptTestSink marked a full claim as a result... ");
          if (1 == ba_test_sink.countFullClaims() && ba_test_sink.callCountsBalance()) {
            printf("Pass.\n\tprovideBuffer() correctly adjusts the buffer following a full claim... ");
            if (0 == offering.length()) {
              printf("Pass.\n\tprovideBuffer should reject on null-pointer... ");
              // Good. As long as our offering is empty, try our malformed and
              //   trivial argument cases. Incoming crash alert...
              ba_test_sink.profiler.markStart();
              if ((-1 == ba_test_sink.provideBuffer(nullptr)) && ba_test_sink.callCountsBalance()) {
                printf("Pass.\n\tprovideBuffer() should report full claim of an empty offering... ");
                ba_test_sink.profiler.markStart();
                if ((1 == ba_test_sink.provideBuffer(&offering)) && ba_test_sink.callCountsBalance()) {
                  // Good. Now test partial claim by trying to over-stuff a single
                  //   call. Four times the declared buffer limit ought to do it...
                  // The resulting StringBuilder will be nearly a worst-case for
                  //   efficiency. But that is part of the point... BufferAccepter
                  //   should manage it.
                  printf("Pass.\n\tprovideBuffer() should only be able to take some of an offering and report a partial claim... ");
                  for (int i = 0; i < ba_test_sink.bufferLimit(); i++) {
                    uint32_t longword_to_add = randomUInt32();
                    offering.concat((uint8_t*) &longword_to_add, sizeof(uint32_t));
                  }
                  const int LEN_PARTIAL_OFFERING_0 = offering.length();
                  ba_test_sink.profiler.markStart();
                  if (0 == ba_test_sink.provideBuffer(&offering)) {
                    printf("Pass.\n\tBufAcceptTestSink marked a partial claim as a result... ");
                    if (1 == ba_test_sink.countPartialClaims() && ba_test_sink.callCountsBalance()) {
                      printf("Pass.\n\tprovideBuffer() adjusts the buffer after its partial take... ");
                      const int LEN_PARTIAL_OFFERING_1 = offering.length();
                      if (LEN_PARTIAL_OFFERING_0 > LEN_PARTIAL_OFFERING_1) {
                        printf("Pass.\n\tprovideBuffer() adjusted by the correct amount... ");
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
                            printf("Pass.\n\tBufAcceptTestSink passes its trivial tests.\n\n");
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
* All tests for the sink side of the harness.
*/
int ba_harness_sink_tests() {
  int ret = ba_harness_sink_trivial_tests();
  if (0 == ret) {
    ret = ba_harness_sink_expectation_tests();
    if (0 != ret) {
      printf("BufAcceptTestSink failed its expectation tests.\n");
    }
  }
  else printf("BufAcceptTestSink failed its trivial tests.\n");
  return ret;
}


/*
* All tests for the source side of the harness.
*/
int ba_harness_source_tests() {
  int ret = 0;
  // TODO: This
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

    ret = 0;
  }
  return ret;
}


/*******************************************************************************
* Test blocks for specific CoDecs not covered by their own tests elsewhere.
*******************************************************************************/
int line_term_codec_tests() {
  int ret = -1;
  return ret;
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int buffer_accepter_main() {
  int ret = 1;   // Failure is the default result.
  printf("===< BufferAccepter >=======================================\n");

  if (0 == ba_harness_test()) {
    ret = 0;
  }
  else printTestFailure("BufferAccepter doesn't have a reliable test harness.");


  return ret;
}
