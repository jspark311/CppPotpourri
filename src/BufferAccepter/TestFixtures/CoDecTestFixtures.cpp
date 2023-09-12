/*
File:   CoDecTestFixtures.cpp
Author: J. Ian Lindsay
Date:   2023.08.25

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


Test fixtures for CoDecs. Only programs concerned with unit testing need to
  compile this file.
*/

#include "CoDecTestFixtures.h"


/*******************************************************************************
* Source
*******************************************************************************/
/*
* BufAcceptTestSource will always accept the entire buffer, and will meter it
*   out to the efferant BufferAccepter over successive polling cycles.
*
*/
int8_t BufAcceptTestSource::pushBuffer(StringBuilder* buf) {
  int8_t ret = -1;
  if (nullptr != _efferant) {
    ret = 1;
    _backlog.concatHandoff(buf);
  }

  return ret;
}


int32_t BufAcceptTestSource::bufferAvailable() {
  return ((nullptr == _efferant) ? 0 : _efferant->bufferAvailable());
}


/*
* Print object state.
*/
void BufAcceptTestSource::printDebug(StringBuilder* text_return) {
  StringBuilder::styleHeader1(text_return, "BufAcceptTestSource");
  text_return->concatf("\tBuffer_limit:   %u\n", _fake_buffer_limit);
  text_return->concatf("\tBacklog length: %u\n", backlogLength());
  text_return->concat("\tCall counts:\n");
  text_return->concatf("\t  Rejections:     %u\n", _pb_call_count_rej);
  text_return->concatf("\t  Partial claims: %u\n", _pb_call_count_partial);
  text_return->concatf("\t  Full claims:    %u\n", _pb_call_count_full);
  text_return->concatf("\t  Total:          %u\n", _call_count);
  text_return->concat("\tContract evaluation:\n");
  text_return->concatf("\t  Return conventions respected?   %s\n",   callCountsBalance() ? "Conforms" : "Fails");
  text_return->concatf("\t  Rejection semantics?            %s\n",   (0 == _false_rejections) ? "Conforms" : "Fails");
  text_return->concatf("\t  Partial claim semantics?        %s\n",   (0 == _false_partial_claims) ? "Conforms" : "Fails");
  text_return->concatf("\t  Full claim semantics?           %s\n\n", (0 == _false_full_claims) ? "Conforms" : "Fails");
}


bool BufAcceptTestSource::efferantViolatesContract() {
  bool ret = !callCountsBalance();
  ret |= (0 < _false_rejections);
  ret |= (0 < _false_partial_claims);
  ret |= (0 < _false_full_claims);
  return ret;
}



int8_t BufAcceptTestSource::poll() {
  int8_t ret = -1;
  if (nullptr != _efferant) {
    ret = 0;
    const int PUSH_LENGTH = strict_min(_fake_buffer_limit, backlogLength());
    if (PUSH_LENGTH > 0) {
      StringBuilder buf_to_push;
      //buf_to_push.concatHandoffLimit(&_backlog, _fake_buffer_limit);
      buf_to_push.concatHandoffLimit(&_backlog, _fake_buffer_limit);
      if (nullptr != _profiler) {
        _profiler->markStart();
      }
      // Note the return code, and bin it.
      switch (_efferant->pushBuffer(&buf_to_push)) {
        case -1:
          _pb_call_count_rej++;
          if (PUSH_LENGTH != buf_to_push.length()) {  _false_rejections++;  }
          break;
        case 0:
          _pb_call_count_partial++;
          if (PUSH_LENGTH == buf_to_push.length()) {  _false_partial_claims++;  }
          break;
        case 1:
          _pb_call_count_full++;
          if (0 != buf_to_push.length()) {  _false_full_claims++;  }
          break;
        default:
          break;
      }
      if (buf_to_push.length() > 0) {
        // Return any unclaimed buffer to the backlog.
        _backlog.prependHandoff(&buf_to_push);
      }
      _call_count++;
      ret = 1;
    }
  }
  return ret;
}


int8_t BufAcceptTestSource::pollUntilStagnant() {
  const int BACKLOG_LEN_0 = _backlog.length();
  int8_t ret = 0;
  bool keep_polling = true;
  while (keep_polling) {
    keep_polling = false;
    if (1 == poll()) {
      keep_polling = !efferantViolatesContract();
      ret++;
    }
  }
  return ret;
}


/*
* Reset the source's tracking in preparation for a new test.
* NOTE: Will not reset the externally-held profiler.
*/
void BufAcceptTestSource::reset() {
  _fake_buffer_limit     = 0;  // Implies never propagate buffers.
  _call_count            = 0;
  _pb_call_count_rej     = 0;
  _pb_call_count_partial = 0;
  _pb_call_count_full    = 0;
  _false_rejections      = 0;
  _false_partial_claims  = 0;
  _false_full_claims     = 0;
  _call_count            = 0;
  _backlog.clear();
}


