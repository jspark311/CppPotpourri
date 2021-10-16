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

#define TEST_BUFFER_SIZE  16


/*******************************************************************************
* Globals
*******************************************************************************/

StopWatch stopwatch_0;
StopWatch stopwatch_1;
StopWatch stopwatch_2;
StopWatch stopwatch_3;
StopWatch stopwatch_4;
StopWatch stopwatch_99;


/*******************************************************************************
* Vector3 test routines
*******************************************************************************/

/**
 * [vector3_float_test description]
 * @param  log Buffer to receive test log.
 * @return   0 on success. Non-zero on failure.
 */
int vector3_float_test() {
  int return_value = -1;
  bool print_vectors = true;
  Vector3<float>* print0 = nullptr;
  Vector3<float>* print1 = nullptr;
  Vector3<float>* print2 = nullptr;

  Vector3<float> x_axis(1, 0, 0);
  Vector3<float> y_axis(0, 1, 0);
  Vector3<float> z_axis(0, 0, 1);

  StringBuilder log("===< Vector3<float> >===================================\n");
  float x0 = randomUInt32()/(float)randomUInt32();
  float y0 = randomUInt32()/(float)randomUInt32();
  float z0 = randomUInt32()/(float)randomUInt32();
  float x1 = randomUInt32()/(float)randomUInt32();
  float y1 = randomUInt32()/(float)randomUInt32();
  float z1 = randomUInt32()/(float)randomUInt32();
  float x2 = randomUInt32()/(float)randomUInt32();
  float y2 = randomUInt32()/(float)randomUInt32();
  float z2 = randomUInt32()/(float)randomUInt32();
  float x3 = randomUInt32()/(float)randomUInt32();
  float y3 = randomUInt32()/(float)randomUInt32();
  float z3 = randomUInt32()/(float)randomUInt32();
  float xR0 = x0 + x1;
  float yR0 = y0 + y1;
  float zR0 = z0 + z1;
  float xR1 = x0 * 5;
  float yR1 = y0 * 5;
  float zR1 = z0 * 5;
  float xR2 = x0 - x1;
  float yR2 = y0 - y1;
  float zR2 = z0 - z1;
  Vector3<float> test;
  Vector3<float> test_vect_0(x0, y0, z0);
  Vector3<float> test_vect_1(x1, y1, z1);
  Vector3<float> test_vect_2(x2, y2, z2);
  Vector3<float> test_vect_4(&test_vect_2);
  Vector3<float> test_vect_3(x3, y3, z3);
  Vector3<float> test_vect_5;
  test_vect_5.set(x0, y0, z0);

  Vector3<float> result_vect_0 = test_vect_0 + test_vect_1;
  Vector3<float> result_vect_1 = test_vect_0 * 5;
  Vector3<float> result_vect_2 = test_vect_0 - test_vect_1;
  Vector3<float> result_vect_3(x3, y3, z3);
  Vector3<float> result_vect_4 = -test_vect_1;
  Vector3<float> result_vect_5;
  result_vect_5.set(&test_vect_0);
  result_vect_5 += test_vect_1;

  float length_r_0 = test_vect_0.length();
  float length_r_1 = test_vect_1.length();
  float length_r_2 = test_vect_1.length_squared();

  print0 = &test_vect_0;
  print1 = &test_vect_1;
  print2 = &result_vect_0;

  if ((result_vect_0.x == xR0) && (result_vect_0.y == yR0) && (result_vect_0.z == zR0)) {
    print2 = &result_vect_5;
    if ((result_vect_5.x == xR0) && (result_vect_5.y == yR0) && (result_vect_5.z == zR0)) {
      print2 = &result_vect_1;
      if ((result_vect_1.x == xR1) && (result_vect_1.y == yR1) && (result_vect_1.z == zR1)) {
        print2 = &result_vect_2;
        if ((result_vect_2.x == xR2) && (result_vect_2.y == yR2) && (result_vect_2.z == zR2)) {
          if (round(1000 * length_r_1) == round(1000 * sqrt(length_r_2))) {
            print0 = &result_vect_0;
            print1 = &result_vect_4;
            print2 = &test_vect_0;
            result_vect_0 += result_vect_4;
            result_vect_0 -= test_vect_0;
            if (0 == round(1000 * result_vect_0.length())) {
              float length_r_3 = test_vect_3.length();
              float scalar_0   = test_vect_3.normalize();
              if (1000 == round(1000 * test_vect_3.length())) {
                if (1000 == round(1000 * scalar_0*length_r_3)) {
                  if (result_vect_3 != test_vect_3) {
                    result_vect_3.normalize();   // Independently normalized vector.
                    if (result_vect_3 == test_vect_3) {
                      print0 = &result_vect_3;
                      print1 = &test_vect_3;
                      float angle_0 = Vector3<float>::angle_normalized(result_vect_3, test_vect_3);
                      if (0.0 == round(100 * angle_0)) {
                        test_vect_2.reflect(x_axis);
                        float angle_1 = Vector3<float>::angle(test_vect_2, x_axis);
                        float angle_2 = Vector3<float>::angle(test_vect_2, test_vect_4);
                        if (round(100 * angle_1*2) == round(100 * angle_2)) {
                          const float RENORM_SCALAR = 6.5f;
                          result_vect_3 *= RENORM_SCALAR;   // Stretch.
                          if ((RENORM_SCALAR*1000) == round(1000 * result_vect_3.length())) {
                            // Normalize to a given length.
                            result_vect_3.normalize(result_vect_3.length());
                            if (100 == round(100 * result_vect_3.length())) {
                              Vector3<float> cross_product = test_vect_0 % test_vect_1;
                              float angle_3 = Vector3<float>::angle(cross_product, test_vect_0);
                              float angle_4 = Vector3<float>::angle(cross_product, test_vect_1);
                              if ((round(100 * (PI/2)) == round(100 * angle_4)) && (round(100 * angle_3) == round(100 * angle_4))) {
                                log.concat("Vector3 tests pass.\n");
                                print_vectors = false;
                                return_value = 0;
                              }
                              else log.concatf("The cross-product of two vectors was not orthogonal to both. %.3f and %.3f.\n", (double) angle_3, (double) angle_4);
                            }
                            else log.concat("Failed vector Scaling/renormalizing.\n");
                          }
                          else log.concatf("Scaled vector should be length %.3f, but got %.3f.\n", (double) RENORM_SCALAR, (double) result_vect_3.length());
                        }
                        else log.concatf("The angle between vector0 and its reflection about vector1 should be twice the angle between vector0 nad vector1, but got %.3f and %.3f, respectively.\n", (double) angle_1, (double) angle_2);
                      }
                      else log.concatf("The angle between two equal vectors should be 0.0, but got %.6f.\n", (double) angle_0);
                    }
                    else log.concat("Failed vector equality test.\n");
                  }
                  else log.concat("Failed vector inequality test.\n");
                }
                else log.concatf("The scalar value returned by normalize (%.3f) doesn't comport with the original length (%.3f).\n", (double) scalar_0, (double) length_r_3);
              }
              else log.concatf("Normalized vector should be length 1.0, but got %.3f.\n", (double) test_vect_3.length());
            }
            else log.concat("Failed test of -= operator.\n");
          }
          else log.concat("Failed len^2.\n");
        }
        else log.concat("Failed vector subtraction.\n");
      }
      else log.concat("Failed vector multiplication.\n");
    }
    else log.concat("Failed test of += operator.\n");
  }
  else log.concat("Failed vector addition.\n");

  if (print_vectors) {
    log.concatf("\top0:    (%.4f, %.4f, %.4f)\n", (double)(print0->x), (double)(print0->y), (double)(print0->z));
    log.concatf("\top1:    (%.4f, %.4f, %.4f)\n", (double)(print1->x), (double)(print1->y), (double)(print1->z));
    log.concatf("\tresult: (%.4f, %.4f, %.4f)\n", (double)(print2->x), (double)(print2->y), (double)(print2->z));
  }

  printf("%s\n\n", (const char*) log.string());
  return return_value;
}


