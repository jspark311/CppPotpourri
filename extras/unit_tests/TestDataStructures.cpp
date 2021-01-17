/*
File:   TestDataStructures.cpp
Author: J. Ian Lindsay
Date:   2016.09.20

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


This program runs tests against our raw data-handling classes.
*/

#include <cstdio>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>

#include <fstream>
#include <iostream>

#include "CppPotpourri.h"
#include "StringBuilder.h"
#include "RingBuffer.h"
#include "PriorityQueue.h"
#include "LightLinkedList.h"
#include "SensorFilter.h"
#include "Vector3.h"
#include "StopWatch.h"
#include "uuid.h"
#include "cbor-cpp/cbor.h"
#include "Image/Image.h"


/*******************************************************************************
* Globals
*******************************************************************************/

SensorFilter<float> test_filter_0(FilteringStrategy::RAW, 64, 0);
SensorFilter<float> test_filter_1(FilteringStrategy::RAW, 64, 0);
SensorFilter<float> test_filter_2(FilteringStrategy::RAW, 64, 0);
SensorFilter<float> test_filter_3(FilteringStrategy::RAW, 64, 0);

StopWatch stopwatch_0;
StopWatch stopwatch_1;
StopWatch stopwatch_2;
StopWatch stopwatch_3;

struct timeval start_micros;

uint32_t randomUInt32() {
  uint32_t ret = ((uint8_t) rand()) << 24;
  ret += ((uint8_t) rand()) << 16;
  ret += ((uint8_t) rand()) << 8;
  ret += ((uint8_t) rand());
  return ret;
}

int8_t random_fill(uint8_t* buf, uint len) {
  uint i = 0;
  while (i < len) {
    *(buf + i++) = ((uint8_t) rand());
  }
  return 0;
}

unsigned long micros() {
	uint32_t ret = 0;
	struct timeval current;
	gettimeofday(&current, nullptr);
	return (current.tv_usec - start_micros.tv_usec);
}

unsigned long millis() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000L);
}



/*******************************************************************************
* Type info
*******************************************************************************/

/**
* Prints the sizes of various types. Informational only. No test.
*/
void printTypeSizes(StringBuilder* output) {
  output->concat("===< Type sizes >=======================================\n-- Primitives:\n");
  output->concatf("\tvoid*                 %u\n", sizeof(void*));
  output->concatf("\tFloat                 %u\n", sizeof(float));
  output->concatf("\tDouble                %u\n", sizeof(double));
  output->concat("-- Elemental data structures:\n");
  output->concatf("\tStringBuilder         %u\n", sizeof(StringBuilder));
  output->concatf("\tVector3<float>        %u\n", sizeof(Vector3<float>));
  output->concatf("\tLinkedList<void*>     %u\n", sizeof(LinkedList<void*>));
  output->concatf("\tPriorityQueue<void*>  %u\n", sizeof(PriorityQueue<void*>));
  output->concatf("\tRingBuffer<void*>     %u\n", sizeof(RingBuffer<void*>));
  output->concatf("\tUUID                  %u\n", sizeof(UUID));
  output->concatf("\tStopWatch             %u\n", sizeof(StopWatch));
  output->concatf("\tSensorFilter<float>   %u\n\n", sizeof(SensorFilter<float>));
}


/*******************************************************************************
* Vector3 test routines
*******************************************************************************/

/**
 * [vector3_float_test description]
 * @param  log Buffer to receive test log.
 * @return   0 on success. Non-zero on failure.
 */
int vector3_float_test(StringBuilder* log) {
  float x = 0.7f;
  float y = 0.8f;
  float z = 0.01f;
  log->concat("===< Vector3<float> >===================================\n");
  Vector3<float> test;
  Vector3<float> test_vect_0(-0.4f, -0.1f, 0.4f);
  Vector3<float> *test1 = &test_vect_0;
  log->concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(1.0f, 0.5f, 0.24f);
  log->concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(test_vect_0.x, test_vect_0.y, test_vect_0.z);
  log->concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(test1->x, test1->y, test1->z);
  log->concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));

  test(x, y, z);
  log->concatf("\t (test) (%.4f, %.4f, %.4f)\n", (double)(test.x), (double)(test.y), (double)(test.z));
  printf("%s\n\n", (const char*) log->string());
  return 0;
}


/*******************************************************************************
* PriorityQueue test routines
*******************************************************************************/
// Tests for:
//   int insert(T);
//   T get(void);
//   T get(int position);
//   bool contains(T);
//   bool hasNext(void);
//   int clear(void);
int test_PriorityQueue0(StringBuilder* log) {
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
          log->concatf("Returned index from queue insertion didn't match the natural order. %d verus %d.\n", i, q_pos);
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
                  else log->concat("hasNext() reports true, when it ought to report false.\n");
                }
                else log->concat("The queue's size ought to be zero, but it isn't.\n");
              }
              else log->concatf("clear() ought to have cleared %d value. But it reports %d.\n", q_size, q_clear_val);
            }
            else log->concat("The queue's first element return didn't match the first element.\n");
          }
          else log->concat("Queue didn't contain all elements in their natural order.\n");
        }
        else log->concat("hasNext() reports false, when it ought to report true.\n");
      }
      else log->concatf("Queue didn't take all elements. Expected %u, but got %d.\n", sizeof(vals), q_size);
    }
    else log->concat("Queue claims to have a value it does not.\n");
  }
  else log->concat("Empty queue reports a non-zero size.\n");
  return return_value;
}


