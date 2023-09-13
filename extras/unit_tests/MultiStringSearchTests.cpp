/*
File:   MultiStringSearchTests.cpp
Author: J. Ian Lindsay
Date:   2023.09.13


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


This program tests MultiStringSearch, which underpins several high-level CoDecs.

Lingo used in this test file:
"KAT":          "Known-answer test"
*/

#include "BufferAccepter/LineCoDec/LineCoDec.h"

/*******************************************************************************
* Tests for MultiStringSearch
*******************************************************************************/
#define MULT_SRCH_TEST_MAX_SEARCHES  5

/*
*
*/
int multisearch_trivial_tests() {
  int ret = -1;
  printf("Running MultiStringSearch tests...\n");

  MultiStringSearch search(MULT_SRCH_TEST_MAX_SEARCHES);

  if (0 != ret) {
    printf(" Fail.\n");
    StringBuilder log;
    search.printDebug(&log);
    printf("\n%s\n", (char*) log.string());
  }
  return ret;
}



/*
*
*/
int multisearch_known_answer_tests() {
  printf("Running known-answer tests...\n");
  //const int CASE_COUNT = (sizeof(lineterm_test_cases) / sizeof(lineterm_test_case));
  bool test_failed = false;
  int  case_idx    = 0;
  int  ret         = -1;
  return (test_failed ? -1 : 0);
}



void print_types_multisearch() {
  printf("\tMultiStringSearch        %u\t%u\n", sizeof(MultiStringSearch), alignof(MultiStringSearch));
  printf("\tStrSearchDef             %u\t%u\n", sizeof(StrSearchDef), alignof(StrSearchDef));
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int c3p_multisearch_test_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "MultiStringSearch";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == multisearch_trivial_tests()) {
    if (0 == multisearch_known_answer_tests()) {
      ret = 0;
    }
    else printTestFailure(MODULE_NAME, "Known-answer tests");
  }
  else printTestFailure(MODULE_NAME, "Trivial tests");

  return ret;
}
