/*
File:   C3PTypePipe.cpp
Author: J. Ian Lindsay
Date:   2024.03.21

A BufferCoDec for transparently piping raw typed values into and out of strings.

These classes should strive to be as stateless as possible, apart from hook-up,
  profiling, etc. The encoder should not cache values fed to it, and the decoder
  should not buffer resolved (that is: parsed) values.
*/

#include "C3PTypePipe.h"
#include "../../../C3PValue/C3PValue.h"
#include "../../../C3PValue/KeyValuePair.h"


/*******************************************************************************
* Encoder
*
* TODO: These should return an error code and not push if the full buffer was
*   not accepted. Check the downstream vacancy prior to push, but after
*   serializing.
*******************************************************************************/

int8_t C3PTypePipeSource::pushValue(C3PValue* val) {
  int8_t ret = -1;
  if (_push_ok_locally(val)) {
    ret--;
    StringBuilder tmp;
    if (0 == val->serialize(&tmp, _FORMAT)) {
      ret--;
      if (0 == _private_push(&tmp)) {
        ret = 0;
      }
    }
  }
  return ret;
}


int8_t C3PTypePipeSource::pushValue(KeyValuePair* val) {
  int8_t ret = -1;
  if (_push_ok_locally(val)) {
    ret--;
    StringBuilder tmp;
    if (0 == val->serialize(&tmp, _FORMAT)) {
      ret--;
      if (0 == _private_push(&tmp)) {
        ret = 0;
      }
    }
  }
  return ret;
}


/*
* Use this function for any direct-from-native types we want to push. Has the
*   advantage of side-stepping what might be useless overhead associated with
*   using C3PType.
*/
int8_t C3PTypePipeSource::_private_push(const TCode TC, void* val) {
  int8_t ret = -1;
  if (_push_ok_locally(val)) {
    ret--;
    C3PType* t_helper = getTypeHelper(TC);
    if (nullptr != t_helper) {
      ret--;
      StringBuilder tmp;
      if (0 == t_helper->serialize(val, &tmp, _FORMAT)) {
        ret--;
        if (0 == _private_push(&tmp)) {
          ret = 0;
        }
      }
    }
  }
  return ret;
}


/**
* Push a serilaized string into the BufferAccepter pipeline.
* It is very important that this call be all-or-nothing.
*
* @param str_data is the buffer to push into the pipeline.
* @return 0 on success, -1 if the data won't fit, -2 if it wasn't fully-claimed.
*/
int8_t C3PTypePipeSource::_private_push(StringBuilder* str_data) {
  int8_t ret = -1;
  const int32_t INITIAL_LENTH = str_data->length();
  if (INITIAL_LENTH <= _efferant->bufferAvailable()) {
    ret--;
    if (1 == _efferant->pushBuffer(str_data)) {
      _byte_count += INITIAL_LENTH;
      ret = 0;
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


/*
* Tries to inflate as many complete types as it can, and returns any unused
*   buffer to the caller.
* Will probably mutate the memory layout of incoming buffers, but not their
*   content (unless claimed).
*/
int8_t C3PTypePipeSink::pushBuffer(StringBuilder* incoming) {
  int8_t ret = -1;
  C3PValue* val_to_emit = nullptr;
  if (nullptr != _value_cb) {
    const uint32_t INCOMING_LEN = incoming->length();
    const uint32_t SAFE_LEN     = strict_min(_MAX_BUFFER, INCOMING_LEN);
    do {
      StringBuilder tmp_stbldr;
      tmp_stbldr.concatHandoffLimit(incoming, SAFE_LEN);
      val_to_emit = C3PValue::deserialize(&tmp_stbldr, _FORMAT);
      if (nullptr != val_to_emit) {
        _value_cb(val_to_emit);
      }
      if (!tmp_stbldr.isEmpty(true)) {
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
