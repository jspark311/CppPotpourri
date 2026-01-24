/*
File:   SmallDataStructureTests.cpp
Author: J. Ian Lindsay
Date:   2026.01.17

File:   LinkedListTests.cpp
Date:   2021.10.08

File:   RingBufferTests.cpp
Date:   2023.08.20

File:   C3PNumericPlaneTests.cpp
Date:   2026.01.10


This program tests several basic data structures in the library that are widely
  relied upon.

RingBuffer<T>
LinkedList<T>
PriorityQueue<T>
C3PStack<T>
C3PNumericPlane<T>
C3PStatBlock<T>
ElementPool<T>
*/

#include "C3PNumericPlane.h"
#include "C3PStack.h"
#include "C3PStatBlock.h"
#include "RingBuffer.h"
#include "PriorityQueue.h"
#include "LightLinkedList.h"


/*******************************************************************************
* Local test helpers
*******************************************************************************/

static inline float _rand_f32_range(float LO, float HI) {
  /* randomUInt32() is assumed to be provided by CppPotpourri runtime. */
  const uint32_t R = randomUInt32();
  const double   U = ((double) (R & 0xFFFFFF)) / (double) 0x1000000;  // [0,1)
  return (float) (LO + (float) (U * (double) (HI - LO)));
}

static inline uint16_t _rand_u16_range(uint16_t LO, uint16_t HI) {
  const uint32_t R = randomUInt32();
  const uint16_t SPAN = (uint16_t) (HI - LO);
  return (uint16_t) (LO + (uint16_t) (R % (SPAN + 1)));
}



/*******************************************************************************
* PriorityQueue test routines
* TODO:
*   T dequeue(void);              // Removes the first element of the list. Return the node's data on success, or nullptr on empty queue.
*   T recycle(void);              // Reycle this element. Return the node's data on success, or nullptr on empty queue.
*   int insert(T, int priority);  // Returns the ID of the data, or -1 on failure. Makes only a reference to the payload.
*   int insertIfAbsent(T, int);   // Same as above, but also specifies the priority if successful.
*   int getPriority(T);           // Returns the priority in the queue for the given element.
*   int getPriority(int position);           // Returns the priority in the queue for the given element.
*   bool incrementPriority(T);    // Finds the given T and increments its priority by one.
*   bool decrementPriority(T);    // Finds the given T and decrements its priority by one.
*******************************************************************************/
// Tests for:
//   int insert(T);
//   T get(void);
//   T get(int position);
//   bool contains(T);
//   bool hasNext(void);
//   int clear(void);
int test_PriorityQueue0() {
  printf("===< PriorityQueue >====================================\n");
  int return_value = -1;
  PriorityQueue<uint32_t*> queue0;
  uint32_t vals[16] = { randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32(),
                        randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32(),
                        randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32(),
                        randomUInt32(), randomUInt32(), randomUInt32(), randomUInt32() };
  int q_size = queue0.size();
  if (0 == q_size) {
    if (!queue0.contains(&vals[5])) {  // Futile search for non-existant value.
      // Populate the queue...
      for (int i = 0; i < (int) sizeof(vals); i++) {
        int q_pos = queue0.insert(&vals[i]);
        if (i != q_pos) {
          printf("Returned index from queue insertion didn't match the natural order. %d verus %d.\n", i, q_pos);
          return -1;
        }
      }
      q_size = queue0.size();
      if (sizeof(vals) == q_size) {
        if (queue0.hasNext()) {
          bool contains_all_elements = true;
          bool contains_all_elements_in_order = true;
          for (int i = 0; i < q_size; i++) {
            contains_all_elements &= queue0.contains(&vals[i]);
            contains_all_elements_in_order &= (*queue0.get(i) == vals[i]);
          }
          if (contains_all_elements & contains_all_elements_in_order) {
            if (*queue0.get(0) == vals[0]) {
              int q_clear_val = queue0.clear();
              if (q_size == q_clear_val) {
                if (0 == queue0.size()) {
                  if (!queue0.hasNext()) {
                    return_value = 0;
                  }
                  else printf("hasNext() reports true, when it ought to report false.\n");
                }
                else printf("The queue's size ought to be zero, but it isn't.\n");
              }
              else printf("clear() ought to have cleared %d value. But it reports %d.\n", q_size, q_clear_val);
            }
            else printf("The queue's first element return didn't match the first element.\n");
          }
          else printf("Queue didn't contain all elements in their natural order.\n");
        }
        else printf("hasNext() reports false, when it ought to report true.\n");
      }
      else printf("Queue didn't take all elements. Expected %u, but got %d.\n", sizeof(vals), q_size);
    }
    else printf("Queue claims to have a value it does not.\n");
  }
  else printf("Empty queue reports a non-zero size.\n");
  return return_value;
}


