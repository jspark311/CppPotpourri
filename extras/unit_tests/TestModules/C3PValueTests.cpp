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
* Test the numeric aspects.
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


/*
* Test the vector aspects.
*/
int c3p_value_test_vectors() {
  int ret = -1;
  printf("Testing C3PValue wrapping of vector types...\n");
  Vector3<float> test_3float(
    generate_random_float(),
    generate_random_float(),
    generate_random_float()
  );
  C3PValue value_3float(&test_3float);

  printf("\tConstruction semantics for (Vector3f*)...\n\t\tHas proper length (%u)... ", 12);
  if (value_3float.length() == 12) {
    printf("Pass\n\t\tCan fetch with no conversion... ");
    Vector3<float> ret_3float;
    if (0 == value_3float.get_as(&ret_3float)) {
      printf("Pass\n\t\tIs properly marked as no-reap... ");
      if (!value_3float.reapValue()) {
        printf("Pass\n\t\tThe contents of the wrapped vector match those of the original... ");
        if (test_3float == ret_3float) {
          printf("Pass\n\t\tAll (Vector3f*) tests pass.\n");
          ret = 0;
        }
      }
    }
  }

  if (0 == ret) {
  }

  if (0 != ret) {
    printf("Fail.\n");
  }
  return ret;
}



