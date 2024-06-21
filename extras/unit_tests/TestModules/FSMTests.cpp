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
  UNINIT =  0,
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
#define FSM_WAYPOINT_DEPTH  8


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

    inline bool isIdle() {   return ((StateTest::IDLE == currentState()) & _fsm_is_stable());   };
    inline int8_t poll() {   return _fsm_poll();     };

    void reset() {
      counter_state_0 = 0;
      counter_state_1 = 0;
      counter_state_2 = 0;
      counter_state_3 = 0;
      counter_idle    = 0;
      _fsm_reset(StateTest::UNINIT);
    };

    void printDebug(StringBuilder* output) {
      output->concat("State counts:\n");
      output->concatf("\tcounter_state_0:  %d\n", counter_state_0);
      output->concatf("\tcounter_state_1:  %d\n", counter_state_1);
      output->concatf("\tcounter_state_2:  %d\n", counter_state_2);
      output->concatf("\tcounter_state_3:  %d\n", counter_state_3);
      output->concatf("\tcounter_idle:     %d\n", counter_idle);
      output->concatf(
        "\nImplementing a StateMachine on a novel enum costs:\n\t%u bytes of RAM\n\t%u bytes that can be segregated to flash\n",
        (sizeof(Example_FSM) + FSM_WAYPOINT_DEPTH), sizeof(FSM_STATE_LIST)
      );
    };

    bool counters_match(const int A, const int B, const int C, const int D, const int I) {
      bool ret = (A == counter_state_0);
      ret &= (B == counter_state_1);
      ret &= (C == counter_state_2);
      ret &= (D == counter_state_3);
      ret &= (I == counter_idle);
      return ret;
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

    // Things like hardware drivers often have several sophisticated things they
    //   need to do before being able to present themselves as ready-for-use.
    int example_init() {
      return _fsm_set_route(3, StateTest::STATE_0, StateTest::STATE_1, StateTest::IDLE);
    };

    // After the class has passed through its init stages, high-level asynchronous
    //   requests can be wrapped up neatly...
    int run_business_loop() {
      if (isIdle()) {
        return _fsm_append_route(3, StateTest::STATE_2, StateTest::STATE_3, StateTest::IDLE);
      }
      return -1;
    };

    // After the class has passed through its init stages, high-level asynchronous
    //   requests can be wrapped up neatly...
    int run_report_loop() {
      return _fsm_append_route(2, StateTest::STATE_3, StateTest::IDLE);
    };


  private:
    int counter_state_0 = 0;
    int counter_state_1 = 0;
    int counter_state_2 = 0;
    int counter_state_3 = 0;
    int counter_idle    = 0;

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
  int8_t ret = 0;
  bool fsm_advance = false;
  if (_fsm_is_waiting()) {
    return fsm_advance;
  }

  switch (currentState()) {
    // Exit conditions:
    case StateTest::UNINIT:    fsm_advance = true;               break;
    case StateTest::IDLE:      fsm_advance = !_fsm_is_stable();  break;

    case StateTest::STATE_0:
    case StateTest::STATE_1:
    case StateTest::STATE_2:
    case StateTest::STATE_3:
      fsm_advance = flip_coin();
      break;

    default:   // Can't exit from an unknown state.
      ret = -1;
      break;
  }

  // If the current state's exit criteria is met, we advance the FSM.
  if (fsm_advance & (-1 != ret)) {
    ret = (0 == _fsm_advance()) ? 1 : 0;
  }
  return ret;
}


int8_t Example_FSM::_fsm_set_position(StateTest new_state) {
  int8_t ret = -1;
  bool state_entry_success = false;   // Succeed by default.
  switch (new_state) {
    // Entry conditions:
    case StateTest::UNINIT:    state_entry_success = true;    break;
    case StateTest::IDLE:      state_entry_success = true;    break;

    case StateTest::STATE_0:
    case StateTest::STATE_1:
    case StateTest::STATE_2:
    case StateTest::STATE_3:
      state_entry_success = flip_coin();
      break;

    default:
      break;
  }

  if (state_entry_success) {
    ret = 0;  // By returning 0, the FSM template will update the states.
    switch (new_state) {
      case StateTest::IDLE:     counter_idle++;     break;
      case StateTest::STATE_0:  counter_state_0++;  break;
      case StateTest::STATE_1:  counter_state_1++;  break;
      case StateTest::STATE_2:  counter_state_2++;  break;
      case StateTest::STATE_3:  counter_state_3++;  break;
      default:  break;
    }
  }
  return ret;
}


// Declare an instance of our test driver.
Example_FSM test_driver;



/*******************************************************************************
* EnumDefList test routines
*******************************************************************************/

