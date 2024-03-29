/*
File:   C3PTypePipe.cpp
Author: J. Ian Lindsay
Date:   2024.03.21

A BufferCoDec for transparently piping raw typed values into and out of strings.

These classes should strive to be as stateless as possible, apart from hook-up,
  profiling. The encoder should not cache values fed to it, and the decoder
  should not buffer resolved (that is: parsed) values.
*/

#include "C3PTypePipe.h"
#include "../../../C3PValue/C3PValue.h"
#include "../../../C3PValue/KeyValuePair.h"


/*******************************************************************************
* Encoder
*******************************************************************************/

int8_t C3PTypePipeSource::pushValue(C3PValue* val) {
  int8_t ret = -1;
  if (nullptr != _efferant) {
    ret--;
    if (nullptr != val) {
      StringBuilder tmp;
      if (0 == val->serialize(&tmp, _FORMAT)) {
        const int32_t INITIAL_LENTH = tmp.length();
        ret--;
        if (1 == _efferant->pushBuffer(&tmp)) {
          _byte_count += INITIAL_LENTH;
          ret = 0;
        }
      }
    }
  }
  return ret;
}


int8_t C3PTypePipeSource::pushValue(KeyValuePair* val) {
  int8_t ret = -1;
  if (nullptr != _efferant) {
    ret--;
    if (nullptr != val) {
      StringBuilder tmp;
      if (0 == val->serialize(&tmp, _FORMAT)) {
        const int32_t INITIAL_LENTH = tmp.length();
        ret--;
        if (1 == _efferant->pushBuffer(&tmp)) {
          _byte_count += INITIAL_LENTH;
          ret = 0;
        }
      }
    }
  }
  return ret;
}


int8_t C3PTypePipeSource::_private_push(const TCode TC, void* val) {
  int8_t ret = -1;
  if (nullptr != _efferant) {
    ret--;
    C3PType* t_helper = getTypeHelper(TC);
    if (nullptr != t_helper) {
      ret--;
      StringBuilder tmp;
      const int SER_RET = t_helper->serialize(val, &tmp, _FORMAT);
      if (0 == SER_RET) {
        ret--;
        if (1 == _efferant->pushBuffer(&tmp)) {
          _byte_count += SER_RET;
          ret = 0;
        }
      }
    }
  }
  return ret;
}



/*******************************************************************************
* Decoder
* Here, we are having type data pushed to us. It may be many values with
*   heterogeneous types, or it may be a type that doesn't have enough data to
*   fill.
*******************************************************************************/

C3PTypePipeSink::~C3PTypePipeSink() {
  if (nullptr != _working) {
    delete _working;
    _working = nullptr;
  }
}


int8_t C3PTypePipeSink::pushBuffer(StringBuilder* incoming) {
  int8_t ret = -1;
  C3PValue* val_to_emit = nullptr;
  if (nullptr != _value_cb) {
    const uint32_t INCOMING_LEN = incoming->length();
    const uint32_t SAFE_LEN     = strict_min(_MAX_BUFFER, INCOMING_LEN);
    StringBuilder tmp_stbldr;
    do {
      if (SAFE_LEN < INCOMING_LEN) {
        tmp_stbldr.concatHandoffLimit(incoming, _MAX_BUFFER);
        val_to_emit = C3PValue::deserialize(&tmp_stbldr, _FORMAT);
        if (nullptr != val_to_emit) {
          _value_cb(val_to_emit);
        }
      }
      else {
        // Use the buffer directly.
        val_to_emit = C3PValue::deserialize(incoming, _FORMAT);
      }
      if (nullptr != val_to_emit) {
        _value_cb(val_to_emit);
      }
      if (!tmp_stbldr.isEmpty()) {
        incoming->prependHandoff(&tmp_stbldr);
      }
    } while (nullptr != val_to_emit);
    return (incoming->isEmpty(true) ? 1: 0);
  }
  return -1;
}


/*
* NOTE: This class is a pure sink. So it will eat everything it is given.
*/
int32_t C3PTypePipeSink::bufferAvailable() {
  return _MAX_BUFFER;
}