// Tests for:
//   int insertIfAbsent(T);
//   bool remove(T);
//   bool remove(int position);
//   int getPosition(T);
int test_PriorityQueue1() {
  int return_value = -1;
  PriorityQueue<uint32_t*> queue0;
  uint32_t vals[16] = { 234, 734, 733, 7456, 819, 943, 223, 936,
                        134, 634, 633, 6456, 719, 843, 123, 836 };
  int vals_accepted = 0;
  int vals_rejected = 0;
  for (int n = 0; n < 2; n++) {
    for (unsigned int i = 0; i < sizeof(vals); i++) {
      if (-1 != queue0.insertIfAbsent(&vals[i])) {
        vals_accepted++;
      }
      else {
        vals_rejected++;
      }
    }
  }
  int q_size = queue0.size();
  if (vals_accepted == q_size) {
    if (vals_rejected == sizeof(vals)) {
      // Try some removal...
      if (!queue0.remove((int) sizeof(vals))) {  // This ought to fail.
        if (!queue0.remove((int) -1)) {   // This is not a PHP array. Negative indicies are disallowed.
          if (vals_accepted == q_size) {  // Is the size unchanged?
            if (queue0.remove((int) vals_accepted - 1)) {  // Remove the last element.
              if (queue0.remove((int) 1)) {                // Remove element at position 1.
                if (queue0.remove(&vals[4])) {             // Remove the value 819.
                  if (234 == *queue0.get()) {              // Does not change queue.
                    if (234 == *queue0.dequeue()) {        // Removes the first element.
                      q_size = queue0.size();
                      if (q_size == (vals_accepted - 4)) { // Four removals have happened.
                        if (2 == queue0.getPosition(&vals[5])) {
                          if (-1 == queue0.getPosition(&vals[4])) {
                            return_value = 0;
                          }
                          else printf("A previously removed element was found.\n");
                        }
                        else printf("Known element is not at the position it is expected to be.\n");
                      }
                      else printf("The queue is not the expected size following removals.\n");
                    }
                    else printf("dequeue(): First element is wrong.\n");
                  }
                  else printf("get(): First element is wrong.\n");
                }
                else printf("Queue remove() returned failure when it ought not to have (named value).\n");
              }
              else printf("Queue remove() returned failure when it ought not to have (intermediary index).\n");
            }
            else printf("Queue remove() returned failure when it ought not to have (last index).\n");
          }
          else printf("Queue operation that ought not to have changed the size have done so anyhow.\n");
        }
        else printf("Queue remove() returned success when it ought not to have (negative index).\n");
      }
      else printf("Queue remove() returned success when it ought not to have (out-of-bounds index).\n");
    }
    else printf("vals_rejected=%d, but should have been %d.\n", vals_rejected, sizeof(vals));
  }
  else printf("Queue acceptance mismatch. q_size=%d   vals_accepted=%d   vals_rejected=%d\n", q_size, vals_accepted, vals_rejected);
  return return_value;
}



/*******************************************************************************
* LinkedList test routines
*******************************************************************************/

