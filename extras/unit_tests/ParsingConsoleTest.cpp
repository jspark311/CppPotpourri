/*
File:   ParsingConsoleTest.cpp
Author: J. Ian Lindsay
Date:   2021.04.24

TODO: This test does not yet cover:
  * Line endings for RX/TX
  * Argument type parsing


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


#define TOTAL_TEST_COUNT   8


/*******************************************************************************
* Globals
*******************************************************************************/
ParsingConsole console(128);   // This is the console object under test.

static bool test_result_array[TOTAL_TEST_COUNT] = {false, };
static uint test_result_count = 0;


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

int console_error_callback(StringBuilder* text_return, const ConsoleErr err, const ConsoleCommand* cmd, StringBuilder* split) {
  text_return->concatf("CALLBACK ENTERED: console_error_callback(%s)\n", ParsingConsole::errToStr(err));
  switch (err) {
    case ConsoleErr::NONE:
      break;
    case ConsoleErr::NO_MEM:
      printf("Test fails hard due to NO_MEM error callback.\n");
      exit(1);
    case ConsoleErr::MISSING_ARG:      // We induce this on purpose.
      test_result_array[7] = true;
      break;
    case ConsoleErr::INVALID_ARG:
      break;
    case ConsoleErr::CMD_NOT_FOUND:    // We induce this on purpose.
      test_result_array[6] = true;
      break;
    case ConsoleErr::RESERVED:   // This is a general failure condition that ought to fail the test.
      printf("Test fails hard due to RESERVED error callback.\n");
      exit(1);
  }
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
    int console_ret = console.provideBuffer(&temp_buf);
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
  int8_t ret = -1;
  console.errorCallback(console_error_callback);
  console.setRXTerminator(LineTerm::LF);
  console.setTXTerminator(LineTerm::CRLF);
  console.localEcho(false);  // We do not want local echo for testing.

  if (0 == console.defineCommands(commands_that_should_be_added, 4)) {
    ret--;
    if (0 == console.defineCommand(&cmd5)) {
      ret--;
      if (0 == console.defineCommand("test6", '6', "Test callback #6", "Detailed help for test6", 0, callback_test6)) {
        if (0 == console.init()) {
          output->concat("setup_console() passed.\n");
          ret = 0;
        }
        else output->concat("Failed to console.init().\n");
      }
      else output->concat("Failed to console.defineCommand() explicitly.\n");
    }
    else output->concat("Failed to console.defineCommand() by reference.\n");
  }
  else output->concat("Failed to console.defineCommands().\n");

  console.fetchLog(output);
  return ret;
}


/**
* Issues commands to execute test battery.
*/
int run_command_tests(StringBuilder* output) {
  int8_t ret = -1;
  uint idx = 0;
  bool continue_testing = true;
  const char* BYTEWISE_TESTS[] = {
    "test6\n",   // Should result in a callback.
    "teST6\n",   // Should result in a callback.
    "TesT6  \n", // Should result in a callback.
    "  teST6\n", // Should result in a callback.
    "test5\n",   // Should result in a callback.
    "test4\n",   // Should result in an callback for insufficient arg count.
    "test4 545 678 422\n",  // Should result in a callback.
    "1\n",       // Should result in a callback.
    "bogus\n"    // Should result in an callback for unknown command.
  };

  while (continue_testing && (idx < (sizeof(BYTEWISE_TESTS) / sizeof(const char*)))) {
    if (1 != feed_console_bytewise(BYTEWISE_TESTS[idx])) {
      continue_testing = false;
    }
    idx++;
  }

  if (continue_testing) {
    StringBuilder multi_cmd_buf("test2\ntest3\n");
    if (1 == console.provideBuffer(&multi_cmd_buf)) {  // Should result in two callbacks.
      output->concat("run_command_tests() passed.\n");
      ret = 0;
    }
    else output->concat("provideBuffer() failed.\n");
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



/*******************************************************************************
* The main function.
*******************************************************************************/
int parsing_console_main() {
  int ret = 1;   // Failure is the default result.
  StringBuilder log;
  if (0 == setup_console(&log)) {
    if (0 == run_command_tests(&log)) {
      if (0 == run_history_tests(&log)) {
        bool test_result_union = true;
        uint idx = 0;
        while (test_result_union & (idx < TOTAL_TEST_COUNT)) {
          test_result_union &= test_result_array[idx];
          if (!test_result_union) {
            log.concatf("FAILED test %d.\n", idx);
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
      else printTestFailure("run_history_tests()");
    }
    else printTestFailure("run_command_tests()");
  }
  else printTestFailure("setup_console()");

  if (0 < log.length()) {
    printf("%s\n\n", (const char*) log.string());
  }
  return ret;
}