/*******************************************************************************
* RingBuffer test routines
*******************************************************************************/

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


/*******************************************************************************
* UUID test routines
*******************************************************************************/

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


void printStopWatches() {
  StringBuilder out;
  StopWatch::printDebugHeader(&out);
  stopwatch_0.printDebug("LinkedList", &out);
  stopwatch_1.printDebug("Vector", &out);
  stopwatch_2.printDebug("KVP", &out);
  stopwatch_3.printDebug("UUID", &out);
  stopwatch_4.printDebug("RingBuffer", &out);
  stopwatch_99.printDebug("UNUSED", &out);
  printf("%s\n\n", (const char*) out.string());
}


/*******************************************************************************
* Something terrible.
*******************************************************************************/

#include "KVPTests.cpp"
#include "LinkedListTests.cpp"



/****************************************************************************************************
* The main function.                                                                                *
****************************************************************************************************/
int data_structure_main() {
  int ret = 1;   // Failure is the default result.

  stopwatch_0.markStart();
  if (0 == test_LinkedList()) {
    if (0 == test_PriorityQueue()) {
      stopwatch_0.markStop();
      stopwatch_1.markStart();
      if (0 == vector3_float_test()) {
        stopwatch_1.markStop();
        stopwatch_2.markStart();
        if (0 == test_KeyValuePair()) {
          stopwatch_2.markStop();
          stopwatch_3.markStart();
          if (0 == test_UUID()) {
            stopwatch_3.markStop();
            stopwatch_4.markStart();
            if (0 == test_RingBuffer()) {
              stopwatch_4.markStop();
              printf("**********************************\n");
              printf("*  DataStructure tests all pass  *\n");
              printf("**********************************\n");
              ret = 0;
            }
            else printTestFailure("RingBuffer");
          }
          else printTestFailure("UUID");
        }
        else printTestFailure("KeyValuePair");
      }
      else printTestFailure("Vector3");
    }
    else printTestFailure("PriorityQueue");
  }
  else printTestFailure("LinkedList");

  printStopWatches();
  return ret;
}
