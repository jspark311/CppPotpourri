/*
File:   UUIDTests.cpp
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


This program runs tests against UUID.
*/

/*******************************************************************************
* UUID test routines
*******************************************************************************/

void print_types_uuid() {
  printf("\tUUID                     %u\t%u\n", sizeof(UUID),                 alignof(UUID));
}


/**
* UUID battery.
* @return 0 on pass. Non-zero otherwise.
*/
int uuid_test_main() {
  const char* const MODULE_NAME = "UUID";
  printf("===< %s >=======================================\n", MODULE_NAME);
  StringBuilder log;
  StringBuilder temp;
  UUID test0;
  UUID test1;

  // Do UUID's initialize to zero?
  for (int i = 0; i < 16; i++) {
    if (0 != *((uint8_t*) &test0.id[i])) {
      printf("UUID should be initialized to zeros. It was not. Failing...\n");
      return -1;
    }
  }

  // Does the comparison function work?
  printf("UUID comparison... ");
  if (uuid_compare(&test0, &test1)) {
    printf(" considers these distinct. Failing...\n");
    temp.concat((uint8_t*) &test0, sizeof(test0));
    temp.printDebug(&log);
    temp.clear();
    temp.concat((uint8_t*) &test1, sizeof(test1));
    temp.printDebug(&log);
    printf("%s\n\n", (const char*) log.string());
    return -1;
  }
  printf("success.\n");

  printf("UUID generation... ");
  uuid_gen(&test0);
  // Does the comparison function work?
  if (0 == uuid_compare(&test0, &test1)) {
    printf("produced no change in the UUID. Failing...\n");
    temp.concat((uint8_t*) &test0, sizeof(test0));
    temp.printDebug(&log);
    temp.clear();
    temp.concat((uint8_t*) &test1, sizeof(test1));
    temp.printDebug(&log);
    printf("%s\n\n", (const char*) log.string());
    return -1;
  }
  printf("success.\n");

  // Generate a whole mess of UUID and ensure that they are different.
  printf("UUID generation (closer look)... ");
  for (int i = 0; i < 10; i++) {
    temp.concat((uint8_t*) &test0, sizeof(test0));
    log.concat("temp0 bytes:  ");
    temp.printDebug(&log);
    temp.clear();

    if (0 == uuid_compare(&test0, &test1)) {
      printf("UUID generator gave us a repeat UUID. Fail...\n");
      printf("%s\n\n", (const char*) log.string());
      return -1;
    }
    uuid_copy(&test0, &test1);
    if (0 != uuid_compare(&test0, &test1)) {
      log.concat("UUID copy appears to have failed...\n");
      temp.concat((uint8_t*) &test0, sizeof(test0));
      temp.printDebug(&log);
      temp.clear();
      temp.concat((uint8_t*) &test1, sizeof(test1));
      temp.printDebug(&log);
      printf("%s\n\n", (const char*) log.string());
      return -1;
    }
    uuid_gen(&test0);
  }
  printf("success.\n");

  printf("UUID packing...\n");
  char* str_buffer = (char*) alloca(40);
  bzero(str_buffer, 40);
  uuid_to_str(&test0, str_buffer, 40);

  //log.concatf("test0 string: %s\n", str_buffer);   // TODO: Why does this line crash the test?
  log.concat("uuid_to_sb(test0): ");
  uuid_to_sb(&test0, &log);
  log.concat("\n");

  printf("UUID parsing...\n");
  uuid_from_str(str_buffer, &test1);

  log.concat("uuid_to_sb(test1): ");
  uuid_to_sb(&test1, &log);

  log.concat("temp1 bytes:  ");
  temp.concat((uint8_t*) &test1, sizeof(test1));
  temp.printDebug(&log);

  if (0 != uuid_compare(&test0, &test1)) {
    printf("UUID parsing of the string previously packed did not yield the same value. Failing...\n");
    printf("%s\n\n", (const char*) log.string());
    return -1;
  }

  // TODO: This is the end of the happy-path. Now we should abuse the program
  // by feeding it garbage and ensure that its behavior is defined.

  printf("%s\n\n", (const char*) log.string());
  return 0;
}
