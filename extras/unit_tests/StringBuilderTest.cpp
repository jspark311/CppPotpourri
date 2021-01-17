/*
File:   StringBuilderTest.cpp
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


This program tests StringBuilder, which is our preferred buffer abstraction.
This class makes extensive use of the heap, low-level memory assumptions, and is
  used as a premise for basically every program built on CppPotpourri. It should
  be extensively unit-tested.
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

#define STRBUILDER_STATICTEST_STRING "I CAN cOUNT to PoTaTo   "


/*******************************************************************************
* Support functions
* TODO: Some of this should be subsumed by the general linux platform.
*   some of what remains should be collected into a general testing framework?
* TODO: Research testing frameworks for C++ again.
*******************************************************************************/
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
* StringBuilder test routines
*******************************************************************************/


int test_StringBuilderStatics(StringBuilder* log) {
  int return_value = -1;
  log->concat("===< Static function tests >====================================\n");
  if (0 == StringBuilder::strcasestr("CHARACTER CONST STRING COMPARE", "CHARACTER CONST STRING COMPARE")) {
    if (0 == StringBuilder::strcasestr("cHArACTER CONST sTRING COMpARE", "CHARACTER CONST STRING COMPARE")) {
      if (0 != StringBuilder::strcasestr("CHARACTER CONST STRING 1OMPARE", "CHARACTER CONST STRING !OMPARE")) {
        if (0 == StringBuilder::strcasestr("CHARACTER CONST STRING COMPARE", "CHARACTER CONST STRING COMPARE ")) {
          if (0 != StringBuilder::strcasestr(" CHARACTER CONST STRING COMPARE", "CHARACTER CONST STRING COMPARE")) {
            if (0 != StringBuilder::strcasestr(nullptr, "CHARACTER CONST STRING COMPARE")) {
              if (0 != StringBuilder::strcasestr("CHARACTER CONST STRING COMPARE", nullptr)) {
                const unsigned int TEST_LEN = strlen(STRBUILDER_STATICTEST_STRING);
                uint8_t out_buf[256];
                if (0 == random_fill(out_buf, 256)) {
                  StringBuilder stack_obj((uint8_t*) STRBUILDER_STATICTEST_STRING, TEST_LEN+1);
                  stack_obj.trim();
                  StringBuilder::printBuffer(&stack_obj, out_buf, TEST_LEN, "INDENTSTR\t");
                  printf("%s\n\n", (const char*) stack_obj.string());
                  return_value = 0;
                }
                else log->concat("Failed to random_fill() test buffer.\n");
              }
              else log->concat("strcasestr() failed test 7.\n");
            }
            else log->concat("strcasestr() failed test 6.\n");
          }
          else log->concat("strcasestr() failed test 5.\n");
        }
        else log->concat("strcasestr() failed test 4.\n");
      }
      else log->concat("strcasestr() failed test 3.\n");
    }
    else log->concat("strcasestr() failed test 2.\n");
  }
  else log->concat("strcasestr() failed test 1.\n");
  return return_value;
}


int test_Tokenizer(StringBuilder* log) {
  StringBuilder stack_obj;
  int ret = -1;
  log->concat("===< Tokenizer tests >====================================\n");
  stack_obj.concat("                 _______  \n");
  stack_obj.concat("                / _____ \\ \n");
  stack_obj.concat("          _____/ /     \\ \\_____ \n");
  stack_obj.concat("         / _____/  000  \\_____ \\ \n");
  stack_obj.concat("   _____/ /     \\       /     \\ \\_____ \n");
  stack_obj.concat("  / _____/  001  \\_____/  002  \\_____ \\ \n");
  stack_obj.concat(" / /     \\       /     \\       /     \\ \\ \n");
  stack_obj.concat("/ /  003  \\_____/  004  \\_____/  005  \\ \\ \n");
  stack_obj.concat("\\ \\       /     \\       /     \\       / / \n");
  stack_obj.concat(" \\ \\_____/  006  \\_____/  007  \\_____/ / \n");
  stack_obj.concat(" / /     \\       /     \\       /     \\ \\ \n");
  stack_obj.concat("/ /  008  \\_____/  009  \\_____/  010  \\ \\ \n");
  stack_obj.concat("\\ \\       /     \\       /     \\       / / \n");
  stack_obj.concat(" \\ \\_____/  011  \\_____/  012  \\_____/ / \n");
  stack_obj.concat(" / /     \\       /     \\       /     \\ \\ \n");
  stack_obj.concat("/ /  013  \\_____/  014  \\_____/  015  \\ \\ \n");
  stack_obj.concat("\\ \\       /     \\       /     \\       / / \n");
  stack_obj.concat(" \\ \\_____/  016  \\_____/  017  \\_____/ / \n");
  stack_obj.concat("  \\_____ \\       /     \\       / _____/ \n");
  stack_obj.concat("        \\ \\_____/  018  \\_____/ / \n");
  stack_obj.concat("         \\_____ \\       / _____/ \n");
  stack_obj.concat("               \\ \\_____/ / \n");
  stack_obj.concat("                \\_______/ \n");

  int i_length = stack_obj.length();
  int i_count  = stack_obj.count();
  int chunks   = stack_obj.chunk(21);
  int p_length = stack_obj.length();
  int p_count  = stack_obj.count();
  stack_obj.string();
  int f_length = stack_obj.length();
  int f_count  = stack_obj.count();
  log->concatf("Initial:\n\t Length:    %d\n", i_length);
  log->concatf("\t Elements:  %d\n", i_count);
  log->concatf("Post-chunk:\n\t Length:    %d\n", p_length);
  log->concatf("\t Elements:  %d\n", p_count);
  log->concatf("\t Chunks:    %d\n", chunks);
  log->concatf("Final:\n\t Length:    %d\n", f_length);
  log->concatf("\t Elements:  %d\n", f_count);
  log->concat("Final Stack obj:\n");
  log->concat((char*) stack_obj.string());
  log->concat("\n\n");

  if ((-1 != chunks) & (p_count == chunks)) {
    if ((i_length == p_length) & (i_length == f_length)) {
      ret = 0;
    }
  }
  return ret;
}


