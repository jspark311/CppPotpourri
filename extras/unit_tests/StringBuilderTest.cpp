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


/*******************************************************************************
* TODO: Generalization block
*******************************************************************************/

void print_types_stringbuilder() {
  printf("\tStringBuilder         %u\t%u\n", sizeof(StringBuilder), alignof(StringBuilder));
  printf("\tStrLL                 %u\t%u\n", sizeof(StrLL), alignof(StrLL));
}


// TODO: End of generalization block.



/*******************************************************************************
* StringBuilder test routines
*******************************************************************************/
/* DRY function to print metrics for a StringBuilder. */
void print_sb_metrics(const char* item_name, const int l, const int c, const int s) {
  printf("\t(%20s) Length, count, size:    %5d, %5d, %5d bytes\n", item_name, l, c, s);
}
void print_sb_metrics(const char* item_name, StringBuilder* obj) {
  print_sb_metrics(item_name, obj->length(), obj->count(), obj->memoryCost());
}



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


/*
  Tests chunk(const int)
  Depends on
*/
int test_stringbuilder_chunk() {
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
  int i_mem_sz = stack_obj.memoryCost();

  int chunks   = stack_obj.chunk(21);
  int p_length = stack_obj.length();
  int p_count  = stack_obj.count();
  int p_mem_sz = stack_obj.memoryCost();

  stack_obj.string();
  int f_length = stack_obj.length();
  int f_count  = stack_obj.count();
  int f_mem_sz = stack_obj.memoryCost();

  print_sb_metrics("Initial conditions", i_length, i_count, i_mem_sz);
  print_sb_metrics("Post-chunk", p_length, p_count, p_mem_sz);
  print_sb_metrics("Final (collapsed)", f_length, f_count, f_mem_sz);

  printf("Final Stack obj:\n");
  printf("%s", (char*) stack_obj.string());
  printf("\n\n");

  if ((-1 != chunks) & (p_count == chunks)) {
    if ((i_length == p_length) & (i_length == f_length)) {
      printf("\tTokenizer tests pass:\n");
      ret = 0;
    }
    else printf("\tLength of string did not stay constant throughout test (I, P, D):  %d, %d, %d).\n", i_length, p_length, f_length);
  }
  else printf("\tChunk request disagreement with measurement (%d versus %d).\n", chunks, p_count);
  return ret;
}


