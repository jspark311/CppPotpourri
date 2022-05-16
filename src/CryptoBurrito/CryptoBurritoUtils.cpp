/*
File:   CryptoBurritoUtils.c
Author: J. Ian Lindsay
Date:   2016.08.13

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

#include "CryptoBurrito.h"

#if defined(__HAS_CRYPT_WRAPPER)


/*******************************************************************************
*      _______.___________.    ___   .___________. __    ______     _______.
*     /       |           |   /   \  |           ||  |  /      |   /       |
*    |   (----`---|  |----`  /  ^  \ `---|  |----`|  | |  ,----'  |   (----`
*     \   \       |  |      /  /_\  \    |  |     |  | |  |        \   \
* .----)   |      |  |     /  _____  \   |  |     |  | |  `----.----)   |
* |_______/       |__|    /__/     \__\  |__|     |__|  \______|_______/
*
* Static members and initializers should be located here.
*******************************************************************************/

/**
* Debug and logging support.
*
* @return a const char* containing a human-readable representation of a state.
*/
const char* CryptOp::stateString(CryptOpState state) {
  switch (state) {
    case CryptOpState::IDLE:      return "IDLE";
    case CryptOpState::QUEUED:    return "QUEUED";
    case CryptOpState::INITIATE:  return "INITIATE";
    case CryptOpState::WAIT:      return "WAIT";
    case CryptOpState::CLEANUP:   return "CLEANUP";
    case CryptOpState::COMPLETE:  return "COMPLETE";
    default:                      return "<UNDEF>";
  }
}

/**
* Debug and logging support.
*
* @return a const char* containing a human-readable representation of an opcode.
*/
const char* CryptOp::opcodeString(CryptOpcode code) {
  switch (code) {
    case CryptOpcode::DIGEST:   return "DIGEST";
    case CryptOpcode::ENCODE:   return "ENCODE";
    case CryptOpcode::DECODE:   return "DECODE";
    case CryptOpcode::ENCRYPT:  return "ENCRYPT";
    case CryptOpcode::DECRYPT:  return "DECRYPT";
    case CryptOpcode::SIGN:     return "SIGN";
    case CryptOpcode::VERIFY:   return "VERIFY";
    case CryptOpcode::KEYGEN:   return "KEYGEN";
    case CryptOpcode::RNG_FILL: return "RNG_FILL";
    default:                    return "<UNDEF>";
  }
}

/**
* Debug and logging support.
*
* @return a const char* containing a human-readable representation of a fault code.
*/
const char* CryptOp::errorString(CryptoFault code) {
  switch (code) {
    case CryptoFault::NONE:            return "NONE";
    case CryptoFault::NO_REASON:       return "NO_REASON";
    case CryptoFault::UNHANDLED_ALGO:  return "UNHANDLED_ALGO";
    case CryptoFault::BAD_PARAM:       return "BAD_PARAM";
    case CryptoFault::ILLEGAL_STATE:   return "ILLEGAL_STATE";
    case CryptoFault::TIMEOUT:         return "TIMEOUT";
    case CryptoFault::HW_FAULT:        return "HW_FAULT";
    case CryptoFault::RECALLED:        return "RECALLED";
    case CryptoFault::QUEUE_FLUSH:     return "QUEUE_FLUSH";
    default:                         return "<UNDEF>";
  }
}


/*******************************************************************************
* CryptOp base class support that isn't inlined.
*******************************************************************************/

CryptOp::~CryptOp() {
  if (shouldFreeBuffer() && (nullptr != _buf)) {
    free(_buf);  // TODO: Use BurritoPlate's free().
    _buf      = nullptr;
    _buf_len  = 0;
  }
};


CryptoFault CryptOp::advance() {
  return _advance();
}


void CryptOp::printOp(StringBuilder* output) {
  output->concatf("\t---[ CryptOp::%s %p ]---\n", CryptOp::opcodeString(_opcode));
  output->concatf("\t job_state        %s\n", CryptOp::stateString(_op_state));
  if (CryptoFault::NONE != _op_fault) {
    output->concatf("\t job_fault        %s\n", CryptOp::errorString(_op_fault));
  }
  if (_buf_len > 0) {
    output->concatf("\t buf *(%p): (%u bytes)\n", _buf, _buf_len);
    //StringBuilder::printBuffer(output, _buf, _buf_len, "\t ");
  }
  _print(output);
}


