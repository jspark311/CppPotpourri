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
                log->concat("\tstrcasestr() tests pass:\n");
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
      log->concat("\tTokenizer tests pass:\n");
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
          log->concat("\tImplode tests pass:\n");
          ret = 0;
        }
      }
    }
  }
  return ret;
}


int test_replace(StringBuilder* log) {
  const char* DELIM_STR = "\n\t";
  StringBuilder stack_obj;
  int ret = -1;
  int replacements = 0;
  log->concat("===< Replace tests >====================================\n");

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
              log->concatf("\tReplacement test block 0 passes:\t%s\n", (char*)stack_obj.string());
              ret = 0;
            }
            else log->concatf("replace() result does not match ref_string_0_2.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_0_2);
          }
          else log->concatf("replace(\"--\", \"\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_0, (char*)stack_obj.string());
        }
        else log->concatf("replace() result does not match ref_string_0_1.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_0_1);
      }
      else log->concatf("replace(\".\", \"--\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_0, (char*)stack_obj.string());
    }
    else log->concatf("replace() result does not match ref_string_0_0.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_0_0);
  }
  else log->concatf("replace(\"|\", \".\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_0, (char*)stack_obj.string());

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
                log->concatf("\tReplacement test block 1 passes:\t%s\n", (char*)stack_obj.string());
                ret = 0;
              }
              else log->concatf("replace() result does not match ref_string_1_2.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_1_2);
            }
            else log->concatf("replace(\"***\", \"*\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_1, (char*)stack_obj.string());
          }
          else log->concatf("replace() result does not match ref_string_1_1.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_1_1);
        }
        else log->concatf("replace(\":tag:\", \"***\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_1, (char*)stack_obj.string());
      }
      else log->concatf("replace() result does not match ref_string_1_0.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_1_0);
    }
    else log->concatf("replace(\":TAG:\", \":tag:\") returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_1, (char*)stack_obj.string());
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
                log->concatf("\tReplacement test block 2 passes:\t%s\n", (char*)stack_obj.string());
                print_err = false;
                ret = 0;
              }
              else log->concatf("replace() result does not match ref_string_2_2.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_2_2);
            }
            else log->concatf("replace(LF, TAB+LF) returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_2, (char*)stack_obj.string());
          }
          else log->concatf("replace() result does not match ref_string_2_1.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_2_1);
        }
        else log->concatf("replace(CR+LF, LF) returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_2, (char*)stack_obj.string());
      }
      else log->concatf("replace() result does not match ref_string_2_0.\n\t%s\n\t%s\n", (char*)stack_obj.string(), ref_string_2_0);
    }
    else log->concatf("replace(LF, CR+LF) returned %d when it should have returned %d.\n\t%s\n", replacements, REPLACE_COUNT_2, (char*)stack_obj.string());

    if (print_err) {
      log->concat("\n\nExpected:\n");
      StringBuilder::printBuffer(log, debug_str, debug_str_len);
      log->concat("\n\nActual:\n");
      StringBuilder::printBuffer(log, stack_obj.string(), stack_obj.length());
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
            log->concatf("\tReplacement test block 3 passes:\t%s\n", (char*)stack_obj.string());
            ret = 0;
          }
          else log->concatf("replace() called with a needle longer than the subject string should return 0 but returned %d instead.\n", replacements);
        }
        else log->concatf("replace() called on an empty needle should return 0 but returned %d instead.\n", replacements);
      }
      else log->concatf("replace() called on an empty StringBuilder and with an empty needle should return 0 but returned %d instead.\n", replacements);
    }
    else log->concatf("replace() called on an empty StringBuilder should return 0 but returned %d instead.\n", replacements);
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
            if (0 == test_replace(&log)) {
              log.concat("**********************************\n");
              log.concat("*  StringBuilder tests all pass  *\n");
              log.concat("**********************************\n");
              ret = 0;
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

  printf("%s\n\n", (const char*) log.string());
  return ret;
}