/*
  Tests implode(const char*)
  Depends on
*/
int test_stringbuilder_implode() {
  const char* DELIM_STR = "\n\t";
  StringBuilder stack_obj;
  int ret = -1;
  printf("Testing StringBuilder::implode(const char*)...\n");
  stack_obj.concat("This string");
  stack_obj.concat("had no tabs");
  stack_obj.concat("or newlines");
  stack_obj.concat("when it was");
  stack_obj.concat("created.");

  int i_length = stack_obj.length();
  int i_count  = stack_obj.count();
  int i_mem_sz = stack_obj.memoryCost();

  printf("\tWe are starting with a fragmented string... ");
  if (1 < i_count) {
    printf("Pass.\n\timplode() should return 0 when given a null delimiter... ");
    if ((0 == stack_obj.implode(nullptr)) && (stack_obj.count() == i_count)) {
      printf("Pass.\n\timplode() should return 0 when given a zero-length delimiter... ");
      if ((0 == stack_obj.implode("")) && (stack_obj.count() == i_count)) {
        printf("Pass.\n\timplode() should return the fragment count on success... ");
        if (i_count == stack_obj.implode(DELIM_STR)) {
          printf("Pass.\n\tcount() should be 1 following implode()... ");
          if (stack_obj.count() == 1) {
            const int expect_delim_count = (i_count - 1);
            const int expect_len = (i_length + (expect_delim_count * strlen(DELIM_STR)));
            printf("Pass.\n\tlength() should be %d following the addition of %d delimiters... ", expect_len, expect_delim_count);
            if (stack_obj.length() == expect_len) {
              printf("Pass.\n\timplode() tests pass:\n");
              print_sb_metrics("Initial conditions", i_length, i_count, i_mem_sz);
              print_sb_metrics("Final conditions", &stack_obj);
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
  Tests toUpper() and toLower()
*/
int test_stringbuilder_case_shifter() {
  const char* const PRIMER_STRING = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  const char* const UPPER_STRING  = "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZ";
  const char* const LOWER_STRING  = "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz";
  const int         TEST_STR_LEN  = strlen(PRIMER_STRING);
  printf("Testing toUpper()...\n");
  int ret = -1;
  StringBuilder stack_obj(PRIMER_STRING);

  printf("\ttoUpper() works... ");
  stack_obj.toUpper();
  if (0 == stack_obj.locate(UPPER_STRING)) {
    printf("Pass.\n\ttoUpper() tests pass.\n");
    ret = 0;
  }

  if (0 == ret) {
    printf("Testing toLower()...\n");
    ret = -2;
    stack_obj.clear();
    stack_obj.concat(PRIMER_STRING);
    printf("\ttoLower() works... ");
    stack_obj.toLower();
    if (0 == stack_obj.locate(LOWER_STRING)) {
      printf("Pass.\n\ttoLower() tests pass.\n");
      ret = 0;
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
  }
  return ret;
}


/*
  Tests byteAt(const int)
  (Needlesly) Depends on chunk() for inducing string fragmentation.
*/
int test_stringbuilder_byteat() {
  printf("Testing byteAt(const int)...\n");
  StringBuilder stack_obj("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
  int ret = -1;
  uint8_t test_byte = 0;
  bool str_collapsed = true;

  printf("\tDoes byteAt(0) return 'A'... ");
  test_byte = stack_obj.byteAt(0);
  if ('A' == test_byte) {
    printf("Pass.\n\tDoes byteAt(26) return 'a'... ");
    test_byte = stack_obj.byteAt(26);
    if ('a' == test_byte) {
      printf("Pass.\n\tDoes byteAt(<out-of-bounds>) return 0... ");
      test_byte = stack_obj.byteAt(stack_obj.length() + 100);
      if (0 == test_byte) {
        printf("Pass.\n\tFragmenting string... ");
        if (9 == stack_obj.chunk(6)) {
          printf("Pass.\n\tDoes byteAt(0) still return 'A'... ");
          test_byte = stack_obj.byteAt(0);
          if ('A' == test_byte) {
            printf("Pass.\n\tDoes byteAt(26) still return 'a'... ");
            test_byte = stack_obj.byteAt(26);
            if ('a' == test_byte) {
              printf("Pass.\n\tDoes byteAt(51) return 'z'... ");
              test_byte = stack_obj.byteAt(51);
              if ('z' == test_byte) {
                printf("Pass.\n\tbyteAt(const int) tests pass.\n");
                ret = 0;
              }
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


int test_stringbuilder_locate() {
  printf("Testing locate(const char*)...\n");
  const char* LOCATE_TEST_STRING = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  StringBuilder stack_obj(LOCATE_TEST_STRING);
  int ret = -1;

  printf("\tWhen called with a single byte needle, locate() returns 0 if it matches the first byte... ");
  if (0 == stack_obj.locate("A")) {
    printf("Pass.\n\tWhen called with a single byte needle, locate() returns (length-1) if it matches the last byte... ");
    if ((stack_obj.length() - 1) == stack_obj.locate("Z")) {
      printf("Pass.\n\tDoes locate() return -1 if the string is not found... ");
      if (-1 == stack_obj.locate("BA")) {
        printf("Pass.\n\tFragmenting string... ");
        if (13 == stack_obj.chunk(2)) {
          printf("Pass.\n\tDoes the first case still match... ");
          if (0 == stack_obj.locate("A")) {
            printf("Pass.\n\tDoes the second case still match... ");
            if ((stack_obj.length() - 1) == stack_obj.locate("Z")) {
              printf("Pass.\n\tDoes an exact match return 0... ");
              if (0 == stack_obj.locate(LOCATE_TEST_STRING)) {
                printf("Pass.\n\tDoes a a multibyte locate() work on haystack terminus... ");
                if ((stack_obj.length() - 4) == stack_obj.locate("WXYZ")) {
                  printf("Pass.\n\tlocate(const char*) tests pass.\n");
                  ret = 0;
                }
              }
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



int test_stringbuilder_split() {
  const char* DELIM_STR = "\n\t";
  StringBuilder stack_obj;
  int ret = -1;
  printf("Testing StringBuilder::split(const char*)...\n");
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

  printf("\tDid we start and end with the same length and token count... ");
  if ((i_count == f_count) & (i_length == f_length)) {
    printf("Pass.\n\tDid we start and end with the same length and token count... ");
    if (f_count == toks) {
      printf("Pass.\n\tImplode fully reduced the original set of tokens.... ");
      if (p_count == 1) {
        printf("Pass.\n\tToken count equals the return value from implode... ");
        if (toks == retoks) {
          printf("Pass.\n\timplode() tests pass.\n");
          ret = 0;
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
* A one-off struct to hold test cases for replace(). Each input case is thrice
*   mutated to test behavior on each string under both collapsed and fragmentary
*   conditions.
*/
struct sb_replace_case {
  const char* const input;
  const char* const search_0;
  const char* const replace_0;
  const char* const mutant_0;
  const int         replacements_0;
  const char* const test_description_0;
  const char* const search_1;
  const char* const replace_1;
  const char* const mutant_1;
  const int         replacements_1;
  const char* const test_description_1;
  const char* const search_2;
  const char* const replace_2;
  const char* const mutant_2;
  const int         replacements_2;
  const char* const test_description_2;
};

// String replacement is a subtle problem. We have a battery of test cases to
//   ensure edge-cases don't slip through testing, and to ensure our machinary
//   is still readable and maintainable.
// NOTE: replace() is meant to be textual (not buffer safe).
// TODO: Test assurances of non-mutation of memory layout.
sb_replace_case sb_replace_cases[] = {
  // Basics (Part 1):
  { "ANOTHER|||DELIMITER||TEST|||STRING",
      "|||", "^^^", "ANOTHER^^^DELIMITER||TEST^^^STRING", 2, "Straight-across multi-byte replacement.",
      "|", "+",     "ANOTHER^^^DELIMITER++TEST^^^STRING", 2, "Straight-across single-byte replacement.",
      "^", "",      "ANOTHERDELIMITER++TESTSTRING",       6, "Empty replacement of single-byte search term." },
  // Basics (Part 2):
  { "strings must be able to be length-scaled",
      " ",  "   ", "strings   must   be   able   to   be   length-scaled", 6, "Simple-case length scaling (upward).",
      "  ", " ",   "strings  must  be  able  to  be  length-scaled",       6, "Simple-case length scaling (downward).",
      "  ", "",    "stringsmustbeabletobelength-scaled",                   6, "Empty replacement of multi-byte search term." },
  // Empty haystacks:
  { "",
      "needle", "(error-made-here)", "", 0, "Legal operators on empty haystack.",
      "",       "(error-made-here)", "", 0, "Zero-length needle (illegal).",
      "",       "",                  "", 0, "Both operators zero-length (illegal)." },
  // Absurdities:
  //replace() called with a needle longer than the subject string should return 0.
  { "This string should remain unchanged.",
      nullptr,  "(error-made-here)", "This string should remain unchanged.", 0, "Search term undefined.",
      "",       "(error-made-here)", "This string should remain unchanged.", 0, "Search term zero-length.",
      "wombat", "(error-made-here)", "This string should remain unchanged.", 0, "Search term not found." },
  // Multi-byte edge-cases (Part 1):
  { "-....-...-.-...-.--...-.-----.-.....",  // "testStringInMorse"
      ".....", "",  "-....-...-.-...-.--...-.-----.-", 1, "Single multi-byte replacement to nothing at terminus.",
      "-....", "",  "-...-.-...-.--...-.-----.-",      1, "Single multi-byte replacement to nothing at origin.",
      "-...-.-...-.--...-.-----.-", "",       "",      1, "Single multi-byte replacement where the needle is the haystack." },
  // Multi-byte edge-cases (Part 2):
  { "-....-...-.-...-.--...-.-----.-.....",  // "testStringInMorse"
      ".-...-", "---", "-...------.--...-.-----.-.....",                2, "Consecutive multi-byte replacement resulting in a length decrease.",
      ".",      "--",  "--------------------------------------------", 14, "Consecutive single-byte replacement resulting in a length increase.",
      "-",      "",    "",                                             44, "Consecutive single-byte replacement resulting in a zero-length result." },
  // Literal edge-cases (Part 1):
  { "------ANOTHER|DELIMITER||TEST|STRING-||||||",
      "||",  "-", "------ANOTHER|DELIMITER-TEST|STRING----", 4, "Consecutive multi-byte replacement resulting in a length decrease at terminus.",
      "---", "-", "--ANOTHER|DELIMITER-TEST|STRING--",       3, "Consecutive multi-byte replacements resulting in a length decrease at origin and terminus.",
      "-",    "", "ANOTHER|DELIMITERTEST|STRING",            5, "Consecutive single-byte replacements resulting in a length decrease at origin and terminus." },
  // Tag torture cases:
  { ":TAG:torture:TAG:case:TAG::TAG:With:TAG long:TAG:NEEDLE:TAG::T",
      ":TAG:", ":tag:", ":tag:torture:tag:case:tag::tag:With:TAG long:tag:NEEDLE:tag::T", 6,  "Tag torture case #1.",
      ":tag:", "***",   "***torture***case******With:TAG long***NEEDLE***:T",             6,  "Tag torture case #2.",
      "**",    "*",     "**torture**case***With:TAG long**NEEDLE**:T",                    7,  "Replacement is single-pass." },
  // Common patterns of use surrounding line-endings.
  { "Typical text layout.\n\nIt has double-spacing,\nas well as a terminal\nline ending.\n",
      "\n", "\r\n", "Typical text layout.\r\n\r\nIt has double-spacing,\r\nas well as a terminal\r\nline ending.\r\n", 5, "LF->CRLF",
      "\r\n", "\n", "Typical text layout.\n\nIt has double-spacing,\nas well as a terminal\nline ending.\n",           5, "CRLF->LF",
      "\n", "\n\t", "Typical text layout.\n\t\n\tIt has double-spacing,\n\tas well as a terminal\n\tline ending.\n\t", 5, "Block indentation." },
};


int test_stringbuilder_replace() {
  const int REPLACEMENT_CASE_COUNT = (sizeof(sb_replace_cases) / sizeof(sb_replace_case));
  StringBuilder stack_obj;
  int ret = -1;
  printf("Testing replace(const char*, const char*) with %d test cases (3 mutations each)...\n", REPLACEMENT_CASE_COUNT);

  int  case_idx     = 0;
  int  replacements = 0;
  bool test_failed  = false;
  char* search_term           = (char*) "";
  char* replacement_term      = (char*) "";
  char* expected_mutant       = (char*) "";
  int   expected_replacements = 0;

  while ((case_idx < REPLACEMENT_CASE_COUNT) & !test_failed) {
    printf("\tBeginning block %d...\n", case_idx);
    test_failed  = true;  // Force an ascent to the top of the pyramid of doom.
    search_term           = (char*) sb_replace_cases[case_idx].search_0;
    replacement_term      = (char*) sb_replace_cases[case_idx].replace_0;
    expected_mutant       = (char*) sb_replace_cases[case_idx].mutant_0;
    expected_replacements = sb_replace_cases[case_idx].replacements_0;
    const int INPUT_STR_LEN = strlen(sb_replace_cases[case_idx].input);
    printf("\t\tTest string has same length as the source (%d)... ", INPUT_STR_LEN);
    stack_obj.clear();
    stack_obj.concat(sb_replace_cases[case_idx].input);
    if (INPUT_STR_LEN == stack_obj.length()) {
      printf("Pass.\n\tTest block %d, case: %s... \n", case_idx, sb_replace_cases[case_idx].test_description_0);
      replacements = stack_obj.replace(search_term, replacement_term);
      printf("\t\treplace(\"%s\", \"%s\") return value of %d matches expectation (%d)... ", search_term, replacement_term, replacements, expected_replacements);
      if (expected_replacements == replacements) {
        printf("Pass.\n\t\treplace(\"%s\", \"%s\") produced the expected mutant... ", search_term, replacement_term);
        if (0 == StringBuilder::strcasecmp((char*)stack_obj.string(), expected_mutant)) {
          printf("Pass.\n\tTest block %d, case: %s... \n", case_idx, sb_replace_cases[case_idx].test_description_1);
          search_term           = (char*) sb_replace_cases[case_idx].search_1;
          replacement_term      = (char*) sb_replace_cases[case_idx].replace_1;
          expected_mutant       = (char*) sb_replace_cases[case_idx].mutant_1;
          expected_replacements = sb_replace_cases[case_idx].replacements_1;
          replacements = stack_obj.replace(search_term, replacement_term);
          printf("\t\treplace(\"%s\", \"%s\") return value of %d matches expectation (%d)... ", search_term, replacement_term, replacements, expected_replacements);
          if (expected_replacements == replacements) {
            printf("Pass.\n\t\treplace(\"%s\", \"%s\") produced the expected mutant... ", search_term, replacement_term);
            if (0 == StringBuilder::strcasecmp((char*)stack_obj.string(), expected_mutant)) {
              printf("Pass.\n\tTest block %d, case: %s... \n", case_idx, sb_replace_cases[case_idx].test_description_2);
              search_term           = (char*) sb_replace_cases[case_idx].search_2;
              replacement_term      = (char*) sb_replace_cases[case_idx].replace_2;
              expected_mutant       = (char*) sb_replace_cases[case_idx].mutant_2;
              expected_replacements = sb_replace_cases[case_idx].replacements_2;
              replacements = stack_obj.replace(search_term, replacement_term);
              printf("\t\treplace(\"%s\", \"%s\") return value of %d matches expectation (%d)... ", search_term, replacement_term, replacements, expected_replacements);
              if (expected_replacements == replacements) {
                printf("Pass.\n\t\treplace(\"%s\", \"%s\") produced the expected mutant... ", search_term, replacement_term);
                if (0 == StringBuilder::strcasecmp((char*)stack_obj.string(), expected_mutant)) {
                  printf("Pass.\n\t\tTest block %d passes.\n", case_idx);
                  test_failed  = false;
                }
              }
            }
          }
        }
      }
    }

    // Only advance the test case if it passed. Otherwise, we can't safely dump
    //   the test parameters in the failure handing after the loop.
    if (!test_failed) {
      case_idx++;
    }
  }

  if (test_failed) {
    StringBuilder log;
    printf("Fail.\n");
    printf("Case index %d failed.\n", case_idx);
    print_sb_metrics("Final Stack obj", &stack_obj);
    log.concat("\nExpected:\n");
    StringBuilder::printBuffer(&log, (uint8_t*) expected_mutant, strlen(expected_mutant));
    log.concat("\nProduced:\n");
    StringBuilder::printBuffer(&log, stack_obj.string(), stack_obj.length());
    printf("%s", (char*) log.string());
    ret = -1;
  }
  else {
    ret = 0;
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

  if (0 != return_value) {
    printf("\t Final Stack obj:          %s\n", stack_obj.string());
  }

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


/*
* Many use-cases that would otherwise need to call length() will be happy with
*  the (much cheaper) isEmpty().
*/
int test_stringbuilder_isempty() {
  int return_value = -1;
  printf("Testing isEmpty()...\n");
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
* Taking ownership of a buffer malloc'd from elsewhere.
*/
int test_stringbuilder_concat_handoff_raw() {
  int ret = -1;
  printf("Testing concatHandoff(uint8_t*, int)...\n");
  StringBuilder dest("Something already in the string. ");
  const int         BASE_STR_LENGTH      = dest.length();
  const char* const SOME_STRING_IN_FLASH = "Some string in flash.";
  const int         SOME_STRING_LENGTH   = strlen(SOME_STRING_IN_FLASH);
  const int         COMBINED_STR_LENGTH  = (BASE_STR_LENGTH + SOME_STRING_LENGTH);

  uint8_t* heap_ptr = (uint8_t*) malloc(SOME_STRING_LENGTH + 1);
  printf("\tHeap-allocating test string... ");
  if (nullptr != heap_ptr) {
    printf("Pass.\n\tAdding it to the existing StringBuilder should increase the count by 1 and the length to %d... ", COMBINED_STR_LENGTH);
    memcpy(heap_ptr, SOME_STRING_IN_FLASH, SOME_STRING_LENGTH);
    *(heap_ptr + SOME_STRING_LENGTH) = 0;
    dest.concatHandoff(heap_ptr, SOME_STRING_LENGTH);
    if ((2 == dest.count()) & (COMBINED_STR_LENGTH == dest.length())) {
      printf("Pass. Full memory cost is %d bytes.\n", dest.memoryCost(true));
      printf("\tCollapsing the SringBuilder should result in a heap free without crashing... ");
      dest.string();
      if ((1 == dest.count()) & (COMBINED_STR_LENGTH == dest.length())) {
        printf("Pass.\n\tconcatHandoff(uint8_t*, int) passes.\n");
        ret = 0;
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
    StringBuilder log;
    log.concatf("\ndest: (%d bytes) (%u frags)\n", dest.length(), dest.count());
    dest.printDebug(&log);
    log.concat("\n");
    printf("\n%s\n\n", (const char*) log.string());
  }
  return ret;
}


/*
* The structure-preserving ownership transfer functions.
*/
int test_stringbuilder_concat_handoff() {
  int ret = -1;
  printf("Testing concatHandoff(StringBuilder*)...\n");
  const uint32_t TEST_BUF_LEN = (20 + (randomUInt32() % 10));
  StringBuilder should_be_empty;
  StringBuilder should_have_things;
  generate_random_text_buffer(&should_be_empty, TEST_BUF_LEN);
  printf("\tGenerating test string (%d bytes): %s\n", TEST_BUF_LEN, (char*) should_be_empty.string());
  const uint8_t* PTR_MUTATION_CHECK_0 = should_be_empty.string();

  should_have_things.concatHandoff(&should_be_empty);
  printf("\tshould_be_empty.isEmpty() should return true... ");
  if (should_be_empty.isEmpty()) {
    printf("Pass.\n\tshould_be_empty.isEmpty(true) should return true... ");
    if (should_be_empty.isEmpty(true)) {
      if (PTR_MUTATION_CHECK_0 == should_have_things.string()) {
        printf("Pass.\n\tconcatHandoff(StringBuilder*) passes.\n");
        ret = 0;
      }
    }
  }

  if (0 != ret) {
    printf("Fail.\n");
  }
  return ret;
}


/*
* printBuffer(StringBuilder*, uint8_t*, uint32_t, const char*)
*/
int test_stringbuilder_print_buffer() {
  int ret = 0;
  printf("Testing printBuffer(StringBuilder*, uint8_t*, uint32_t, const char*)...\n");
  StringBuilder log;
  uint8_t buf[83];
  random_fill(buf, sizeof(buf));
  StringBuilder::printBuffer(&log, nullptr, 0, "\t");
  StringBuilder::printBuffer(&log, buf, sizeof(buf), "\t");
  printf("%s\n", (const char*) log.string());
  return ret;
}


/*
* The structure-preserving ownership transfer functions.
*/
int test_stringbuilder_concat_handoff_limit() {
  int ret = -1;
  printf("Testing concatHandoffLimit(StringBuilder*, unsigned int)...\n");
  const uint32_t TEST_BUF_LEN = (30 + (randomUInt32() % 10));
  const uint32_t LIMIT_LEN    = (5 + (randomUInt32() % 5));
  StringBuilder src;
  StringBuilder dest;
  generate_random_text_buffer(&src, TEST_BUF_LEN);
  printf("\tGenerating test string (%d bytes): %s\n", TEST_BUF_LEN, (char*) src.string());
  printf("\tconcatHandoffLimit() should take no action if passed a length of 0... ");
  dest.concatHandoffLimit(&src, 0);
  if ((dest.length() == 0) & (src.length() == TEST_BUF_LEN)) {
    printf("Pass.\n\tdest.length() should return %d... ", LIMIT_LEN);
    dest.concatHandoffLimit(&src, LIMIT_LEN);
    if (dest.length() == LIMIT_LEN) {
      const int REMAINING_SRC_LEN = (TEST_BUF_LEN - LIMIT_LEN);
      printf("Pass.\n\tsrc.length() should return %d... ", REMAINING_SRC_LEN);
      if (src.length() == REMAINING_SRC_LEN) {
        printf("Pass.\n\tconcatHandoffLimit() should be able to copy less than the directed length... ");
        dest.concatHandoffLimit(&src, TEST_BUF_LEN);
        if ((TEST_BUF_LEN == dest.length()) & (0 == src.length())) {
          printf("Pass.\n");
          dest.clear();
          const int      FRAGMENTS_IN_SRC   = 4;
          const int      FRAGMENTS_TO_MOVE  = 2;
          const uint32_t LIMIT_LEN_TIMES_4  = (LIMIT_LEN * FRAGMENTS_IN_SRC);
          printf("\tGenerating fragmented test string (%d bytes over %d fragments)... ", LIMIT_LEN_TIMES_4, FRAGMENTS_IN_SRC);
          while (src.count() < FRAGMENTS_IN_SRC) {  generate_random_text_buffer(&src, LIMIT_LEN);  }
          if ((src.length() == LIMIT_LEN_TIMES_4) & (src.count() == FRAGMENTS_IN_SRC)) {
            printf("Pass.\n\tLimit falling cleanly on the first fragment of a multipart source... ");
            dest.concatHandoffLimit(&src, LIMIT_LEN);
            if ((dest.length() == LIMIT_LEN) & (src.length() == (LIMIT_LEN_TIMES_4 - LIMIT_LEN))) {
              printf("Pass.\n\tAre the source and destimation counts (1 and %d) correct?... ", (FRAGMENTS_IN_SRC - 1));
              if ((dest.count() == 1) & (src.count() == (FRAGMENTS_IN_SRC - 1))) {
                printf("Pass.\n\tLimit falling cleanly on a middle fragment boundary... ");
                dest.clear();
                while (src.count() < FRAGMENTS_IN_SRC) {  generate_random_text_buffer(&src, LIMIT_LEN);  }
                dest.concatHandoffLimit(&src, (LIMIT_LEN * FRAGMENTS_TO_MOVE));
                if ((dest.length() == (LIMIT_LEN * FRAGMENTS_TO_MOVE)) & (src.length() == (LIMIT_LEN_TIMES_4 - (LIMIT_LEN * FRAGMENTS_TO_MOVE)))) {
                  printf("Pass.\n\tAre the source and destimation counts (%d and %d) correct?... ", (FRAGMENTS_IN_SRC - FRAGMENTS_TO_MOVE), FRAGMENTS_TO_MOVE);
                  if ((dest.count() == FRAGMENTS_TO_MOVE) & (src.count() == (FRAGMENTS_IN_SRC - FRAGMENTS_TO_MOVE))) {
                    dest.clear();
                    while (src.count() < FRAGMENTS_IN_SRC) {  generate_random_text_buffer(&src, LIMIT_LEN);  }
                    const int BYTES_TO_MOVE = (FRAGMENTS_TO_MOVE * LIMIT_LEN) + (2 + (randomUInt32() % (LIMIT_LEN - 4)));
                    printf("Pass.\n\tLimit falling in a messy place in the middle (%d byte offset)... ", BYTES_TO_MOVE);
                    dest.concatHandoffLimit(&src, BYTES_TO_MOVE);
                    if ((dest.length() == BYTES_TO_MOVE) & (src.length() == (LIMIT_LEN_TIMES_4 - BYTES_TO_MOVE))) {
                      const int DEST_SPLIT_FRAG_COUNT = (FRAGMENTS_TO_MOVE+1);
                      printf("Pass.\n\tAre the source and destimation counts (%d and %d) correct?... ", (FRAGMENTS_IN_SRC - FRAGMENTS_TO_MOVE), DEST_SPLIT_FRAG_COUNT);
                      if ((dest.count() == DEST_SPLIT_FRAG_COUNT) & (src.count() == (FRAGMENTS_IN_SRC - FRAGMENTS_TO_MOVE))) {
                        printf("Pass.\n\tconcatHandoffLimit(StringBuilder*, unsigned int) passes.\n");
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

  if (0 != ret) {
    printf("Fail.\n");
    StringBuilder log;
    log.concatf("\nsrc:  (%u bytes)\n", src.length());
    src.printDebug(&log);
    log.concat("\n");
    log.concatf("\ndest: (%u bytes)\n", dest.length());
    dest.printDebug(&log);
    log.concat("\n");
    printf("\n%s\n\n", (const char*) log.string());
  }
  return ret;
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
* StringBuilder test plan
* Testing a large class with concealed internal dependencies is a good use-case
*   for AsyncSequencer. The code below defines a test plan that accounts for
*   those hidden dependencies, and helps readability of both the tests, and the
*   results.
*******************************************************************************/
#define CHKLST_SB_TEST_STRCASESTR     0x00000001  // strcasestr(const char* haystack, const char* needle)
#define CHKLST_SB_TEST_STRCASECMP     0x00000002  // strcasecmp(const char*, const char*)
#define CHKLST_SB_TEST_BASICS         0x00000004  // concat(uint8_t*, int), prepend(uint8_t*, int), length(), clear()
#define CHKLST_SB_TEST_CMPBINSTRING   0x00000008  // cmpBinString(uint8_t*, int)
#define CHKLST_SB_TEST_CASE_CONVERT   0x00000010  // toUpper() and toLower()
#define CHKLST_SB_TEST_BYTEAT         0x00000020  // byteAt(const int)
#define CHKLST_SB_TEST_ISEMPTY        0x00000040  // isEmpty(const bool)
#define CHKLST_SB_TEST_LOCATE         0x00000080  // locate(const uint8_t*, int len, int start_offset)
#define CHKLST_SB_TEST_CONTAINS_1     0x00000100  // contains(char)
#define CHKLST_SB_TEST_CONTAINS_2     0x00000200  // contains(const char*)
#define CHKLST_SB_TEST_CULL_1         0x00000400  // cull(int length)
#define CHKLST_SB_TEST_CULL_2         0x00000800  // cull(int offset, int length)
#define CHKLST_SB_TEST_SPLIT          0x00001000  // split(const char*)
#define CHKLST_SB_TEST_IMPLODE        0x00002000  // implode(const char*)
#define CHKLST_SB_TEST_CHUNK          0x00004000  // chunk(const int)
#define CHKLST_SB_TEST_REPLACE        0x00008000  // replace(const char*, const char*)
#define CHKLST_SB_TEST_HANDOFFS_1     0x00010000  // concatHandoff(StringBuilder*), prependHandoff(StringBuilder*)
#define CHKLST_SB_TEST_HANDOFFS_2     0x00020000  // concatHandoff(uint8_t*, int)
#define CHKLST_SB_TEST_HANDOFFS_3     0x00040000  // concatHandoffLimit(uint8_t*, int)
#define CHKLST_SB_TEST_COUNT          0x00080000  // count()
#define CHKLST_SB_TEST_POSITION       0x00100000  // position(int) functions, and drop_position(unsigned int)
#define CHKLST_SB_TEST_CONCATF        0x00200000  // concatf(const char* format, va_list)
#define CHKLST_SB_TEST_PRINTDEBUG     0x00400000  // printDebug(StringBuilder*)
#define CHKLST_SB_TEST_PRINTBUFFER    0x00800000  // printBuffer(StringBuilder* output, uint8_t* buf, uint32_t len, const char* indent)
#define CHKLST_SB_TEST_MEM_MUTATION   0x01000000  // Memory layout non-mutation assurances.
#define CHKLST_SB_TEST_MISUSE         0x02000000  // Foreseeable misuse tests.
#define CHKLST_SB_TEST_MISCELLANEOUS  0x04000000  // Scattered small tests.

#define CHKLST_SB_TESTS_ALL ( \
  CHKLST_SB_TEST_STRCASESTR | CHKLST_SB_TEST_STRCASECMP | CHKLST_SB_TEST_BASICS | \
  CHKLST_SB_TEST_CMPBINSTRING | CHKLST_SB_TEST_CASE_CONVERT | CHKLST_SB_TEST_BYTEAT | \
  CHKLST_SB_TEST_ISEMPTY | CHKLST_SB_TEST_LOCATE | CHKLST_SB_TEST_CONTAINS_1 | \
  CHKLST_SB_TEST_CONTAINS_2 | CHKLST_SB_TEST_CULL_1 | CHKLST_SB_TEST_CULL_2 | \
  CHKLST_SB_TEST_SPLIT | CHKLST_SB_TEST_IMPLODE | CHKLST_SB_TEST_CHUNK | \
  CHKLST_SB_TEST_REPLACE | CHKLST_SB_TEST_HANDOFFS_1 | CHKLST_SB_TEST_HANDOFFS_2 | \
  CHKLST_SB_TEST_HANDOFFS_3 | \
  CHKLST_SB_TEST_COUNT | CHKLST_SB_TEST_POSITION | CHKLST_SB_TEST_CONCATF | \
  CHKLST_SB_TEST_PRINTDEBUG | CHKLST_SB_TEST_PRINTBUFFER | \
  CHKLST_SB_TEST_MEM_MUTATION | CHKLST_SB_TEST_MISUSE | CHKLST_SB_TEST_MISCELLANEOUS)

const StepSequenceList TOP_LEVEL_SB_TEST_LIST[] = {
  { .FLAG         = CHKLST_SB_TEST_STRCASESTR,
    .LABEL        = "strcasestr(const char*, const char*)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_strcasestr()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_STRCASECMP,
    .LABEL        = "strcasecmp(const char*, const char*)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_strcasecmp()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_BASICS,
    .LABEL        = "concat(uint8_t*, int), prepend(uint8_t*, int), length(), clear()",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_CMPBINSTRING,
    .LABEL        = "cmpBinString(uint8_t*, int)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_CASE_CONVERT,
    .LABEL        = "toUpper() and toLower()",
    .DEP_MASK     = (CHKLST_SB_TEST_CMPBINSTRING),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_case_shifter()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_BYTEAT,
    .LABEL        = "byteAt(const int)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_byteat()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_ISEMPTY,
    .LABEL        = "isEmpty(const bool)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_isempty()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_LOCATE,
    .LABEL        = "locate(const uint8_t*, int len, int)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_locate()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_CONTAINS_1,
    .LABEL        = "contains(char)",
    .DEP_MASK     = (CHKLST_SB_TEST_LOCATE),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_CONTAINS_2,
    .LABEL        = "contains(const char*)",
    .DEP_MASK     = (CHKLST_SB_TEST_LOCATE),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_CULL_1,
    .LABEL        = "cull(int)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_StringBuilderCull()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_CULL_2,
    .LABEL        = "cull(int, int)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_SPLIT,
    .LABEL        = "split(const char*)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_IMPLODE,
    .LABEL        = "implode(const char*)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_implode()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_CHUNK,
    .LABEL        = "chunk(const int)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_chunk()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_REPLACE,
    .LABEL        = "replace(const char*, const char*)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_replace()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_HANDOFFS_1,
    .LABEL        = "concatHandoff(StringBuilder*), prependHandoff(StringBuilder*)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_concat_handoff()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_HANDOFFS_2,
    .LABEL        = "concatHandoff(uint8_t*, int)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_concat_handoff_raw()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_HANDOFFS_3,
    .LABEL        = "concatHandoffLimit(StringBuilder*, unsigned int)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_concat_handoff_limit()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_COUNT,
    .LABEL        = "count()",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_POSITION,
    .LABEL        = "position(int) / drop_position(unsigned int)",
    .DEP_MASK     = (CHKLST_SB_TEST_COUNT),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_CONCATF,
    .LABEL        = "concatf(const char*, va_list)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_PRINTDEBUG,
    .LABEL        = "printDebug(StringBuilder*)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_PRINTBUFFER,
    .LABEL        = "printBuffer(StringBuilder*, uint8_t*, uint32_t, const char*)",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_stringbuilder_print_buffer()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_MEM_MUTATION,
    .LABEL        = "Memory layout non-mutation assurances",
    .DEP_MASK     = (CHKLST_SB_TEST_COUNT),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return 1;  }    // TODO: Separate
  },
  { .FLAG         = CHKLST_SB_TEST_MISUSE,
    .LABEL        = "Guardrails against misuse",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_misuse_cases()) ? 1:-1);  }
  },
  { .FLAG         = CHKLST_SB_TEST_MISCELLANEOUS,
    .LABEL        = "Scattered small tests",
    .DEP_MASK     = (0),
    .DISPATCH_FXN = []() { return 1;  },
    .POLL_FXN     = []() { return ((0 == test_StringBuilder()) ? 1:-1);  }
  },
};

AsyncSequencer sb_test_plan(TOP_LEVEL_SB_TEST_LIST, (sizeof(TOP_LEVEL_SB_TEST_LIST) / sizeof(TOP_LEVEL_SB_TEST_LIST[0])));


/*******************************************************************************
* The main function.
*******************************************************************************/

int stringbuilder_main() {
  int ret = 0;   // Failure is the default result.
  const char* const MODULE_NAME = "StringBuilder";

  sb_test_plan.requestSteps(CHKLST_SB_TESTS_ALL);
  while (!sb_test_plan.request_completed() && (0 == sb_test_plan.failed_steps(false))) {
    sb_test_plan.poll();
  }
  ret = (sb_test_plan.request_fulfilled() ? 0 : 1);

  StringBuilder report_output;
  sb_test_plan.printDebug(&report_output, "StringBuilder test report");
  printf("%s\n", (char*) report_output.string());

  return ret;
}
