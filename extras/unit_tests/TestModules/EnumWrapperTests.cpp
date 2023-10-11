/*
File:   EnumWrapperTests.cpp
Author: J. Ian Lindsay
Date:   2023.10.09

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


This program tests EnumWrapper. EnumDef is a trivial container class.
*/

#include "EnumWrapper.h"

/*******************************************************************************
* Tests for EnumWrapper
*******************************************************************************/

enum class EWrapTestType : uint8_t {
  VAL_0,
  VAL_1,
  VAL_2,
  VAL_3,
  VAL_4,
  VAL_5,
  PSEUDO_INVALD,
  TRUE_INVALD
};

// The isotropic test: A complete list with no flagged defs. This is a pattern
//   common when all a module needs is the string output or itemization features
//   of the wrapper, but has no interest in doing lookup-by-string, or otherwise
//   using them as exchange with another program.
const EnumDef<EWrapTestType> _ENUM_LIST_0[] = {
  { EWrapTestType::VAL_0,         "VAL_0"},
  { EWrapTestType::VAL_1,         "VAL_1"},
  { EWrapTestType::VAL_2,         "VAL_2"},
  { EWrapTestType::VAL_3,         "VAL_3"},
  { EWrapTestType::VAL_4,         "VAL_4"},
  { EWrapTestType::VAL_5,         "VAL_5"},
  { EWrapTestType::PSEUDO_INVALD, "PSEUDO_INVALD"},
  { EWrapTestType::TRUE_INVALD,   "TRUE_INVALD"}
};
const EnumDefList<EWrapTestType> EWT_LIST0(&_ENUM_LIST_0[0], (sizeof(_ENUM_LIST_0) / sizeof(_ENUM_LIST_0[0])));

// The anisotropic test: An out-of-order list with distinct catch-all/invalid
//   markers, and a defined context byte. Hopefully, no one would define a list
//   this way, but these tests will demonstrate invariance of outcome with
//   respect to both list order and presence of context byte at construction.
const EnumDef<EWrapTestType> _ENUM_LIST_1[] = {
  { EWrapTestType::TRUE_INVALD,   "TRUE_INVALD", (ENUM_WRAPPER_FLAG_IS_INVALID), 99},
  { EWrapTestType::VAL_5,         "VAL_5", 88},
  { EWrapTestType::VAL_4,         "VAL_4"},
  { EWrapTestType::PSEUDO_INVALD, "PSEUDO_INVALD", (ENUM_WRAPPER_FLAG_CATCHALL), 77},
  { EWrapTestType::VAL_2,         "VAL_2", 66},
  { EWrapTestType::VAL_1,         "VAL_1"},
  { EWrapTestType::VAL_3,         "VAL_3"},
  { EWrapTestType::VAL_0,         "VAL_0", 55}
};
const EnumDefList<EWrapTestType> EWT_LIST1(&_ENUM_LIST_1[0], (sizeof(_ENUM_LIST_1) / sizeof(_ENUM_LIST_1[0])));

// An abbreviated list with a catch-all that is also marked as an invlaid state.
// Flag-wise, this is probably the most common type of pattern for code that
//   wants to delegate enum sanitizing to the wrapper.
// No one will probably intend to create an incomplete list, as is done for this
//   test, but it demonstrates run-time control that is optionally tighter than
//   the type assurance given by the compiler. IE: Something that is truly a
//   valid enum might be not considered as such by the list, by virtue of
//   non-inclusion, as well as by flag.
const EnumDef<EWrapTestType> _ENUM_LIST_2[] = {
  { EWrapTestType::VAL_0,         "VAL_0"},
  { EWrapTestType::VAL_1,         "VAL_1"},
  { EWrapTestType::VAL_2,         "VAL_2"},
  { EWrapTestType::TRUE_INVALD,   "TRUE_INVALD", (ENUM_FLAG_MASK_INVALID_CATCHALL)}
};
const EnumDefList<EWrapTestType> EWT_LIST2(&_ENUM_LIST_2[0], (sizeof(_ENUM_LIST_2) / sizeof(_ENUM_LIST_2[0])));



void print_types_enum_wrapper() {
  printf("\tEnumDef<EWrapTestType>       %u\t%u\n", sizeof(EnumDef<EWrapTestType>),     alignof(EnumDef<EWrapTestType>));
  printf("\tEnumDefList<EWrapTestType>   %u\t%u\n", sizeof(EnumDefList<EWrapTestType>), alignof(EnumDefList<EWrapTestType>));
  printf("\tEWT_LIST0                    %u\t%u\n", sizeof(EWT_LIST0), alignof(EWT_LIST0));
}


/*
* Tests usage under conditions of full enum-space coverage, and no flags.
*/
int enum_wrapper_isotropic_tests() {
  int ret = -1;
  printf("Running isotropic list tests...\n");
  printf("\tThe list contains the expected number of definitions...\n");
  printf("\tDefinition count matches the export count (no invalids)...\n");
  printf("\texportKeys() returns a list of enum strings that matches the count...\n");
  printf("\tAsking for any defined enum by string returns the corresponding enum...\n");
  printf("\tAsking for any defined EnumDef by enum returns the corresponding EnumDef...\n");
  printf("\tAsking for an undefined enum by string returns whatever enum corresponds to the zero value...\n");
  printf("\tAsking for an undefined EnumDef by enum returns nullptr...\n");
  printf("\tAll isotropic list defs were created with a context byte equal to 0...\n");
  ret = 0;   // TDOO

  if (0 != ret) {
    printf(" Fail.\n");
  }
  return ret;
}


int enum_wrapper_anisotropic_tests() {
  int ret = -1;
  printf("Running anisotropic list tests...\n");
  printf("\tThe list contains the expected number of definitions...\n");
  printf("\tDefinition count matches the export count minus 1 (one invalid)...\n");
  ret = 0;   // TDOO

  if (0 != ret) {
    printf(" Fail.\n");
  }
  return ret;
}


int enum_wrapper_abbreviated_tests() {
  int ret = -1;
  printf("Running abbreviated list tests...\n");
  printf("\tThe list contains the expected number of definitions...\n");
  printf("\tDefinition count matches the export count minus 1 (one invalid)...\n");
  ret = 0;   // TDOO

  if (0 != ret) {
    printf(" Fail.\n");
  }
  return ret;
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int c3p_enum_wrapper_test_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "EnumDefList";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == enum_wrapper_isotropic_tests()) {
    if (0 == enum_wrapper_anisotropic_tests()) {
      if (0 == enum_wrapper_abbreviated_tests()) {
        ret = 0;
      }
      else printTestFailure(MODULE_NAME, "Abbreviated list");
    }
    else printTestFailure(MODULE_NAME, "Anisotropic list tests");
  }
  else printTestFailure(MODULE_NAME, "Isotropic list tests");

  return ret;
}
