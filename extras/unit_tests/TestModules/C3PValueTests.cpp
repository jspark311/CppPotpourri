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


This program tests C3PValue, which is C3P's internal type-wrapper for singular
  values.
*/

#include "C3PValue/C3PValue.h"


/*******************************************************************************
* C3PValue test routines
*******************************************************************************/

/*
*
*/
int c3p_value_test_numerics() {
  int ret = -1;
  printf("Testing C3PValue wrapping of numeric types...\n");

  ret = 0;  // TODO: KATs for mis-use and absurdities.

  const uint32_t FUZZ_CYCLES = 3;
  uint32_t i = 0;
  while ((i++ < FUZZ_CYCLES) & (0 == ret)) {
    const bool     TEST_VAL_BOOL   = flip_coin();
    const float    TEST_VAL_FLOAT  = generate_random_float();
    const double   TEST_VAL_DOUBLE = generate_random_double();
    const uint64_t TEST_VAL_UINT64 = generate_random_uint64();
    const int64_t  TEST_VAL_INT64  = generate_random_int64();
    const uint32_t TEST_VAL_UINT32 = randomUInt32();
    const int32_t  TEST_VAL_INT32  = (int32_t)  randomUInt32();
    const uint16_t TEST_VAL_UINT16 = (uint16_t) randomUInt32();
    const int16_t  TEST_VAL_INT16  = (int16_t)  randomUInt32();
    const uint8_t  TEST_VAL_UINT8  = (uint8_t)  randomUInt32();
    const int8_t   TEST_VAL_INT8   = (int8_t)   randomUInt32();

    C3PValue test_val_bool(TEST_VAL_BOOL);
    C3PValue test_val_float(TEST_VAL_FLOAT);
    C3PValue test_val_double(TEST_VAL_DOUBLE);
    C3PValue test_val_uint64(TEST_VAL_UINT64);
    C3PValue test_val_int64(TEST_VAL_INT64);
    C3PValue test_val_uint32(TEST_VAL_UINT32);
    C3PValue test_val_int32(TEST_VAL_INT32);
    C3PValue test_val_uint16(TEST_VAL_UINT16);
    C3PValue test_val_int16(TEST_VAL_INT16);
    C3PValue test_val_uint8(TEST_VAL_UINT8);
    C3PValue test_val_int8(TEST_VAL_INT8);

    C3PValue conv_val_bool(test_val_bool.tcode());
    C3PValue conv_val_float(test_val_float.tcode());
    C3PValue conv_val_double(test_val_double.tcode());
    C3PValue conv_val_uint64(test_val_uint64.tcode());
    C3PValue conv_val_int64(test_val_int64.tcode());
    C3PValue conv_val_uint32(test_val_uint32.tcode());
    C3PValue conv_val_int32(test_val_int32.tcode());
    C3PValue conv_val_uint16(test_val_uint16.tcode());
    C3PValue conv_val_int16(test_val_int16.tcode());
    C3PValue conv_val_uint8(test_val_uint8.tcode());
    C3PValue conv_val_int8(test_val_int8.tcode());

    C3PValue* input_values[] = {
      &test_val_bool,
      &test_val_float,
      &test_val_double,
      &test_val_uint64,
      &test_val_int64,
      &test_val_uint32,
      &test_val_int32,
      &test_val_uint16,
      &test_val_int16,
      &test_val_uint8,
      &test_val_int8
    };
    C3PValue* output_values[] = {
      &conv_val_bool,
      &conv_val_float,
      &conv_val_double,
      &conv_val_uint64,
      &conv_val_int64,
      &conv_val_uint32,
      &conv_val_int32,
      &conv_val_uint16,
      &conv_val_int16,
      &conv_val_uint8,
      &conv_val_int8
    };

    const uint32_t INPUT_VAL_COUNT  = (sizeof(input_values) / sizeof(input_values[0]));
    const uint32_t CONVERSION_COUNT = (sizeof(output_values) / sizeof(output_values[0]));

    uint32_t x = 0;
    while ((0 == ret) & (x < INPUT_VAL_COUNT)) {
      C3PValue* input_value = input_values[x++];
      StringBuilder tmp_str("\tConverting ");
      input_value->toString(&tmp_str, true);

      printf("%s...\n", (char*) tmp_str.string());
      uint32_t y = 0;
      while ((0 == ret) & (y < CONVERSION_COUNT)) {
        ret = -1;
        C3PValue* output_value = output_values[y++];
        if (output_value->is_numeric()) {
          printf("\t\t...into %s ", typecodeToStr(output_value->tcode()));
          output_value->set(0);    // Zero the existing value of the output type.
          // Apply the numeric conversion matrix to input and output types with
          //   the given input value.
          const int8_t TCONV_RISK = C3PType::conversionRisk(input_value->tcode(), output_value->tcode());

          // If this bool is set, the conversion is always going to be possible
          //   based on type alone. Any input value will map sensibly.
          const bool CONV_IS_RELIABLE      = (0 == TCONV_RISK);

          // If this bool is set, the conversion might be possible if there is
          //   no width conflict (IE, can store a UINT64 in an INT8 if the value
          //   of the UINT64 is less-than 128).
          const bool CONV_CONTINGENT_WIDTH = ((TCONV_RISK > 0) & (1 & TCONV_RISK));

          // If this bool is set, the conversion might be possible if there is
          //   no signage conflict (IE, can't store -14 in a UINT of any size).
          const bool CONV_CONTINGENT_SIGN  = ((TCONV_RISK > 0) & (2 & TCONV_RISK));

          printf("(Reliable: %c)", (CONV_IS_RELIABLE ? 'y':'n'));
          if (!CONV_IS_RELIABLE) {
            printf("  (Sign contingent: %c)", (CONV_CONTINGENT_SIGN ? 'y':'n'));
            printf("  (Width contingent: %c)", (CONV_CONTINGENT_WIDTH ? 'y':'n'));
          }
          printf("... ");

          tmp_str.clear();
          int8_t conv_result = output_value->set(input_value);  // Set conv from input.

          if ((0 == conv_result) & (TCONV_RISK >= 0)) {
            // A conversion that succeeds when expected is a pass.
            ret = 0;
          }
          else if ((-1 == conv_result) & (!CONV_IS_RELIABLE)) {
            // A conversion that fails when expected is a pass.
            ret = 0;
          }

          if (0 == ret) {
            output_value->toString(&tmp_str);
            printf("Pass with result %d (%s).\n", conv_result, (char*) tmp_str.string());
          }
          else {
            printf("Fail with result %d (%s).\n", conv_result, (char*) tmp_str.string());
          }
        }
        else {
          printf("Non-numeric type was used in numeric conversion tests.\n");
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
  }
  return ret;
}



int c3p_value_test_type_conversion() {
  int ret = 0;
  return ret;
}


int c3p_value_test_packing() {
  int ret = 0;
  return ret;
}


int c3p_value_test_parsing() {
  int ret = 0;
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

  if (0 == c3p_value_test_numerics()) {
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
  else printTestFailure(MODULE_NAME, "Numerics");

  return ret;
}
