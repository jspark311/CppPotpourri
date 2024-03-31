/*
File:   ParsingConsoleTest.cpp
Author: J. Ian Lindsay
Date:   2021.04.24


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


*/


#define TOTAL_TEST_COUNT   8


/*******************************************************************************
* Globals
*******************************************************************************/
ParsingConsole console(128);   // This is the console object under test.

static bool test_result_array[TOTAL_TEST_COUNT] = {false, };
static uint32_t test_result_count = 0;


/*******************************************************************************
* Console callbacks
*******************************************************************************/

int callback_test1(StringBuilder* text_return, StringBuilder* args) {
  console.printHistory(text_return);
  test_result_array[0] = true;
  text_return->concat("CALLBACK ENTERED: callback_test1\n");
  return 0;
}

int callback_test2(StringBuilder* text_return, StringBuilder* args) {
  test_result_array[1] = true;
  text_return->concat("CALLBACK ENTERED: callback_test2\n");
  return 0;
}

int callback_test3(StringBuilder* text_return, StringBuilder* args) {
  test_result_array[2] = true;
  text_return->concat("CALLBACK ENTERED: callback_test3\n");
  return 0;
}

int callback_test4(StringBuilder* text_return, StringBuilder* args) {
  test_result_array[3] = true;
  text_return->concat("CALLBACK ENTERED: callback_test4\n");
  return 0;
}

int callback_test5(StringBuilder* text_return, StringBuilder* args) {
  if (0 < args->count()) {
    console.printHelp(text_return, args->position_trimmed(0));
  }
  else {
    console.printHelp(text_return);
  }
  test_result_array[4] = true;
  text_return->concat("CALLBACK ENTERED: callback_test5\n");
  return 0;
}

int callback_test6(StringBuilder* text_return, StringBuilder* args) {
  test_result_array[5] = true;
  test_result_count++;
  text_return->concat("CALLBACK ENTERED: callback_test6\n");
  return 0;
}



/*
* These. Just.
*/
const ConsoleCommand commands_that_should_be_added[] = {
  ConsoleCommand("test1",  '1', "Test callback #1", "Detailed help for test1", 0, callback_test1),
  ConsoleCommand("test2",  '2', "Test callback #2", "Detailed help for test2", 0, callback_test2),
  ConsoleCommand("test3",  '3', "Test callback #3", "Detailed help for test3", 0, callback_test3),
  ConsoleCommand("test4",  '4', "Test callback #4", "Detailed help for test4", 2, callback_test4)
};
const ConsoleCommand cmd5("test5",  '5', "Test callback #5", "Detailed help for test5", 0, callback_test5);


/*******************************************************************************
* Console test routines
*******************************************************************************/

int feed_console_bytewise(const char* str) {
  const int STR_LEN_TO_SEND = strlen(str);
  int ret = -1;
  StringBuilder temp_buf;
  for (int i = 0; i < STR_LEN_TO_SEND; i++) {
    temp_buf.concat(*(str + i));
    int console_ret = console.pushBuffer(&temp_buf);
    switch (console_ret) {
      case -1:   // console buffered the data, but took no other action.
      default:
        if (STR_LEN_TO_SEND == (i+1)) {
          // If this is the last character to be fed, this is the wrong answer.
          ret = -2;
        }
        break;
      case 0:   // A full line came in.
      case 1:   // A callback was called.
        ret = console_ret;
        break;
    }
    temp_buf.clear();
  }
  return ret;
}


/**
* Configures the console and adds commands.
*/
int setup_console(StringBuilder* output) {
  printf("\tParsingConsole: Setup and command definition...\n");
  int8_t ret = -1;
  test_result_array[6] = true;
  test_result_array[7] = true;
  console.setRXTerminator(LineTerm::LF);
  console.localEcho(false);  // We do not want local echo for testing.

  printf("\t\tdefineCommands(*, length) accepts a block of many command definitions... ");
  if (0 == console.defineCommands(commands_that_should_be_added, 4)) {
    ret--;
    printf("Pass.\n\t\tdefineCommands(*) accepts a single command definition... ");
    if (0 == console.defineCommand(&cmd5)) {
      ret--;
      printf("Pass.\n\t\tdefineCommands() accepts an inline command definition... ");
      if (0 == console.defineCommand("test6", '6', "Test callback #6", "Detailed help for test6", 0, callback_test6)) {
        printf("Pass.\n\t\tinit() returns success... ");
        if (0 == console.init()) {
          printf("Pass.\n\tsetup_console() passed.\n");
          ret = 0;
        }
      }
    }
  }

  console.fetchLog(output);
  return ret;
}


