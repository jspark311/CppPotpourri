/*
File:   GlueTests.cpp
Author: J. Ian Lindsay
Date:   2023.09.07


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


This program tests several basic functions in the library that are widely
  relied upon.
NOTE: RNG is tested elsewhere, but it needs to be promoted to a top-level test
  to give dependency assurances, and ensure that the RNG returning all zeros
  doesn't allow a test to pass on false grounds.
*/

#include "TimerTools/TimerTools.h"


/*******************************************************************************
* Test-case class, and case definitions...
*******************************************************************************/

class C3PHeaderTestCase {
  public:
    C3PHeaderTestCase(const TCode TCODE) : test_type(TCODE) {};
    ~C3PHeaderTestCase() {};

    int8_t run_test_swap();
    int8_t run_test_min();
    int8_t run_test_max();
    int8_t run_test_abs_delta();
    int8_t run_test_wrap_delta();
    int8_t run_test_range_bind();
    int8_t run_test_endian_flip();

    void printDebug(StringBuilder*);

    TCode test_type;
    StopWatch profiler_swap;
    StopWatch profiler_min;
    StopWatch profiler_max;
    StopWatch profiler_abs_delta;
    StopWatch profiler_wrap_delta;
    StopWatch profiler_range_bind;
    StopWatch profiler_end_flip;
};


// The following type codes have type-strict wrapper inlines in C3P.
const TCode NUMERIC_TEST_TYPES[] = {
  TCode::INT8,    // 8-bit integer
  TCode::INT16,   // 16-bit integer
  TCode::INT32,   // 32-bit integer
  TCode::INT64,   // 64-bit integer
  TCode::UINT8,   // Unsigned 8-bit integer
  TCode::UINT16,  // Unsigned 16-bit integer
  TCode::UINT32,  // Unsigned 32-bit integer
  TCode::UINT64,  // Unsigned 64-bit integer
  TCode::FLOAT,   // A float
  TCode::DOUBLE   // A double
};


/*******************************************************************************
* Test implementation
*******************************************************************************/
int8_t C3PHeaderTestCase::run_test_swap() {
  // Value-swap is an easy test.
  const uint8_t  CA_U8  = (uint8_t)  randomUInt32();
  const uint8_t  CB_U8  = (uint8_t)  randomUInt32();
  const uint16_t CA_U16 = (uint16_t) randomUInt32();
  const uint16_t CB_U16 = (uint16_t) randomUInt32();
  const uint32_t CA_U32 = randomUInt32();
  const uint32_t CB_U32 = randomUInt32();
  const uint64_t CA_U64 = generate_random_uint64();
  const uint64_t CB_U64 = generate_random_uint64();
  const int8_t  CA_I8   = (int8_t)  randomUInt32();
  const int8_t  CB_I8   = (int8_t)  randomUInt32();
  const int16_t CA_I16  = (int16_t) randomUInt32();
  const int16_t CB_I16  = (int16_t) randomUInt32();
  const int32_t CA_I32  = (int32_t) randomUInt32();
  const int32_t CB_I32  = (int32_t) randomUInt32();
  const int64_t CA_I64  = generate_random_int64();
  const int64_t CB_I64  = generate_random_int64();
  uint8_t  a_u8  = CA_U8;
  uint8_t  b_u8  = CB_U8;
  uint16_t a_u16 = CA_U16;
  uint16_t b_u16 = CB_U16;
  uint32_t a_u32 = CA_U32;
  uint32_t b_u32 = CB_U32;
  uint64_t a_u64 = CA_U64;
  uint64_t b_u64 = CB_U64;
  int8_t  a_i8   = CA_I8;
  int8_t  b_i8   = CB_I8;
  int16_t a_i16  = CA_I16;
  int16_t b_i16  = CB_I16;
  int32_t a_i32  = CA_I32;
  int32_t b_i32  = CB_I32;
  int64_t a_i64  = CA_I64;
  int64_t b_i64  = CB_I64;

  profiler_swap.markStart();

  bool test_failed  = true;
  switch (test_type) {
    case TCode::FLOAT:
    case TCode::DOUBLE:
      return 0;  // TODO: This is supported.
      break;

    case TCode::UINT8:
      strict_swap(&a_u8, &b_u8);
      test_failed  = ((CA_U8 != b_u8) | (CB_U8 != a_u8));
      break;
    case TCode::UINT16:
      strict_swap(&a_u16, &b_u16);
      test_failed  = ((CA_U16 != b_u16) | (CB_U16 != a_u16));
      break;
    case TCode::UINT32:
      strict_swap(&a_u32, &b_u32);
      test_failed  = ((CA_U32 != b_u32) | (CB_U32 != a_u32));
      break;
    case TCode::UINT64:
      strict_swap(&a_u64, &b_u64);
      test_failed  = ((CA_U64 != b_u64) | (CB_U64 != a_u64));
      break;
    case TCode::INT8:
      strict_swap(&a_i8, &b_i8);
      test_failed  = ((CA_I8 != b_i8) | (CB_I8 != a_i8));
      break;
    case TCode::INT16:
      strict_swap(&a_i16, &b_i16);
      test_failed  = ((CA_I16 != b_i16) | (CB_I16 != a_i16));
      break;
    case TCode::INT32:
      strict_swap(&a_i32, &b_i32);
      test_failed  = ((CA_I32 != b_i32) | (CB_I32 != a_i32));
      break;
    case TCode::INT64:
      strict_swap(&a_i64, &b_i64);
      test_failed  = ((CA_I64 != b_i64) | (CB_I64 != a_i64));
      break;
    default:
      test_failed = false;
      break;
  }
  profiler_swap.markStop();
  return (test_failed ? -1 : 0);
}