int test_LinkedList() {
  int return_value = -1;
  printf("===< LinkedList >=======================================\n");
  const int TEST_SIZE = 18;
  LinkedList<uint32_t*> a;
  uint32_t ref_vals[TEST_SIZE] = {0, };
  for (unsigned int i = 0; i < TEST_SIZE; i++) {
    ref_vals[i] = randomUInt32();
    if (0 > a.insert(&ref_vals[i])) {
      printf("\nFailed to insert.\n\n");
      return -1;
    }
    printf(" (%u: %08x)", a.size(), ref_vals[i]);
  }
  if (TEST_SIZE == a.size()) {
    printf("\n\tGetting:  ");
    for (unsigned int i = 0; i < TEST_SIZE/2; i++) {
      uint32_t* val = a.get(i);
      printf(" (%u: %08x)", i, *val);
      if (*val != ref_vals[i]) {
        printf("Value mismatch at index %u.\n\n", i);
        return -2;
      }
    }
    if (TEST_SIZE == a.size()) {
      printf("\n\tRemoving:  ");
      for (unsigned int i = 0; i < TEST_SIZE; i++) {
        uint32_t* val = a.remove();
        printf(" (%u: %08x)", i, *val);
        if (*val != ref_vals[i]) {
          printf("Value mismatch at index %u.\n\n", i);
          return -3;
        }
      }
      if (0 == a.size()) {
        if (nullptr == a.remove()) {
          a.insert(&ref_vals[0]);
          a.insert(&ref_vals[1]);
          if (nullptr == a.remove(15)) {
            if (nullptr == a.get(15)) {
              a.clear();
              if (0 == a.size()) {
                a.insert(&ref_vals[2]);
                return_value = 0;
              }
              else printf("Count should be 0, but is %u\n", a.size());
            }
            else printf("Sadly worked. Count is %u\n", a.size());
          }
          else printf("Sadly worked. Count is %u\n", a.size());
        }
        else printf("Sadly worked. Count is %u\n", a.size());
      }
      else printf("Count should have been 0 but is %u\n", a.size());
    }
    else printf("It appears get() removed elements. The count says %u.\n", a.size());
  }
  else printf("Fairly certain we inserted %u elements, but the count says %u.\n", TEST_SIZE, a.size());

  return return_value;
}


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



/*******************************************************************************
* C3PStack
*******************************************************************************/

int test_c3pstack() {
  int ret = -1;
  return ret;
}



/*******************************************************************************
* C3PNumericPlane Test routines
*******************************************************************************/