/**
* Issues commands to execute test battery.
*/
int run_command_tests(StringBuilder* output) {
  int8_t ret = -1;
  bool continue_testing = true;
  uint32_t idx = 0;
  const char* BYTEWISE_TESTS[] = {
    "test6\n",   // Should result in a callback.
    "teST6\n",   // Should result in a callback.
    "TesT6  \n", // Should result in a callback.
    "  teST6\n", // Should result in a callback.
    "test5\n",   // Should result in a callback.
    "test4\n",   // Should result in a response for insufficient arg count.
    "test4 545 678 422\n",  // Should result in a callback.
    "1\n",       // Should result in a callback.
    "bogus\n"    // Should result in a response for unknown command.
  };

  while (continue_testing && (idx < (sizeof(BYTEWISE_TESTS) / sizeof(const char*)))) {
    if (1 != feed_console_bytewise(BYTEWISE_TESTS[idx])) {
      continue_testing = false;
    }
    idx++;
  }

  if (continue_testing) {
    StringBuilder multi_cmd_buf("test2\ntest3\n");
    if (1 == console.pushBuffer(&multi_cmd_buf)) {  // Should result in two callbacks.
      output->concat("run_command_tests() passed.\n");
      ret = 0;
    }
    else output->concat("pushBuffer() failed.\n");
  }
  else output->concat("Command test loop aborted early.\n");

  console.fetchLog(output);
  return ret;
}


int run_history_tests(StringBuilder* output) {
  int8_t ret = -1;
  const uint8_t HISTORY_DEPTH = console.historyDepth();
  if (0 < console.maxHistoryDepth()) {
    ret--;
    if (0 < console.historyDepth()) {   // We should have some history by now.
      ret--;
      console.clearHistory();
      if (0 == console.historyDepth()) {   // NOW history ought to be empty.
        ret = 0;
      }
      else output->concat("History should be empty after clearHistory(), but isn't.\n");
    }
    else output->concat("History is empty, and should not be.\n");
  }
  else output->concat("Maximum history depth is wrong.\n");

  console.fetchLog(output);
  return ret;
}



void print_types_parsing_console() {
  printf("\tParsingConsole        %u\t%u\n", sizeof(ParsingConsole), alignof(ParsingConsole));
  printf("\tConsoleCommand        %u\t%u\n", sizeof(ConsoleCommand), alignof(ConsoleCommand));
}




/*******************************************************************************
* The main function.
*******************************************************************************/
int parsing_console_main() {
  int ret = 1;   // Failure is the default result.
  const char* const MODULE_NAME = "ParsingConsole";
  printf("===< %s >=======================================\n", MODULE_NAME);

  StringBuilder log;
  if (0 == setup_console(&log)) {
    if (0 == run_command_tests(&log)) {
      if (0 == run_history_tests(&log)) {
        bool test_result_union = true;
        uint32_t idx = 0;
        while (test_result_union & (idx < TOTAL_TEST_COUNT)) {
          test_result_union &= test_result_array[idx];
          if (!test_result_union) {
            StringBuilder tmp_sb;
            tmp_sb.concatf("test_result_array[%d]", idx);
            printTestFailure(MODULE_NAME, (char*) tmp_sb.string());
          }
          idx++;
        }
        if (test_result_union) {
          if (4 == test_result_count) {
            log.concat("**********************************\n");
            log.concat("*  ParsingConsole tests all pass *\n");
            log.concat("**********************************\n");
            ret = 0;
          }
          else log.concatf("Callback for test6 was called %d times. This is wrong.\n", test_result_count);
        }
      }
      else printTestFailure(MODULE_NAME, "run_history_tests()");
    }
    else printTestFailure(MODULE_NAME, "run_command_tests()");
  }
  else printTestFailure(MODULE_NAME, "setup_console()");

  if (0 < log.length()) {
    printf("%s\n\n", (const char*) log.string());
  }
  return ret;
}
