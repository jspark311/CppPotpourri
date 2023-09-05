/*
File:   LineTermCoDecTests.cpp
Author: J. Ian Lindsay
Date:   2023.09.04


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


This program tests LineCoDec.
*/

#include "BufferAccepter/LineCoDec/LineCoDec.h"
#include "BufferAccepter/TestFixtures/CoDecTestFixtures.h"

/*******************************************************************************
* Tests for line-termination codec
*******************************************************************************/

/*
* A one-off struct to hold test cases for the base64 codec
*/
struct lineterm_test_case {
  const char* const test_description;
  const char* const input;
  const uint8_t*    ouput_full;
  const int         break_count;
};



void print_types_line_term_codec() {
  printf("\tLineEndingCoDec       %u\t%u\n", sizeof(Image),   alignof(Image));
}

/*******************************************************************************
* The main function.
*******************************************************************************/
int c3p_line_codec_test_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "LineEndingCoDec";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (true) {
    ret = 0;
  }
  else printTestFailure(MODULE_NAME, "LineEndingCoDec fails tests");

  return ret;
}