/*
* Construction can be done with or without an existing memory range.
* Dimensions must be non-zero.
*
* Covers:
*  - ctors
*  - width/height/valueCount/buffer/allocated/bytesUsed
*  - lazy allocation on READ (getValue)
*  - setBuffer()
*  - destructor behavior with external buffer (no double-free: best-effort)
*/
int test_plane_construction() {
  int ret = -1;
  printf("Testing C3PNumericPlane construction...\n");

  const uint16_t TEST_X_sz = (uint16_t) (37 + (randomUInt32() % 151));
  const uint16_t TEST_Y_sz = (uint16_t) (37 + (randomUInt32() % 151));
  const uint32_t TEST_VALUE_COUNT = ((uint32_t) TEST_X_sz * (uint32_t) TEST_Y_sz);
  const uint32_t TEST_BYTES = (uint32_t) (TEST_VALUE_COUNT * (uint32_t) sizeof(float));


  /* Default ctor */
  {
    printf("\tNo argument constructor produces an uninteresting object... \n");
    C3PNumericPlane<float> p0;
    printf("\t\twidth() and height() both return zero... ");
    if ((p0.width() == 0) && (p0.height() == 0)) {
      printf("Pass.\n\t\tvalueCount() returns zero... ");
      if (p0.valueCount() == 0) {
        printf("Pass.\n\t\tbytesUsed() returns zero... ");
        if (p0.bytesUsed() == 0) {
          printf("Pass.\n\t\tallocated() should refuse to allocate without geometry... ");
          if (!p0.allocated()) {
            printf("PASS.\n");
            ret = 0;
          }
        }
      }
    }
  }

  /* Size ctor (lazy) */
  if (0 == ret) {
    ret = -1;
    printf("\tCreating a test plane of float with size (%u x %u)...\n", TEST_X_sz, TEST_Y_sz);
    C3PNumericPlane<float> p0(TEST_X_sz, TEST_Y_sz);
    printf("\t\twidth() and height() return (%u x %u)... ", TEST_X_sz, TEST_Y_sz);
    if ((p0.width() == TEST_X_sz) && (p0.height() == TEST_Y_sz)) {
      printf("Pass.\n\t\tvalueCount() returns %u... ", TEST_VALUE_COUNT);
      if (p0.valueCount() == TEST_VALUE_COUNT) {
        printf("Pass.\n\t\tbytesUsed() returns 0 (having NOT previously allocated)... ");
        if (p0.bytesUsed() == 0) {
          printf("Pass.\n\t\tallocated() should return true... ");
          if (p0.allocated()) {
            printf("Pass.\n\t\tbytesUsed() returns %u (having allocated lazily)... ", TEST_BYTES);
            if (p0.bytesUsed() == TEST_BYTES) {
              printf("Pass.\n\t\tgetValue() returns 0.0f... ");
              if (0.0f == p0.getValue(0, 0)) {
                printf("PASS.\n");
                ret = 0;
              }
            }
          }
        }
      }
    }
  }

  /* External-buffer ctor (non-owning) */
  if (0 == ret) {
    ret = -1;
    printf("\tCreating a test plane of float with size (%u x %u) and an externally-managed buffer... ", TEST_X_sz, TEST_Y_sz);
    uint8_t* ext = (uint8_t*) malloc((size_t) TEST_BYTES);
    if (nullptr != ext) {
      memset(ext, 0, (size_t) TEST_BYTES);
      C3PNumericPlane<float> p0(TEST_X_sz, TEST_Y_sz, ext);
      printf("Pass.\n\t\twidth() and height() return (%u x %u)... ", TEST_X_sz, TEST_Y_sz);
      if ((p0.width() == TEST_X_sz) && (p0.height() == TEST_Y_sz)) {
        printf("Pass.\n\t\tvalueCount() returns %u... ", TEST_VALUE_COUNT);
        if (p0.valueCount() == TEST_VALUE_COUNT) {
          printf("Pass.\n\t\tbytesUsed() returns %u (having NOT allocated)... ", TEST_BYTES);
          if ((p0.allocated()) && (p0.bytesUsed() == TEST_BYTES) && (p0.buffer() == ext)) {
            printf("PASS.\n");
            ret = 0;
          }
        }
      }
    }
    /* If destructor incorrectly freed ext, this free() would likely crash. */
    free(ext);
  }


  /* setBuffer() on size-ctor plane (should attach and clear dirty) */
  if (0 == ret) {
    ret = -1;
    printf("\tCreating a test plane of float with size (%u x %u) and an externally-managed buffer by explicit post-constructor assignment... ", TEST_X_sz, TEST_Y_sz);
    uint8_t* ext = (uint8_t*) malloc((size_t) TEST_BYTES);
    if (nullptr != ext) {
      C3PNumericPlane<float> p0(TEST_X_sz, TEST_Y_sz);
      memset(ext, 0, (size_t) TEST_BYTES);
      printf("Pass.\t\twidth() and height() return (%u x %u)... ", TEST_X_sz, TEST_Y_sz);
      if ((p0.width() == TEST_X_sz) && (p0.height() == TEST_Y_sz)) {
        printf("Pass.\n\t\tvalueCount() returns %u... ", TEST_VALUE_COUNT);
        if (p0.valueCount() == TEST_VALUE_COUNT) {
          printf("Pass.\n\t\tsetBuffer() returns true... ");
          if (p0.setBuffer(ext)) {
            printf("Pass.\n\t\tbytesUsed() returns %u (having NOT allocated)... ", TEST_BYTES);
            if ((p0.bytesUsed() == TEST_BYTES) && (p0.allocated()) && (p0.buffer() == ext)) {
              printf("Pass.\n\t\tdirty() returns false... ");
              if (!p0.dirty()) {
                printf("PASS.\n");
                ret = 0;
              }
            }
          }
        }
      }
    }
    /* Again: if p0 freed ext in dtor, this free would crash. */
    free(ext);
  }

  if (0 != ret) { printf("FAIL.\n"); }
  return ret;
}


