/*
File:   FiniteStateMachine.h
Author: J. Ian Lindsay
Date:   2021.05.03

Template for a (very) finite state machine with enum controlled states.
*/

#ifndef __C3P_FINITE_STATE_MACHINE
#define __C3P_FINITE_STATE_MACHINE

#include <stdlib.h>
#include "EnumWrapper.h"
#include "RingBuffer.h"
#include "StringBuilder.h"
#include "AbstractPlatform.h"



/*
* A template for enum-controlled finite state machines with route storage and
*   rate-limiting.
*/
template <class T> class StateMachine {
  public:
    /**
    * Constructor
    */
    StateMachine(
      const char* FSM_NAME, const EnumDefList<T>* const EDEFS, const T I_STATE, const unsigned int MAX_DEPTH
    ) :
      _NAME(FSM_NAME), _ENUM_DEFS(EDEFS),
      _lockout_timer(0), _slowdown_ms(0), _waypoints(MAX_DEPTH),
      _current_state(I_STATE), _prior_state(I_STATE), _fsm_flags(0) {};

    ~StateMachine() {};

    T priorState() {        return _prior_state;     };
    T currentState() {      return _current_state;   };

    void printFSM(StringBuilder* output);
    int8_t fsm_console_handler(StringBuilder*, StringBuilder*);  // TODO: Would be better if private.


  protected:
    /*
    * These functions are not provided by this header, and will need to be
    *   implemented by the class that makes this template a concrete object.
    */
    virtual int8_t   _fsm_poll()          =0;  // Polling for state exit.
    virtual int8_t   _fsm_set_position(T) =0;  // Attempt a state entry.

    /* State machine functions usable by the extending class. */
    int8_t   _fsm_set_route(int count, ...);
    int8_t   _fsm_append_route(int count, ...);
    int8_t   _fsm_prepend_state(T);

    int8_t   _fsm_advance();

    inline void     _fsm_lockout(uint32_t x) {  _lockout_timer.reset(x);            };
    inline uint32_t _fsm_lockout() {            return _lockout_timer.remaining();  };
    inline bool     _fsm_is_waiting() {         return !_lockout_timer.expired();   };
    inline T        _fsm_pos_next() {           return _waypoints.get(false);       };
    inline bool     _fsm_is_stable() {          return (0 == _waypoints.count());   };
    inline uint32_t _fsm_slowdown() {           return _lockout_timer.period();     };

    void   _fsm_mark_current_state(T new_state) {
      _prior_state   = _current_state;
      _current_state = new_state;
    };


  private:
    const char*                 _NAME;
    const EnumDefList<T>* const _ENUM_DEFS;
    PeriodicTimeout             _lockout_timer;   // Used to enforce a delay between state transitions.
    uint32_t                    _slowdown_ms;
    RingBuffer<T>               _waypoints;
    T                           _current_state;
    T                           _prior_state;
    uint8_t                     _fsm_flags;
};




/******************************************************************************
* StateMachine template follows...
******************************************************************************/

/**
* Internal function responsible for advancing the state machine.
* NOTE: This function does no checks for IF the FSM should move forward. It only
*   performs the actions required to do it.
*
* @return 0 on state change, -1 otherwise.
*/
template <class T> int8_t StateMachine<T>::_fsm_advance() {
  int8_t ret = -1;
  if (0 < _waypoints.count()) {
    T STACKED_ENUM = _waypoints.get(false);
    if (0 == _fsm_set_position(STACKED_ENUM)) {
      STACKED_ENUM = _waypoints.get();
      ret = 0;
      if (0 < _slowdown_ms) {
        // Be sure to preserve at least as much margin as the transistion code
        //   might have asked for by calling _fsm_lockout(uint32).
        _lockout_timer.reset(strict_max(_slowdown_ms, _lockout_timer.remaining()));
      }
    }
  }
  return ret;
}


/**
* This function checks each state codes for validity, but does not error-check
*   the validity of the FSM traversal route specified in the arguments. It just
*   adds them to the list if they all correspond to valid state codes.
* This function will accept a maximum of _waypoints.count() arguments, and
*   will clobber the contents of that member if the call succeeds. Arguments
*   provided in excess of the limit will be truncated with no error.
*
* @return 0 on success, -1 on no params, -2 on invalid FSM code.
*/
template <class T> int8_t StateMachine<T>::_fsm_set_route(int arg_count, ...) {
  int8_t ret = -1;
  const int32_t PARAM_COUNT = strict_min((int32_t) arg_count, (int32_t) _waypoints.capacity());
  if (PARAM_COUNT > 0) {
    va_list args;
    va_start(args, arg_count);
    T test_values[PARAM_COUNT];
    ret = 0;
    for (int i = 0; i < PARAM_COUNT; i++) {
      test_values[i] = (T) va_arg(args, int);
    }
    va_end(args);   // Close out the va_args, and error-check each value.
    for (int i = 0; i < PARAM_COUNT; i++) {
      if (!_ENUM_DEFS->enumValid(test_values[i])) {
        return -2;
      }
    }
    if (0 == ret) {
      // If everything looks good, clear the state traversal list and add the
      //   new route to it.
      _waypoints.clear();
      for (int i = 0; i < PARAM_COUNT; i++) {
        _waypoints.insert(test_values[i]);
      }
    }
  }
  return ret;
}


