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
void AsyncSequencer::printDebug(StringBuilder* output, const char* hdr) {
  StringBuilder::styleHeader1(output, ((nullptr != hdr) ? hdr : "AsyncSequencer"));
  output->concatf("\tRequest Fulfilled:    %c\n", request_fulfilled() ? 'y':'n');
  output->concatf("\tSteps outstanding:    %c\n", all_steps_have_run() ? 'n':'y');
  output->concatf("\tAll steps pass:       %c\n", all_steps_have_passed() ? 'y':'n');
  output->concatf("\tSteps are running:    %c\n", steps_running() ? 'y':'n');
  const char* COL_0_HEADER = "Step";
  const int OBLIGATORY_MIN_WIDTH_COL_0 = strlen(COL_0_HEADER);
  uint32_t col_width_0 = OBLIGATORY_MIN_WIDTH_COL_0;
  for (uint32_t i = 0; i < _STEP_COUNT; i++) {
    const StepSequenceList* STEP = (_STEP_LIST + i);
    col_width_0 = strict_max(col_width_0, (uint32_t) strlen(STEP->LABEL));
  }
  const uint32_t SPACER_LENGTH = (col_width_0 - OBLIGATORY_MIN_WIDTH_COL_0);
  // TODO: What follows is confusing and brittle. But it works. Might be nice
  //   to port it over to a columnar output formatting class, since we commonly
  //   build tables such as this, and it would be ok to take a long time to do it.
  StringBuilder row_def_string;
  row_def_string.concatf("\t%%%us |         %%c |        %%c |       %%c |        %%c |   %%s\n", col_width_0);
  char dyn_row_0[SPACER_LENGTH + 1];
  char dyn_row_1[SPACER_LENGTH + 1];
  memset(dyn_row_0, ' ', SPACER_LENGTH);
  memset(dyn_row_1, '-', SPACER_LENGTH);
  dyn_row_0[SPACER_LENGTH] = 0;
  dyn_row_1[SPACER_LENGTH] = 0;
  output->concatf("\t%s ", COL_0_HEADER);
  output->concat(dyn_row_0);
  output->concat("| Requested | Runnable | Running | Complete | Result\n");
  output->concat("\t-----");
  output->concat(dyn_row_1);
  output->concat("|-----------|----------|---------|----------|-------\n");
  for (uint32_t i = 0; i < _STEP_COUNT; i++) {
    const StepSequenceList* STEP = (_STEP_LIST + i);
    output->concatf(
      (const char*) row_def_string.string(),
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
      if (all_steps_have_passed(STEP->DEP_MASK)) { // ...and all of its dependepcies have passed...
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
  // ...diff them against the initial states...
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


void AsyncSequencer::setState(const uint32_t REQ, const uint32_t RUNABLE, const uint32_t RUNNING, const uint32_t COMPLETE, const uint32_t PASSED) {
  _steps_requested.raw = REQ;
  _steps_runnable.raw  = RUNABLE;
  _steps_running.raw   = RUNNING;
  _steps_complete.raw  = COMPLETE;
  _steps_passed.raw    = PASSED;
}


void AsyncSequencer::getState(uint32_t* REQ, uint32_t* RUNABLE, uint32_t* RUNNING, uint32_t* COMPLETE, uint32_t* PASSED) {
  if (nullptr != REQ) {       *REQ      = _steps_requested.raw;  }
  if (nullptr != RUNABLE) {   *RUNABLE  = _steps_runnable.raw;   }
  if (nullptr != RUNNING) {   *RUNNING  = _steps_running.raw;    }
  if (nullptr != COMPLETE) {  *COMPLETE = _steps_complete.raw;   }
  if (nullptr != PASSED) {    *PASSED   = _steps_passed.raw;     }
}



uint32_t AsyncSequencer::failed_steps(const bool INC_RUNNING) {
  const uint32_t SCOPE_MASK = (INC_RUNNING ? _steps_runnable.raw : _steps_complete.raw);
  return (SCOPE_MASK & ~_steps_passed.raw);
}


/*
* @return The number of steps in the checklist.
*/
uint32_t AsyncSequencer::stepList(StringBuilder* output) {
  if (nullptr != output) {
    for (uint32_t i = 0; i < _STEP_COUNT; i++) {
      output->concat((_STEP_LIST+i)->LABEL);
    }
  }
  return _STEP_COUNT;
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
          if (strict_min((uint32_t) 32, _STEP_COUNT) > limiter) {
            ret |= _get_dependency_mask(UNCOVERED_DEPS, (limiter+1));
          }
        }
      }
    }
  }
  return ret;
}