/*
* Covers:
*  - setBufferByCopy()
*  - verifies copied content independence from source memory
*  - dirty set on copy
*  - stats invalidation smoke test (mean changes after mutation)
*/
int test_plane_buffer_by_copy() {
  int ret = -1;
  const uint16_t X = _rand_u16_range(31, 97);
  const uint16_t Y = _rand_u16_range(31, 97);
  const uint32_t COUNT = ((uint32_t) X * (uint32_t) Y);
  const uint32_t BYTES = (uint32_t) (COUNT * (uint32_t) sizeof(float));
  float src[COUNT];
  printf("Testing C3PNumericPlane<float> setBufferByCopy()...\n");

  /* Fuzz fill source with floats. */
  float* src_f = (float*) src;
  for (uint32_t i = 0; i < COUNT; i++) {
    src_f[i] = _rand_f32_range(-10.0f, 10.0f);
  }

  C3PNumericPlane<float> p(X, Y);
  if (p.setBufferByCopy((uint8_t*) src)) {
    if (p.allocated()) {
      if (p.dirty()) {
        if (BYTES == p.bytesUsed()) {
          ///* Snapshot mean, mutate plane, ensure mean changes (stat invalidation path). */
          //const double m0 = p.mean();
          //if (p.setValue(0, 0, p.getValue(0, 0) + 1.0f)) {
          //  const double m1 = p.mean();
          //  if (m0 != m1) {
              ret = 0;
          //  }
          //}
          //if (0 != ret) {
          //  printf("\t mean() snapshot failed to differ after mutating plane.\n");
          //}
        }
        else printf("\t bytesUsed() is not the expected value. (%u != %u).\n", BYTES, p.bytesUsed());
      }
      else printf("\t Fresh allocation should be dirty.\n");
    }
    else printf("\t setBufferByCopy(src) ought to allocate in a fresh object. But allocated() returned false.\n");
  }
  else printf("\t setBufferByCopy(src) returned false.\n");

  if (0 == ret) {
    /* Confirm content equality immediately after copy. */
    printf("\t\tContent in buffers should match... ");
    for (uint16_t y = 0; y < Y; y++) {
      for (uint16_t x = 0; x < X; x++) {
        const uint32_t IDX = ((uint32_t) y * (uint32_t) X) + (uint32_t) x;
        const float A = p.getValue(x, y);
        const float B = src_f[IDX];
        if (!nearly_equal(A, B, 0.001)) {
          printf("\t getValue(%u, %u) doesn't match the source buffer, and it should.\n", X, Y);
          printf("%.6f, %.6f", A, B);
          ret--;
        }
      }
    }
    if (0 == ret) {
      printf("PASS.\n");
    }
  }

  if (0 == ret) {
    printf("\t\tContent should differ following source mutation... ");
    for (uint32_t i = 0; i < COUNT; i++) {
      src_f[i] = _rand_f32_range(-128.0f, 127.0f);
    }
    for (uint16_t y = 0; y < Y; y++) {
      for (uint16_t x = 0; x < X; x++) {
        const uint32_t IDX = ((uint32_t) y * (uint32_t) X) + (uint32_t) x;
        const float A = p.getValue(x, y);
        const float B = src_f[IDX];
        if (nearly_equal(A, B, 0.001)) {  // high probability; fuzz should make collisions rare
          ret--;
        }
      }
    }
    if (0 == ret) {
      printf("PASS.\n");
    }
  }

  if (0 != ret) { printf("FAIL.\n"); }
  return ret;
}



int test_plane_parse_pack() {
  int ret = -1;
  return ret;
}

int test_plane_value_access() {
  int ret = -1;
  return ret;
}



/*******************************************************************************
* C3PStatBlock
*******************************************************************************/
// Need to create a shim to expose a protected function.
template <class T> class C3PStatBlockTestShim : public C3PStatBlock<T> {
  public:
    C3PStatBlockTestShim(T* buf, const uint32_t N_VAL) {
      this->_set_stat_source_data(buf, N_VAL);
    };
};