/**
* This function checks each state code for validity, but does not error-check
*   the validity of the FSM traversal route specified in the arguments. It just
*   adds them to the list if they all correspond to valid state codes.
* This function will accept a maximum of _waypoints.count() arguments, and
*   will append to the contents of that member if the call succeeds. Arguments
*   provided in excess of the limit will be truncated with no error.
*
* @return 0 on success, -1 on no params, -2 on invalid FSM code.
*/
template <class T> int8_t StateMachine<T>::_fsm_append_route(int arg_count, ...) {
  int8_t ret = -1;
  const int32_t PARAM_COUNT = strict_min((int32_t) arg_count, (int32_t) (_waypoints.capacity() - _waypoints.count()));
  if (PARAM_COUNT > 0) {
    va_list args;
    va_start(args, arg_count);
    T test_values[PARAM_COUNT];
    ret = 0;
    for (int i = 0; i < PARAM_COUNT; i++) {
      test_values[i] = (T) va_arg(args, int);
    }
    va_end(args);   // Close out the va_args, and error-check each value.
    for (int i = 0; i < PARAM_COUNT; i++) {
      if (!_ENUM_DEFS->enumValid(test_values[i])) {
        return -2;
      }
    }
    if (0 == ret) {
      // If everything looks good, append the new route to the state traversal list.
      for (int i = 0; i < PARAM_COUNT; i++) {
        _waypoints.insert(test_values[i]);
      }
    }
  }
  return ret;
}


/**
* This function checks the state code for validity, but does not error-check
*   the validity of the FSM traversal route specified in the argument. It just
*   adds it to the list if it corresponds to a valid state code.
* This function will accept a single state code and will prepend it to the
*   contents of the state traversal list.
*
* @return 0 on success, -1 on no params, -2 on invalid FSM code.
*/
template <class T> int8_t StateMachine<T>::_fsm_prepend_state(T nxt) {
  int8_t ret = -1;
  if (_ENUM_DEFS->enumValid(nxt)) {
    ret--;
    const uint32_t STATES_TO_CYCLE = _waypoints.count();
    // We need at least enough space for our one addition.
    if (STATES_TO_CYCLE < _waypoints.capacity()) {
      _waypoints.insert(nxt);
      for (uint32_t i = 0; i < STATES_TO_CYCLE; i++) {
        _waypoints.insert(_waypoints.get());
      }
      ret = 0;
    }
  }
  return ret;
}


/******************************************************************************
* Console and debugging
******************************************************************************/

// TODO: Is this causing string replication because it is templated? BusOp would also like to know...
template <class T> void StateMachine<T>::printFSM(StringBuilder* output) {
  bool keep_looping = true;
  uint32_t i = 0;
  StringBuilder::styleHeader1(output, _NAME);
  output->concatf("\tPrior state:   %s\n", _ENUM_DEFS->enumStr(_prior_state));
  output->concatf("\tCurrent state: %s%s\n\tNext states:   ", _ENUM_DEFS->enumStr(_current_state), _fsm_is_waiting() ? " (LOCKED)":" ");
  while (keep_looping & (i <= _waypoints.count())) {
    if (i == _waypoints.count()) {
      output->concat("(stable)");
      keep_looping = false;
    }
    else {
      output->concatf("%s, ", _ENUM_DEFS->enumStr(_waypoints.get(i)));
    }
    i++;
  }
  if (_fsm_is_waiting()) {
    output->concatf("\tFSM locked for another %ums\n", _lockout_timer.remaining());
  }
  output->concat('\n');
}



/**
* @page console-handlers
* @section state-machine-tools State Machine Tools
*
* This is a console subhandler, which might be exposed by any class which implements a finite state machine.
*
* @subsection cmd-actions Actions
*
* Action        | Description | Additional arguments
* ------------- | ----------- | --------------------
* `slowdown`    | Enforce a minimum period between FSM transitions. | [delay in milliseconds]
*
*/
template <class T> int8_t StateMachine<T>::fsm_console_handler(StringBuilder* text_return, StringBuilder* args) {
  int8_t ret = 0;
  char*  cmd = args->position_trimmed(0);
  if (0 == args->count()) {
    printFSM(text_return);
  }
  else if (0 == StringBuilder::strcasecmp(cmd, "slowdown")) {
    if (1 < args->count()) {
      _slowdown_ms = args->position_as_int(1);
    }
    text_return->concatf("%s slowdown is %u.\n", _NAME, _slowdown_ms);
  }
  return ret;
}


#endif // __C3P_FINITE_STATE_MACHINE