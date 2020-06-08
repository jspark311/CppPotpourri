#include "WakeLock.h"


/**
* Aquires a WakeLock.
*
* @return true if the call resulted in a state change.
*/
bool WakeLock::acquire(uint32_t timeout_ms) {
  // TODO: timeout_ms is unused until a support choice is made.
  //   For now, we make it always return true, unless the parameter is 0.
  bool ret = (0 == _refs);   // We will know this immediately.
  if (ret) {
    _refs++;   // Needs to be done even if IDEMPOTENT_LOCK is true.
    _process_wl_callback(true);
  }
  else if (!_wl_flag(WL_FLAG_IDEMPOTENT_LOCK)) {
    // If we are reference counting, increment the reference.
    _refs++;
  }
  // TODO: This might be a good place to consider DEFER_STATE_CHANGE in an else,
  //   and call the callback again if it is set?
  return ret;
}


/**
* Releases a WakeLock.
*
* @return true if the call resulted in a state change.
*/
bool WakeLock::release() {
  bool ret = false;
  if (0 < _refs) {
    _refs--;
    if (0 == _refs) {
      ret = true;
      _process_wl_callback(false);
    }
  }
  // TODO: This might be a good place to consider DEFER_STATE_CHANGE in an else,
  //   and call the callback again if it is set?
  return ret;
}


/*
*/
void WakeLock::referenceCounted(bool refd) {
  if (refd != referenceCounted()) {
    _wl_set_flag(WL_FLAG_IDEMPOTENT_LOCK, !refd);
    if (!refd & (1 < _refs)) {
      // If we are moving to an idempotent behavior, and the class is already
      // locked, Fix state such that the next release() causes a state change.
      _refs = 1;
    }
  }
}


/**
* Notifies the owner that the WakeLock changed state. The new state is passed as
*   a bool on the stack so as to minimize grief with possible concurrency control
*   efforts later on.
*
* TODO: I would rather inline this in the header, but the contract needs to be
*   formalized first.
*/
void WakeLock::_process_wl_callback(bool held) {
  if (nullptr != _OWNER) {
    _wl_set_flag(WL_FLAG_DEFER_STATE_CHANGE, !_OWNER->_wakelock_notify(held));
  }
}