/*
* Tests the statistical functions using a handful of KATs.
* This test needs to be phrased as a known-answer test to avoid comparison
*   against a "golden implementation" reproduced in this testing program.
*/
int test_c3pstatblock() {
  const uint32_t TEST_SAMPLE_COUNT   = 1500;
  const double   TEST_PRECISION      = 0.0002D;
  const int32_t  TEST_EPSILON_FACTOR = (int32_t) (TEST_PRECISION / std::numeric_limits<double>::epsilon());
  printf("Statistical KATs with a sample count of %u, and an epsilon factor of %.d required for success...\n", TEST_SAMPLE_COUNT, TEST_EPSILON_FACTOR);

  int ret = -1;
  float osc_val = 153.0;

  const double  EXPECTED_DBL_MIN  = 102.442193159035;
  const double  EXPECTED_DBL_MAX  = 153000;
  const double  EXPECTED_DBL_MEDN = 206.415273504598;
  const double  EXPECTED_DBL_MEAN = 804.898759643693;
  const double  EXPECTED_DBL_RMS  = 5065.69080921953;
  const double  EXPECTED_DBL_STDV = 5001.33595765524;
  const double  EXPECTED_DBL_SNR  = 0.025900637819809;

  const int32_t EXPECTED_INT_MIN  = 102;
  const int32_t EXPECTED_INT_MAX  = 153000;
  const int32_t EXPECTED_INT_MEDN = 206;
  const double  EXPECTED_INT_MEAN = 804.402;
  const double  EXPECTED_INT_RMS  = 5065.62458083897;
  const double  EXPECTED_INT_STDV = 5001.34879971353;
  const double  EXPECTED_INT_SNR  = 0.025868544627461;

  double  data_dbl[TEST_SAMPLE_COUNT] = {0.0D};
  int32_t data_int[TEST_SAMPLE_COUNT] = {0};

  // Generate the test curve, and fill the series...
  for (uint32_t i = 0; i < TEST_SAMPLE_COUNT; i++) {
    const double TEST_CURVE = (double) (((osc_val/(i+1)) + (sin(i/13.0D)/350.0D)) * 1000);
    data_dbl[i] = TEST_CURVE;
    data_int[i] = (int32_t) TEST_CURVE;
    //printf("%5d \t %.12f\n", (int32_t) TEST_CURVE, TEST_CURVE);
  }

  C3PStatBlockTestShim<double>  series_dbl(data_dbl, TEST_SAMPLE_COUNT);
  C3PStatBlockTestShim<int32_t> series_int(data_int, TEST_SAMPLE_COUNT);

  const double  RESULT_DBL_MIN  = series_dbl.minValue();
  const double  RESULT_DBL_MAX  = series_dbl.maxValue();
  const double  RESULT_DBL_MEAN = series_dbl.mean();
  const double  RESULT_DBL_MEDN = series_dbl.median();
  const double  RESULT_DBL_RMS  = series_dbl.rms();
  const double  RESULT_DBL_STDV = series_dbl.stdev();
  const double  RESULT_DBL_SNR  = series_dbl.snr();

  const int32_t RESULT_INT_MIN  = series_int.minValue();
  const int32_t RESULT_INT_MAX  = series_int.maxValue();
  const double  RESULT_INT_MEAN = series_int.mean();
  const int32_t RESULT_INT_MEDN = series_int.median();
  const double  RESULT_INT_RMS  = series_int.rms();
  const double  RESULT_INT_STDV = series_int.stdev();
  const double  RESULT_INT_SNR  = series_int.snr();

  printf("\tTesting with type DOUBLE...\n");
  printf("\t\tminValue() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_MIN);
  if (nearly_equal(EXPECTED_DBL_MIN, RESULT_DBL_MIN, TEST_PRECISION)) {
    printf("Pass.\n\t\tmaxValue() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_MAX);
    if (nearly_equal(EXPECTED_DBL_MAX, RESULT_DBL_MAX, TEST_PRECISION)) {
      printf("Pass.\n\t\tmean() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_MEAN);
      if (nearly_equal(EXPECTED_DBL_MEAN, RESULT_DBL_MEAN, TEST_PRECISION)) {
        printf("Pass.\n\t\tmedian() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_MEDN);
        if (nearly_equal(EXPECTED_DBL_MEDN, RESULT_DBL_MEDN, TEST_PRECISION)) {
          printf("Pass.\n\t\trms() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_RMS);
          if (nearly_equal(EXPECTED_DBL_RMS, RESULT_DBL_RMS, TEST_PRECISION)) {
            printf("Pass.\n\t\tstdev() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_STDV);
            if (nearly_equal(EXPECTED_DBL_STDV, RESULT_DBL_STDV, TEST_PRECISION)) {
              printf("Pass.\n\t\tsnr() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_DBL_SNR);
              if (nearly_equal(EXPECTED_DBL_SNR, RESULT_DBL_SNR, TEST_PRECISION)) {
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
    printf("PASS\n");
    printf("\tTesting with type INT32...\n");
    printf("\t\tminValue() matches within expected value (%d)... ", EXPECTED_INT_MIN);
    if (EXPECTED_INT_MIN == RESULT_INT_MIN) {
      printf("Pass.\n\t\tmaxValue() matches expected value (%d)... ", EXPECTED_INT_MAX);
      if (EXPECTED_INT_MAX == RESULT_INT_MAX) {
        printf("Pass.\n\t\tmean() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_INT_MEAN);
        if (nearly_equal(EXPECTED_INT_MEAN, RESULT_INT_MEAN, TEST_PRECISION)) {
          printf("Pass.\n\t\tmedian() matches expected value (%d)... ", EXPECTED_INT_MEDN);
          if (EXPECTED_INT_MEDN == RESULT_INT_MEDN) {
            printf("Pass.\n\t\trms() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_INT_RMS);
            if (nearly_equal(EXPECTED_INT_RMS, RESULT_INT_RMS, TEST_PRECISION)) {
              printf("Pass.\n\t\tstdev() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_INT_STDV);
              if (nearly_equal(EXPECTED_INT_STDV, RESULT_INT_STDV, TEST_PRECISION)) {
                printf("Pass.\n\t\tsnr() matches within +/-%.8f of expected value (%f)... ", TEST_PRECISION, EXPECTED_INT_SNR);
                if (nearly_equal(EXPECTED_INT_SNR, RESULT_INT_SNR, TEST_PRECISION)) {
                  ret = 0;
                }
              }
            }
          }
        }
      }
    }
  }

  printf("%s.\n", ((0 != ret) ? "Fail" : "PASS"));
  return ret;
}




/*******************************************************************************
* Test plan
*******************************************************************************/
// RingBuffer is a mem-efficient data structure with strict concurrency guards.
#define CHKLST_C3PDS_TEST_RINGBUFFER_GENERAL     0x00000001  //
#define CHKLST_C3PDS_TEST_RINGBUFFER_CONTAINS    0x00000002  //
#define CHKLST_C3PDS_TEST_RINGBUFFER_API_GENERAL 0x00000004  //

// LinkedList and Priority queue are sister templates with _almost_ matching
//   APIs and implementations. Both are heap-resident.
// One or the other of these classes is the library's go-to for orderd lists
//   of things.
#define CHKLST_C3PDS_TEST_LINKED_LIST_API_0      0x00000008  //
#define CHKLST_C3PDS_TEST_PRI_QUEUE_API_0        0x00000010  //
#define CHKLST_C3PDS_TEST_PRI_QUEUE_API_1        0x00000020  //

// NumericPlane is a template for handling a cartesian plane of number data.
#define CHKLST_C3PDS_TEST_PLANE_ALLOCATION       0x00000100  // Tests the constructors and allocation semantics.
#define CHKLST_C3PDS_TEST_PLANE_SET_BUF_BY_COPY  0x00000200  // setBufferByCopy()
#define CHKLST_C3PDS_TEST_PLANE_VALUE_API        0x00000400  // Can values be read and written?
#define CHKLST_C3PDS_TEST_PLANE_PARSE_PACK       0x00000800  // Parsing and packing.

// Many classes in C3P hold aggregates of numbers from which we often want to
//   collect statistical measurements.
#define CHKLST_C3PDS_TEST_STAT_CONTAINER         0x00001000  //

// Creating shared allocation pools of elements is fairly common.
#define CHKLST_C3PDS_TEST_ELEMENT_POOL           0x00010000  //

// It is less common to need a stack, but here is one anyhow.
#define CHKLST_C3PDS_TEST_STACK                  0x00100000  //


#define CHKLST_C3PDS_TESTS_ALL ( \
  CHKLST_C3PDS_TEST_RINGBUFFER_GENERAL | CHKLST_C3PDS_TEST_RINGBUFFER_CONTAINS | \
  CHKLST_C3PDS_TEST_RINGBUFFER_API_GENERAL | CHKLST_C3PDS_TEST_LINKED_LIST_API_0 | \
  CHKLST_C3PDS_TEST_PRI_QUEUE_API_0 | CHKLST_C3PDS_TEST_PRI_QUEUE_API_1 | \
  CHKLST_C3PDS_TEST_PLANE_ALLOCATION | CHKLST_C3PDS_TEST_PLANE_SET_BUF_BY_COPY | \
  CHKLST_C3PDS_TEST_STAT_CONTAINER)

  //CHKLST_C3PDS_TEST_PLANE_VALUE_API | CHKLST_C3PDS_TEST_PLANE_PARSE_PACK | \


const StepSequenceList TOP_LEVEL_C3PDS_TEST_LIST[] = {
  { .FLAG         = CHKLST_C3PDS_TEST_RINGBUFFER_GENERAL,
    .LABEL        = "RingBuffer<T> general",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_RingBuffer_general()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PDS_TEST_RINGBUFFER_CONTAINS,
    .LABEL        = "RingBuffer<T> contains(T), insert(T)",
    .DEP_MASK     = (CHKLST_C3PDS_TEST_RINGBUFFER_GENERAL),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_RingBuffer_contains()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PDS_TEST_RINGBUFFER_API_GENERAL,
    .LABEL        = "RingBuffer<T>: general API",
    .DEP_MASK     = (CHKLST_C3PDS_TEST_RINGBUFFER_CONTAINS),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_RingBuffer_multiple_element_api()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PDS_TEST_LINKED_LIST_API_0,
    .LABEL        = "tLinkedList<T>: general API",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_LinkedList()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PDS_TEST_PRI_QUEUE_API_0,
    .LABEL        = "PriorityQueue<T>: API-0",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_PriorityQueue0()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PDS_TEST_PRI_QUEUE_API_1,
    .LABEL        = "PriorityQueue<T>: API-1",
    .DEP_MASK     = (CHKLST_C3PDS_TEST_PRI_QUEUE_API_0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_PriorityQueue1()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PDS_TEST_STACK,
    .LABEL        = "C3PStack<t>: General API",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_c3pstack()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PDS_TEST_PLANE_ALLOCATION,
    .LABEL        = "C3PNumericPlane<T>: Construction and allocation",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_plane_construction()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PDS_TEST_PLANE_SET_BUF_BY_COPY,
    .LABEL        = "C3PNumericPlane<T>: setBufferByCopy()",
    .DEP_MASK     = (CHKLST_C3PDS_TEST_PLANE_ALLOCATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_plane_buffer_by_copy()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PDS_TEST_PLANE_VALUE_API,
    .LABEL        = "Value manipulation",
    .DEP_MASK     = (CHKLST_C3PDS_TEST_PLANE_ALLOCATION),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_plane_value_access()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_C3PDS_TEST_PLANE_PARSE_PACK,
    .LABEL        = "Value manipulation",
    .DEP_MASK     = (CHKLST_C3PDS_TEST_PLANE_VALUE_API),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_plane_parse_pack()) ? 1:-1);  }
  },

  { .FLAG         = CHKLST_C3PDS_TEST_STAT_CONTAINER,
    .LABEL        = "C3PStatBlock<T>: General API",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_c3pstatblock()) ? 1:-1);  }
  },
};

AsyncSequencer c3pds_test_plan(TOP_LEVEL_C3PDS_TEST_LIST, (sizeof(TOP_LEVEL_C3PDS_TEST_LIST) / sizeof(TOP_LEVEL_C3PDS_TEST_LIST[0])));


/*******************************************************************************
* The main function.
*******************************************************************************/

void print_types_small_ds() {
  printf("\tRingBuffer<uint8_t>      %u\t%u\n", sizeof(RingBuffer<uint8_t>),     alignof(RingBuffer<uint8_t>));
  printf("\tRingBuffer<uint32_t>     %u\t%u\n", sizeof(RingBuffer<uint32_t>),    alignof(RingBuffer<uint32_t>));
  printf("\tRingBuffer<void*>        %u\t%u\n", sizeof(RingBuffer<void*>),       alignof(RingBuffer<void*>));
  printf("\tLinkedList<uint8_t>      %u\t%u\n", sizeof(LinkedList<uint8_t>),     alignof(LinkedList<uint8_t>));
  printf("\tLinkedList<void*>        %u\t%u\n", sizeof(LinkedList<void*>),       alignof(LinkedList<void*>));
  printf("\tPriorityQueue<uint8_t>   %u\t%u\n", sizeof(PriorityQueue<uint8_t>),  alignof(PriorityQueue<uint8_t>));
  printf("\tPriorityQueue<void*>     %u\t%u\n", sizeof(PriorityQueue<void*>),    alignof(PriorityQueue<void*>));
  printf("\tC3PStack<float>          %u\t%u\n", sizeof(C3PStack<float>),         alignof(C3PStack<float>));
  printf("\tC3PNumericPlane<float>   %u\t%u\n", sizeof(C3PNumericPlane<float>),  alignof(C3PNumericPlane<float>));
}


int c3p_small_ds_test_main() {
  const char* const MODULE_NAME = "C3P Templated Datastructs";
  printf("===< %s >=======================================\n", MODULE_NAME);

  c3pds_test_plan.requestSteps(CHKLST_C3PDS_TESTS_ALL);
  while (!c3pds_test_plan.request_completed() && (0 == c3pds_test_plan.failed_steps(false))) {
    c3pds_test_plan.poll();
  }
  int ret = (c3pds_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  c3pds_test_plan.printDebug(&report_output, "C3P Small Datastructs test report");
  printf("%s\n", (char*) report_output.string());

  return ret;
}
