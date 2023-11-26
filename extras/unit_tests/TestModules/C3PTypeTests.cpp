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

int c3p_type_wrapping_basics() {
  int ret = 0;  // TODO
  return ret;
}


int c3p_type_test_type_conversion() {
  int ret = 0;  // TODO
  return ret;
}


int c3p_type_test_packing() {
  int ret = 0;  // TODO
  return ret;
}


int c3p_type_test_parsing() {
  int ret = 0;  // TODO
  return ret;
}

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


/*******************************************************************************
* The main function.
*******************************************************************************/
int c3p_type_test_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "C3PType";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == c3p_type_wrapping_basics()) {
    if (0 == c3p_type_test_type_conversion()) {
      if (0 == c3p_type_test_packing()) {
        if (0 == c3p_type_test_parsing()) {
          ret = 0;
        }
        else printTestFailure(MODULE_NAME, "Type parsing");
      }
      else printTestFailure(MODULE_NAME, "Type packing");
    }
    else printTestFailure(MODULE_NAME, "Type conversion");
  }
  else printTestFailure(MODULE_NAME, "Basics");

  return ret;
}
