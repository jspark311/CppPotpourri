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
* Test routines
*******************************************************************************/

/*
*/
int test_3ap_flags() {
  int ret = -1;
  return ret;
}


/*
*/
int test_3ap_relay() {
  int ret = -1;
  return ret;
}

/*
*/
int test_3ap_fork() {
  int ret = -1;
  return ret;
}

/*
*/
int test_3ap_axis_converter() {
  int ret = -1;
  return ret;
}


/*
*/
int test_3ap_terminus() {
  int ret = -1;
  return ret;
}

/*
*/
int test_3ap_single_filter() {
  int ret = -1;
  return ret;
}

/*
*/
int test_3ap_orientation() {
  int ret = -1;
  return ret;
}



void print_types_3ap() {
  printf("\tTripleAxisFork            %u\t%u\n", sizeof(TripleAxisFork),         alignof(TripleAxisFork));
  printf("\tTripleAxisConvention      %u\t%u\n", sizeof(TripleAxisConvention),   alignof(TripleAxisConvention));
  printf("\tTripleAxisTerminus        %u\t%u\n", sizeof(TripleAxisTerminus),     alignof(TripleAxisTerminus));
  printf("\tTripleAxisSingleFilter    %u\t%u\n", sizeof(TripleAxisSingleFilter), alignof(TripleAxisSingleFilter));
  printf("\tTripleAxisOrientation     %u\t%u\n", sizeof(TripleAxisOrientation),  alignof(TripleAxisOrientation));
}


/*******************************************************************************
* Test plan
*******************************************************************************/
#define CHKLST_3AP_TEST_WITH_FLAGS   0x00000001  // Ensures that TripleAxisPipeWithFlags behaves correctly.
#define CHKLST_3AP_TEST_RELAY        0x00000002  // Tests the fork utility class.
//#define CHKLST_3AP_TEST_  0x00000004  //
//#define CHKLST_3AP_TEST_  0x00000008  //
//#define CHKLST_3AP_TEST_  0x00000010  //
//#define CHKLST_3AP_TEST_  0x00000020  //
//#define CHKLST_3AP_TEST_  0x00000040  //
//#define CHKLST_3AP_TEST_  0x00000080  //
#define CHKLST_3AP_TEST_FORK          0x00000100  // The fork utility class.
#define CHKLST_3AP_TEST_CONV          0x00000200  // The axis reference converter.
#define CHKLST_3AP_TEST_TERMINUS      0x00000400  // Tests the pipelien terminator class.
#define CHKLST_3AP_TEST_SINGLE_FILTER 0x00000800  // Tests the time-series and filtering class.
#define CHKLST_3AP_TEST_ORIENTATION   0x00001000  // Tests the orientation filter.


#define CHKLST_3AP_TESTS_ALL ( \
  CHKLST_3AP_TEST_WITH_FLAGS | CHKLST_3AP_TEST_RELAY | \
  CHKLST_3AP_TEST_FORK | \
  CHKLST_3AP_TEST_CONV | CHKLST_3AP_TEST_TERMINUS | \
  CHKLST_3AP_TEST_SINGLE_FILTER | CHKLST_3AP_TEST_ORIENTATION)

const StepSequenceList TOP_LEVEL_3AP_TEST_LIST[] = {
  { .FLAG         = CHKLST_3AP_TEST_WITH_FLAGS,
    .LABEL        = "TripleAxisPipeWithFlags",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_flags()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_RELAY,
    .LABEL        = "Relay flag handling",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_relay()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_FORK,
    .LABEL        = "TripleAxisFork",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_fork()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_CONV,
    .LABEL        = "TripleAxisConvention",
    .DEP_MASK     = (CHKLST_SB_TEST_COUNT),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_axis_converter()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_TERMINUS,
    .LABEL        = "TripleAxisTerminus",
    .DEP_MASK     = (CHKLST_3AP_TEST_WITH_FLAGS | CHKLST_3AP_TEST_RELAY),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_terminus()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_SINGLE_FILTER,
    .LABEL        = "TripleAxisSingleFilter",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_single_filter()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_3AP_TEST_ORIENTATION,
    .LABEL        = "TripleAxisOrientation",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_3ap_orientation()) ? 1:-1);  }
  },
};

AsyncSequencer tap_test_plan(TOP_LEVEL_3AP_TEST_LIST, (sizeof(TOP_LEVEL_3AP_TEST_LIST) / sizeof(TOP_LEVEL_3AP_TEST_LIST[0])));



/*******************************************************************************
* The main function
*******************************************************************************/

int tripleaxispipe_tests_main() {
  const char* const MODULE_NAME = "TripleAxisPipe";
  printf("===< %s >=======================================\n", MODULE_NAME);

  //tap_test_plan.requestSteps(CHKLST_3AP_TESTS_ALL);
  tap_test_plan.requestSteps(0);
  while (!tap_test_plan.request_completed() && (0 == tap_test_plan.failed_steps(false))) {
    tap_test_plan.poll();
  }
  int ret = (tap_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  tap_test_plan.printDebug(&report_output, "TripleAxisPipe test report");
  printf("%s\n", (char*) report_output.string());

  return ret;
}