// Tests for:
//   int insertIfAbsent(T);
//   bool remove(T);
//   bool remove(int position);
//   int getPosition(T);
int test_PriorityQueue1(StringBuilder* log) {
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
                          else log->concat("A previously removed element was found.\n");
                        }
                        else log->concat("Known element is not at the position it is expected to be.\n");
                      }
                      else log->concat("The queue is not the expected size following removals.\n");
                    }
                    else log->concat("dequeue(): First element is wrong.\n");
                  }
                  else log->concat("get(): First element is wrong.\n");
                }
                else log->concat("Queue remove() returned failure when it ought not to have (named value).\n");
              }
              else log->concat("Queue remove() returned failure when it ought not to have (intermediary index).\n");
            }
            else log->concat("Queue remove() returned failure when it ought not to have (last index).\n");
          }
          else log->concat("Queue operation that ought not to have changed the size have done so anyhow.\n");
        }
        else log->concat("Queue remove() returned success when it ought not to have (negative index).\n");
      }
      else log->concat("Queue remove() returned success when it ought not to have (out-of-bounds index).\n");
    }
    else log->concatf("vals_rejected=%d, but should have been %d.\n", vals_rejected, sizeof(vals));
  }
  else log->concatf("Queue acceptance mismatch. q_size=%d   vals_accepted=%d   vals_rejected=%d\n", q_size, vals_accepted, vals_rejected);
  return return_value;
}


int test_PriorityQueue(void) {
  StringBuilder log("===< PriorityQueue >====================================\n");
  int return_value = -1;
  if (0 == test_PriorityQueue0(&log)) {
    if (0 == test_PriorityQueue1(&log)) {
      return_value = 0;
    }
  }

  //T dequeue(void);              // Removes the first element of the list. Return the node's data on success, or nullptr on empty queue.
  //T recycle(void);              // Reycle this element. Return the node's data on success, or nullptr on empty queue.

  //int insert(T, int priority);  // Returns the ID of the data, or -1 on failure. Makes only a reference to the payload.
  //int insertIfAbsent(T, int);   // Same as above, but also specifies the priority if successful.
  //int getPriority(T);           // Returns the priority in the queue for the given element.
  //int getPriority(int position);           // Returns the priority in the queue for the given element.
  //bool incrementPriority(T);    // Finds the given T and increments its priority by one.
  //bool decrementPriority(T);    // Finds the given T and decrements its priority by one.
  printf("%s\n\n", (const char*) log.string());
  return return_value;
}


int test_RingBuffer() {
  int return_value = -1;
  StringBuilder log("===< RingBuffer >=======================================\n");
  const int TEST_SIZE = 18;
  RingBuffer<uint32_t> a(TEST_SIZE);
  if (a.allocated()) {
    log.concatf("RingBuffer under test is using %u bytes of heap to hold %u elements.\n", a.heap_use(), a.capacity());
    if (0 == a.count()) {
      unsigned int test_num = TEST_SIZE/3;
      uint32_t val;
      log.concat("\tInserting:");
      for (unsigned int i = 0; i < test_num; i++) {
        val = randomUInt32();
        if (a.insert(val)) {
          log.concat("\nFailed to insert.\n");
          printf("%s\n\n", (const char*) log.string());
          return -1;
        }
        log.concatf(" (%u: %08x)", a.count(), val);
      }
      if (test_num == a.count()) {
        log.concat("\n\tGetting:  ");
        for (unsigned int i = 0; i < test_num/2; i++) {
          unsigned int count = a.count();
          val = a.get();
          log.concatf(" (%u: %08x)", count, val);
        }
        unsigned int n = TEST_SIZE - a.count();
        log.concatf("\n\tRingBuffer should have space for %u more elements... ", n);
        for (unsigned int i = 0; i < n; i++) {
          if (a.insert(randomUInt32())) {
            log.concatf("Falsified. Count is %u\n", a.count());
            printf("%s\n\n", (const char*) log.string());
            return -1;
          }
        }
        if (a.count() == TEST_SIZE) {
          log.concatf("Verified. Count is %u\n", a.count());
          log.concat("\tOverflowing... ");
          if (a.insert(randomUInt32())) {
            log.concatf("Is handled correctly. Count is %u\n", a.count());
            log.concat("\tDraining... ");
            for (unsigned int i = 0; i < TEST_SIZE; i++) {
              val = a.get();
            }
            if (0 == a.count()) {
              log.concat("done.\n\tTrying to drive count negative... ");
              if (0 == a.get()) {
                if (0 == a.count()) {
                  log.concat("pass.\n");
                  return_value = 0;
                }
                else log.concatf("Count should still be 0 but is %u\n", a.count());
              }
              else log.concatf("Get on an empty buffer should return 0.\n");
            }
            else log.concatf("Count should have been 0 but is %u\n", a.count());
          }
          else log.concatf("Sadly worked. Count is %u\n", a.count());
        }
        else log.concatf("Count mismatch. Got %u but was expecting %u.\n", a.count(), TEST_SIZE);
      }
      else log.concatf("Fairly certain we inserted %u elements, but the count says %u.\n", test_num, a.count());
    }
    else log.concatf("Newly created RingBuffers ought to be empty. This one reports %u.\n", a.count());
  }
  else log.concat("\nFailed to allocate.\n");

  printf("%s\n\n", (const char*) log.string());
  return return_value;
}


