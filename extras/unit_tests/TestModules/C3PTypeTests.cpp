/*
File:   C3PTypeTests.cpp
Author: J. Ian Lindsay
Date:   2023.10.22

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


This program tests C3PType, which is C3P's internal manifest of types and their
  handlers.
*/

#include "C3PValue/C3PType.h"


/*******************************************************************************
* C3PType test routines
*******************************************************************************/



/*******************************************************************************
* C3PType test plan
*******************************************************************************/
#define CHKLST_C3PTYPE_TEST_PRIMITIVES    0x00000001  // Basal types with no memory implications.
#define CHKLST_C3PTYPE_TEST_VECTORS       0x00000002  //
#define CHKLST_C3PTYPE_TEST_STRINGS       0x00000004  //
#define CHKLST_C3PTYPE_TEST_KVP           0x00000008  //
#define CHKLST_C3PTYPE_TEST_IDENTITY      0x00000010  //
#define CHKLST_C3PTYPE_TEST_BLOBS         0x00000020  //

#define CHKLST_C3PTYPE_TEST_ALL ( \
  CHKLST_C3PTYPE_TEST_PRIMITIVES | CHKLST_C3PTYPE_TEST_VECTORS | \
  CHKLST_C3PTYPE_TEST_STRINGS | CHKLST_C3PTYPE_TEST_KVP | \
  CHKLST_C3PTYPE_TEST_IDENTITY | CHKLST_C3PTYPE_TEST_BLOBS)

const StepSequenceList TOP_LEVEL_C3PTYPE_TEST_LIST[] = {
  { .FLAG         = CHKLST_C3PTYPE_TEST_PRIMITIVES,
    .LABEL        = "Primitives",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = CHKLST_C3PTYPE_TEST_VECTORS,
    .LABEL        = "Vectors",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = CHKLST_C3PTYPE_TEST_STRINGS,
    .LABEL        = "String types",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = CHKLST_C3PTYPE_TEST_KVP,
    .LABEL        = "KeyValuePair",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = CHKLST_C3PTYPE_TEST_IDENTITY,
    .LABEL        = "Identity",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
  { .FLAG         = CHKLST_C3PTYPE_TEST_BLOBS,
    .LABEL        = "Big lists of bytes",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }
  },
};

AsyncSequencer c3ptype_test_plan(TOP_LEVEL_C3PTYPE_TEST_LIST, (sizeof(TOP_LEVEL_C3PTYPE_TEST_LIST) / sizeof(TOP_LEVEL_C3PTYPE_TEST_LIST[0])));



/*******************************************************************************
* The main function.
*******************************************************************************/

void print_types_c3p_type() {
  printf("\tvoid*                    %u\t%u\n", sizeof(void*),  alignof(void*));
  printf("\tunsigned int             %u\t%u\t%016x\n", sizeof(unsigned int),  alignof(unsigned int), UINT_MAX);
  printf("\tunsigned long            %u\t%u\t%lu\n", sizeof(unsigned long),  alignof(unsigned long), ULONG_MAX);
  printf("\tbool                     %u\t%u\n", sizeof(bool),   alignof(bool));
  printf("\tfloat                    %u\t%u\n", sizeof(float),  alignof(float));
  printf("\tdouble                   %u\t%u\n", sizeof(double), alignof(double));
  printf("\tC3PBinBinder             %u\t%u\n", sizeof(C3PBinBinder), alignof(C3PBinBinder));
  printf("\tC3PType                  %u\t%u\n", sizeof(C3PType),      alignof(C3PType));
}


int c3p_type_test_main() {
  const char* const MODULE_NAME = "C3PType";
  printf("===< %s >=======================================\n", MODULE_NAME);

  c3ptype_test_plan.requestSteps(CHKLST_C3PTYPE_TEST_ALL);
  while (!c3ptype_test_plan.request_completed() && (0 == c3ptype_test_plan.failed_steps(false))) {
    c3ptype_test_plan.poll();
  }
  int ret = (c3ptype_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  c3ptype_test_plan.printDebug(&report_output, "C3PValue test report");
  printf("%s\n", (char*) report_output.string());
  return ret;
}