int test_StringBuilder(StringBuilder* log) {
  int return_value = -1;
  log->concat("===< StringBuilder >====================================\n");
  StringBuilder* heap_obj = new StringBuilder((char*) "This is datas we want to transfer.");
  StringBuilder  stack_obj;
  StringBuilder  tok_obj;

  char* empty_str = (char*) stack_obj.string();
  if (0 == strlen(empty_str)) {
    free(empty_str);
    stack_obj.concat("a test of the StringBuilder ");
    stack_obj.concat("used in stack. ");
    stack_obj.prepend("This is ");
    stack_obj.string();

    tok_obj.concat("This");
    log->concatf("\t tok_obj split:   %d\n", tok_obj.split(" "));
    log->concatf("\t tok_obj count:   %d\n", tok_obj.count());
    tok_obj.concat(" This");
    log->concatf("\t tok_obj split:   %d\n", tok_obj.split(" "));
    log->concatf("\t tok_obj count:   %d\n", tok_obj.count());
    tok_obj.concat("   This");
    log->concatf("\t tok_obj split:   %d\n", tok_obj.split(" "));
    log->concatf("\t tok_obj count:   %d\n", tok_obj.count());

    log->concatf("\t Heap obj before culling:   %s\n", heap_obj->string());

    while (heap_obj->length() > 10) {
      heap_obj->cull(5);
      log->concatf("\t Heap obj during culling:   %s\n", heap_obj->string());
    }

    log->concatf("\t Heap obj after culling:   %s\n", heap_obj->string());

    heap_obj->prepend("Meaningless data ");
    heap_obj->concat(" And stuff tackt onto the end.");

    stack_obj.concatHandoff(heap_obj);

    delete heap_obj;

    stack_obj.split(" ");

    log->concatf("\t Final Stack obj:          %s\n", stack_obj.string());
    return_value = 0;
  }
  else log->concat("StringBuilder.string() failed to produce an empty string.\n");

  return return_value;
}



int test_StringBuilderHeapVersusStack(StringBuilder* log) {
  int return_value = -1;
  StringBuilder *heap_obj = new StringBuilder("This is datas we want to transfer.");
  StringBuilder stack_obj;

  stack_obj.concat("a test of the StringBuilder ");
  stack_obj.concat("used in stack. ");
  stack_obj.prepend("This is ");
  stack_obj.string();

  printf("Heap obj before culling:   %s\n", heap_obj->string());

  while (heap_obj->length() > 10) {
    heap_obj->cull(5);
    printf("Heap obj during culling:   %s\n", heap_obj->string());
  }

  printf("Heap obj after culling:   %s\n", heap_obj->string());

  heap_obj->prepend("Meaningless data ");
  heap_obj->concat(" And stuff tackt onto the end.");

  stack_obj.concatHandoff(heap_obj);

  delete heap_obj;

  stack_obj.split(" ");

  printf("Final Stack obj:          %s\n", stack_obj.string());
  return return_value;
}




void printTestFailure(const char* test) {
  printf("\n");
  printf("*********************************************\n");
  printf("* %s FAILED tests.\n", test);
  printf("*********************************************\n");
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int main() {
  int exit_value = 1;   // Failure is the default result.
  StringBuilder log;
  gettimeofday(&start_micros, nullptr);
  srand(start_micros.tv_usec);

  if (0 == test_StringBuilderStatics(&log)) {
    if (0 == test_StringBuilder(&log)) {
      if (0 == test_Tokenizer(&log)) {
        printf("**********************************\n");
        printf("*  StringBuilder tests all pass  *\n");
        printf("**********************************\n");
        exit_value = 0;
      }
      else printTestFailure("Tokenizer");
    }
    else printTestFailure("General");
  }
  else printTestFailure("Statics");

  printf("%s\n\n", (const char*) log.string());
  exit(exit_value);
}