/*
* Instrumentation function.
*
* @return true if the codes we returned were always within contract.
*/
bool BufAcceptTestSource::callCountsBalance() {
  return ((_pb_call_count_rej + _pb_call_count_partial + _pb_call_count_full) == _call_count);
}



/*******************************************************************************
* Sink
*******************************************************************************/


int8_t BufAcceptTestSink::pushBuffer(StringBuilder* buf) {
  int8_t ret = -1;
  if (nullptr != buf) {
    int taken_len = 0;
    const int32_t BUF_AVAILABLE  = bufferAvailable();
    if (0 < BUF_AVAILABLE) {
      const int32_t OFFERED_LENGTH = buf->length();
      const int32_t TAKE_LENGTH    = strict_min(BUF_AVAILABLE, OFFERED_LENGTH);
      // NOTE: Normal class operation would happen here if (TAKE_LENGTH > 0);

      //buf->cull(taken_len);
      // NOTE: Concurrency issues aside, the commented line above is sufficient
      //   to trim the incoming buffer following a claim (partial or full). But
      //   it isn't structure-preserving. If your class doesn't care about such
      //   things, and is willing to possibly endure the heap spike, this line
      //   will replace the entire loop below.
      // Heap penalties are incurred to preserve visibility into the class being
      //   gripped by the harness.
      // In order to avoid spiking the heap and destroying structural
      //   information about the incoming buffer, we expend complexity to
      //   zero-copy the input buffer as much as we can.
      // A "normal" implementation of BufferAccepter (one that isn't test
      //   equipment) wouldn't need all this bureaucracy. It would be able
      //   to make choices appropriate to its purpose (all-or-nothing
      //   behaviors, mutation concerns, etc), and avoid the copy and heap
      //   thrash implied by the copy implicit buried in the creation of the
      //   intermediate StringBuilder call_item below.

      int frag_idx  = 0;
      // NOTE: This will naturally not execute if the offered buffer is empty.
      while ((taken_len < TAKE_LENGTH) & (frag_idx >= 0)) {
        int tmp_len = 0;
        uint8_t* tmp_ptr = buf->position(0, &tmp_len);
        if (tmp_len > 0) {
          StringBuilder call_item(tmp_ptr, tmp_len);
          buf->drop_position(0);  // Drop the original fragment.
          if ((tmp_len + taken_len) <= TAKE_LENGTH) {
            // We can take this entire fragment. Zero-copy it.
            // This pathway preserves both structure and pointer locations of
            //   fragments. It does not deep-copy. It is an exchange of
            //   ownership only. It is very fast.
            take_log.concatHandoff(&call_item);
            frag_idx++;
            taken_len += tmp_len;
          }
          else {
            // If we can't take the whole fragment, we take as much as we can and
            //   prepend the difference back into the buffer. This implies that the
            //   structural count of the tak_log will exceed that of the input
            //   by an amount equal to the number of partial claims in the
            //   history of the take_log. This represents real heap churn. Not
            //   much... but it IS mutation, nonetheless.
            const int FRAGMENT_LEN_TAKE = (TAKE_LENGTH - taken_len);
            take_log.concat(call_item.string(), FRAGMENT_LEN_TAKE);
            call_item.cull(FRAGMENT_LEN_TAKE);
            buf->prependHandoff(&call_item);
            taken_len += FRAGMENT_LEN_TAKE;
            frag_idx = -1;  // Bail out, and report what we took.
          }
        }
        else {
          frag_idx = -1;  // Bail out, and report what we took.
        }
      }

      if (taken_len == OFFERED_LENGTH) {
        // NOTE: Full claim will be signalled if the offered buffer is of zero
        //   length. It is, after all, true, and will allow the caller to take
        //   actions appropriate for full success.
        ret = 1;  // Full claim
      }
      else {
        ret = 0;  // Partial claim
      }
    }

    if (0 <= ret) {
      // If we took something, and we have expectations, check to see if they
      //   were met or violated.
      // NOTE: All tests for expectations are mutually independent. Length
      //   expectations must include the length of any terminator, and the
      //   termination check requires some implication of length.
      if (0 < _expected_length) {
        if (taken_len == _expected_length) {  _expectations_met++;  }
        else {  _expectations_violated++;  }
      }
      if (LineTerm::ZEROBYTE != _expected_terminator) {
        if (_does_terminator_match()) {  _expectations_met++;  }
        else {  _expectations_violated++;  }
      }
    }

    profiler.markStop();  // Close out the profiler measurement.
    switch (ret) {        // Note the return code, and bin it.
      case -1:  _pb_call_count_rej++;      break;
      case 0:   _pb_call_count_partial++;  break;
      case 1:   _pb_call_count_full++;     break;
      default:  break;
    }
  }
  return ret;
}


int32_t BufAcceptTestSink::bufferAvailable() {
  // NOTE: This function intentionally ignores the content of the take_log,
  //   which is intended to be an unbounded log that does not impact
  //   the behavior of the tests.
  return _fake_buffer_limit;
}


