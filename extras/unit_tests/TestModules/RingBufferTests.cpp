/*
File:   RingBufferTests.cpp
Author: J. Ian Lindsay
Date:   2023.08.20

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


This program runs tests against the RingBuffer template.
*/


/*******************************************************************************
* RingBuffer test routines
*******************************************************************************/


/*
* Tests:
* int vacancy()
* int isEmpty()
* int insert(T*, unsigned int)
* int peek(T*, unsigned int)
* int cull(unsigned int)
* int get(T*, unsigned int)
*/
int test_RingBuffer_multiple_element_api() {
  int ret = -1;
  printf("Testing insert(T*, unsigned int)...\n");
  const uint32_t TEST_SIZE   = (67 + (randomUInt32() % 53));
  const uint32_t JUNK_SIZE   = (TEST_SIZE << 1);
  const uint32_t PEEK_SIZE   = ((TEST_SIZE >> 1) - (randomUInt32() % 12));
  const uint32_t GET_SIZE    = (TEST_SIZE - PEEK_SIZE);
  RingBuffer<int16_t> a(TEST_SIZE);
  int16_t junk_field[JUNK_SIZE];
  int16_t result_field[TEST_SIZE];
  for (uint32_t i = 0; i < JUNK_SIZE; i++) {  junk_field[i]   = (int16_t) randomUInt32();  }
  for (uint32_t i = 0; i < TEST_SIZE; i++) {  result_field[i] = 0;  }

  printf("\tvacancy() and capacity() should return the same number for an empty buffer... ");
  if (a.capacity() == a.vacancy()) {
    printf("Pass.\n\tinsert(T*, unsigned int) takes all elements offered... ");
    const int MORE_THAN_HALF        = (TEST_SIZE >> 1) + 1;
    const int EXPECTED_PARTIAL_TAKE = (TEST_SIZE - MORE_THAN_HALF);
    // Generate a field of junk twice the size that we need and try to
    //   bulk-add more than half of it...
    int first_take_count = a.insert(junk_field, MORE_THAN_HALF);
    if (MORE_THAN_HALF == first_take_count) {
      // Try to overfill...
      printf("Pass.\n\tinsert(T*, unsigned int) handles overfill attemps correctly... ");
      int second_take_count = a.insert(&junk_field[first_take_count], MORE_THAN_HALF);
      if (EXPECTED_PARTIAL_TAKE == second_take_count) {
        printf("Pass.\n\tvacancy() should now read zero, and the take counts should equal capacity()... ");
        if (((second_take_count + first_take_count) == (int) a.capacity()) & (0 == a.vacancy())) {
          // Check for order and continuity...
          printf("Pass.\n\tIndependent content record matches content... ");
          for (uint32_t i = 0; i < a.capacity(); i++) {
            if (a.get() != junk_field[i]) {
              printf("Failed: Resulting buffer doesn't match what was fed to it at index %u.\n", i);
              return -1;
            }
          }
          printf("Pass.\n\tThe ring is once again empty... ");
          if (a.isEmpty()) {
            printf("Pass.\n\tpeek(%u) fails on an empty ring by returning 0... ", PEEK_SIZE);
            if (0 == a.peek(result_field, PEEK_SIZE)) {
              printf("Pass.\n\tcull(%u) fails on an empty ring by returning 0... ", PEEK_SIZE);
              if (0 == a.cull(PEEK_SIZE)) {
                printf("Pass.\n\tget(%u) fails on an empty ring by returning 0... ", GET_SIZE);
                if (0 == a.get(result_field, PEEK_SIZE)) {
                  printf("Pass.\n\tRe-filling the ring in a single call for the next test... ");
                  if (TEST_SIZE == a.insert(junk_field, TEST_SIZE)) {
                    printf("Pass.\n\tpeek(0) fails on an full ring by returning -1... ");
                    if (-1 == a.peek(result_field, 0)) {
                      printf("Pass.\n\tcull(0) fails on an full ring by returning -1... ");
                      if (-1 == a.cull(0)) {
                        printf("Pass.\n\tget(0) fails on an full ring by returning -1... ");
                        if (-1 == a.get(result_field, 0)) {
                          // The rest of this test wil be trying to re-assemble the
                          //   junk_field in result_field with only the multi-element API.
                          printf("PASS.\n");
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
  }

  if (0 == ret) {
    ret = -1;
    printf("\tpeek(%u) succeeds by returning its count argument... ", PEEK_SIZE);
    if (PEEK_SIZE == a.peek(result_field, PEEK_SIZE)) {
      printf("Pass.\n\tThe ring didn't change... ");
      if (0 == a.vacancy()) {
        printf("Pass.\n\tcull(%u) succeeds by returning its count argument... ", PEEK_SIZE);
        if (PEEK_SIZE == a.cull(PEEK_SIZE)) {
          printf("Pass.\n\tThe ring now has the expected amount of vacancy()... ");
          if (PEEK_SIZE == a.vacancy()) {
            printf("Pass.\n\tget(%u) succeeds by returning its count argument... ", GET_SIZE);
            if (GET_SIZE == a.get(&result_field[PEEK_SIZE], GET_SIZE)) {
              printf("Checking results...\n");
              for (uint32_t i = 0; i < TEST_SIZE; i++) {
                if (result_field[i] != junk_field[i]) {
                  printf("Failed: result_field[%u] != junk_field[%u]: %u / %u.\n", i, i, result_field[i], junk_field[i]);
                  return -1;
                }
              }
              printf("PASS.\n");
              ret = 0;
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
* Tests:
* bool contains();
* int insert(T)
*/
int test_RingBuffer_contains() {
  int return_value = -1;
  const int TEST_SIZE = 9;
  RingBuffer<uint32_t> a(TEST_SIZE);
  // RingBuffer is allocate on-demand. At this point, no heap activity has taken place.
  // contains(anything) should return false and not crash. We deliberatly choose
  //   zero (the reset value) to ensure this.
  if (!a.contains(0)) {
    uint32_t val = randomUInt32();
    if (0 == a.insert(val)) {
      if (a.contains(val)) {
        if (!a.contains(0)) {
          if (0 == a.insert(0)) {
            if (a.contains(0)) {
              printf("\tInserted test values 0 and %08x. Count is at %u.\n\tFilling:", val, a.count());
              bool keep_filling = true;
              while (keep_filling) {
                // Fill the buffer with anything but zero or our initial test
                //   value. We want to make sure they don't get lost when the
                //   buffer is driven to capacity.
                uint32_t filtered_val = randomUInt32();
                if ((0 != val) & (filtered_val != val)) {
                  keep_filling = (0 == a.insert(filtered_val));
                  printf(" %08x", filtered_val);
                  if (!keep_filling) {
                    printf(" <terminated fill at count = %u>\n", a.count());
                  }
                }
                else {
                  printf("Rejecting repeated value (%08x).\n", filtered_val);
                  keep_filling = false;
                }
              }
              if (a.contains(val)) {
                if (a.contains(0)) {
                  a.clear();
                  if (!a.contains(val)) {
                    if (!a.contains(0)) {
                      return_value = 0;
                    }
                    else printf("Failed: contains(0) ought to have returned false after clear, but did not.\n");
                  }
                  else printf("Failed: contains(%08x) ought to have returned false after clear, but did not.\n", val);
                }
                else printf("Failed: contains(0) ought to have returned true after fill, but did not.\n");
              }
              else printf("Failed: contains(%08x) ought to have returned true after fill, but did not.\n", val);
            }
            else printf("Failed: contains(0) finally ought to have returned true, but did not.\n");
          }
          else printf("Failed: contains(0) STILL ought to have returned false, but did not.\n");
        }
        else printf("Failed: contains(0) STILL ought to have returned false, but did not.\n");
      }
      else printf("Failed: contains(%08x) ought to have returned true, but did not.\n", val);
    }
    else printf("Failed to insert(%08x).\n", val);
  }
  else printf("Failed: contains(0) ought to have returned false, but did not.\n");

  return return_value;
}


int test_RingBuffer_general() {
  int return_value = -1;
  const int TEST_SIZE = 18;
  RingBuffer<uint32_t> a(TEST_SIZE);
  if (a.allocated()) {
    printf("RingBuffer under test is using %u bytes of heap to hold %u elements.\n", a.heap_use(), a.capacity());
    if (0 == a.count()) {
      unsigned int test_num = TEST_SIZE/3;
      uint32_t val;
      printf("\tInserting:");
      for (unsigned int i = 0; i < test_num; i++) {
        val = randomUInt32();
        if (a.insert(val)) {
          printf("\nFailed to insert.\n");
          return -1;
        }
        printf(" (%u: %08x)", a.count(), val);
      }
      if (test_num == a.count()) {
        printf("\n\tGetting:  ");
        for (unsigned int i = 0; i < test_num/2; i++) {
          unsigned int count = a.count();
          val = a.get();
          printf(" (%u: %08x)", count, val);
        }
        unsigned int n = TEST_SIZE - a.count();
        printf("\n\tRingBuffer should have space for %u more elements... ", n);
        for (unsigned int i = 0; i < n; i++) {
          if (a.insert(randomUInt32())) {
            printf("Falsified. Count is %u\n", a.count());
            return -1;
          }
        }
        if (a.count() == TEST_SIZE) {
          printf("Verified. Count is %u\n", a.count());
          printf("\tOverflowing... ");
          if (a.insert(randomUInt32())) {
            printf("Is handled correctly. Count is %u\n", a.count());
            printf("\tDraining... ");
            for (unsigned int i = 0; i < TEST_SIZE; i++) {
              val = a.get();
            }
            if (0 == a.count()) {
              printf("done.\n\tTrying to drive count negative... ");
              if (0 == a.get()) {
                if (0 == a.count()) {
                  printf("done.\n\tEnsuring that OOB get() returns the trivial value... ");
                  uint32_t should_be_zero = a.peek(a.capacity() + 10);
                  if (0 == should_be_zero) {
                    printf("it does.\n");
                    return_value = 0;
                  }
                  else printf("Fail. Returned %08x instead.\n", should_be_zero);
                }
                else printf("Count should still be 0 but is %u\n", a.count());
              }
              else printf("Get on an empty buffer should return 0.\n");
            }
            else printf("Count should have been 0 but is %u\n", a.count());
          }
          else printf("Sadly worked. Count is %u\n", a.count());
        }
        else printf("Count mismatch. Got %u but was expecting %u.\n", a.count(), TEST_SIZE);
      }
      else printf("Fairly certain we inserted %u elements, but the count says %u.\n", test_num, a.count());
    }
    else printf("Newly created RingBuffers ought to be empty. This one reports %u.\n", a.count());
  }
  else printf("Failed to allocate.\n");

  return return_value;
}


void print_types_ringbuffer() {
  printf("\tRingBuffer<uint8_t>   %u\t%u\n", sizeof(RingBuffer<uint8_t>),  alignof(RingBuffer<uint8_t>));
  printf("\tRingBuffer<uint32_t>  %u\t%u\n", sizeof(RingBuffer<uint32_t>), alignof(RingBuffer<uint32_t>));
  printf("\tRingBuffer<void*>     %u\t%u\n", sizeof(RingBuffer<void*>),    alignof(RingBuffer<void*>));
}


/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int ringbuffer_main() {
  int ret = 1;   // Failure is the default result.
  const char* const MODULE_NAME = "RingBuffer";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == test_RingBuffer_general()) {
    if (0 == test_RingBuffer_contains()) {
      if (0 == test_RingBuffer_multiple_element_api()) {
        ret = 0;
      }
    }
  }

  return ret;
}
