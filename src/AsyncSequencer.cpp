/*
* @file      AsyncSequencer.cpp
* @brief     A class for tracking up tp 32 different asynchronous states.
* @author    J. Ian Lindsay
* @date      2023.02.17
*
*/

#include "AsyncSequencer.h"
#include "StringBuilder.h"
#include "CppPotpourri.h"
#include "AbstractPlatform.h"
#include <functional>


/**
* Print the class state.
*
* @param output is the buffer to receive the text.
*/
void AsyncSequencer::printDebug(StringBuilder* output) {
  StringBuilder::styleHeader1(output, "AsyncSequencer");
  output->concatf("\tRequest Fulfilled:    %c\n", request_fulfilled() ? 'y':'n');
  output->concatf("\tSteps outstanding:    %c\n", all_steps_have_run() ? 'n':'y');
  output->concatf("\tAll steps pass:       %c\n", all_steps_have_passed() ? 'y':'n');
  output->concatf("\tSteps are running:    %c\n", _no_steps_running() ? 'n':'y');
  output->concat("\tStep                   | Requested | Runnable | Running | Complete | Result\n");
  output->concat("\t-----------------------|-----------|----------|---------|----------|-------\n");
  for (uint32_t i = 0; i < _STEP_COUNT; i++) {
    const StepSequenceList* STEP = (_STEP_LIST + i);
    output->concatf(
        "\t%22s |         %c |        %c |       %c |        %c |   %s\n",
        STEP->LABEL,
        (_steps_requested.value(STEP->FLAG) ? 'y':'n'),
        (_steps_runnable.value(STEP->FLAG) ? 'y':'n'),
        (_steps_running.value(STEP->FLAG) ? 'y':'n'),
        (_steps_complete.value(STEP->FLAG) ? 'y':'n'),
        (_steps_complete.value(STEP->FLAG) ? (_steps_passed.value(STEP->FLAG) ? "pass":"fail") : "")
    );
  }
}




/*******************************************************************************
* Semantic breakouts for flags.
*******************************************************************************/

void AsyncSequencer::_mark_step_for_rerun(const uint32_t MASK) {
  _steps_complete.clear(MASK);
  _steps_passed.clear(MASK);
}


void AsyncSequencer::_mark_step_dispatched(const uint32_t MASK) {
  _steps_complete.clear(MASK);
  _steps_passed.clear(MASK);
  _steps_running.set(MASK);
}


void AsyncSequencer::_mark_step_complete(const uint32_t MASK, const bool PASSED) {
  _steps_running.clear(MASK);
  _steps_complete.set(MASK);
  _steps_passed.set(MASK, PASSED);
}


void AsyncSequencer::_reset_failed_steps(const bool INC_RUNNING) {
  const uint32_t SCOPE_MASK = (INC_RUNNING ? _steps_runnable.raw : _steps_complete.raw);
  const uint32_t RESET_MASK = (SCOPE_MASK & !_steps_passed.raw);
  _steps_running.clear(RESET_MASK);
  _steps_complete.clear(RESET_MASK);
}


/**
* Considers what is requested, and what is possible, and updates runnable.
*/
void AsyncSequencer::_check_dependencies() {
  uint32_t new_runnable_mask = 0;
  for (uint32_t i = 0; i < _STEP_COUNT; i++) {
    const StepSequenceList* STEP = (_STEP_LIST + i);
    if (!_steps_runnable.value(STEP->FLAG)) {      // If the step isn't already marked runnable...
      if (_all_steps_passed(STEP->DEP_MASK)) {     // ...and all of its dependepcies have passed...
        if (_steps_requested.value(STEP->FLAG)) {  // ...and it has been requested (even if only implicitly)...
          new_runnable_mask |= STEP->FLAG;         // ...mark it ready for dispatch.
        }
      }
    }
  }
  if (0 != new_runnable_mask) {  // Update the runnable flags, if required.
    _steps_runnable.set(new_runnable_mask);
  }
}


int8_t AsyncSequencer::_dispatch_steps(const uint32_t DISPTCH_MASK) {
  int8_t ret = 0;
  for (uint32_t i = 0; i < _STEP_COUNT; i++) {
    const StepSequenceList* STEP = (_STEP_LIST + i);
    if (STEP->FLAG == (STEP->FLAG & DISPTCH_MASK)) {
      switch (STEP->DISPATCH_FXN()) {
        case 1:   _mark_step_dispatched(STEP->FLAG);   break;  // Dispatch succeeded.
        case 0:   break;      // Not great. Not terrible. Retry on a later pass.
        // Anything other return value will be construed as terminal failure.
        default:  _mark_step_complete(STEP->FLAG, false);   break;
      }
    }
  }
  return ret;
}


