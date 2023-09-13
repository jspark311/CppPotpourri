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

#define MULT_SRCH_TEST_MAX_SEARCHES  5   // How many needles should the tests support?
#define NEEDLE_BASE_LEN              4   // How big should the smallest test needle be?


/*******************************************************************************
* TODO: Over the past few weeks, I've written enough unit tests to start getting
*   a good sense of what concerns the testing of diverse objects have in-common.
* Even without a defined testing framework, abstractions are probably called for
*   by now. The functions below are being considered for migration to a testing
*   template/class/pattern/whatever.
* Also included in this consideration is the test's main function, which isn't
*  located under this comment block for (bad) structural reasons.
*******************************************************************************/

/* Type info dump */
void print_types_multisearch() {
  printf("\tMultiStringSearch        %u\t%u\n", sizeof(MultiStringSearch), alignof(MultiStringSearch));
  printf("\tStrSearchDef             %u\t%u\n", sizeof(StrSearchDef), alignof(StrSearchDef));
}


/*
* Default state check.
*
* Passed a search object, confirms the default/reset state.
* Does not consider needle definition length functions, since those are not
*   impacted by reset.
*/
bool multisearch_tests_obj_in_default_state(MultiStringSearch* obj) {
  bool ret = !obj->searchRunning();
  if (ret) {
    ret = (0 == obj->resolvedLength());
    if (ret) {
      ret = (0 == obj->unresolvedSearches());
      if (ret) {
        ret = (0 == obj->needlesFound());
        if (ret) {
          ret = (nullptr == obj->lastMatch());
          if (!ret) printf("(last_match != nullptr)");
        }
        else printf("(needles_found != 0)");
      }
      else printf("(unresolved needles)");
    }
    else printf("(resolved_length != 0)");
  }
  else printf("(search running)");

  return ret;
}


/*
* Test terminal routine.
*
* In an effort to not construct "pyramids of doom" (PoD), this function will
*   handle test conclusion by printing the object that was under test, and
*   returning the given return code to make usage a consistent one-liner.
*
* This also assures that an object's printDebug() function is still exercised
*   when all tests succeed, and thus don't have a reason to call printDebug().
*
* Hopefully, other tests can be easily re-phrase to use a descendant of this
*   pattern, since it is _far_ easier to read and maintain versus a PoD.
*/
int multisearch_tests_print_obj_and_conclude(MultiStringSearch* obj, const int RET_VALUE) {
  printf(" %s (%d).\nObject at (%p):\n", ((0 == RET_VALUE) ? "Pass":"Fail"), RET_VALUE, obj);
  if (nullptr != obj) {
    StringBuilder log;
    obj->printDebug(&log);
    printf("\n%s\n", (char*) log.string());
  }
  return RET_VALUE;
}


/*******************************************************************************
* Tests for MultiStringSearch
*******************************************************************************/