/* Reset the sink's tracking in preparation for a new test. */
void BufAcceptTestSink::reset() {
  profiler.reset();
  take_log.clear();
  _fake_buffer_limit      = 0;  // Implies reject all offered buffers.
  _pb_call_count_rej      = 0;
  _pb_call_count_partial  = 0;
  _pb_call_count_full     = 0;
  _expectations_met       = 0;
  _expectations_violated  = 0;
  _expected_length        = 0;                   // Implies no expectaion.
  _expected_terminator    = LineTerm::ZEROBYTE;  // Implies no expectaion.
}


/*
* Instrumentation function.
*
* @return true if the codes we returned were always within contract.
*/
bool BufAcceptTestSink::callCountsBalance() {
  return ((_pb_call_count_rej + _pb_call_count_partial + _pb_call_count_full) == profiler.executions());
}

/*
* Print object state.
*/
void BufAcceptTestSink::printDebug(StringBuilder* text_return) {
  StringBuilder::styleHeader1(text_return, "BufAcceptTestSink");
  text_return->concatf("\tBuffer_limit:   %u\n", _fake_buffer_limit);
  text_return->concat("\tCall counts:\n");
  text_return->concatf("\t  Rejections:     %u\n", _pb_call_count_rej);
  text_return->concatf("\t  Partial claims: %u\n", _pb_call_count_partial);
  text_return->concatf("\t  Full claims:    %u\n", _pb_call_count_full);
  text_return->concatf("\t  Total:          %u\n", profiler.executions());
  text_return->concatf("\tExpectations:  %u bytes terminated by %s\n", _expected_length, lineTerminatorNameStr(_expected_terminator));
  text_return->concatf("\t  Met:       %u\n", _expectations_met);
  text_return->concatf("\t  Violated:  %u\n", _expectations_violated);
  const uint32_t TAKE_LOG_COUNT = take_log.count();
  text_return->concatf("\tTake log:      %u entries (total length: %d)\n", TAKE_LOG_COUNT, take_log.length());
  if (0 < TAKE_LOG_COUNT) {
    StringBuilder line_list;
    for (uint32_t i = 0; i < TAKE_LOG_COUNT; i++) {
      int tmp_len = 0;
      uint8_t* tmp_ptr = take_log.position(i, &tmp_len);
      if (tmp_len > 0) {
        StringBuilder call_item(tmp_ptr, tmp_len);
        StringBuilder ascii_render;
        call_item.printDebug(&ascii_render);
        line_list.concatf("\t  %u (%d):\t %s", i, tmp_len, (char*) ascii_render.string());
      }
      else {
        line_list.concatf("\t  Fault rendering entry %u\n", i);
      }
    }
    line_list.string(); // Let's not be too wasteful...
    text_return->concatHandoff(&line_list);
  }
  text_return->concat("\tContract evaluation:\n");
  text_return->concatf("\t  Return conventions respected?   %s\n\n", callCountsBalance() ? "Conforms" : "Fails");

  StopWatch::printDebugHeader(text_return);
  profiler.printDebug("pushBuffer()", text_return);
}



bool BufAcceptTestSink::_does_terminator_match() {
  // Non-mutating seek through the offered buffer to locate the final
  //   bytes of the taken buffer.
  // TODO: Contract question... should this test against offer length,
  //   or taken length? A "normal" implementation would only care to
  //   differentiate if it irrationally (and counter to contract)
  //   demanded that lines must coincide with calls to pushBuffer(),
  //   as helpful as it might be to some pipelines that have a concept
  //   of "lines". But that isn't our concern here.
  // For now, it will test against taken_len. Just make sure the harness
  //   has ample "buffer" allowance to fully capture your module's
  //   output, and this nuance should never matter.
  // Get the most-distal fragment in the take_log that gives us enough
  //   bytes to compare.
  const uint8_t TERMINATOR_LEN = lineTerminatorLength(_expected_terminator);
  bool expected_term = false;
  if (TERMINATOR_LEN) {
    // Once again, we do obnoxious-to-read things and bend into a pretzel
    //   to preserve structure.
    StringBuilder distal_bytes;
    int frag_idx = take_log.count();
    while ((distal_bytes.length() < TERMINATOR_LEN) & (0 < frag_idx)) {
      frag_idx--;
      int tmp_len = 0;
      uint8_t* tmp_ptr = take_log.position(frag_idx, &tmp_len);
      if (tmp_len > 0) {
        distal_bytes.prepend(tmp_ptr, tmp_len);
      }
    }

    const int HAYSTACK_LENGTH = distal_bytes.length();
    if (HAYSTACK_LENGTH >= TERMINATOR_LEN) {
      const char* TERMINATOR_BYTES = lineTerminatorLiteralStr(_expected_terminator);
      uint8_t* haystack = distal_bytes.string();
      haystack += (HAYSTACK_LENGTH - TERMINATOR_LEN);

      // Finally, do the comparison.
      expected_term = true;
      for (uint8_t i = 0; i < TERMINATOR_LEN; i++) {
        expected_term &= (*(haystack + i) == *(TERMINATOR_BYTES + i));
      }
    }
  }

  return expected_term;
}
