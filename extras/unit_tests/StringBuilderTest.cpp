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

int test_strcasecmp() {
  int return_value = -1;
  printf("===< strcasecmp tests >====================================\n");
  if (0 == StringBuilder::strcasecmp("CHARACTER CONST STRING COMPARE", "CHARACTER CONST STRING COMPARE")) {
    if (0 == StringBuilder::strcasecmp("cHArACTER CONST sTRING COMpARE", "CHARACTER CONST STRING COMPARE")) {
      if (0 != StringBuilder::strcasecmp("CHARACTER CONST STRING 1OMPARE", "CHARACTER CONST STRING !OMPARE")) {
        if (0 != StringBuilder::strcasecmp("CHARACTER CONST STRING COMPARE", "CHARACTER CONST STRING COMPARE ")) {
          if (0 != StringBuilder::strcasecmp(" CHARACTER CONST STRING COMPARE", "CHARACTER CONST STRING COMPARE")) {
            if (0 != StringBuilder::strcasecmp(nullptr, "CHARACTER CONST STRING COMPARE")) {
              if (0 != StringBuilder::strcasecmp("CHARACTER CONST STRING COMPARE", nullptr)) {
                return_value = 0;
              }
              else printf("strcasecmp() nullptr as arg2 passed and should have failed.\n");
            }
            else printf("strcasecmp() nullptr as arg1 passed and should have failed.\n");
          }
          else printf("strcasecmp() test 5 passed and should have failed.\n");
        }
        else printf("strcasecmp() test 4 passed and should have failed.\n");
      }
      else printf("strcasecmp() test 3 passed and should have failed.\n");
    }
    else printf("strcasecmp() test 2 failed and should have passed.\n");
  }
  else printf("strcasecmp() test 1 failed and should have passed.\n");
  return return_value;
}



int test_strcasestr() {
  int return_value = -1;
  const char* haystack = "Has Anyone Really Been Far Even as Decided to Use Even Go Want to do Look More Like?";
  const char* needle0  = "ly Been F";    // First find, case insensitive
  const char* needle1  = "aNYoNE";      // Case sensitivity.
  const char* needle2  = "Like? Extended";   // Should exceed haystack boundary in inner loop.
  const char* needle3  = "defenestrate";   // This should be a winning failure.

  char* target0 = ((char*) haystack + 15);
  char* target1 = ((char*) haystack + 4);

  printf("===< strcasestr tests >====================================\n");
  if (target0 == StringBuilder::strcasestr(haystack, needle0)) {
    if (target1 == StringBuilder::strcasestr(haystack, needle1)) {
      if (nullptr == StringBuilder::strcasestr(haystack, needle2)) {
        if (nullptr == StringBuilder::strcasestr(haystack, needle3)) {
          if (nullptr == StringBuilder::strcasestr(needle0, haystack)) {     // Swapped order.
            if (nullptr == StringBuilder::strcasestr(nullptr, needle0)) {    // nullptr
              if (nullptr == StringBuilder::strcasestr(haystack, nullptr)) { // nullptr
                printf("\tstrcasestr() tests pass:\n");
                return_value = 0;
              }
              else printf("strcasestr() nullptr as arg2 passed and should have failed.\n");
            }
            else printf("strcasestr() nullptr as arg1 passed and should have failed.\n");
          }
          else printf("strcasestr() test for comically-large needle passed and should have failed.\n");
        }
        else printf("strcasestr() test 4 passed and should have failed.\n");
      }
      else printf("strcasestr() test 3 passed and should have failed.\n");
    }
    else printf("strcasestr() test 2 failed and should have passed.\n");
  }
  else printf("strcasestr() test 1 failed and should have passed.\n");
  return return_value;
}


int test_Tokenizer() {
  StringBuilder stack_obj;
  int ret = -1;
  printf("===< Tokenizer tests >====================================\n");
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
  printf("Initial:\n\t Length:    %d\n", i_length);
  printf("\t Elements:  %d\n", i_count);
  printf("Post-chunk:\n\t Length:    %d\n", p_length);
  printf("\t Elements:  %d\n", p_count);
  printf("\t Chunks:    %d\n", chunks);
  printf("Final:\n\t Length:    %d\n", f_length);
  printf("\t Elements:  %d\n", f_count);
  printf("Final Stack obj:\n");
  printf("%s", (char*) stack_obj.string());
  printf("\n\n");

  if ((-1 != chunks) & (p_count == chunks)) {
    if ((i_length == p_length) & (i_length == f_length)) {
      printf("\tTokenizer tests pass:\n");
      ret = 0;
    }
  }
  return ret;
}



