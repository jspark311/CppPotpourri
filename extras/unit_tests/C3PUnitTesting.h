/*
File:   C3PUnitTesting.h
Author: J. Ian Lindsay
Date:   2024.04.14


Common types, includes, and definitions for the library's unit tests.
*/
#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

#include <fstream>
#include <iostream>

#include "CppPotpourri.h"
#include "StringBuilder.h"
#include "AsyncSequencer.h"
#include "PriorityQueue.h"


class C3PTestGroup {
  public:
    C3PTestGroup(const char* const NAME, const uint32_t REQ_MASK, const StepSequenceList* const TEST_LIST, const uint8_t COUNT)
      : _GROUP_NAME(NAME), _REQUEST_MASK(REQ_MASK), _test_list(TEST_LIST, COUNT) {};

    ~C3PTestGroup() {};

    /* Print the report for the test group. */
    int8_t runTestGroup() {
      printf("===< %s >=======================================\n", _GROUP_NAME);
      _test_list.requestSteps(_REQUEST_MASK);
      while (!_test_list.request_completed() && (0 == _test_list.failed_steps(false))) {
        _test_list.poll();
      }
      return ((_test_list.request_fulfilled() ? 0 : 1));
    };

    /* Print the report for the test group. */
    void printTestReport() {
      StringBuilder report_output;
      _test_list.printDebug(&report_output, _GROUP_NAME);
      printf("%s\n", (char*) report_output.string());
    };


  private:
    const char* const _GROUP_NAME;
    const uint32_t _REQUEST_MASK;
    AsyncSequencer _test_list;
};


extern PriorityQueue<C3PTestGroup*> TOP_LEVEL_TESTS;