void CryptOp::wipe() {
  // NOTE: Does not change _flags.
  // Wipe the implementation first in case it depends on the base state.
  _wipe();
  _cb           = nullptr;
  _opcode       = CryptOpcode::UNDEF;
  _op_state     = CryptOpState::IDLE;
  _op_fault     = CryptoFault::NONE;
  _nxt_step     = nullptr;
  _buf          = nullptr;
  _buf_len      = 0;
}



/*******************************************************************************
* CryptoProcessor base class support that isn't inlined.
*******************************************************************************/
int8_t CryptoProcessor::poll() {
  int8_t ret = 0;
  _advance_work_queue();
  _advance_callback_queue();
  return ret;
}

int8_t CryptoProcessor::init() {
  int8_t ret = 0;
  _flags |= CRYPTPROC_FLAG_INITIALIZED;
  return ret;
}

int8_t CryptoProcessor::deinit() {
  int8_t ret = 0;
  _flags &= (~CRYPTPROC_FLAG_INITIALIZED);
  return ret;
}


void CryptoProcessor::printDebug(StringBuilder* output) {
  StringBuilder prod_str("CryptoProcessor (");
  if (!initialized()) prod_str.concat("un");
  prod_str.concat("initialized)\n");
  StringBuilder::styleHeader2(output, (const char*) prod_str.string());

  output->concatf("-- Jobs (fail/total)  %u/%u\n", _failed_jobs, _total_jobs);
  output->concat("-- Work queue:\n");
  output->concatf("\t\t depth/max        %u/%u\n", work_queue.size(), MAX_Q_DEPTH);
  output->concatf("\t\t frees            %u\n", _heap_frees);
  output->concatf("\t\t floods           %u\n",  _queue_floods);
}


void CryptoProcessor::printQueues(StringBuilder* output, uint8_t max_print) {
  if (_current_job) {
    output->concat("--\n- Current active job:\n");
    _current_job->printOp(output);
  }
  else {
    output->concat("--\n-- No active job.\n--\n");
  }
  uint8_t wqs = work_queue.size();
  if (wqs > 0) {
    int print_depth = strict_min(wqs, max_print);
    output->concatf("-- Queue Listing (top %d of %d total)\n", print_depth, wqs);
    for (int i = 0; i < print_depth; i++) {
      work_queue.get(i)->printOp(output);
    }
  }
  else {
    output->concat("-- Empty queue.\n");
  }
}



