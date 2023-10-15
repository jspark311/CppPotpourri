/*
File:   C3PValueTests.cpp
Author: J. Ian Lindsay
Date:   2023.06.17

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


This program tests C3PValue, which is its internal type-wrapper.
*/

#include "C3PValue/C3PValue.h"


/*******************************************************************************
* C3PValue test routines
*******************************************************************************/

int c3p_value_test_basics() {
  int ret = -1;
  return ret;
}


int c3p_value_test_type_wrapping() {
  int ret = -1;
  return ret;
}


int c3p_value_test_type_conversion() {
  int ret = -1;
  return ret;
}


int c3p_value_test_packing() {
  int ret = -1;
  return ret;
}


int c3p_value_test_parsing() {
  int ret = -1;
  return ret;
}


void print_types_c3p_value() {
  printf("\tC3PValue              %u\t%u\n", sizeof(C3PValue),  alignof(C3PValue));
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int c3p_value_test_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "C3PValue";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == c3p_value_test_basics()) {
    if (0 == c3p_value_test_type_wrapping()) {
      if (0 == c3p_value_test_type_conversion()) {
        if (0 == c3p_value_test_packing()) {
          if (0 == c3p_value_test_parsing()) {
            ret = 0;
          }
          else printTestFailure(MODULE_NAME, "Type parsing");
        }
        else printTestFailure(MODULE_NAME, "Type packing");
      }
      else printTestFailure(MODULE_NAME, "Type conversion");
    }
    else printTestFailure(MODULE_NAME, "Type wrapping");
  }
  else printTestFailure(MODULE_NAME, "Basics");

  ret = 0;  // TODO
  return ret;
}