int8_t test_enumlist_catchall() {
  int8_t ret = -1;
  printf("Testing EnumDefList...\n");
  int8_t enum_found = 0;
  printf("\tAsking for an enum that doesn't exist ought to return our designated catch-all... ");
  if (StateTest::INVALID == FSM_STATE_LIST.getEnumByStr("NON-EXISTANT-STATE", &enum_found)) {
    printf("Pass.\n\tenum_found parameter was properly modified by reference... ");
    if (0 == enum_found) {
      printf("Pass.\n\tEnumDefList tests pass.\n");
      ret = 0;
    }
  }
  if (0 != ret) {  printf("Fail.\n");  }
  return ret;
}


/*******************************************************************************
* StateMachine test routines
*******************************************************************************/
int8_t fsm_test_poll_until_idle() {
  int max_count = 50000;
  while (0 < max_count--) {
    test_driver.poll();
    if (test_driver.isIdle()) {
      return 0;
    }
  }
  return -1;
}



int test_fsm_init() {
  int ret = -1;
  int ret_local = -1;
  printf("Testing StateMachine<StateTest> basics...\n");
  // The StateMachine object doesn't require explicit init, so it should come up
  //   in the state we declared.
  printf("\tStateMachine is constructed with the correct initial state... ");
  if (StateTest::UNINIT == test_driver.currentState()) {
    printf("Pass.\n\tThe state machine's init state matches expectations... ");
    ret_local = test_driver.test_passed_init_state();
    if (0 == ret_local) {
      printf("Pass.\n\tStateMachine<StateTest> basic tests pass.\n");
      ret = 0;
    }
  }

  if (0 != ret) {
    printf("Fail (ret_local = %d).\n", ret_local);
    StringBuilder output;
    test_driver.printFSM(&output);
    printf("%s\n", (char*) output.string());
  }
  return ret;
}


int test_fsm_evolution() {
  int ret = -1;
  printf("Testing StateMachine<StateTest> evolution...\n");
  printf("\tStateMachine::_fsm_set_route() accepts new states... ");
  if (0 == test_driver.example_init()) {
    printf("Pass.\n\tTest class is no longer IDLE... ");
    if (!test_driver.isIdle()) {
      printf("Pass.\n\tPolling the FSM eventually returns to IDLE... ");
      if (0 == fsm_test_poll_until_idle()) {
        printf("Pass.\n\tAll impacted states were hit exactly once... ");
        if (test_driver.counters_match(1, 1, 0, 0, 1)) {
          printf("Pass.\n\tStateMachine::_fsm_append_route() accepts new states... ");
          if (0 == test_driver.run_business_loop()) {
            if (0 == test_driver.run_report_loop()) {
              printf("Pass.\n\tPolling the FSM eventually returns to IDLE... ");
              if (0 == fsm_test_poll_until_idle()) {
                printf("Pass.\n\tAll impacted states were hit the expected number of times... ");
                if (test_driver.counters_match(1, 1, 1, 2, 3)) {
                  printf("Pass.\n\treset() works... ");
                  test_driver.reset();
                  if (test_driver.counters_match(0, 0, 0, 0, 0)) {
                    if (StateTest::UNINIT == test_driver.currentState()) {
                      printf("Pass.\n\tState evolution tests pass.\n");
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

  if (0 != ret) {
    printf("Fail.\n");
  }
  StringBuilder output;
  test_driver.printFSM(&output);
  test_driver.printDebug(&output);
  printf("\n%s\n", (char*) output.string());
  return ret;
}


void print_types_state_machine() {
  printf("\tEnumDefList<StateTest>   %u\t%u\n", sizeof(EnumDefList<StateTest>),   alignof(EnumDefList<StateTest>));
  printf("\tStateMachine<StateTest>  %u\t%u\n", sizeof(StateMachine<StateTest>),  alignof(StateMachine<StateTest>));
  printf("\tFSM_STATE_LIST           %u\t%u\n", sizeof(FSM_STATE_LIST),           alignof(FSM_STATE_LIST));
  printf("\tExample_FSM              %u\t%u\n", sizeof(Example_FSM),  alignof(Example_FSM));
}


/*******************************************************************************
* The main function.
*******************************************************************************/
int fsm_test_main() {
  int ret = 1;   // Failure is the default result.
  const char* const MODULE_NAME = "StateMachine";
  printf("===< %s >=======================================\n", MODULE_NAME);

  if (0 == test_enumlist_catchall()) {
    if (0 == test_fsm_init()) {
      if (0 == test_fsm_evolution()) {
        printf("StateMachine tests all pass\n");
        ret = 0;
      }
    }
  }

  return ret;
}