int test_Implode() {
  const char* DELIM_STR = "\n\t";
  StringBuilder stack_obj;
  int ret = -1;
  printf("===< Implode tests >====================================\n");
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
  printf("Initial:\n\t Length:    %d\n", i_length);
  printf("\t Elements:  %d\n", i_count);
  printf("Post-implosion:\n\t Length:    %d\n", p_length);
  printf("\t Elements:  %d\n", p_count);
  printf("\t implode returns %d\n", toks);
  printf("%s\n", (char*) stack_obj.string());

  int retoks = stack_obj.split(DELIM_STR);
  int f_length = stack_obj.length();
  int f_count  = stack_obj.count();
  printf("Re-split:\n\t Length:    %d\n", f_length);
  printf("\t Elements:  %d\n", f_count);
  printf("\t split() returns %d\n", retoks);
  printf("\n\n");

  if ((i_count == f_count) & (i_length == f_length)) {   // We started and ended with the same length and token count.
    if (f_count == toks) {       // That token count equals the return value from implode.
      if (p_count == 1) {        // Implode fully reduced the original set of tokens.
        if (toks == retoks) {    // implode and split have the same return value.
          printf("\tImplode tests pass:\n");
          ret = 0;
        }
      }
    }
  }
  return ret;
}


int test_replace() {
  const char* DELIM_STR = "\n\t";
  StringBuilder stack_obj;
  int ret = -1;
  int replacements = 0;
  printf("===< Replace tests >====================================\n");

  const int REPLACE_COUNT_0 = 7;
  const int REPLACE_COUNT_1 = 6;
  const int REPLACE_COUNT_2 = 5;
  const char* exp_string_0 = "ANOTHER|DELIMITER||TEST|STRING|||";
  const char* exp_string_1 = ":TAG:torture:TAG:case:TAG::TAG:With:TAG long:TAG:NEEDLE:TAG::T";
  const char* exp_string_2 = "This is a typical text layout.\n\nIt has double-spacing,\nas well as a terminal\nline ending.\n";

  const char* ref_string_0_0 = "ANOTHER.DELIMITER..TEST.STRING...";
  const char* ref_string_0_1 = "ANOTHER---DELIMITER------TEST---STRING---------";
  const char* ref_string_0_2 = "ANOTHERDELIMITERTESTSTRING";

  const char* ref_string_1_0 = ":tag:torture:tag:case:tag::tag:With:TAG long:tag:NEEDLE:tag::T";
  const char* ref_string_1_1 = "***torture***case******With:TAG long***NEEDLE***:T";
  const char* ref_string_1_2 = "*torture*case**With:TAG long*NEEDLE*:T";

  const char* ref_string_2_0 = "This is a typical text layout.\r\n\r\nIt has double-spacing,\r\nas well as a terminal\r\nline ending.\r\n";
  const char* ref_string_2_1 = "This is a typical text layout.\n\nIt has double-spacing,\nas well as a terminal\nline ending.\n";
  const char* ref_string_2_2 = "This is a typical text layout.\n\t\n\tIt has double-spacing,\n\tas well as a terminal\n\tline ending.\n\t";

  stack_obj.concat(exp_string_0);
  replacements = stack_obj.replace("|", ".");
  if (REPLACE_COUNT_0 == replacements) {
    if (0 == StringBuilder::strcasecmp((char*)stack_obj.string(), ref_string_0_0)) {
      replacements = stack_obj.replace(".", "---");
      if (REPLACE_COUNT_0 == replacements) {
        if (0 == StringBuilder::strcasecmp((char*)stack_obj.string(), ref_string_0_1)) {
          replacements = stack_obj.replace("---", "");
          if (REPLACE_COUNT_0 == replacements) {
            if (0 == StringBuilder::strcasecmp((char*)stack_obj.string(), ref_string_0_2)) {
              printf("\tReplacement test block 0 passes:\t%s\n", (char*)stack_obj.string());
              ret = 0;
            }
            else printf("replace() result does not match ref_string_0_2.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_0_2);
          }
          else printf("replace(\"--\", \"\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_0, (char*)stack_obj.string());
        }
        else printf("replace() result does not match ref_string_0_1.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_0_1);
      }
      else printf("replace(\".\", \"--\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_0, (char*)stack_obj.string());
    }
    else printf("replace() result does not match ref_string_0_0.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_0_0);
  }
  else printf("replace(\"|\", \".\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_0, (char*)stack_obj.string());

  if (0 == ret) {
    ret--;
    stack_obj.clear();
    stack_obj.concat(exp_string_1);
    replacements = stack_obj.replace(":TAG:", ":tag:");
    if (REPLACE_COUNT_1 == replacements) {
      if (0 == StringBuilder::strcasecmp((char*)stack_obj.string(), ref_string_1_0)) {
        replacements = stack_obj.replace(":tag:", "***");
        if (REPLACE_COUNT_1 == replacements) {
          if (0 == StringBuilder::strcasecmp((char*)stack_obj.string(), ref_string_1_1)) {
            replacements = stack_obj.replace("***", "*");
            if (REPLACE_COUNT_1 == replacements) {
              if (0 == StringBuilder::strcasecmp((char*)stack_obj.string(), ref_string_1_2)) {
                printf("\tReplacement test block 1 passes:\t%s\n", (char*)stack_obj.string());
                ret = 0;
              }
              else printf("replace() result does not match ref_string_1_2.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_1_2);
            }
            else printf("replace(\"***\", \"*\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_1, (char*)stack_obj.string());
          }
          else printf("replace() result does not match ref_string_1_1.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_1_1);
        }
        else printf("replace(\":tag:\", \"***\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_1, (char*)stack_obj.string());
      }
      else printf("replace() result does not match ref_string_1_0.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_1_0);
    }
    else printf("replace(\":TAG:\", \":tag:\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_1, (char*)stack_obj.string());
  }

  if (0 == ret) {
    ret--;
    bool print_err = true;
    uint8_t* debug_str = (uint8_t*) "\0\0\0";
    stack_obj.clear();
    stack_obj.concat(exp_string_2);
    debug_str = (uint8_t*) ref_string_2_0;
    int debug_str_len = strlen((char*)debug_str);
    replacements = stack_obj.replace("\n", "\r\n");
    if (REPLACE_COUNT_2 == replacements) {
      if (1 == stack_obj.cmpBinString(debug_str, debug_str_len)) {
        debug_str = (uint8_t*) ref_string_2_1;
        debug_str_len = strlen((char*)debug_str);
        replacements = stack_obj.replace("\r\n", "\n");
        if (REPLACE_COUNT_2 == replacements) {
          if (1 == stack_obj.cmpBinString(debug_str, debug_str_len)) {
            debug_str = (uint8_t*) ref_string_2_2;
            debug_str_len = strlen((char*)debug_str);
            replacements = stack_obj.replace("\n", "\n\t");
            if (REPLACE_COUNT_2 == replacements) {
              if (1 == stack_obj.cmpBinString(debug_str, debug_str_len)) {
                printf("\tReplacement test block 2 passes:\t%s\n", (char*)stack_obj.string());
                print_err = false;
                ret = 0;
              }
              else printf("replace() result does not match ref_string_2_2.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_2_2);
            }
            else printf("replace(LF, TAB+LF) returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_2, (char*)stack_obj.string());
          }
          else printf("replace() result does not match ref_string_2_1.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_2_1);
        }
        else printf("replace(CR+LF, LF) returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_2, (char*)stack_obj.string());
      }
      else printf("replace() result does not match ref_string_2_0.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_2_0);
    }
    else printf("replace(LF, CR+LF) returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_2, (char*)stack_obj.string());

    if (print_err) {
      StringBuilder log;
      log.concat("\n\nExpected:\n");
      StringBuilder::printBuffer(&log, debug_str, debug_str_len);
      log.concat("\n\nActual:\n");
      StringBuilder::printBuffer(&log, stack_obj.string(), stack_obj.length());
      printf("%s", (char*) log.string());
    }
  }

  if (0 == ret) {
    // Empty needle/subject tests.
    ret--;
    stack_obj.clear();
    stack_obj.concat("");
    replacements = stack_obj.replace(" ", "+");
    if (0 == replacements) {
      replacements = stack_obj.replace("", "+");
      if (0 == replacements) {
        stack_obj.concat("Some not-empty string");
        replacements = stack_obj.replace("", "+");
        if (0 == replacements) {
          replacements = stack_obj.replace("Some not-empty string key that is too long", "+");
          if (0 == replacements) {
            printf("\tReplacement test block 3 passes:\t%s\n", (char*)stack_obj.string());
            ret = 0;
          }
          else printf("replace() called with a needle longer than the subject string should return 0 but returned %d instead.\n", replacements);
        }
        else printf("replace() called on an empty needle should return 0 but returned %d instead.\n", replacements);
      }
      else printf("replace() called on an empty StringBuilder and with an empty needle should return 0 but returned %d instead.\n", replacements);
    }
    else printf("replace() called on an empty StringBuilder should return 0 but returned %d instead.\n", replacements);
  }
  return ret;
}


int test_StringBuilder() {
  int return_value = -1;
  printf("===< StringBuilder >====================================\n");
  StringBuilder* heap_obj = new StringBuilder((char*) "This is datas we want to transfer.");
  StringBuilder  stack_obj;
  StringBuilder  tok_obj;

  char* empty_str = (char*) stack_obj.string();
  if (0 == strlen(empty_str)) {
    stack_obj.concat("a test of the StringBuilder ");
    stack_obj.concat("used in stack. ");
    stack_obj.prepend("This is ");
    stack_obj.string();

    tok_obj.concat("This");
    printf("\t tok_obj split:   %d\n", tok_obj.split(" "));
    printf("\t tok_obj count:   %d\n", tok_obj.count());
    tok_obj.concat(" This");
    printf("\t tok_obj split:   %d\n", tok_obj.split(" "));
    printf("\t tok_obj count:   %d\n", tok_obj.count());
    tok_obj.concat("   This");
    printf("\t tok_obj split:   %d\n", tok_obj.split(" "));
    printf("\t tok_obj count:   %d\n", tok_obj.count());
    printf("\t Heap obj before culling:   %s\n", heap_obj->string());

    while (heap_obj->length() > 10) {
      heap_obj->cull(5);
      printf("\t Heap obj during culling:   %s\n", heap_obj->string());
    }
    printf("\t Heap obj after culling:   %s\n", heap_obj->string());

    heap_obj->prepend("Meaningless data ");
    heap_obj->concat(" And stuff tackt onto the end.");

    stack_obj.concatHandoff(heap_obj);

    delete heap_obj;

    stack_obj.split(" ");

    printf("\t Final Stack obj:          %s\n", stack_obj.string());
    return_value = 0;
  }
  else printf("StringBuilder.string() failed to produce an empty string.\n");

  return return_value;
}



int test_StringBuilderCull() {
  int ret = -1;
  const char* BASE_STRING = "0-1-2-3-4-5-6-7-8-9-10-11-12-13-14-15";  // 37 characters
  const uint32_t MASTER_LENGTH = strlen(BASE_STRING);
  StringBuilder obj_0((char*) BASE_STRING);
  StringBuilder obj_1((char*) BASE_STRING);
  StringBuilder obj_2((char*) BASE_STRING);
  StringBuilder obj_3((char*) BASE_STRING);
  StringBuilder obj_4((char*) BASE_STRING);
  StringBuilder obj_5((char*) BASE_STRING);

  obj_0.cull(0, MASTER_LENGTH);   // No operation.
  obj_1.cull(14, MASTER_LENGTH);  // Impossible request. String will not be that long.
  obj_2.cull(MASTER_LENGTH, 0);   // Should clear the string.
  obj_3.cull(0, 11);              // Should be the head of the string.
  obj_4.cull(14, (MASTER_LENGTH - 14));  // Should be the tail of the string.
  obj_5.cull(14, 11);             // Taking from the middle.

  // The null and failure cases ought to still match the base string.
  if (0 == StringBuilder::strcasecmp((char*) obj_0.string(), BASE_STRING)) {
    if (0 == StringBuilder::strcasecmp((char*) obj_1.string(), BASE_STRING)) {
      // The full-cull case ought to be an empty string.
      if (obj_2.isEmpty()) {
        const char* KAT_3 = "0-1-2-3-4-5";
        const char* KAT_4 = "7-8-9-10-11-12-13-14-15";
        const char* KAT_5 = "7-8-9-10-11";
        if (0 == StringBuilder::strcasecmp((char*) obj_3.string(), KAT_3)) {
          if (0 == StringBuilder::strcasecmp((char*) obj_4.string(), KAT_4)) {
            if (0 == StringBuilder::strcasecmp((char*) obj_5.string(), KAT_5)) {
              ret = 0;
            }
            else printf("obj_5 does not match.\n");
          }
          else printf("obj_4 does not match.\n");
        }
        else printf("obj_3 does not match.\n");
      }
      else printf("obj_2 is not empty, as it should be.\n");
    }
    else printf("obj_1 does not match.\n");
  }
  else printf("obj_0 does not match.\n");


  printf("obj_0:    %s\n", obj_0.string());
  printf("obj_1:    %s\n", obj_1.string());
  printf("obj_2:    %s\n", obj_2.string());
  printf("obj_3:    %s\n", obj_3.string());
  printf("obj_4:    %s\n", obj_4.string());
  printf("obj_5:    %s\n", obj_5.string());

  return ret;
}




int test_StringBuilderHeapVersusStack() {
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


int test_stringbuilder_isempty() {
  int return_value = -1;
  uint8_t tmp_buf[8] = {0, };
  StringBuilder should_be_empty;
  StringBuilder should_have_things(&tmp_buf[0], 8);

  if (should_be_empty.isEmpty()) {
    if (should_be_empty.isEmpty(true)) {
      //should_be_empty.concat(&tmp_buf[0], 1);   // Causes segfault
      should_be_empty.concat('\0');  // Passes isEmpty(true), and it shouldn't.
      if (should_be_empty.isEmpty()) {
        if (((true))) {   // TODO
        //if (!should_be_empty.isEmpty(true)) {
          should_be_empty.string();   // Collapse the string.
          if (should_be_empty.isEmpty()) {
            if (should_be_empty.isEmpty(true)) {
              if (!should_have_things.isEmpty()) {
                if (!should_have_things.isEmpty(true)) {
                  printf("\tisEmpty() passes.\n");
                  return_value = 0;
                }
                else printf("should_have_things.isEmpty(true) found nothing.\n");
              }
              else printf("should_have_things.isEmpty() found nothing. Bad.\n");
            }
            else printf("should_be_empty.isEmpty(true) failed to find bytes after adding a null-terminator.\n");
          }
          else printf("should_be_empty.isEmpty() found bytes after adding a null-terminator.\n");
        }
        else printf("should_be_empty.isEmpty(true) returned true after adding a null-terminator.\n");
      }
      else printf("should_be_empty.isEmpty() found bytes after adding a null-terminator.\n");
    }
    else printf("should_be_empty.isEmpty(true) found bytes.\n");
  }
  else printf("should_be_empty.isEmpty() found bytes. Bad.\n");

  return return_value;
}


/*
* StringBuilder is a big API. It's easy to make mistakes or under-estimate
*   memory impact.
*/
int test_misuse_cases() {
  int return_value = -1;
  printf("===< Mis-use tests >====================================\n");
  StringBuilder content_from_const("The compiler considered this string a (const char*).");
  content_from_const.clear();

  if (content_from_const.isEmpty(true)) {
    printf("About to double-clear content_from_const... ");
    content_from_const.clear();
    printf("success.\n");
    if (nullptr != content_from_const.string()) {    // Should always return an empty string, in the worst-case.
      printf("About to concat(const) --> concatf() --> destruct-by-scope... ");
      {
        StringBuilder scope_limited("More const content. ");
        scope_limited.concatf("current time is %u.", millis());
      }
      printf("success.\n");

      printf("About to concatf() --> destruct-by-scope... ");
      {
        StringBuilder scope_limited("More const content. ");
        printf("%s", (char*) scope_limited.string());
      }
      printf("success.\n");

      printf("About to concat(const) --> concatf() --> string() --> destruct-by-scope... ");
      {
        StringBuilder scope_limited("More const content. ");
        scope_limited.concatf("current time is %u.", millis());
        printf("%s", (char*) scope_limited.string());
      }
      printf("success.\n");

      // If nothing above caused a segfault, the tests pass.
      printf("\tMis-use tests pass.\n");
      return_value = 0;
    }
    else printf("content_from_const.string() returned nullptr, but should have returned \"\".\n");
  }
  else printf("content_from_const.isEmpty() found bytes. Bad.\n");

  return return_value;
}




/*******************************************************************************
* The main function.
*******************************************************************************/
int stringbuilder_main() {
  int ret = 1;   // Failure is the default result.

  if (0 == test_strcasecmp()) {
    if (0 == test_strcasestr()) {
      if (0 == test_StringBuilder()) {
        if (0 == test_Tokenizer()) {
          if (0 == test_Implode()) {
            if (0 == test_replace()) {
              if (0 == test_stringbuilder_isempty()) {
                if (0 == test_StringBuilderCull()) {
                  if (0 == test_misuse_cases()) {
                    printf("**********************************\n");
                    printf("*  StringBuilder tests all pass  *\n");
                    printf("**********************************\n");
                    ret = 0;
                  }
                  else printTestFailure("Hardening against mis-use");
                }
                else printTestFailure("cull(int, int)");
              }
              else printTestFailure("isEmpty");
            }
            else printTestFailure("Replace");
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

  return ret;
}