/*
* In an effort to not construct "pyramids of doom" (PoD), this function eschews
*   all concern for cyclomatic complexity, and succeeds by never bailing out.
*/
int multisearch_trivial_tests() {
  printf("Running trivial tests...\n\tGenerating test data... ");

  StringBuilder empty_subject;
  StringBuilder search_subject;
  StringBuilder test_needles[MULT_SRCH_TEST_MAX_SEARCHES];
  MultiStringSearch search(MULT_SRCH_TEST_MAX_SEARCHES);

  // Generate a non-empty string that is intentionally too short.
  generate_random_text_buffer(&search_subject, (NEEDLE_BASE_LEN - 1));
  // Generate needles...
  for (int i = 0; i < MULT_SRCH_TEST_MAX_SEARCHES; i++) {
    generate_random_text_buffer(&test_needles[i], (NEEDLE_BASE_LEN + i));
  }
  printf("Done.\n\tMemory initializes on-demand... ");
  if (!search.initialized()) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tObject is in the correct default state... ");
  if (!multisearch_tests_obj_in_default_state(&search)) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tAdding an invalid needle definition returns failure... ");
  bool bad_needle_added = (0 == search.addSearchTerm(nullptr, 0));    // Clearly bad.
  bad_needle_added     |= (0 == search.addSearchTerm(nullptr, 10));   // Even worse.
  bad_needle_added     |= (0 == search.addSearchTerm((const uint8_t*) "!empty", 0));   // Better, but still bad.
  bad_needle_added     |= (0 < search.needlesDefined());
  if (bad_needle_added) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tNeedle length functions both return 0 without any defined needles... ");
  if ((0 != search.maxNeedleLength()) | (search.minNeedleLength() != search.maxNeedleLength())) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tAdding a valid needle definition works... ");
  if (0 != search.addSearchTerm(test_needles[0].string(), test_needles[0].length())) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tNeedle length functions both return (%d) for a single needle... ", NEEDLE_BASE_LEN);
  if ((NEEDLE_BASE_LEN != search.maxNeedleLength()) | (search.minNeedleLength() != search.maxNeedleLength())) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tBeginning a search fails if given invalid parameters... ");
  bool search_failed_successfully = (-1 == search.runSearch(nullptr, 0));   // Someone will try it...
  search_failed_successfully     &= (-1 == search.runSearch(nullptr, 10));  // Someone will try it...
  if (!search_failed_successfully) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tBeginning a search on an empty haystack fails... ");
  search_failed_successfully     &= (-3 == search.runSearch(&empty_subject, 0));
  search_failed_successfully     &= (-3 == search.runSearch(&empty_subject, 10));
  if (!search_failed_successfully) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tBeginning a search fails if params imply it should move outside of the haystack... ");
  search_failed_successfully     &= (-3 == search.runSearch(&search_subject, search_subject.length(), 1));
  search_failed_successfully     &= (-3 == search.runSearch(&search_subject, 1, search_subject.length()));
  // None of what just happened should have changed the state.
  search_failed_successfully     &= multisearch_tests_obj_in_default_state(&search);
  if (!search_failed_successfully) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tBeginning a search with a string shorter than the needle results in a finished search... ");
  int fxn_ret = search.runSearch(&search_subject, search_subject.length());
  if (0 != fxn_ret) {
    return multisearch_tests_print_obj_and_conclude(&search, fxn_ret);
  }
  printf("Pass.\n\tThat search should have no results... ");
  if ((0 != search.needlesFound()) | (nullptr != search.lastMatch())) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tContinuing an already-finished search returns 0... ");
  //printf("Pass.\n\tContinuing an already-finished search results in no state change... ");
  if (0 != search.continueSearch()) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tTrying to start a search prior to the conclusion of the existing search fails... ");
  search_failed_successfully     &= (-2 == search.runSearch(&search_subject, search_subject.length()));
  if (!search_failed_successfully) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\treset() works... ");
  search.reset();
  if (!multisearch_tests_obj_in_default_state(&search)) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n");
  // Wipe the subject and make it large enough for a real search.
  // Create some random junk. Then add one of the (yet un-added) needles onto
  //   the end of it. Finally, add some more random junk. This will allow us to
  //   test our test, as well as set us up for the next two test cases.
  // Loop until the subject meets criteria.
  bool regen_subject = true;
  const uint32_t JUNK_BYTE_COUNT = (NEEDLE_BASE_LEN + (randomUInt32() % 13));
  while (regen_subject) {
    const uint8_t  UNKNOWN_NEEDLE_IDX = (1 + (randomUInt32() % (MULT_SRCH_TEST_MAX_SEARCHES-1)));
    printf("\tRegenerating a subject that does not contain the needle (<%d junk><needle_def %d><%d junk>)... ", JUNK_BYTE_COUNT, UNKNOWN_NEEDLE_IDX, JUNK_BYTE_COUNT);
    search_subject.clear();
    generate_random_text_buffer(&search_subject, JUNK_BYTE_COUNT);
    search_subject.concat(test_needles[UNKNOWN_NEEDLE_IDX].string(), test_needles[UNKNOWN_NEEDLE_IDX].length());
    generate_random_text_buffer(&search_subject, JUNK_BYTE_COUNT);
    regen_subject  = (search_subject.contains((const char*) test_needles[0].string()));
    regen_subject |= (!search_subject.contains((const char*) test_needles[UNKNOWN_NEEDLE_IDX].string()));
    printf(regen_subject ? "Fail.\n" : "Done.\n");
  }

  printf("\tSearching a subject that does not contain the needle results in a finished search... ");
  if (0 != search.runSearch(&search_subject, search_subject.length())) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }
  printf("Pass.\n\tThat search should have no results... ");
  if ((0 != search.needlesFound()) | (nullptr != search.lastMatch())) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }
  search.reset();

  printf("Pass.\n\tNeedles can be added up to the declared capacity (%d)... ", MULT_SRCH_TEST_MAX_SEARCHES);
  bool add_failed = false;
  for (int i = 1; i < MULT_SRCH_TEST_MAX_SEARCHES; i++) {
    add_failed |= (0 != search.addSearchTerm(test_needles[i].string(), test_needles[i].length()));
  }
  if (add_failed) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tThe number of defined needles (%d) matches the expectation... ", search.needlesDefined());
  if (MULT_SRCH_TEST_MAX_SEARCHES != search.needlesDefined()) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tNeedle length functions consider all needles correctly... ");
  if ((NEEDLE_BASE_LEN != search.minNeedleLength()) | ((NEEDLE_BASE_LEN + MULT_SRCH_TEST_MAX_SEARCHES - 1) != search.maxNeedleLength())) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tSearching a subject that contains the needle results in an unfinished search... ");
  if (1 != search.runSearch(&search_subject, search_subject.length())) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }
  printf("Pass.\n\tThat search should have a result... ");
  StrSearchDef* result = search.lastMatch();
  if ((1 != search.needlesFound()) | (nullptr == result)) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tsearchRunning() returns true... ");
  if (!search.searchRunning()) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass (needle of length %d occured at offset %d).\n\tThat result matches where we placed it... ", result->SEARCH_STR_LEN, result->offset_start);
  if (JUNK_BYTE_COUNT != result->offset_start) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tContinuing an unfinished search with no further results returns 0... ");
  if (0 != search.continueSearch()) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  printf("Pass.\n\tlastMatch() was invalidated by the re-execution of the search... ");
  if (nullptr != search.lastMatch()) {
    return multisearch_tests_print_obj_and_conclude(&search, -1);
  }

  return multisearch_tests_print_obj_and_conclude(&search, 0);
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