int8_t C3PHeaderTestCase::run_test_min() {
  bool test_failed  = true;
  switch (test_type) {
    case TCode::INT8:
      break;
    case TCode::INT16:
      break;
    case TCode::INT32:
      break;
    case TCode::UINT8:
      break;
    case TCode::UINT16:
      break;
    case TCode::UINT32:
      break;
    case TCode::INT64:
      break;
    case TCode::INT128:
      break;
    case TCode::UINT64:
      break;
    case TCode::FLOAT:
      break;
    case TCode::DOUBLE:
      break;
    default:
      break;
  }
  return (test_failed ? -1 : 0);
}



int8_t C3PHeaderTestCase::run_test_max() {
  bool test_failed  = false;
  return (test_failed ? -1 : 0);
}



int8_t C3PHeaderTestCase::run_test_abs_delta() {
  bool test_failed  = false;
  return (test_failed ? -1 : 0);
}



int8_t C3PHeaderTestCase::run_test_wrap_delta() {
  bool test_failed  = false;
  const uint8_t   C_NOW_U8     = (uint8_t) randomUInt32();
  const uint8_t   C_BEHIND_U8  = (C_NOW_U8 - 1);  // The non-wrapped case.
  const uint8_t   C_AHEAD_U8   = (C_NOW_U8 + 1);  // The overflow case.
  const uint16_t  C_NOW_U16    = (uint16_t) randomUInt32();
  const uint16_t  C_BEHIND_U16 = (C_NOW_U16 - 1);  // The non-wrapped case.
  const uint16_t  C_AHEAD_U16  = (C_NOW_U16 + 1);  // The overflow case.
  const uint32_t  C_NOW_U32    = (uint32_t) randomUInt32();
  const uint32_t  C_BEHIND_U32 = (C_NOW_U32 - 1);  // The non-wrapped case.
  const uint32_t  C_AHEAD_U32  = (C_NOW_U32 + 1);  // The overflow case.
  const uint64_t  C_NOW_U64    = generate_random_uint64();
  const uint64_t  C_BEHIND_U64 = (C_NOW_U64 - 1);  // The non-wrapped case.
  const uint64_t  C_AHEAD_U64  = (C_NOW_U64 + 1);  // The overflow case.

  // NOTE: Integer overflow is here being assumed to happen "naturally". But
  //   your compiler/hardware may not handle overflow in a manner that allows
  //   this test to make sense. YMMV.
  profiler_wrap_delta.markStart();
  switch (test_type) {
    case TCode::UINT8:
      test_failed  = (delta_assume_wrap(C_NOW_U8, C_BEHIND_U8) != delta_assume_wrap(C_AHEAD_U8, C_NOW_U8));
      test_failed |= ((1 << 8) != (delta_assume_wrap(C_NOW_U8, C_BEHIND_U8) + delta_assume_wrap(C_BEHIND_U8, C_NOW_U8)));
      test_failed |= ((1 << 8) != (delta_assume_wrap(C_NOW_U8, C_AHEAD_U8)  + delta_assume_wrap(C_AHEAD_U8, C_NOW_U8)));
      test_failed |= (C_NOW_U8 != (delta_assume_wrap(C_NOW_U8, (uint8_t) 0)));
      test_failed |= (0 != (delta_assume_wrap((uint8_t) 0, (uint8_t) 0)));
      test_failed |= (0 != (delta_assume_wrap(C_NOW_U8, C_NOW_U8)));
      break;
    case TCode::UINT16:
      test_failed  = (delta_assume_wrap(C_NOW_U16, C_BEHIND_U16) != delta_assume_wrap(C_AHEAD_U16, C_NOW_U16));
      test_failed |= ((1 << 16) != (delta_assume_wrap(C_NOW_U16, C_BEHIND_U16) + delta_assume_wrap(C_BEHIND_U16, C_NOW_U16)));
      test_failed |= ((1 << 16) != (delta_assume_wrap(C_NOW_U16, C_AHEAD_U16)  + delta_assume_wrap(C_AHEAD_U16, C_NOW_U16)));
      test_failed |= (C_NOW_U16 != (delta_assume_wrap(C_NOW_U16, (uint16_t) 0)));
      test_failed |= (0 != (delta_assume_wrap((uint16_t) 0, (uint16_t) 0)));
      test_failed |= (0 != (delta_assume_wrap(C_NOW_U16, C_NOW_U16)));
      break;
    case TCode::UINT32:
      test_failed  = (delta_assume_wrap(C_NOW_U32, C_BEHIND_U32) != delta_assume_wrap(C_AHEAD_U32, C_NOW_U32));
      // NOTE: Slight methodology difference to account for the fact that
      //   we aren't running tests on 8 or 16-bit ALUs, but don't know if the
      //   ALU is 32 or 64.
      test_failed |= (0xFFFFFFFF != (delta_assume_wrap(C_NOW_U32, C_BEHIND_U32) + (delta_assume_wrap(C_BEHIND_U32, C_NOW_U32) - 1)));
      test_failed |= (0xFFFFFFFF != (delta_assume_wrap(C_NOW_U32, C_AHEAD_U32)  + (delta_assume_wrap(C_AHEAD_U32, C_NOW_U32) - 1)));
      test_failed |= (C_NOW_U32 != (delta_assume_wrap(C_NOW_U32, (uint32_t) 0)));
      test_failed |= (0 != (delta_assume_wrap((uint32_t) 0, (uint32_t) 0)));
      test_failed |= (0 != (delta_assume_wrap(C_NOW_U32, C_NOW_U32)));
      break;
    case TCode::UINT64:
      test_failed  = (delta_assume_wrap(C_NOW_U64, C_BEHIND_U64) != delta_assume_wrap(C_AHEAD_U64, C_NOW_U64));
      // NOTE: Slight methodology difference to account for the fact that
      //   we aren't running tests on 8 or 16-bit ALUs, but don't know if the
      //   ALU is 32 or 64.
      test_failed |= (0xFFFFFFFFFFFFFFFF != (delta_assume_wrap(C_NOW_U64, C_BEHIND_U64) + (delta_assume_wrap(C_BEHIND_U64, C_NOW_U64) - 1)));
      test_failed |= (0xFFFFFFFFFFFFFFFF != (delta_assume_wrap(C_NOW_U64, C_AHEAD_U64)  + (delta_assume_wrap(C_AHEAD_U64, C_NOW_U64) - 1)));
      test_failed |= (C_NOW_U64 != (delta_assume_wrap(C_NOW_U64, (uint64_t) 0)));
      test_failed |= (0 != (delta_assume_wrap((uint64_t) 0, (uint64_t) 0)));
      test_failed |= (0 != (delta_assume_wrap(C_NOW_U64, C_NOW_U64)));
      break;
    default:  return 0;   // NOTE: Hard bail-out.
  }
  profiler_wrap_delta.markStop();

  return (test_failed ? -1 : 0);
}



