/*
File:   CBORWrapper.cpp
Author: J. Ian Lindsay
Date:   2024.03.21

A BufferCoDec for transparently piping raw typed values into and out of strings.

These classes should strive to be as stateless as possible, apart from hook-up,
  profiling. The encoder should not cache values fed to it, and the decoder
  should not buffer resolved (that is: parsed) values.
*/

#ifndef __C3P_CODEC_C3PTYPE_PIPE_H__
#define __C3P_CODEC_C3PTYPE_PIPE_H__

#include "../BufferAccepter.h"


/*******************************************************************************
* Encoder
*******************************************************************************/
int8_t C3PTypeSource::_private_push(const TCode TC, void* val) {
  int8_t ret = -1;
  if (nullptr != _efferant) {
    ret--;
    C3PType* t_helper = getTypeHelper(TC);
    if (nullptr != t_helper) {
      ret--;
      StringBuilder tmp;
      const int SER_RET = t_helper->serialize(val, &tmp, _FORMAT);
      if (SER_RET > 0) {
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
* This function should never reject a buffer. Even if it takes nothing, it should
*   indicate partial claim.
*******************************************************************************/
int8_t C3PTypeSink::pushBuffer(StringBuilder* incoming) {
  int8_t ret = -1;
  C3PValue* val_to_emit = nullptr;
  do {
    val_to_emit = C3PValue::deserialize(incoming, _FORMAT);
    if (nullptr != val_to_emit) {
      _value_cb(&val_to_emit);
    }
  } while (nullptr != val_to_emit);
  return (incoming->isEmpty(true) ? 1: 0);
}


/*
* NOTE: This class is a pure sink. So it will eat everything it is given.
*/
int32_t C3PTypeSink::bufferAvailable() {
  return _MAX_BUFFER;
}

#endif // __C3P_CODEC_C3PTYPE_PIPE_H__
