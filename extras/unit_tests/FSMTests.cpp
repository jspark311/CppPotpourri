/*
File:   FSMTests.cpp
Author: J. Ian Lindsay
Date:   2023.02.13

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


This program tests StateMachine<T>, which underpins many drivers and applications.
*/

#include "FiniteStateMachine.h"


/*******************************************************************************
* Enum support
* We'll need an enum to represent the states used for the test.
*******************************************************************************/
enum class StateTest : uint8_t {
  UNINIT =  0,  // The init state of the board is unknown.
  STATE_0,
  STATE_1,
  IDLE,
  STATE_2,
  STATE_3,
  INVALID
};

const EnumDef<StateTest> _ENUM_LIST[] = {
  { StateTest::UNINIT,  "UNINIT"},
  { StateTest::STATE_0, "STATE_0"},
  { StateTest::STATE_1, "STATE_1"},
  { StateTest::IDLE,    "IDLE"},
  { StateTest::STATE_2, "STATE_2"},
  { StateTest::STATE_3, "STATE_3"},
  { StateTest::INVALID, "INVALID", (ENUM_WRAPPER_FLAG_CATCHALL)}
};
const EnumDefList<StateTest> FSM_STATE_LIST(&_ENUM_LIST[0], (sizeof(_ENUM_LIST) / sizeof(_ENUM_LIST[0])));


/*******************************************************************************
* StateMachine variables
*******************************************************************************/
#define FSM_WAYPOINT_DEPTH  16


// The state machine was meant to be extended. Compositional use is
//   straight-forward, but we won't cover that here.
// We define an example driver class that needs an FSM of some sort to match
//   the hardware.
class Example_FSM : public StateMachine<StateTest> {
  public:
    // The example driver will be named, take a list of defined states. The first
    //   state is declared to be UNINIT, and we expect to plan out no more than
    //   FSM_WAYPOINT_DEPTH states in advance.
    Example_FSM() : StateMachine<StateTest>(
      "Example_FSM", &FSM_STATE_LIST, StateTest::UNINIT, FSM_WAYPOINT_DEPTH
    ) {};
    ~Example_FSM() {};

    inline bool isIdle() {   return (StateTest::IDLE == currentState());   };
    inline int8_t poll() {   return _fsm_poll();     };

    void dumpTypeSizes(StringBuilder* output) {
      output->concatf("EnumDef<StateTest>       %u\n", sizeof(EnumDef<StateTest>));
      output->concatf("EnumDefList<StateTest>   %u\n", sizeof(EnumDefList<StateTest>));
      output->concatf("StateMachine<StateTest>  %u\n", sizeof(StateMachine<StateTest>));
      output->concatf(
        "Implementing a StateMachine on a novel enum costs:\n\t%u bytes of RAM\n\t%u bytes that can be segregated to flash\n",
        (sizeof(Example_FSM) + FSM_WAYPOINT_DEPTH),
        sizeof(FSM_STATE_LIST)
      );
    };

    int8_t test_passed_init_state() {
      int ret = -1;
      if (_fsm_is_stable()) {
        ret--;
        _fsm_lockout(0);
        if (!_fsm_is_waiting()) {
          ret = 0;
        }
      }
      return ret;
    };


  private:
    /* Mandatory overrides from StateMachine. */
    // These are the required functions to use the state machine. They ask the
    //   the driver implementation if it is ok to leave the current state (_fsm_poll()),
    //   and if the next state was successfully entered (_fsm_set_position(<T>)).
    // These are pure-virtual in the template, so if they are not implemented on the
    //   same enum as those that were passed in as definition, compilation will fail.
    int8_t _fsm_poll();                  // Polling for state exit.
    int8_t _fsm_set_position(StateTest); // Attempt a state entry.
};



int8_t Example_FSM::_fsm_poll() {
  int8_t ret = -1;
  return ret;
}


int8_t Example_FSM::_fsm_set_position(StateTest new_state) {
  int8_t ret = -1;
  return ret;
}


// Declare an instance of our test driver.
Example_FSM test_driver;



/*******************************************************************************
* EnumDefList test routines
*******************************************************************************/

int8_t test_enumlist_catchall() {
  int8_t ret = -1;
  int8_t enum_found = 0;
  // Asking for an enum that doesn't exist ought to return our designated catch-all.
  if (StateTest::INVALID == FSM_STATE_LIST.getEnumByStr("NON-EXISTANT-STATE", &enum_found)) {
    ret--;
    if (0 == enum_found) {
      ret = 0;
    }
    else printf("getEnumByStr() returned the catch-all, but found was set to an affirmative value.\n");
  }
  else printf("getEnumByStr() failed to return the catch-all.\n");
  return ret;
}


/*******************************************************************************
* StateMachine test routines
*******************************************************************************/


int test_fsm_init() {
  int ret = -1;
  int ret_local = -1;
  // The StateMachine object doesn't require explicit init, so it should come up
  //   in the state we declared.
  if (StateTest::UNINIT == test_driver.currentState()) {
    ret--;
    ret_local = test_driver.test_passed_init_state();
    if (0 == ret_local) {
      ret = 0;
    }
    else printf("test_passed_init_state() returned %d.\n", ret_local);
  }
  else printf("currentState() is not UNINIT.\n");

  if (0 != ret) {
    StringBuilder output;
    test_driver.printFSM(&output);
    printf("%s\n", (char*) output.string());
  }
  return ret;
}


int test_fsm_execution_to_idle() {
  int ret = 0;
  return ret;
}



/*******************************************************************************
* The main function.
*******************************************************************************/
int fsm_test_main() {
  int ret = 1;   // Failure is the default result.
  if (0 == test_enumlist_catchall()) {
    if (0 == test_fsm_init()) {
      if (0 == test_fsm_execution_to_idle()) {
        printf("**********************************\n");
        printf("*  StateMachine tests all pass   *\n");
        printf("**********************************\n");
        ret = 0;
      }
      else printTestFailure("Execution to IDLE");
    }
    else printTestFailure("FSM initial states");
  }
  else printTestFailure("Enum catch-all");

  return ret;
}