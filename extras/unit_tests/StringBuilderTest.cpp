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

#define STRBUILDER_STATICTEST_STRING "I CAN cOUNT to PoTaTo   "


/*******************************************************************************
* StringBuilder test routines
*******************************************************************************/

int test_strcasecmp(StringBuilder* log) {
  int return_value = -1;
  log->concat("===< strcasecmp tests >====================================\n");
  if (0 == StringBuilder::strcasecmp("CHARACTER CONST STRING COMPARE", "CHARACTER CONST STRING COMPARE")) {
    if (0 == StringBuilder::strcasecmp("cHArACTER CONST sTRING COMpARE", "CHARACTER CONST STRING COMPARE")) {
      if (0 != StringBuilder::strcasecmp("CHARACTER CONST STRING 1OMPARE", "CHARACTER CONST STRING !OMPARE")) {
        if (0 != StringBuilder::strcasecmp("CHARACTER CONST STRING COMPARE", "CHARACTER CONST STRING COMPARE ")) {
          if (0 != StringBuilder::strcasecmp(" CHARACTER CONST STRING COMPARE", "CHARACTER CONST STRING COMPARE")) {
            if (0 != StringBuilder::strcasecmp(nullptr, "CHARACTER CONST STRING COMPARE")) {
              if (0 != StringBuilder::strcasecmp("CHARACTER CONST STRING COMPARE", nullptr)) {
                return_value = 0;
              }
              else log->concat("strcasecmp() nullptr as arg2 passed and should have failed.\n");
            }
            else log->concat("strcasecmp() nullptr as arg1 passed and should have failed.\n");
          }
          else log->concat("strcasecmp() test 5 passed and should have failed.\n");
        }
        else log->concat("strcasecmp() test 4 passed and should have failed.\n");
      }
      else log->concat("strcasecmp() test 3 passed and should have failed.\n");
    }
    else log->concat("strcasecmp() test 2 failed and should have passed.\n");
  }
  else log->concat("strcasecmp() test 1 failed and should have passed.\n");
  return return_value;
}



int test_strcasestr(StringBuilder* log) {
  int return_value = -1;
  const char* haystack = "Has Anyone Really Been Far Even as Decided to Use Even Go Want to do Look More Like?";
  const char* needle0  = "ly Been F";    // First find, case insensitive
  const char* needle1  = "aNYoNE";      // Case sensitivity.
  const char* needle2  = "Like? Extended";   // Should exceed haystack boundary in inner loop.
  const char* needle3  = "defenestrate";   // This should be a winning failure.

  char* target0 = ((char*) haystack + 15);
  char* target1 = ((char*) haystack + 4);

  log->concat("===< strcasestr tests >====================================\n");
  if (target0 == StringBuilder::strcasestr(haystack, needle0)) {
    if (target1 == StringBuilder::strcasestr(haystack, needle1)) {
      if (nullptr == StringBuilder::strcasestr(haystack, needle2)) {
        if (nullptr == StringBuilder::strcasestr(haystack, needle3)) {
          if (nullptr == StringBuilder::strcasestr(needle0, haystack)) {     // Swapped order.
            if (nullptr == StringBuilder::strcasestr(nullptr, needle0)) {    // nullptr
              if (nullptr == StringBuilder::strcasestr(haystack, nullptr)) { // nullptr
                return_value = 0;
              }
              else log->concat("strcasestr() nullptr as arg2 passed and should have failed.\n");
            }
            else log->concat("strcasestr() nullptr as arg1 passed and should have failed.\n");
          }
          else log->concat("strcasestr() test for comically-large needle passed and should have failed.\n");
        }
        else log->concat("strcasestr() test 4 passed and should have failed.\n");
      }
      else log->concat("strcasestr() test 3 passed and should have failed.\n");
    }
    else log->concat("strcasestr() test 2 failed and should have passed.\n");
  }
  else log->concat("strcasestr() test 1 failed and should have passed.\n");
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



int test_Implode(StringBuilder* log) {
  const char* DELIM_STR = "\n\t";
  StringBuilder stack_obj;
  int ret = -1;
  log->concat("===< Implode tests >====================================\n");
  stack_obj.concat("This string");
  stack_obj.concat("had no tabs");
  stack_obj.concat("or newlines");
  stack_obj.concat("when it was");
  stack_obj.concat("created.");

  int i_length = stack_obj.length();
  int i_count  = stack_obj.count();
  int toks = stack_obj.implode(DELIM_STR);

  int p_length = stack_obj.length();
  int p_count  = stack_obj.count();
  log->concatf("Initial:\n\t Length:    %d\n", i_length);
  log->concatf("\t Elements:  %d\n", i_count);
  log->concatf("Post-implosion:\n\t Length:    %d\n", p_length);
  log->concatf("\t Elements:  %d\n", p_count);
  log->concatf("\t implode returns %d\n", toks);
  log->concat((char*) stack_obj.string());
  log->concat("\n");

  int retoks = stack_obj.split(DELIM_STR);
  int f_length = stack_obj.length();
  int f_count  = stack_obj.count();
  log->concatf("Re-split:\n\t Length:    %d\n", f_length);
  log->concatf("\t Elements:  %d\n", f_count);
  log->concatf("\t split() returns %d\n", retoks);
  log->concat("\n\n");

  if ((i_count == f_count) & (i_length == f_length)) {   // We started and ended with the same length and token count.
    if (f_count == toks) {       // That token count equals the return value from implode.
      if (p_count == 1) {        // Implode fully reduced the original set of tokens.
        if (toks == retoks) {    // implode and split have the same return value.
          ret = 0;
        }
      }
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



/*******************************************************************************
* The main function.
*******************************************************************************/
int stringbuilder_main() {
  int ret = 1;   // Failure is the default result.
  StringBuilder log;

  if (0 == test_strcasecmp(&log)) {
    if (0 == test_strcasestr(&log)) {
      if (0 == test_StringBuilder(&log)) {
        if (0 == test_Tokenizer(&log)) {
          if (0 == test_Implode(&log)) {
            printf("**********************************\n");
            printf("*  StringBuilder tests all pass  *\n");
            printf("**********************************\n");
            ret = 0;
          }
          else printTestFailure("Implode");
        }
        else printTestFailure("Tokenizer");
      }
      else printTestFailure("General");
    }
    else printTestFailure("strcasestr");
  }
  else printTestFailure("strcasecmp");

  printf("%s\n\n", (const char*) log.string());
  return ret;
}
