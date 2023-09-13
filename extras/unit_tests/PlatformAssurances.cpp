/*
File:   PlatformAssurances.cpp
Author: J. Ian Lindsay
Date:   2023.09.13


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


This program tests the implementation of the platform as it is used to execute
  all of our unit tests. This is the proper place for verifying the operation
  of things like pinRead()/pinSet(), RNG, millis()/micros(), as well as any
  dependency-injection strategies that are employed therein.

In the future, we may also run tests on the platform contract itself, or
  implementation of same that is provided by C3P itself.

TODO: John_00_Fleming_-_JOOF_Radio_045
*/

/*******************************************************************************
* Platform assurance testing
*******************************************************************************/

/*
* Testing RNG implementations is a whole deep topic in itself. But for the sake
*   of this test program, we only care that the values are sufficiently
*   different between calls to generate unique test cases.
* We aren't doing any serious cryptography in this program, nor is this a test
*   of CryptoBurrito. CryptoBurrito will do its own RNG testing to a standard
*   appropriate to its purpose.
*/
int platform_rng_tests() {
  int ret = -1;
  // TODO: This
  return ret;
}


/*
* C3P system time is given by millis() and micros(). Those functions should
*   have a direct mutual relationship. That is, (millis() / 1000) should always
*   be equal to the return from micros(), wrap-cases exempted.
* Also, the two counters should evolve at the same rate.
*/
int platform_system_time_tests() {
  int ret = -1;
  // TODO: This
  return ret;
}


/*
* C3P has an optional abstract thread model. This test ensures that it actually
*   works in the test program.
*/
int platform_threading_tests() {
  int ret = -1;
  // TODO: This
  return ret;
}


/*
* This test program should have provided the implementations of all of the GPIO
*   functions demanded by AbstractPlatform.h. In doing so, it also supplied a
*   set of emulated pins, some of which are permanently cross-connected for the
*   sake of testing.
*/
int platform_gpio_tests() {
  int ret = -1;
  // TODO: This
  return ret;
}


void print_types_platform() {
  printf("\tAbstractPlatform         %u\t%u\n", sizeof(AbstractPlatform),     alignof(AbstractPlatform));
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int platform_assurance_test_main() {
  int ret = -1;   // Failure is the default result.
  const char* const MODULE_NAME = "AbstractPlatform";
  printf("===< %s >=======================================\n", MODULE_NAME);

  // TODO: Enable tests
  ret = 0;
  // if (0 == platform_rng_tests()) {
  //   if (0 == platform_system_time_tests()) {
  //     if (0 == platform_threading_tests()) {
  //       if (0 == platform_gpio_tests()) {
  //         ret = 0;
  //       }
  //       else printTestFailure(MODULE_NAME, "GPIO");
  //     }
  //     else printTestFailure(MODULE_NAME, "Threading");
  //   }
  //   else printTestFailure(MODULE_NAME, "millis() / micros()");
  // }
  // else printTestFailure(MODULE_NAME, "RNG");

  return ret;
}
