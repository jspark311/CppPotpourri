/*
* @file      AsyncSequencer.h
* @brief     A class for tracking up tp 32 different asynchronous states.
* @author    J. Ian Lindsay
* @date      2023.02.17
*
*/

#include "FlagContainer.h"
#include "StringBuilder.h"
#include <functional>

#ifndef __STATE_SEQUENCER_H_
#define __STATE_SEQUENCER_H_


/*
* A type to keep a const list of named steps, their dependencies, and lambdas
*   for a dispatch and polling function.
*/
typedef struct {
  const uint32_t           FLAG;          // A flag associated with a step.
  const char* const        LABEL;         // A name to identify it.
  const uint32_t           DEP_MASK;      // A flag mask of other steps that this step depnds upon.
  std::function<int(void)> DISPATCH_FXN;  // Should return 1 on success, 0 on retry, -1 on failure.
  std::function<int(void)> POLL_FXN;      // Should return 1 on success, 0 on retry, -1 on failure.
} StepSequenceList;


/*
* The actual working class.
*/
class AsyncSequencer {
  public:
    AsyncSequencer(const StepSequenceList* const STEP_LIST, const uint32_t COUNT) :
      _STEP_LIST(STEP_LIST), _STEP_COUNT(COUNT) {};
    ~AsyncSequencer() {};

    void     printDebug(StringBuilder*, const char* hdr = "AsyncSequencer");
    int8_t   poll();
    void     resetSequencer();
    void     resetSteps(const uint32_t STEP_MASK);
    void     requestSteps(const uint32_t STEP_MASK);
    void     setState(const uint32_t REQ, const uint32_t RUNABLE, const uint32_t RUNNING, const uint32_t COMPLETE, const uint32_t PASSED);
    void     getState(uint32_t* REQ, uint32_t* RUNABLE, uint32_t* RUNNING, uint32_t* COMPLETE, uint32_t* PASSED);
    uint32_t failed_steps(const bool INC_RUNNING);

    inline bool steps_running() {           return (0 != _steps_running.raw);  };
    inline bool request_fulfilled() {       return (_steps_requested.raw == (_steps_requested.raw & _steps_passed.raw & _steps_complete.raw));  };
    inline bool request_completed() {       return (_steps_requested.raw == (_steps_requested.raw & _steps_complete.raw));  };
    inline bool all_steps_have_run() {      return (_steps_runnable.raw  == (_steps_runnable.raw & _steps_complete.raw));  };
    inline bool all_steps_have_passed() {   return (_steps_runnable.raw  == (_steps_runnable.raw & _steps_passed.raw));    };
    inline bool all_steps_have_passed(const uint32_t MASK) {    return (MASK  == (MASK & _steps_passed.raw));              };


  private:
    const StepSequenceList* const _STEP_LIST;
    const uint32_t _STEP_COUNT;

    FlagContainer32 _steps_requested; // We wait for steps to be specifically requested.
    FlagContainer32 _steps_runnable;  // List of runnable steps.
    FlagContainer32 _steps_running;   // If set, the step is in-progress.
    FlagContainer32 _steps_complete;  // If set, the step has been done.
    FlagContainer32 _steps_passed;    // If set, the step passes.

    void     _mark_step_for_rerun(const uint32_t MASK);
    void     _mark_step_dispatched(const uint32_t MASK);
    void     _mark_step_complete(const uint32_t MASK, const bool PASSED);
    void     _reset_failed_steps(const bool INC_RUNNING = false);
    void     _check_dependencies();
    uint32_t _get_dependency_mask(const uint32_t, uint8_t limiter = 0);
    int8_t   _dispatch_steps(const uint32_t);
    int8_t   _poll_steps(const uint32_t);
};

#endif /* __STATE_SEQUENCER_H_ */