/**
* UUID battery.
* @return 0 on pass. Non-zero otherwise.
*/
int test_UUID() {
  StringBuilder log("===< UUID >=============================================\n");
  StringBuilder temp;
  UUID test0;
  UUID test1;

  // Do UUID's initialize to zero?
  for (int i = 0; i < 16; i++) {
    if (0 != *((uint8_t*) &test0.id[i])) {
      log.concat("UUID should be initialized to zeros. It was not. Failing...\n");
      printf("%s\n\n", (const char*) log.string());
      return -1;
    }
  }

  // Does the comparison function work?
  if (uuid_compare(&test0, &test1)) {
    log.concat("UUID function considers these distinct. Failing...\n");
    temp.concat((uint8_t*) &test0.id, sizeof(test0));
    temp.printDebug(&log);
    temp.clear();
    temp.concat((uint8_t*) &test1.id, sizeof(test1));
    temp.printDebug(&log);
    printf("%s\n\n", (const char*) log.string());
    return -1;
  }
  uuid_gen(&test0);
  // Does the comparison function work?
  if (0 == uuid_compare(&test0, &test1)) {
    log.concat("UUID function considers these the same. Failing...\n");
    temp.concat((uint8_t*) &test0.id, sizeof(test0));
    temp.printDebug(&log);
    temp.clear();
    temp.concat((uint8_t*) &test1.id, sizeof(test1));
    temp.printDebug(&log);
    printf("%s\n\n", (const char*) log.string());
    return -1;
  }

  // Generate a whole mess of UUID and ensure that they are different.
  for (int i = 0; i < 10; i++) {
    temp.concat((uint8_t*) &test0.id, sizeof(test0));
    log.concat("temp0 bytes:  ");
    temp.printDebug(&log);
    temp.clear();

    if (0 == uuid_compare(&test0, &test1)) {
      log.concat("UUID generator gave us a repeat UUID. Fail...\n");
      printf("%s\n\n", (const char*) log.string());
      return -1;
    }
    uuid_copy(&test0, &test1);
    if (0 != uuid_compare(&test0, &test1)) {
      log.concat("UUID copy appears to have failed...\n");
      temp.concat((uint8_t*) &test0.id, sizeof(test0));
      temp.printDebug(&log);
      temp.clear();
      temp.concat((uint8_t*) &test1.id, sizeof(test1));
      temp.printDebug(&log);
      printf("%s\n\n", (const char*) log.string());
      return -1;
    }
    uuid_gen(&test0);
  }

  char str_buffer[40] = "";
  uuid_to_str(&test0, str_buffer, 40);
  log.concatf("test0 string: %s\n", str_buffer);
  log.concat("uuid_to_sb(test0): ");
  uuid_to_sb(&test0, &log);
  log.concat("\n");

  uuid_from_str(str_buffer, &test1);
  log.concat("temp1 bytes:  ");
  temp.concat((uint8_t*) &test1.id, sizeof(test1));
  temp.printDebug(&log);

  // TODO: This is the end of the happy-path. Now we should abuse the program
  // by feeding it garbage and ensure that its behavior is defined.

  printf("%s\n\n", (const char*) log.string());
  return 0;
}


void printTestFailure(const char* test) {
  printf("\n");
  printf("*********************************************\n");
  printf("* %s FAILED tests.\n", test);
  printf("*********************************************\n");
}

/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int main(int argc, char *argv[]) {
  int exit_value = 1;   // Failure is the default result.
  StringBuilder out;
  srand(time(NULL));
  gettimeofday(&start_micros, nullptr);
  printTypeSizes(&out);

  if (0 == test_PriorityQueue()) {
    if (0 == vector3_float_test(&out)) {
      if (0 == test_UUID()) {
        if (0 == test_RingBuffer()) {
          printf("**********************************\n");
          printf("*  DataStructure tests all pass  *\n");
          printf("**********************************\n");
          exit_value = 0;
        }
        else printTestFailure("RingBuffer");
      }
      else printTestFailure("UUID");
    }
    else printTestFailure("Vector3");
  }
  else printTestFailure("PriorityQueue");

  exit(exit_value);
}
