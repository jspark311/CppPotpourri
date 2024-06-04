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
  Vector3<float> test_set(
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
          printf("Pass\n\t\tset() works for the native type... ");
          Vector3<float> test_set(
            generate_random_float(),
            generate_random_float(),
            generate_random_float()
          );
          if (0 == value_3float.set(&test_set)) {
            printf("Pass\n\t\tAll (Vector3f*) tests pass.\n");
            ret = 0;
          }
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
          //StringBuilder output;
          //value_blob.toString(&output);
          //printf("%s\n", (char*) output.string());
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
* Test the timer aspects. That means StopWatch and C3PTrace.
*/
int c3p_value_test_timer_types() {
  int ret = -1;
  // Fill out a StopWatch (with a random tag) to test with. Then wrap it into a
  //   C3PValue object in preparation for export.
  const uint32_t STOPWATCH_CYCLES    = (15 + (randomUInt32() % 14));
  const uint32_t STOPWATCH_FUZZ      = (1103 + (randomUInt32() % 140));
  const uint32_t STOPWATCH_LENGTH    = sizeof(StopWatch);
  printf("Testing C3PValue wrapping of timer types (Cycles/Fuzz: %u / %u)...\n", STOPWATCH_CYCLES, STOPWATCH_FUZZ);
  StopWatch test_sw(randomUInt32());
  for (uint32_t i = 0; i < STOPWATCH_CYCLES; i++) {
    test_sw.markStart();
    sleep_us(STOPWATCH_FUZZ);
    test_sw.markStop();
  }
  C3PValue value_sw(&test_sw);
  StringBuilder packed;
  C3PValue* deser_val = nullptr;

  printf("\tConstruction semantics for (StopWatch*)...\n\t\tHas proper length (%u)... ", STOPWATCH_LENGTH);
  if (value_sw.length() == STOPWATCH_LENGTH) {
    printf("Pass\n\t\tCan fetch with no conversion... ");
    StopWatch* ret_sw = nullptr;
    if (0 == value_sw.get_as(&ret_sw)) {
      printf("Pass\n\t\tValue object pointer (%p) indicates value-by-reference operation... ", &test_sw);
      if ((void*) ret_sw == (void*) &test_sw) {
        printf("Pass\n\t\tIs properly marked as no-reap... ");
        if (!value_sw.reapValue()) {
          printf("Pass.\n\t\tStopWatch can be serialized... ");
          if (0 == value_sw.serialize(&packed, TCode::CBOR)) {
            printf("Pass.\n\t\tStopWatch can be deserialized... ");
            //dump_strbldr(&packed);
            deser_val = C3PValue::deserialize(&packed, TCode::CBOR);
            if (nullptr != deser_val) {
              printf("Pass.\n\t\tDeserialized value is a StopWatch... ");
              ret_sw = nullptr;
              if ((0 == deser_val->get_as(&ret_sw)) && (nullptr != ret_sw)) {
                printf("Pass.\n\t\tDeserialized value contains a distict pointer (%p)... ", (void*) ret_sw);
                if ((void*) ret_sw != (void*) &test_sw) {
                  printf("Pass.\n\t\tThe source buffer was entirely consumed... ");
                  if (packed.isEmpty()) {
                    printf("Pass.\n\t\tDeserialized value is marked for reap (both container and value)... ");
                    if (deser_val->reapValue() & deser_val->reapContainer()) {
                      printf("Pass.\n\t\tDeserialized StopWatch matches input... ");
                      bool pest_tasses = (ret_sw->tag() == test_sw.tag());
                      pest_tasses &= (ret_sw->bestTime() == test_sw.bestTime());
                      pest_tasses &= (ret_sw->lastTime() == test_sw.lastTime());
                      pest_tasses &= (ret_sw->worstTime() == test_sw.worstTime());
                      pest_tasses &= (ret_sw->meanTime() == test_sw.meanTime());
                      pest_tasses &= (ret_sw->totalTime() == test_sw.totalTime());
                      pest_tasses &= (ret_sw->executions() == test_sw.executions());
                      if (pest_tasses) {
                        printf("Pass\n\t\tAll (StopWatch) tests pass.\n");
                        StringBuilder output;
                        value_sw.toString(&output);
                        printf("%s\n", (char*) output.string());
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
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
    printf("=====> source_val:\t");  dump_c3pvalue(&value_sw);
    if (nullptr != deser_val) {  printf("=====> deser_val:\t");  dump_c3pvalue(deser_val);  }
    if (!packed.isEmpty()) {     printf("=====> packed:\t");     (char*) packed.string();   }
  }

  // We are always obliged to clean up anything we've deserialized.
  if (nullptr != deser_val) {  delete deser_val;  }
  return ret;
}




/*
* The type abstractions in C3P allow for direct pointer transactions with the
*   memory involved in the storage of types that it wraps. It should thus be
*   accounting for the possibility of platform alignment requirements that might
*   not be conducive to direct de-reference.
* IE, many 32-bit platforms require float to be aligned on 8-byte boundaries,
*   and some fraction of those platforms will allow void* on 4-byte boundaries.
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
  else {
    printf("\tAll alignment nightmare cases pass.\n");
  }
  return ret;
}



int c3p_value_test_type_conversion() {
  int ret = 0;
  return ret;
}


/*
* C3PValue is also a linked list to facilitate arrays of like types.
* Make sure that
*/
int c3p_value_test_linking() {
  const uint32_t TEST_LINK_LEN    = (8 + (randomUInt32() % 5));
  int ret = 0;
  printf("Testing linking mechanics...\n");
  printf("\tPreparing test cases... ");

  // Create test_val0:  [uint32, uint32, uint32, uint32, uint32, uint32, uint32, uint32]
  uint32_t ref_values[TEST_LINK_LEN] = {0};
  ref_values[0] = randomUInt32();
  C3PValue test_val0(ref_values[0]);  // This will effectively be test_val0[0].

  ret -= (!test_val0.isCompound() ? 0 : 1);  // Not an array yet...
  for (uint32_t i = 1; i < TEST_LINK_LEN; i++) {
    ref_values[i] = randomUInt32();
    // The return value of link() is uninteresting here, as it will just be whatever
    //   pointer we passed into it. It was intended to grease this sort of usage.
    if (nullptr == test_val0.link(new C3PValue(ref_values[i]))) {
      ret--; // It would be amazing if it failed to allocate...
    }
  }

  if (0 == ret) {
    printf("Pass\n\t\tTest object should have %u siblings... ", TEST_LINK_LEN);
    ret -= strict_abs_delta(TEST_LINK_LEN, test_val0.count());
    if (0 == ret) {
      // Some misuse tests. Simulate an OBO.
      printf("Pass\n\t\tOOB index returns nullptr... ");
      ret -= ((nullptr == test_val0.valueWithIdx(TEST_LINK_LEN)) ? 0 : 1);
      if (0 == ret) {
        printf("Pass\n\t\tThe values match the references... ");
        for (uint32_t i = 0; i < TEST_LINK_LEN; i++) {
          C3PValue* tmp_val = test_val0.valueWithIdx(i);
          if (nullptr == tmp_val) {
            ret--;  // Indexing failed failed.
          }
          else if (tmp_val->get_as_uint() != ref_values[i]) {   // TODO: Test is not 64-bit portable.
            ret--;  // Values at given index didn't match.
          }
        }
        if (0 == ret) {
          printf("Pass\n");
          // TODO: drop()
        }
      }
    }
    else printf("It reports %u. ", test_val0.count());
  }

  if (0 != ret) {
    printf("Fail.\n");
    for (uint32_t i = 0; i < TEST_LINK_LEN; i++) {
      C3PValue* tmp_val = test_val0.valueWithIdx(i);
      if (nullptr == tmp_val) {
        printf("\t%10u  (nullptr)\n", ref_values[i]);
      }
      else {
        printf("\t%10u  %10u\n", ref_values[i], tmp_val->get_as_uint());
      }
    }
  }

  return ret;
}

/*
* C3P supports heterogenous arrays internally, but it probably shouldn't officially to avoid
*   confusion and complexity. Thus, the array's underlying type may eventually be dictated
*   by the tcode() used to create the first Value. Violation of this convention is discretionary.
*/
int c3p_value_test_nested_arrays() {
  const uint32_t TEST_LINK_LEN0    = (8 + (randomUInt32() % 5));
  const uint32_t TEST_LINK_LEN1    = (8 + (randomUInt32() % 5));
  const uint32_t TEST_LINK_LEN_TOP = 2;
  int ret = 0;
  printf("Testing nested arrays...\n");
  printf("\tPreparing test cases... ");
  // Create test_val0:  [uint32, uint32, uint32, uint32, uint32, uint32, uint32, uint32]
  // Create test_val1:  [int16, int16, int16, int16, int16, int16, int16, int16]
  // Create top_val:    [test_val0, test_val1]
  uint32_t ref_values0[TEST_LINK_LEN0] = {0};
  int16_t  ref_values1[TEST_LINK_LEN1] = {0};
  ref_values0[0] = randomUInt32();
  ref_values1[0] = (int16_t) randomUInt32();
  C3PValue test_val0(ref_values0[0]);  // This will effectively be test_val0[0].
  C3PValue test_val1(ref_values1[0]);  // This will effectively be test_val1[0].
  for (uint32_t i = 1; i < strict_max(TEST_LINK_LEN0, TEST_LINK_LEN1); i++) {
    if (TEST_LINK_LEN0 > i) {
      ref_values0[i] = randomUInt32();
      if (nullptr == test_val0.link(new C3PValue(ref_values0[i]))) {
        ret--; // It would be amazing if it failed to allocate...
      }
    }
    if (TEST_LINK_LEN1 > i) {
      ref_values1[i] = (int16_t) randomUInt32();
      if (nullptr == test_val1.link(new C3PValue(ref_values1[i]))) {
        ret--; // It would be amazing if it failed to allocate...
      }
    }
  }

  C3PValue top_val(&test_val0);  // This will effectively be test_val1[0].
  top_val.link(new C3PValue(&test_val1), false);   // Stack objects shouldn't be explicitly destroyed.
  if (0 == ret) {
    printf("Pass\n\t\tSubarrays should have %u and %u siblings... ", TEST_LINK_LEN0, TEST_LINK_LEN1);
    ret -= strict_abs_delta(TEST_LINK_LEN0, test_val0.count());
    ret -= strict_abs_delta(TEST_LINK_LEN1, test_val1.count());
    if (0 == ret) {
      printf("Pass\n\t\tTop-level array should have %u siblings... ", TEST_LINK_LEN_TOP);
      ret -= strict_abs_delta(TEST_LINK_LEN_TOP, top_val.count());
      if (0 == ret) {
        printf("Pass\n\t\tThe values match the references... ");
        for (uint32_t i = 0; i < strict_max(TEST_LINK_LEN0, TEST_LINK_LEN1); i++) {
          if (TEST_LINK_LEN0 > i) {
            C3PValue* tmp_val0 = test_val0.valueWithIdx(i);
            if (nullptr == tmp_val0) {
              ret--;  // Indexing failed failed.
            }
            else if (tmp_val0->get_as_uint() != ref_values0[i]) {   // TODO: Test is not 64-bit portable.
              ret--;  // Values at given index didn't match.
            }
          }
          if (TEST_LINK_LEN1 > i) {
            C3PValue* tmp_val1 = test_val1.valueWithIdx(i);
            if (nullptr == tmp_val1) {
              ret--;  // Indexing failed failed.
            }
            else if (tmp_val1->get_as_int() != ref_values1[i]) {
              ret--;  // Values at given index didn't match.
            }
          }
        }
        if (0 == ret) {
          printf("Pass\n");
          // TODO: drop()
        }
      }
    }
    else printf("They report %u and %u. ", test_val0.count(), test_val1.count());
  }

  if (0 != ret) {
    printf("Fail.\n");
  }

  return ret;
}




int c3p_value_test_packing_parsing(const TCode FORMAT) {
  const uint32_t TEST_BUF_LEN    = (13 + (randomUInt32() % 12));
  int ret = 0;   // Success means complete fall-through.
  printf("Testing packing and parsing with format %s...\n", typecodeToStr(FORMAT));
  printf("\tPreparing test cases... ");
  StringBuilder buffer;

  const bool     TEST_VAL_BOOL   = flip_coin();
  const uint8_t  TEST_VAL_UINT8  = (uint8_t)  randomUInt32();
  const int8_t   TEST_VAL_INT8   = (int8_t)   randomUInt32();
  const uint16_t TEST_VAL_UINT16 = (uint16_t) randomUInt32();
  const int16_t  TEST_VAL_INT16  = (int16_t)  randomUInt32();
  const uint32_t TEST_VAL_UINT32 = randomUInt32();
  const int32_t  TEST_VAL_INT32  = (int32_t)  randomUInt32();
  const uint64_t TEST_VAL_UINT64 = generate_random_uint64();
  const int64_t  TEST_VAL_INT64  = generate_random_int64();
  const float    TEST_VAL_FLOAT  = generate_random_float();
  const double   TEST_VAL_DOUBLE = generate_random_double();
  StringBuilder  TEST_VAL_STRING;   // NOTE: Not a const. Keeping with the lexical pattern.
  StringBuilder  TEST_VAL_STRBLDR;  // NOTE: Not a const. Keeping with the lexical pattern.
  // Generate a test string of (TEST_BUF_LEN-1), because the wrapper will
  //   report the binary length of the contained data. Which includes the
  //   null-terminator for a C-style string.
  generate_random_text_buffer(&TEST_VAL_STRING,  (TEST_BUF_LEN-1));
  generate_random_text_buffer(&TEST_VAL_STRBLDR, (TEST_BUF_LEN-1));
  char*          TEST_VAL_STR_PTR = (char*) TEST_VAL_STRING.string();

  bool     parsed_val_bool    = !TEST_VAL_BOOL;
  uint8_t  parsed_val_uint8   = 0;
  int8_t   parsed_val_int8    = 0;
  uint16_t parsed_val_uint16  = 0;
  int16_t  parsed_val_int16   = 0;
  uint32_t parsed_val_uint32  = 0;
  int32_t  parsed_val_int32   = 0;
  uint64_t parsed_val_uint64  = 0;
  int64_t  parsed_val_int64   = 0;
  float    parsed_val_float   = 0.0;
  double   parsed_val_double  = 0.0;
  char*    parsed_val_string  = nullptr;
  StringBuilder* parsed_val_strbldr = nullptr;
  KeyValuePair*  parsed_val_kvp     = nullptr;

  C3PValue test_val_bool(TEST_VAL_BOOL);
  C3PValue test_val_uint8(TEST_VAL_UINT8);
  C3PValue test_val_int8(TEST_VAL_INT8);
  C3PValue test_val_uint16(TEST_VAL_UINT16);
  C3PValue test_val_int16(TEST_VAL_INT16);
  C3PValue test_val_uint32(TEST_VAL_UINT32);
  C3PValue test_val_int32(TEST_VAL_INT32);
  C3PValue test_val_uint64(TEST_VAL_UINT64);
  C3PValue test_val_int64(TEST_VAL_INT64);
  C3PValue test_val_float(TEST_VAL_FLOAT);
  C3PValue test_val_double(TEST_VAL_DOUBLE);
  C3PValue test_val_string(TEST_VAL_STR_PTR);
  //C3PValue test_val_strbldr(&TEST_VAL_STRBLDR);

  KeyValuePair test_val_kvp("key_bool", TEST_VAL_BOOL);
  test_val_kvp.isCompound(true);
  test_val_kvp.link(new KeyValuePair("key_uint8",  TEST_VAL_UINT8),   true);
  test_val_kvp.link(new KeyValuePair("key_int8",   TEST_VAL_INT8),    true);
  test_val_kvp.link(new KeyValuePair("key_uint16", TEST_VAL_UINT16),  true);
  test_val_kvp.link(new KeyValuePair("key_int16",  TEST_VAL_INT16),   true);
  test_val_kvp.link(new KeyValuePair("key_uint32", TEST_VAL_UINT32),  true);
  test_val_kvp.link(new KeyValuePair("key_int32",  TEST_VAL_INT32),   true);
  test_val_kvp.link(new KeyValuePair("key_uint64", TEST_VAL_UINT64),  true);
  test_val_kvp.link(new KeyValuePair("key_int64",  TEST_VAL_INT64),   true);
  test_val_kvp.link(new KeyValuePair("key_float",  TEST_VAL_FLOAT),   true);
  test_val_kvp.link(new KeyValuePair("key_double", TEST_VAL_DOUBLE),  true);
  //test_val_kvp.link(new KeyValuePair("key_string", TEST_VAL_STR_PTR),  true);

  C3PValue* deser_container_bool    = nullptr;
  C3PValue* deser_container_uint8   = nullptr;
  C3PValue* deser_container_int8    = nullptr;
  C3PValue* deser_container_uint16  = nullptr;
  C3PValue* deser_container_int16   = nullptr;
  C3PValue* deser_container_uint32  = nullptr;
  C3PValue* deser_container_int32   = nullptr;
  C3PValue* deser_container_uint64  = nullptr;
  C3PValue* deser_container_int64   = nullptr;
  C3PValue* deser_container_float   = nullptr;
  C3PValue* deser_container_double  = nullptr;
  C3PValue* deser_container_string  = nullptr;
  C3PValue* deser_container_strbldr = nullptr;
  C3PValue* deser_container_kvp     = nullptr;

  printf("Pass\n\t\tSerializing... ");
  ret += test_val_bool.serialize(&buffer, FORMAT);
  ret += test_val_uint8.serialize(&buffer, FORMAT);
  ret += test_val_int8.serialize(&buffer, FORMAT);
  ret += test_val_uint16.serialize(&buffer, FORMAT);
  ret += test_val_int16.serialize(&buffer, FORMAT);
  ret += test_val_uint32.serialize(&buffer, FORMAT);
  ret += test_val_int32.serialize(&buffer, FORMAT);
  ret += test_val_uint64.serialize(&buffer, FORMAT);
  ret += test_val_int64.serialize(&buffer, FORMAT);
  ret += test_val_float.serialize(&buffer, FORMAT);
  ret += test_val_double.serialize(&buffer, FORMAT);
  ret += test_val_string.serialize(&buffer, FORMAT);
  //ret += deser_container_strbldr.serialize(&buffer, FORMAT);
  ret += test_val_kvp.serialize(&buffer, FORMAT);

  if (0 == ret) {
    printf("Pass\n\t\tDeserializing... ");
    deser_container_bool    = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_uint8   = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_int8    = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_uint16  = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_int16   = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_uint32  = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_int32   = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_uint64  = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_int64   = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_float   = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_double  = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_string  = C3PValue::deserialize(&buffer, FORMAT);
    //deser_container_strbldr = C3PValue::deserialize(&buffer, FORMAT);
    deser_container_kvp     = C3PValue::deserialize(&buffer, FORMAT);

    ret -= (nullptr != deser_container_bool)    ? 0 : 1;
    ret -= (nullptr != deser_container_uint8)   ? 0 : 1;
    ret -= (nullptr != deser_container_int8)    ? 0 : 1;
    ret -= (nullptr != deser_container_uint16)  ? 0 : 1;
    ret -= (nullptr != deser_container_int16)   ? 0 : 1;
    ret -= (nullptr != deser_container_uint32)  ? 0 : 1;
    ret -= (nullptr != deser_container_int32)   ? 0 : 1;
    ret -= (nullptr != deser_container_uint64)  ? 0 : 1;
    ret -= (nullptr != deser_container_int64)   ? 0 : 1;
    ret -= (nullptr != deser_container_float)   ? 0 : 1;
    ret -= (nullptr != deser_container_double)  ? 0 : 1;
    ret -= (nullptr != deser_container_string)  ? 0 : 1;
    //ret -= (nullptr != deser_container_strbldr) ? 0 : 1;
    ret -= (nullptr != deser_container_kvp)      ? 0 : 1;
    if (0 == ret) {
      printf("Pass\n\t\tFetching values from container... ");
      ret -= (0 == deser_container_bool->get_as(&parsed_val_bool))       ? 0 : 1;
      ret -= (0 == deser_container_uint8->get_as(&parsed_val_uint8))     ? 0 : 1;
      ret -= (0 == deser_container_int8->get_as(&parsed_val_int8))       ? 0 : 1;
      ret -= (0 == deser_container_uint16->get_as(&parsed_val_uint16))   ? 0 : 1;
      ret -= (0 == deser_container_int16->get_as(&parsed_val_int16))     ? 0 : 1;
      ret -= (0 == deser_container_uint32->get_as(&parsed_val_uint32))   ? 0 : 1;
      ret -= (0 == deser_container_int32->get_as(&parsed_val_int32))     ? 0 : 1;
      ret -= (0 == deser_container_uint64->get_as(&parsed_val_uint64))   ? 0 : 1;
      ret -= (0 == deser_container_int64->get_as(&parsed_val_int64))     ? 0 : 1;
      ret -= (0 == deser_container_float->get_as(&parsed_val_float))     ? 0 : 1;
      ret -= (0 == deser_container_double->get_as(&parsed_val_double))   ? 0 : 1;
      ret -= (0 == deser_container_string->get_as(&parsed_val_string))   ? 0 : 1;
      //ret -= (0 == deser_container_strbldr->get_as(&parsed_val_strbldr)) ? 0 : 1;
      ret -= (deser_container_kvp->has_key() ? 0 : 1);
      if (0 == ret) {
        parsed_val_kvp = (KeyValuePair*) deser_container_kvp;
        printf("Pass\n\t\tComparing values... ");
        ret -= (TEST_VAL_BOOL    == parsed_val_bool   ) ? 0 : 1;
        ret -= (TEST_VAL_UINT8   == parsed_val_uint8  ) ? 0 : 1;
        ret -= (TEST_VAL_INT8    == parsed_val_int8   ) ? 0 : 1;
        ret -= (TEST_VAL_UINT16  == parsed_val_uint16 ) ? 0 : 1;
        ret -= (TEST_VAL_INT16   == parsed_val_int16  ) ? 0 : 1;
        ret -= (TEST_VAL_UINT32  == parsed_val_uint32 ) ? 0 : 1;
        ret -= (TEST_VAL_INT32   == parsed_val_int32  ) ? 0 : 1;
        ret -= (TEST_VAL_UINT64  == parsed_val_uint64 ) ? 0 : 1;
        ret -= (TEST_VAL_INT64   == parsed_val_int64  ) ? 0 : 1;
        ret -= (TEST_VAL_FLOAT   == parsed_val_float  ) ? 0 : 1;
        ret -= (TEST_VAL_DOUBLE  == parsed_val_double ) ? 0 : 1;
        ret -= (0 == StringBuilder::strcasecmp(parsed_val_string,  (const char*) TEST_VAL_STR_PTR) ) ? 0 : 1;
        //ret -= (0 == StringBuilder::strcasecmp(parsed_val_strbldr, (const char*) TEST_VAL_STRBLDR.string())) ? 0 : 1;

        // TODO:
        // parsed_val_kvp->containsStructure(&src_kvp);
        // parsed_val_kvp->equals(&src_kvp);
        if ((test_val_kvp.count() != parsed_val_kvp->count())) {
          ret--;
        }

        if (0 == ret) {
          printf("Pass\n\t\tString was fully consumed... ");
          ret -= buffer.length();
          if (0 == ret) {
            printf("Pass\n");
          }
        }
      }
    }
  }

  if (0 != ret) {
    printf("Fail (%d).\n", ret);
  }

  StringBuilder ascii_buf;
  buffer.printDebug(&ascii_buf);
  printf("Uncomsumed buffer (%u bytes): %s\n", buffer.length(), (char*) ascii_buf.string());
  printf("Test value/Parsed value:\n");
  printf("\t%c / %c\n", (TEST_VAL_BOOL ? 't':'f'), (parsed_val_bool ? 't':'f'));
  printf("\t%u / %u\n", TEST_VAL_UINT8,  parsed_val_uint8);
  printf("\t%d / %d\n", TEST_VAL_INT8,   parsed_val_int8);
  printf("\t%u / %u\n", TEST_VAL_UINT16, parsed_val_uint16);
  printf("\t%d / %d\n", TEST_VAL_INT16,  parsed_val_int16);
  printf("\t%u / %u\n", TEST_VAL_UINT32, parsed_val_uint32);
  printf("\t%d / %d\n", TEST_VAL_INT32,  parsed_val_int32);
  printf("\t%llu / %llu\n", TEST_VAL_UINT64, parsed_val_uint64);
  printf("\t%lld / %lld\n", TEST_VAL_INT64,  parsed_val_int64);
  printf("\t%.3f / %.3f\n", TEST_VAL_FLOAT,  parsed_val_float);
  printf("\t%.6f / %.6f\n", TEST_VAL_DOUBLE, parsed_val_double);
  printf("\t%s / %s\n", TEST_VAL_STR_PTR, (nullptr == parsed_val_string) ? "(null)" : parsed_val_string);
  printf("Source KVP contents: \n");
  dump_kvp(&test_val_kvp);
  if (nullptr != parsed_val_kvp) {
    printf("Parsed KVP contents: \n");
    dump_kvp(parsed_val_kvp);
  }

  if (nullptr != deser_container_bool) {     delete deser_container_bool;     }
  if (nullptr != deser_container_uint8) {    delete deser_container_uint8;    }
  if (nullptr != deser_container_int8) {     delete deser_container_int8;     }
  if (nullptr != deser_container_uint16) {   delete deser_container_uint16;   }
  if (nullptr != deser_container_int16) {    delete deser_container_int16;    }
  if (nullptr != deser_container_uint32) {   delete deser_container_uint32;   }
  if (nullptr != deser_container_int32) {    delete deser_container_int32;    }
  if (nullptr != deser_container_uint64) {   delete deser_container_uint64;   }
  if (nullptr != deser_container_int64) {    delete deser_container_int64;    }
  if (nullptr != deser_container_float) {    delete deser_container_float;    }
  if (nullptr != deser_container_double) {   delete deser_container_double;   }
  if (nullptr != deser_container_string) {   delete deser_container_string;   }
  if (nullptr != deser_container_strbldr) {  delete deser_container_strbldr;  }
  if (nullptr != deser_container_kvp) {      delete deser_container_kvp;      }
  return ret;
}




/*******************************************************************************
* C3PValue test plan
*******************************************************************************/
#define CHKLST_C3PVAL_TEST_NUMERICS        0x00000001  //
#define CHKLST_C3PVAL_TEST_VECTORS         0x00000002  //
#define CHKLST_C3PVAL_TEST_STRINGS         0x00000004  //
#define CHKLST_C3PVAL_TEST_BLOBS           0x00000008  //
#define CHKLST_C3PVAL_TEST_TIMER_TYPES     0x00000010  //
#define CHKLST_C3PVAL_TEST_CONVERSION      0x00000020  //
#define CHKLST_C3PVAL_TEST_PACK_PARSE_BIN  0x00000040  //
#define CHKLST_C3PVAL_TEST_PACK_PARSE_CBOR 0x00000080  //
#define CHKLST_C3PVAL_TEST_ALIGNMENT       0x00000100  //
#define CHKLST_C3PVAL_TEST_LINKING         0x00000200  //
#define CHKLST_C3PVAL_TEST_ARRAYS          0x00000400  //

#define CHKLST_C3PVAL_TESTS_BASICS ( \
  CHKLST_C3PVAL_TEST_NUMERICS | CHKLST_C3PVAL_TEST_VECTORS | \
  CHKLST_C3PVAL_TEST_STRINGS | CHKLST_C3PVAL_TEST_BLOBS | \
  CHKLST_C3PVAL_TEST_TIMER_TYPES | \
  CHKLST_C3PVAL_TEST_ALIGNMENT)

#define CHKLST_C3PVAL_TESTS_ALL ( \
  CHKLST_C3PVAL_TESTS_BASICS | CHKLST_C3PVAL_TEST_CONVERSION | \
  CHKLST_C3PVAL_TEST_LINKING | CHKLST_C3PVAL_TEST_PACK_PARSE_CBOR)

const StepSequenceList TOP_LEVEL_C3PVALUE_TEST_LIST[] = {
  { .FLAG         = CHKLST_C3PVAL_TEST_NUMERICS,
    .LABEL        = "Basic numerics",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_numerics()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PVAL_TEST_VECTORS,
    .LABEL        = "Vectors",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_vectors()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PVAL_TEST_STRINGS,
    .LABEL        = "Strings",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_strings()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PVAL_TEST_BLOBS,
    .LABEL        = "BLOBs",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_blobs()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PVAL_TEST_TIMER_TYPES,
    .LABEL        = "Timer Types",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_timer_types()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PVAL_TEST_ALIGNMENT,
    .LABEL        = "Alignment nightmare case",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_alignment()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PVAL_TEST_CONVERSION,
    .LABEL        = "Type conversion",
    .DEP_MASK     = (CHKLST_C3PVAL_TESTS_BASICS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_type_conversion()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PVAL_TEST_LINKING,
    .LABEL        = "Linking mechanism",
    .DEP_MASK     = (CHKLST_C3PVAL_TESTS_BASICS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_linking()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PVAL_TEST_ARRAYS,
    .LABEL        = "Nested arrays",
    .DEP_MASK     = (CHKLST_C3PVAL_TEST_LINKING | CHKLST_C3PVAL_TEST_CONVERSION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_nested_arrays()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PVAL_TEST_PACK_PARSE_BIN,
    .LABEL        = "Packing and Parsing (BIN)",
    .DEP_MASK     = (CHKLST_C3PVAL_TEST_ARRAYS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_packing_parsing(TCode::BINARY)) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PVAL_TEST_PACK_PARSE_CBOR,
    .LABEL        = "Packing and Parsing (CBOR)",
    .DEP_MASK     = (CHKLST_C3PVAL_TEST_ARRAYS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_value_test_packing_parsing(TCode::CBOR)) ? 1:-1);  }
  },
};

AsyncSequencer c3pvalue_test_plan(TOP_LEVEL_C3PVALUE_TEST_LIST, (sizeof(TOP_LEVEL_C3PVALUE_TEST_LIST) / sizeof(TOP_LEVEL_C3PVALUE_TEST_LIST[0])));



/*******************************************************************************
* The main function.
*******************************************************************************/

void print_types_c3p_value() {
  printf("\tC3PValue              %u\t%u\n", sizeof(C3PValue),  alignof(C3PValue));
}


int c3p_value_test_main() {
  const char* const MODULE_NAME = "C3PValue";
  printf("===< %s >=======================================\n", MODULE_NAME);

  c3pvalue_test_plan.requestSteps(CHKLST_C3PVAL_TESTS_ALL);
  while (!c3pvalue_test_plan.request_completed() && (0 == c3pvalue_test_plan.failed_steps(false))) {
    c3pvalue_test_plan.poll();
  }
  int ret = (c3pvalue_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  c3pvalue_test_plan.printDebug(&report_output, "C3PValue test report");
  printf("%s\n", (char*) report_output.string());
  return ret;
}
