/*
File:   BufferAccepter.cpp
Author: J. Ian Lindsay
Date:   2023.07.29

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


An abstract interface for buffers.
*/

#include "BufferAccepter.h"


/*******************************************************************************
* StringBuilderSink
*******************************************************************************/

int8_t StringBuilderSink::pushBuffer(StringBuilder* buf) {
  int8_t ret = -1;
  const int32_t PUSH_LEN      = ((nullptr != buf) ? buf->length() : 0);
  const int32_t AVAILABLE_LEN = bufferAvailable();
  const int32_t TAKE_LEN      = strict_min(AVAILABLE_LEN, PUSH_LEN);
  if (TAKE_LEN > 0) {
    if (TAKE_LEN == PUSH_LEN) {
      concatHandoff(buf);
      ret = 1;
    }
    else {
      concatHandoffLimit(buf, TAKE_LEN);
      ret = 0;
    }
  }
  return ret;
}


int32_t StringBuilderSink::bufferAvailable() {
  const int32_t RETURN_LEN = (MAX_CAPTURE_LENGTH - length());
  return ((RETURN_LEN < 0) ? 0 : (RETURN_LEN));
}


/*******************************************************************************
* BufferAccepterFork
*******************************************************************************/

int8_t BufferAccepterFork::pushBuffer(StringBuilder* buf) {
  int8_t ret = -1;
  const int32_t BYTES_OFFERED = buf->length();
  const int32_t BYTES_TO_TAKE = strict_min(bufferAvailable(), BYTES_OFFERED);
  if (0 < BYTES_TO_TAKE) {
    int left_took  = BYTES_TO_TAKE;
    int right_took = BYTES_TO_TAKE;
    // We could be risky at this point, so we do a pedantic deep-copy.
    StringBuilder deep_copy_0(buf->string(), BYTES_TO_TAKE);
    if (_left_hand) {
      _left_hand->pushBuffer(&deep_copy_0);
      left_took  = (BYTES_TO_TAKE - deep_copy_0.length());
    }
    if (_right_hand) {
      if (deep_copy_0.length() < BYTES_TO_TAKE) {
        if (0 < left_took) {
          deep_copy_0.clear();
          deep_copy_0.concat(buf->string(), BYTES_TO_TAKE);
        }
      }
      _right_hand->pushBuffer(&deep_copy_0);
      right_took = (BYTES_TO_TAKE - deep_copy_0.length());
    }
    const int TOTAL_TAKEN = strict_min(left_took, right_took);
    buf->cull(TOTAL_TAKEN);
    ret = (BYTES_OFFERED == TOTAL_TAKEN) ? 1 : 0;
  }
  return ret;
}


// Returns the minimum between the two buffers.
int32_t BufferAccepterFork::bufferAvailable() {
  int32_t lh_available = (_left_hand)  ? _left_hand->bufferAvailable()  : 0;
  int32_t rh_available = (_right_hand) ? _right_hand->bufferAvailable() : 0;
  bool have_both   = (lh_available > 0)  & (rh_available > 0);
  bool have_either = (lh_available > 0)  | (rh_available > 0);
  bool force_fail  = ((nullptr != _left_hand) & (lh_available <= 0));
  force_fail |= ((nullptr != _right_hand) & (rh_available <= 0));

  // NOTE: This class isolates the caller from the possibility of seeing a -1 return.
  int32_t ret = 0;
  if (!force_fail) {
    if (have_both) {
      // NOTE: Test harshest condition first, otherwise this will never run.
      // We return the simple minimum between two valid results.
      ret = strict_min(lh_available, rh_available);
    }
    else if (have_either) {
      // One of these values is set, but the other is not. Thus, take the biggest.
      ret = strict_max(lh_available, rh_available);
    }
  }
  return ret;
}