int8_t CryptoProcessor::queue_job(CryptOp* op, int priority) {
  int8_t ret = -1;
  if (op->state() == CryptOpState::IDLE) {
    ret--;
    if (!roomInQueue()) {
      if (verbosity() >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Queue at max size. Dropping transaction.");
      op->abort(CryptoFault::QUEUE_FLUSH);
      callback_queue.insertIfAbsent(op, priority);
    }
    else if (0 > work_queue.insertIfAbsent(op, priority)) {
      if (verbosity() >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Double-insertion. Dropping transaction with no status change.\n");
    }
    else {
      ret = 0;
      op->_op_state = CryptOpState::QUEUED;
    }
  }
  else {
    if (verbosity() >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Tried to run a CryptOp that is not in IDLE state.");
  }
  return ret;
}


/**
* Purges a stalled job from the active slot.
*/
int8_t CryptoProcessor::purge_current_job() {
  int8_t ret = 0;
  if (_current_job) {
    _current_job->abort(CryptoFault::QUEUE_FLUSH);
    _current_job->_exec_call_back();
    _reclaim_queue_item(_current_job);
    _current_job = nullptr;
    ret++;
  }
  return ret;
}

/**
* Purges only the work_queue. Leaves the currently-executing job.
*/
int8_t CryptoProcessor::purge_queued_work() {
  int8_t ret = 0;
  CryptOp* current = work_queue.dequeue();
  while (current) {
    current->abort(CryptoFault::QUEUE_FLUSH);
    current->_exec_call_back();
    _reclaim_queue_item(current);
    current = work_queue.dequeue();
    ret++;
  }
  return ret;
}

/**
* Purges only those jobs from the work_queue that are owned by the specified
*   callback object. Leaves the currently-executing job.
*
* @param  dev  The device pointer that owns jobs we wish purged.
* @return The number of jobs purged.
*/
int8_t CryptoProcessor::purge_queued_work_by_dev(CryptOpCallback* cb_obj) {
  int8_t ret = 0;
  for (int i = 0; i < work_queue.size(); i++) {
    CryptOp* current = work_queue.get(i);
    if ((nullptr != current) && (current->_cb == cb_obj)) {
      work_queue.remove(current);
      current->abort(CryptoFault::QUEUE_FLUSH);
      current->_exec_call_back();
      _reclaim_queue_item(current);
      ret++;
    }
  }
  return ret;
}


/**
* This fxn will either free() the memory associated with the CryptOp object,
*   or it will return it to the preallocation queue.
*
* @param item The CryptOp to be reclaimed.
*/
void CryptoProcessor::_reclaim_queue_item(CryptOp* op) {
  _total_jobs++;
  if (op->hasFault()) {
    _failed_jobs++;
  }
  if (op->shouldReap()) {
    // This job is a transient heap object. Destructor will handle buffer
    //   memory, if required.
    if (verbosity() >= LOG_LEV_DEBUG) c3p_log(LOG_LEV_DEBUG, __PRETTY_FUNCTION__, "About to reap.");
    delete op;
    _heap_frees++;
  }
  else {
    // If we are here, it must mean that some other class fed us a job that
    //   it doesn't want us cleanup. But we should at least set it
    //   back to IDLE, and check the buffer free policy.
    if (op->shouldFreeBuffer() && (nullptr != op->_buf)) {
      if (verbosity() >= LOG_LEV_DEBUG) c3p_log(LOG_LEV_DEBUG, __PRETTY_FUNCTION__, "Freeing buffer...");
      op->shouldFreeBuffer(false);
      free(op->_buf);  // TODO: Use BurritoPlate's free().
      op->setBuffer(nullptr, 0);
    }
    op->_op_state = CryptOpState::IDLE;
  }
}



int8_t CryptoProcessor::_advance_work_queue() {
  int8_t ret = 0;
  if (nullptr == _current_job) {
    _current_job = work_queue.dequeue();
    // Took a job.
    if (_current_job) {
      ret++;
    }
  }

  if (_current_job) {
    switch (_current_job->_op_state) {
      case CryptOpState::IDLE:
        _current_job->_op_state = CryptOpState::QUEUED;
      case CryptOpState::QUEUED:
      case CryptOpState::INITIATE:
      case CryptOpState::WAIT:
      case CryptOpState::CLEANUP:
        if (CryptoFault::NONE != _current_job->advance()) {
          // All faults are terminal.
          if (verbosity() >= LOG_LEV_ERROR) c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, "Failed to advance job: %s\n", CryptOp::errorString(_current_job->fault()));
          callback_queue.insert(_current_job);
          _current_job = nullptr;
        }
        break;

      case CryptOpState::COMPLETE:
        callback_queue.insert(_current_job);
        _current_job = nullptr;
        break;

      default:
        if (verbosity() >= LOG_LEV_INFO) c3p_log(LOG_LEV_INFO, __PRETTY_FUNCTION__, "CryptOp state at poll(): %s", CryptOp::stateString(_current_job->state()));
        break;
    }
  }
  return ret;
}


int8_t CryptoProcessor::_advance_callback_queue() {
  int8_t ret = 0;
  CryptOp* temp_op = callback_queue.dequeue();
  if (nullptr != temp_op) {
    if (nullptr != temp_op->_cb) {
      int8_t cb_code = temp_op->_exec_call_back();
      switch (cb_code) {
        case JOB_Q_CALLBACK_RECYCLE:
          temp_op->markForRequeue();
          queue_job(temp_op);
          break;

        case JOB_Q_CALLBACK_ERROR:
          if (temp_op->hasFault()) {
            if (verbosity() >= LOG_LEV_ERROR) {    // Print failures.
              StringBuilder tmp_str;
              temp_op->printOp(&tmp_str);
              c3p_log(LOG_LEV_ERROR, __PRETTY_FUNCTION__, &tmp_str);
            }
          }
        case JOB_Q_CALLBACK_NOMINAL:
        default:
          _reclaim_queue_item(temp_op);
          break;
      }
    }
    else {
      // We are the responsible party.
      _reclaim_queue_item(temp_op);
    }
    ret++;
  }
  return ret;
}

#endif  // __HAS_CRYPT_WRAPPER
