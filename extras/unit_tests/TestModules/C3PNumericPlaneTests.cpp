/*
File:   C3PNumericPlaneTests.cpp
Author: ChatGPT
Date:   2026.01.10


Copyright 2026 Manuvr, Inc

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.


This program tests C3PNumericPlane.

TODO: C3PNumericPlane tests suck. I tried to have ChatGPT write it, and it did
  all sorts of things I wouldn't have.
  1) Remove all of the malloc/free for test data. Unless the buffers are being
       handed to the class under test, those buffers should be stack-allocated.
  2) Remove the TEST_ASSERT macros. Yes: they read cleaner in the tests. But
      they also make the structure much more brittle. Not exactly the robot's
      fault, since my test "framework" is ugly and brittle, anyway.
  Once the slop is cleaned up, migrate into SmallDataStructureTests.cpp and
    eventually drop this file.
*/

#include "C3PNumericPlane.h"


/*******************************************************************************
* Local test helpers
*******************************************************************************/

#ifndef TEST_ASSERT
#define TEST_ASSERT(cond)   do { if (!(cond)) { ret = -1; goto fail; } } while (0)
#endif

#ifndef TEST_ASSERT_EQ_U32
#define TEST_ASSERT_EQ_U32(a,b)   do { if (((uint32_t)(a)) != ((uint32_t)(b))) { ret = -1; goto fail; } } while (0)
#endif

#ifndef TEST_ASSERT_EQ_I32
#define TEST_ASSERT_EQ_I32(a,b)   do { if (((int32_t)(a)) != ((int32_t)(b))) { ret = -1; goto fail; } } while (0)
#endif

static int _qsort_cmp_float(const void* a, const void* b) {
  const float A = *((const float*) a);
  const float B = *((const float*) b);
  if (A < B) return -1;
  if (A > B) return  1;
  return 0;
}


/*
* Covers:
*  - setSize()
*  - reallocate()
*  - free/malloc resize semantics (content preserved up to min(old,new))
*/
int test_plane_reallocation() {
  int ret = 0;
  printf("Testing C3PNumericPlane reallocation/resize...\n");

  const uint16_t X0 = _rand_u16_range(17, 71);
  const uint16_t Y0 = _rand_u16_range(17, 71);
  const uint16_t X1 = _rand_u16_range(73, 131);
  const uint16_t Y1 = _rand_u16_range(73, 131);
  const uint16_t SNAP_X = (uint16_t) ((X0 > 16) ? 16 : X0);
  const uint16_t SNAP_Y = (uint16_t) ((Y0 > 16) ? 16 : Y0);
  const uint16_t X2 = _rand_u16_range(9, 31);
  const uint16_t Y2 = _rand_u16_range(9, 31);
  const uint16_t CHECK_X = (uint16_t) ((X2 < SNAP_X) ? X2 : SNAP_X);
  const uint16_t CHECK_Y = (uint16_t) ((Y2 < SNAP_Y) ? Y2 : SNAP_Y);

  C3PNumericPlane<float> p(X0, Y0);

  /* Force ownership + allocation deterministically. */
  TEST_ASSERT(p.allocated());
  TEST_ASSERT(p.buffer() != nullptr);

  /* Fill with a deterministic-ish but fuzz-driven pattern. */
  for (uint16_t y = 0; y < Y0; y++) {
    for (uint16_t x = 0; x < X0; x++) {
      const float v = _rand_f32_range(-10.0f, 10.0f) + (float) x * 0.01f + (float) y * 0.001f;
      TEST_ASSERT(p.setValue(x, y, v));
    }
  }

  /* Snapshot top-left block that will survive a grow/shrink. */
  float snap[16 * 16];
  for (uint16_t y = 0; y < SNAP_Y; y++) {
    for (uint16_t x = 0; x < SNAP_X; x++) {
      snap[((uint32_t) y * 16u) + x] = p.getValue(x, y);
    }
  }

  /* Grow via setSize (should resize owned buffer) */
  TEST_ASSERT(p.setSize(X1, Y1));
  TEST_ASSERT_EQ_U32(p.width(), X1);
  TEST_ASSERT_EQ_U32(p.height(), Y1);
  TEST_ASSERT(p.allocated());

  /* Verify preserved region */
  for (uint16_t y = 0; y < SNAP_Y; y++) {
    for (uint16_t x = 0; x < SNAP_X; x++) {
      const float v = p.getValue(x, y);
      const float s = snap[((uint32_t) y * 16u) + x];
      TEST_ASSERT(v == s);
    }
  }

  /* Shrink via setSize */
  TEST_ASSERT(p.setSize(X2, Y2));
  TEST_ASSERT_EQ_U32(p.width(), X2);
  TEST_ASSERT_EQ_U32(p.height(), Y2);

  /* Verify preserved region within shrink bounds */
  for (uint16_t y = 0; y < CHECK_Y; y++) {
    for (uint16_t x = 0; x < CHECK_X; x++) {
      const float v = p.getValue(x, y);
      const float s = snap[((uint32_t) y * 16u) + x];
      TEST_ASSERT(v == s);
    }
  }

fail:
  if (0 == ret) { printf("\tPASS.\n"); }
  else { printf("\tFAIL.\n"); }
  return ret;
}




int test_plane_dirty_lock() {
  int ret = -1;
  return ret;
}




/*******************************************************************************
* Test plan
*******************************************************************************/

#define CHKLST_PLANE_TEST_REALLOCATE         0x00000004  // setSize()/reallocate()
#define CHKLST_PLANE_TEST_DIRTY_LOCK         0x00000200  // Support flags (locked excluded).
#define CHKLST_PLANE_TEST_MINMAX             0x08000000  //
#define CHKLST_PLANE_TEST_STDEV              0x10000000  //
#define CHKLST_PLANE_TEST_MEAN               0x20000000  //
#define CHKLST_PLANE_TEST_MEDIAN             0x40000000  //
#define CHKLST_PLANE_TEST_RMS                0x80000000  //


const StepSequenceList TOP_LEVEL_PLANE_TEST_LIST[] = {
  //
  { .FLAG         = CHKLST_PLANE_TEST_REALLOCATE,
    .LABEL        = "Re-allocation",
    .DEP_MASK     = (CHKLST_PLANE_TEST_ALLOCATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_plane_reallocation()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_PLANE_TEST_VALUE_MANIPULATION,
    .LABEL        = "Value manipulation",
    .DEP_MASK     = (CHKLST_PLANE_TEST_ALLOCATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_plane_value_access()) ? 1:-1);  }
  },

  //
  { .FLAG         = CHKLST_PLANE_TEST_DIRTY_LOCK,
    .LABEL        = "Buffer locking",
    .DEP_MASK     = (CHKLST_PLANE_TEST_VALUE_MANIPULATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_plane_dirty_lock()) ? 1:-1);  }
  },
};

AsyncSequencer plane_test_plan(TOP_LEVEL_PLANE_TEST_LIST, (sizeof(TOP_LEVEL_PLANE_TEST_LIST) / sizeof(TOP_LEVEL_PLANE_TEST_LIST[0])));


/*******************************************************************************
* The main function.
*******************************************************************************/


int c3p_numeric_plane_test_main() {
  const char* const MODULE_NAME = "C3PNumericPlane";
  printf("===< %s >=======================================\n", MODULE_NAME);

  return (plane_test_plan.request_fulfilled() ? 0 : 1);
}