/*
* Test the string aspects.
*/
int c3p_value_test_strings() {
  int ret = -1;
  printf("Testing C3PValue wrapping of string types...\n");
  const uint32_t TEST_BUF_LEN    = (53 + (randomUInt32() % 90));
  StringBuilder test_string;
  // Generate a test string of (TEST_BUF_LEN-1), because the wrapper will
  //   report the binary length of the contained data. Which includes the
  //   null-terminator for a C-style string.
  generate_random_text_buffer(&test_string, (TEST_BUF_LEN-1));
  StringBuilder test_const((char*) test_string.string());
  StringBuilder test_mutable((char*) test_string.string());
  StringBuilder test_sb((char*) test_string.string());

  C3PValue value_const((const char*) test_const.string());
  C3PValue value_mutable((char*) test_mutable.string());
  C3PValue value_sb(&test_sb);

  printf("\tConstruction semantics for (const char*)...\n\t\tHas proper length (%u / %u)... ", TEST_BUF_LEN, value_const.length());
  if (value_const.length() == TEST_BUF_LEN) {
    printf("Pass\n\t\tCan fetch with no conversion... ");
    const char* ret_str = nullptr;
    if (0 == value_const.get_as(&ret_str)) {
      printf("Pass\n\t\tHas proper pointer (%p)... ", test_const.string());
      if ((void*) ret_str == (void*) test_const.string()) {
        printf("Pass\n\t\tIs properly marked as no-reap... ");
        if (!value_const.reapValue()) {
          printf("Pass\n\t\tThe contents of the wrapped string match those of the original... ");
          if (test_string.contains(ret_str)) {
            printf("Pass\n\t\tAll (const char*) tests pass.\n");
            ret = 0;
          }
        }
      }
    }
  }

  if (0 == ret) {
    ret = -1;
    printf("\tConstruction semantics for (char*)...\n\t\tHas proper length (%u / %u)... ", TEST_BUF_LEN, value_mutable.length());
    if (value_mutable.length() == TEST_BUF_LEN) {
      printf("Pass\n\t\tCan fetch with no conversion... ");
      const char* ret_str = nullptr;
      if (0 == value_mutable.get_as(&ret_str)) {
        printf("Pass\n\t\tHas a pointer (%p) that is distinct from (%p)... ", ret_str, test_mutable.string());
        if ((void*) ret_str != (void*) test_mutable.string()) {
          printf("Pass\n\t\tIs properly marked for reap... ");
          if (value_mutable.reapValue()) {
            printf("Pass\n\t\tThe contents of the wrapped string match those of the original... ");
            if (test_string.contains(ret_str)) {
              printf("Pass\n\t\tAll (char*) tests pass.\n");
              ret = 0;
            }
          }
        }
      }
    }
  }

  if (0 == ret) {
    ret = -1;
    // To preserve integrity with its binary API, StringBuilder will not report
    //   the null-terminator for a C-style string. StringBuilder adds a
    //   zero-terminal of its own accord for text safety, and will not include
    //   it in the length unless the length-specified included it.
    printf("\tConstruction semantics for (StringBuilder*)...\n\t\tHas properly-adjusted length (%u / %u)... ", TEST_BUF_LEN, value_sb.length());
    if (value_sb.length() == (TEST_BUF_LEN-1)) {
      printf("Pass\n\t\tCan fetch with no conversion... ");
      StringBuilder* ret_sb = nullptr;
      if (0 == value_sb.get_as(&ret_sb)) {
        printf("Pass\n\t\tHas a pointer (%p) that is identical to (%p)... ", ret_sb, &test_sb);
        if (ret_sb == &test_sb) {
          StringBuilder  ret_deepcopy;
          StringBuilder* ptr_deepcopy = &ret_deepcopy;
          printf("Pass\n\t\tValue can be retrieved by deep-copy... ");
          if (0 == value_sb.get_as(&ptr_deepcopy)) {
            printf("Pass\n\t\tContent pointer (%p) is distinct from that of the source (%p)... ", ret_deepcopy.string(), test_sb.string());
            if (ret_deepcopy.string() != test_sb.string()) {
              printf("Pass\n\t\tContent length matches that in the container (%u / %u)... ", ret_deepcopy.length(), value_sb.length());
              if (ret_deepcopy.length() == value_sb.length()) {
                printf("Pass\n\t\tContent matches that in the source... ");
                if (test_string.contains((char*) ret_deepcopy.string())) {
                  printf("Pass\n\t\tIs properly marked for reap... ");
                  if (!value_sb.reapValue()) {
                    printf("Pass\n\t\tAll (StringBuilder*) tests pass.\n");
                    ret = 0;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
  }
  return ret;
}



/*
* Test the BLOB aspects.
*/
int c3p_value_test_blobs() {
  int ret = -1;
  printf("Testing C3PValue wrapping of pointer-length compound types...\n");
  const uint32_t TEST_BUF_LEN    = (51 + (randomUInt32() % 140));
  StringBuilder test_string;
  generate_random_text_buffer(&test_string, TEST_BUF_LEN);
  StringBuilder test_blob((char*) test_string.string());
  C3PValue value_blob(test_blob.string(), test_blob.length());

  printf("\tConstruction semantics for (uint8*, length)...\n\t\tHas proper length (%u)... ", TEST_BUF_LEN);
  if (value_blob.length() == TEST_BUF_LEN) {
    printf("Pass\n\t\tCan fetch with no conversion... ");
    uint8_t* ret_ptr = nullptr;
    uint32_t ret_len = 0;
    if (0 == value_blob.get_as(&ret_ptr, &ret_len)) {
      printf("Pass\n\t\tHas proper pointer (%p)... ", test_blob.string());
      if ((void*) ret_ptr == (void*) test_blob.string()) {
        printf("Pass\n\t\tIs properly marked as no-reap... ");
        if (!value_blob.reapValue()) {
          printf("Pass\n\t\tAll (pointer-length) tests pass.\n");
          StringBuilder output;
          value_blob.toString(&output);
          printf("%s\n", (char*) output.string());
          ret = 0;
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
  }
  return ret;
}


/*
* The type abstractions in C3P allow for direct pointer transactions with the
*   memory involved in the storage of types that it wraps. It should thus be
*   accounting for the possibility of platform alignment requirements that might
*   not be conducive to direct de-reference.
* Test the alignment-touchy types for proper handling.
*/
int c3p_value_test_alignment() {
  int ret = -1;
  printf("Testing alignment nightmare cases...\n");
  printf("\tPreparing test cases... ");
  // First... parameterize the test...
  const uint32_t MISALIGN_BUFFER_LEN = 16;
  const uint32_t MISALIGN_OFFSET     = (1 | (randomUInt32() % 7));
  // Second... we'll need to generate the test values...
  // TODO: Do a few vectors, as well...
  const float    TEST_VAL_FLOAT  = generate_random_float();
  const double   TEST_VAL_DOUBLE = generate_random_double();
  const uint64_t TEST_VAL_UINT64 = generate_random_uint64();
  const int64_t  TEST_VAL_INT64  = generate_random_int64();
  // Make slots for the fetches...
  float          ret_val_float   = 0.0f;
  double         ret_val_double  = 0.0d;
  uint64_t       ret_val_uint64  = 0;
  int64_t        ret_val_int64   = 0;
  // ...and the containers themselves (which will be zeroed)...
  C3PValue value_float(TCode::FLOAT);
  C3PValue value_double(TCode::DOUBLE);
  C3PValue value_uint64(TCode::UINT64);
  C3PValue value_int64(TCode::INT64);

  // Next, we'll need to make some buffers, since the compiler (sensibly) would
  //   object if it knew what we were trying to do.
  uint8_t buffer_float[MISALIGN_BUFFER_LEN];
  uint8_t buffer_double[MISALIGN_BUFFER_LEN];
  uint8_t buffer_uint64[MISALIGN_BUFFER_LEN];
  uint8_t buffer_int64[MISALIGN_BUFFER_LEN];
  uint8_t ret_buffer_float[MISALIGN_BUFFER_LEN];
  uint8_t ret_buffer_double[MISALIGN_BUFFER_LEN];
  uint8_t ret_buffer_uint64[MISALIGN_BUFFER_LEN];
  uint8_t ret_buffer_int64[MISALIGN_BUFFER_LEN];
  memset(buffer_float,  0, MISALIGN_BUFFER_LEN);
  memset(buffer_double, 0, MISALIGN_BUFFER_LEN);
  memset(buffer_uint64, 0, MISALIGN_BUFFER_LEN);
  memset(buffer_int64,  0, MISALIGN_BUFFER_LEN);
  memset(ret_buffer_float,  0, MISALIGN_BUFFER_LEN);
  memset(ret_buffer_double, 0, MISALIGN_BUFFER_LEN);
  memset(ret_buffer_uint64, 0, MISALIGN_BUFFER_LEN);
  memset(ret_buffer_int64,  0, MISALIGN_BUFFER_LEN);
  memcpy((buffer_float  + MISALIGN_OFFSET), (void*) &TEST_VAL_FLOAT,  sizeof(TEST_VAL_FLOAT));
  memcpy((buffer_double + MISALIGN_OFFSET), (void*) &TEST_VAL_DOUBLE, sizeof(TEST_VAL_DOUBLE));
  memcpy((buffer_uint64 + MISALIGN_OFFSET), (void*) &TEST_VAL_UINT64, sizeof(TEST_VAL_UINT64));
  memcpy((buffer_int64  + MISALIGN_OFFSET), (void*) &TEST_VAL_INT64,  sizeof(TEST_VAL_INT64));
  printf("Misalignment of all types by (%u) bytes into %u-byte buffers:\n", MISALIGN_OFFSET, MISALIGN_BUFFER_LEN);
  StringBuilder tmp;
  StringBuilder table_out;

  table_out.concat("\tbuffer_float:  ");
  tmp.concat(buffer_float, MISALIGN_BUFFER_LEN);
  tmp.printDebug(&table_out);
  table_out.concatf("  %.3f\n", TEST_VAL_FLOAT);
  tmp.clear();

  table_out.concat("\tbuffer_double: ");
  tmp.concat(buffer_double, MISALIGN_BUFFER_LEN);
  tmp.printDebug(&table_out);
  table_out.concatf("  %.6f\n", TEST_VAL_DOUBLE);
  tmp.clear();

  table_out.concat("\tbuffer_uint64: ");
  tmp.concat(buffer_uint64, MISALIGN_BUFFER_LEN);
  tmp.printDebug(&table_out);
  table_out.concatf("  %lu\n", TEST_VAL_UINT64);
  tmp.clear();

  table_out.concat("\tbuffer_int64:  ");
  tmp.concat(buffer_int64, MISALIGN_BUFFER_LEN);
  tmp.printDebug(&table_out);
  table_out.concatf("  %ld\n", TEST_VAL_INT64);
  tmp.clear();
  table_out.concat('\n');
  printf("%s\n", (char*) table_out.string());

  printf("\tTCode::FLOAT\n\t\tSetting from misaligned memory location... ");
  if (0 == value_float.set_from(TCode::FLOAT, (void*)(buffer_float + MISALIGN_OFFSET))) {
    printf("Pass\n\t\tValue matches input... ");
    if (TEST_VAL_FLOAT == value_float.get_as_float()) {
      printf("Pass\n\t\tAlternate API method yields the same result... ");
      if (0 == value_float.get_as(&ret_val_float)) {
        if (TEST_VAL_FLOAT == ret_val_float) {
          printf("Pass\n\t\tFetching into misaligned memory location... ");
          if (0 == value_float.get_as(TCode::FLOAT, (void*)(ret_buffer_float + MISALIGN_OFFSET))) {
            printf("Pass\n\t\tFetched values match original input... ");
            bool values_match = true;
            for (uint32_t i = 0; i < MISALIGN_BUFFER_LEN; i++) {
              values_match |= (*(buffer_float + i) == *(ret_buffer_float + i));
            }
            if (values_match) {
              printf("Pass\n\t\tTCode::FLOAT alignment nightmare tests pass.\n");
              ret = 0;
            }
          }
        }
      }
    }
  }

  if (0 == ret) {
    ret = -1;
    printf("\tTCode::DOUBLE\n\t\tSetting from misaligned memory location... ");
    if (0 == value_double.set_from(TCode::DOUBLE, (void*)(buffer_double + MISALIGN_OFFSET))) {
      printf("Pass\n\t\tValue matches input... ");
      if (TEST_VAL_DOUBLE == value_double.get_as_double()) {
        printf("Pass\n\t\tAlternate API method yields the same result... ");
        if (0 == value_double.get_as(&ret_val_double)) {
          if (TEST_VAL_DOUBLE == ret_val_double) {
            printf("Pass\n\t\tFetching into misaligned memory location... ");
            if (0 == value_double.get_as(TCode::DOUBLE, (void*)(ret_buffer_double + MISALIGN_OFFSET))) {
              printf("Pass\n\t\tFetched values match original input... ");
              bool values_match = true;
              for (uint32_t i = 0; i < MISALIGN_BUFFER_LEN; i++) {
                values_match |= (*(buffer_double + i) == *(ret_buffer_double + i));
              }
              if (values_match) {
                printf("Pass\n\t\tTCode::DOUBLE alignment nightmare tests pass.\n");
                ret = 0;
              }
            }
          }
        }
      }
    }
  }

  if (0 == ret) {
    ret = -1;
    printf("\tTCode::UINT64\n\t\tSetting from misaligned memory location... ");
    if (0 == value_uint64.set_from(TCode::UINT64, (void*)(buffer_uint64 + MISALIGN_OFFSET))) {
      printf("Pass\n\t\tValue matches input... ");
      if (TEST_VAL_UINT64 == value_uint64.get_as_uint64()) {
        printf("Pass\n\t\tAlternate API method yields the same result... ");
        if (0 == value_uint64.get_as(&ret_val_uint64)) {
          if (TEST_VAL_UINT64 == ret_val_uint64) {
            printf("Pass\n\t\tFetching into misaligned memory location... ");
            if (0 == value_uint64.get_as(TCode::UINT64, (void*)(ret_buffer_uint64 + MISALIGN_OFFSET))) {
              printf("Pass\n\t\tFetched values match original input... ");
              bool values_match = true;
              for (uint32_t i = 0; i < MISALIGN_BUFFER_LEN; i++) {
                values_match |= (*(buffer_uint64 + i) == *(ret_buffer_uint64 + i));
              }
              if (values_match) {
                printf("Pass\n\t\tTCode::UINT64 alignment nightmare tests pass.\n");
                ret = 0;
              }
            }
          }
        }
      }
    }
  }

  if (0 == ret) {
    ret = -1;
    printf("\tTCode::INT64\n\t\tSetting from misaligned memory location... ");
    if (0 == value_int64.set_from(TCode::INT64, (void*)(buffer_int64 + MISALIGN_OFFSET))) {
      printf("Pass\n\t\tValue matches input... ");
      if (TEST_VAL_INT64 == value_int64.get_as_int64()) {
        printf("Pass\n\t\tAlternate API method yields the same result... ");
        if (0 == value_int64.get_as(&ret_val_int64)) {
          if (TEST_VAL_INT64 == ret_val_int64) {
            printf("Pass\n\t\tFetching into misaligned memory location... ");
            if (0 == value_int64.get_as(TCode::INT64, (void*)(ret_buffer_int64 + MISALIGN_OFFSET))) {
              printf("Pass\n\t\tFetched values match original input... ");
              bool values_match = true;
              for (uint32_t i = 0; i < MISALIGN_BUFFER_LEN; i++) {
                values_match |= (*(buffer_int64 + i) == *(ret_buffer_int64 + i));
              }
              if (values_match) {
                printf("Pass\n\t\tTCode::INT64 alignment nightmare tests pass.\n");
                ret = 0;
              }
            }
          }
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
    if (0 == c3p_value_test_vectors()) {
      if (0 == c3p_value_test_strings()) {
        if (0 == c3p_value_test_blobs()) {
          if (0 == c3p_value_test_alignment()) {
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
          else printTestFailure(MODULE_NAME, "Alignment nightmare case");
        }
        else printTestFailure(MODULE_NAME, "BLOBs");
      }
      else printTestFailure(MODULE_NAME, "Strings");
    }
    else printTestFailure(MODULE_NAME, "Vectors");
  }
  else printTestFailure(MODULE_NAME, "Numerics");

  return ret;
}