int8_t C3PHeaderTestCase::run_test_range_bind() {
  bool test_failed  = false;
  return (test_failed ? -1 : 0);
}


int8_t C3PHeaderTestCase::run_test_endian_flip() {
  // Endian-swap is an easy test, and only three types support it.
  bool test_failed  = false;
  //while ((!test_failed) && ( < HEADER_TEST_COUNT)) {
    profiler_end_flip.markStart();
    switch (test_type) {
      case TCode::UINT16:
        test_failed = (0x5AA5 != endianSwap16(0xA55A));
        break;
      case TCode::UINT32:
        test_failed = (0x04030201 != endianSwap32(0x01020304));
        break;
      case TCode::UINT64:
        test_failed = (0x0807060504030201 != endianSwap64(0x0102030405060708));
        break;
      default:  return 0;   // NOTE: Hard bail-out.
    }
    profiler_end_flip.markStop();
  //}
  return (test_failed ? -1 : 0);
}


void C3PHeaderTestCase::printDebug(StringBuilder* out) {
  StopWatch::printDebugHeader(out);
  profiler_swap.printDebug("strict_swap()", out);
  profiler_min.printDebug("strict_min()", out);
  profiler_max.printDebug("strict_max()", out);
  profiler_abs_delta.printDebug("strict_abs_delta()", out);
  profiler_wrap_delta.printDebug("delta_assume_wrap()", out);
  profiler_range_bind.printDebug("strict_range_bind()", out);
  switch (test_type) {
    case TCode::UINT8:   profiler_end_flip.printDebug("endianSwap8()",  out);  break;
    case TCode::UINT16:  profiler_end_flip.printDebug("endianSwap16()", out);  break;
    case TCode::UINT32:  profiler_end_flip.printDebug("endianSwap32()", out);  break;
    default:  break;
  }
}