int8_t AsyncSequencer::_poll_steps(const uint32_t POLL_MASK) {
  int8_t ret = 0;
  for (uint32_t i = 0; i < _STEP_COUNT; i++) {
    const StepSequenceList* STEP = (_STEP_LIST + i);
    if (STEP->FLAG == (STEP->FLAG & POLL_MASK)) {
      switch (STEP->POLL_FXN()) {
        case 1:   _mark_step_complete(STEP->FLAG, true);    break;  // Step succeeded.
        case 0:   break;      // Not great. Not terrible. Retry on a later pass.
        // Anything other return value will be construed as terminal failure.
        default:  _mark_step_complete(STEP->FLAG, false);   break;
      }
    }
  }
  return ret;
}



/**
* Top-level call to advance the states.
* NOTE: The return value has nothing to do with the passing of states.
*
* @return The number of state transitions, or -1 on failure.
*/
int8_t AsyncSequencer::poll() {
  int8_t ret = 0;
  const uint32_t PRERUN_POLL = _steps_running.raw;
  const uint32_t PRERUN_COMP = _steps_complete.raw;
  // The incomplete steps that are runnable and not running.
  const uint32_t PRERUN_DISPATCH = _steps_runnable.raw & ~(PRERUN_COMP) & ~(PRERUN_POLL);

  _check_dependencies();
  _dispatch_steps(PRERUN_DISPATCH);
  _poll_steps(_steps_running.raw);

  // Capture the mutated flag states...
  const uint32_t POSTRUN_POLL     = _steps_running.raw;
  const uint32_t POSTRUN_COMP     = _steps_complete.raw;
  const uint32_t POSTRUN_DISPATCH = _steps_runnable.raw & ~(POSTRUN_COMP) & ~(POSTRUN_POLL);
  // ...diff the against the initial states...
  const uint32_t DIFF_POLL        = (PRERUN_POLL ^ POSTRUN_POLL);
  const uint32_t DIFF_COMP        = (PRERUN_COMP ^ POSTRUN_COMP);
  const uint32_t DIFF_DISPATCH    = (PRERUN_DISPATCH ^ POSTRUN_DISPATCH);

  // ...and count the bits of difference between them. This is our return value.
  for (uint8_t i = 0; i < 32; i++) {
    ret += ((DIFF_POLL >> i) & 1);
    ret += ((DIFF_COMP >> i) & 1);
    ret += ((DIFF_DISPATCH >> i) & 1);
  }
  return ret;
}


void AsyncSequencer::resetSequencer() {
  _steps_runnable.raw  = 0;
  _steps_running.raw   = 0;
  _steps_requested.raw = 0;
  _steps_complete.raw  = 0;
  _steps_passed.raw    = 0;
}


/**
* Top-level call to advance the states.
* NOTE: The request will be expounded to include any dependencies.
* NOTE: Any invalid flags will be ignored, and will be filtered.
*
* @return The number of state transitions, or -1 on failure.
*/
void AsyncSequencer::resetSteps(const uint32_t STEP_MASK) {
  _steps_runnable.clear(STEP_MASK);
  _steps_running.clear(STEP_MASK);
  _steps_requested.clear(STEP_MASK);
  _steps_complete.clear(STEP_MASK);
  _steps_passed.clear(STEP_MASK);
}


/**
* Top-level call to advance the states.
* NOTE: The request will be expounded to include any dependencies.
* NOTE: Any invalid flags will be ignored, and will be filtered.
*
* @return The number of state transitions, or -1 on failure.
*/
void AsyncSequencer::requestSteps(const uint32_t STEP_MASK) {
  const uint32_t FULL_REQ_MASK = _get_dependency_mask(STEP_MASK);
  _steps_requested.set(FULL_REQ_MASK);
}



/**
* NOTE: Recursion in use, with bailout. Bailout is set to such a depth that it
*   could only be reached with circular dependencies (which this class does no
*   checking for). So even in the worst valid case (a full list of linear
*   dependencies), the bailout condition will never be encountered. A circular
*   dependency will result in a recursion depth of 32.
* NOTE: Any invalid flags will be ignored, and will be filtered.
*
* @return The argument, OR'd with all dependencies.
*/
uint32_t AsyncSequencer::_get_dependency_mask(const uint32_t REQ_MASK, uint8_t limiter) {
  uint32_t ret = 0;
  for (uint32_t i = 0; i < _STEP_COUNT; i++) {
    const StepSequenceList* STEP = (_STEP_LIST + i);
    if (STEP->FLAG == (REQ_MASK & STEP->FLAG)) {
      ret |= STEP->FLAG;
      if (0 != STEP->DEP_MASK) {  // If there are dependencies...
        // ...take those that we haven't yet plumbed...
        const uint32_t UNCOVERED_DEPS = (STEP->DEP_MASK & (ret ^ STEP->DEP_MASK));
        if (0 != UNCOVERED_DEPS) {  // ...and they are not all already covered...
          // ...and dive to the bottom (if we aren't limited).
          if (strict_min(32, _STEP_COUNT) > limiter) {
            ret |= _get_dependency_mask(UNCOVERED_DEPS, (limiter+1));
          }
        }
      }
    }
  }
  return ret;
}
