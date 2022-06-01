/*
File:   BufferShuttle.cpp
Author: J. Ian Lindsay
Date:   2022.05.28

This class is intended to move large amounts of buffer over some I/O channel
  that would result in unacceptably high peak-memory usage.

NOTE: Instances of this class will require polling, and come with concurrency
  stipulations that will be difficult to meet in many cases, and cannot be
  controlled for in this class. Use this as a debug support tool, and not as a
  premise for actual production features.
NOTE: Polling does not check the avaialbility of space in the target buffer. So
  that must be done by the caller.
NOTE: This class will not free buffers by default. If that behavior is desired,
  it should be requested following buffer definition. Be careful. There is no
  contract enforcement WRT to memory.
TODO: This should turn into a templated class to handle complex types that are
  reducible to buffers.
*/


#include "BufferShuttle.h"

/* All constructors delegate to this one. */
BufferShuttle::BufferShuttle(BufferAccepter* target, uint8_t* buffer, uint32_t start, uint32_t stop)
  : _target(target), _buf(buffer), _start_offset(start), _stop_offset(stop), _current_offset(start)
{
  _free_on_finish = false;
}


/**
* @return 2 on action with completion.
*         1 on action without completion.
*         0 on no action.
*        -1 on error.
*/
int BufferShuttle::poll(uint32_t increment) {
  int ret = 0;
  if (0 < pendingBytes()) {
    if (0 < increment) {
      const uint32_t TXFR_SIZE = strict_min(increment, (uint32_t) pendingBytes());
      StringBuilder tmp;
      ret = 1;
    }
  }
  if (0 == pendingBytes()) {
    if (_free_on_finish) {
      free(_buf);
      setSource(nullptr, 0, 0);
    }
    ret = 2;
  }
  return ret;
}


void BufferShuttle::setSource(uint8_t* buffer, uint32_t start, uint32_t stop) {
  _buf = buffer;
  _start_offset = start;
  _stop_offset = stop;
  _current_offset = start;
}