/*******************************************************************************
* Test orchestration
*******************************************************************************/

/*
*
*/
int c3p_ref_counter_tests() {
  bool test_failed = true;
  printf("Running C3PRefCounter tests...\n");
  const uint16_t INITIAL_REF_COUNT = C3PRefCounter::MAXIMUM_REFS + ((randomUInt32() % 57)|1);
  C3PRefCounter test_ref_zero;
  C3PRefCounter test_ref_nonzero(INITIAL_REF_COUNT);
  printf("\tConstruction semantics are correct... ");
  if ((0 == test_ref_zero.refCount()) & (INITIAL_REF_COUNT == test_ref_nonzero.refCount())) {
    printf("Pass.\n\tTrying to decrement a zero count returns true, and the count remains zero... ");
    if (test_ref_zero.refRelease() && (0 == test_ref_zero.refCount())) {
      printf("Pass.\n\tThe count can be incremented... ");
      if (test_ref_zero.refTake() && (1 == test_ref_zero.refCount())) {
        printf("Pass.\n\tThe count advances to a maximum value of C3PRefCounter::MAXIMUM_REFS (%u)... ", C3PRefCounter::MAXIMUM_REFS);
        while ((C3PRefCounter::MAXIMUM_REFS > test_ref_zero.refCount()) && test_ref_zero.refTake()) {};
        if (C3PRefCounter::MAXIMUM_REFS == test_ref_zero.refCount()) {
          printf("Pass.\n\tNo additional references can be taken past that point... ");
          if (!test_ref_zero.refTake()) {
            printf("Pass.\n\tRefs can decrement... ");
            if (!test_ref_zero.refRelease() && ((C3PRefCounter::MAXIMUM_REFS - 1) == test_ref_zero.refCount())) {
              printf("Pass.\n\tRefs can decrement all the way to zero before refRelease() returns true... ");
              while ((1 < test_ref_zero.refCount()) && !test_ref_zero.refRelease()) {};
              if (1 == test_ref_zero.refCount() && test_ref_zero.refRelease()) {
                printf("Pass.\n\trefTake() returns false for a C3PRefCounter constructed with a value above C3PRefCounter::MAXIMUM_REFS... ");
                if (!test_ref_nonzero.refTake() && (INITIAL_REF_COUNT == test_ref_nonzero.refCount())) {
                  printf("Pass.\n\trefRelease() still works as expected... ");
                  if (!test_ref_nonzero.refRelease() && ((INITIAL_REF_COUNT - 1) == test_ref_nonzero.refCount())) {
                    printf("Pass.\n\tC3PRefCounter tests all pass.\n");
                    test_failed = false;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  if (test_failed) {
    printf(" Fail.\n");
  }
  return (test_failed ? -1 : 0);
}


/*
*
*/
int numeric_helper_tests() {
  printf("Running tests on inline numeric helpers...\n");
  const int CASE_COUNT = sizeof(NUMERIC_TEST_TYPES);
  int  case_idx        = 0;  // NOTE: Cloaked fourth-order loop. Tread carefully.
  bool test_failed  = false;
  //glue_test_cases[case_idx].

  while ((case_idx < CASE_COUNT) & (!test_failed)) {
    printf("\tBeginning case %d (%s)...\n", case_idx, typecodeToStr(NUMERIC_TEST_TYPES[case_idx]));
    C3PHeaderTestCase test_case(NUMERIC_TEST_TYPES[case_idx]);
    test_failed = true;
    printf("\t\tEndian flip... ");
    if (0 == test_case.run_test_endian_flip()) {
      printf("Pass.\n\t\tstrict_swap()... ");
      if (0 == test_case.run_test_swap()) {
        printf("Pass.\n\t\tdelta_assume_wrap()... ");
        if (0 == test_case.run_test_wrap_delta()) {
          // TODO: strict_abs_delta()
          // TODO: strict_range_bind()  <---rename to this for consistency.
          printf("Pass.\n\t\tTest case %d passes.\n", case_idx);
          test_failed  = false;
        }
      }
    }

    if (test_failed) {
      printf(" Fail.\n");
      StringBuilder log;
      test_case.printDebug(&log);
      printf("\n%s\n", (const char*) log.string());
    }
    case_idx++;  // Move to the next case.
  }
  return (test_failed ? -1 : 0);
}



struct SIUnit_KAT{
  const SIUnit* UNIT_STR;
  const char*   EXPECTED_LONG;
  const char*   EXPECTED_SHORT;
};


const SIUnit ORDER_OF_MAG_TEST_0[] = {SIUnit::UNIT_GRAMMAR_MARKER, SIUnit::META_ORDER_OF_MAGNITUDE, (SIUnit) -6, SIUnit::RADIANS, SIUnit::UNITLESS};
const SIUnit ORDER_OF_MAG_TEST_1[] = {SIUnit::UNIT_GRAMMAR_MARKER, SIUnit::META_ORDER_OF_MAGNITUDE, (SIUnit) 6,  SIUnit::METERS,  SIUnit::UNITLESS};
const SIUnit SPECIFIC_IMPULSE[] = {SIUnit::UNIT_GRAMMAR_MARKER, SIUnit::NEWTONS, SIUnit::OPERATOR_MULTIPLIED, SIUnit::SECONDS, SIUnit::OPERATOR_DIVIDED, SIUnit::META_ORDER_OF_MAGNITUDE, (SIUnit) 3, SIUnit::GRAMS, SIUnit::UNITLESS};

SIUnit_KAT SIUNIT_KATS[] = {
  {
    ORDER_OF_MAG_TEST_0,
    "microradians", "urad"
  },
  {
    ORDER_OF_MAG_TEST_1,
    "megameters", "urad"
  },
};

/*
* There is a small collection of functions surrounding the SIUnit enum. Most of
*   their bulk is simple 1-to-1 string lookup and return. But a small fraction
*   of the enum space has special behavior that is not const.
*/
int c3p_siunit_tests() {
  printf("Running tests on SIUnit handling...\n");
  bool test_failed  = false;

  StringBuilder should_be_microradians;
  StringBuilder should_be_megameters;
  StringBuilder should_be_specific_impulse;

  const char EXPECTED_0[] = "microradians";
  const char EXPECTED_1[] = "megameters";
  const char EXPECTED_2[] = "N*s/kg";

  printf("\tOrder-of-magnitude handling...\n");
  SIUnitToStr(ORDER_OF_MAG_TEST_0, &should_be_microradians, false);
  SIUnitToStr(ORDER_OF_MAG_TEST_1, &should_be_megameters, false);
  SIUnitToStr(SPECIFIC_IMPULSE,    &should_be_specific_impulse, true);

  printf("\t\tThe unit string produced the expected output (\"%s\")... ", EXPECTED_0);
  test_failed |= (0 != should_be_microradians.locate((const uint8_t*) EXPECTED_0, strlen(EXPECTED_0)));
  if (!test_failed) {
    printf("Pass.\n\t\tThe unit string produced the expected output (\"%s\")... ", EXPECTED_1);
    test_failed |= (0 != should_be_megameters.locate((const uint8_t*) EXPECTED_1, strlen(EXPECTED_1)));
    if (!test_failed) {
      // TODO: Off-scale handling, patch gaps in range.
    }
  }

  if (!test_failed) {
    printf("Pass.\n");
    printf("\tArbitrary unit construction...\n");
    test_failed |= (0 != should_be_specific_impulse.locate((const uint8_t*) EXPECTED_2, strlen(EXPECTED_2)));
  }

  if (test_failed) {
    printf("Fail.\n");
    StringBuilder log;
    log.concatf("Expected \"%s\"\tand got \"%s\"\n", EXPECTED_0, (char*) should_be_microradians.string());
    log.concatf("Expected \"%s\"\tand got \"%s\"\n", EXPECTED_1, (char*) should_be_megameters.string());
    log.concatf("Expected \"%s\"\tand got \"%s\"\n", EXPECTED_2, (char*) should_be_specific_impulse.string());
    printf("\n%s\n", (const char*) log.string());
  }
  else {
    printf("PASS.\n");
  }

  return (test_failed ? -1 : 0);
}


void print_types_glue() {
  printf("\tC3PRefCounter             %u\t%u\n", sizeof(C3PRefCounter),  alignof(C3PRefCounter));
}


/*******************************************************************************
* Test plan
*******************************************************************************/
#define CHKLST_C3PGLUE_TEST_ANTIMACRO    0x00000001  // strict_max(), strict_min(), etc...
#define CHKLST_C3PGLUE_TEST_REF_COUNTER  0x00000002  // The reference counter class.
#define CHKLST_C3PGLUE_TEST_SIUNIT_ENUM  0x00000004  // The functions surrounding the SIUnit enum.


#define CHKLST_C3PGLUE_TESTS_ALL ( \
  CHKLST_C3PGLUE_TEST_ANTIMACRO | CHKLST_C3PGLUE_TEST_REF_COUNTER | \
  CHKLST_C3PGLUE_TEST_SIUNIT_ENUM)

const StepSequenceList TOP_LEVEL_GLUE_TEST_LIST[] = {
  { .FLAG         = CHKLST_C3PGLUE_TEST_ANTIMACRO,
    .LABEL        = "Anti-macro numeric helpers",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == numeric_helper_tests()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PGLUE_TEST_REF_COUNTER,
    .LABEL        = "ReCounter class",
    .DEP_MASK     = (CHKLST_C3PGLUE_TEST_ANTIMACRO),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_ref_counter_tests()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PGLUE_TEST_SIUNIT_ENUM,
    .LABEL        = "SIUnit enum",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == c3p_siunit_tests()) ? 1:-1);  }
  },
};

AsyncSequencer glue_test_plan(TOP_LEVEL_GLUE_TEST_LIST, (sizeof(TOP_LEVEL_GLUE_TEST_LIST) / sizeof(TOP_LEVEL_GLUE_TEST_LIST[0])));


/*******************************************************************************
* The main function.
*******************************************************************************/


int c3p_header_test_main() {
  const char* const MODULE_NAME = "C3P Header";
  printf("===< %s >=======================================\n", MODULE_NAME);

  glue_test_plan.requestSteps(CHKLST_C3PGLUE_TESTS_ALL);
  while (!glue_test_plan.request_completed() && (0 == glue_test_plan.failed_steps(false))) {
    glue_test_plan.poll();
  }
  int ret = (glue_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  glue_test_plan.printDebug(&report_output, "C3P Header test report");
  printf("%s\n", (char*) report_output.string());

  return ret;
}
