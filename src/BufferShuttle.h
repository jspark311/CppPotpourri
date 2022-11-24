/*
File:   BufferShuttle.h
Author: J. Ian Lindsay
Date:   2022.05.28

This class is intended to move large amounts of buffer over some I/O channel
  that would result in unacceptably high peak-memory usage.

NOTE: Instances of this class will require polling, and come with concurrency
  stipulations that will be difficult to meet in many cases, and cannot be
  controlled for in this class. Use this as a debug support tool, and not as a
  premise for actual production features.
NOTE: This class will not free buffers by default. If that behavior is desired,
  it should be requested following buffer definition. Be careful. There is no
  contract enforcement WRT to memory.
TODO: This should turn into a templated class to handle complex types that are
  reducible to buffers.
*/

#include "CppPotpourri.h"
#include "AbstractPlatform.h"
#include "StringBuilder.h"
#include "ParsingConsole.h"
#include "GPSWrapper.h"
#include "UARTAdapter.h"
#include "KeyValuePair.h"
#include "SensorFilter.h"
#include "cbor-cpp/cbor.h"
#include "Image/Image.h"


class BufferShuttle {
  public:
    BufferShuttle(BufferAccepter* target) : BufferShuttle(target, nullptr, 0, 0) {};
    BufferShuttle(BufferAccepter* target, uint8_t* buffer, uint32_t length) : BufferShuttle(target, buffer, 0, length) {};
    BufferShuttle(BufferAccepter* target, uint8_t* buffer, uint32_t start, uint32_t stop);

    ~BufferShuttle() {};

    // TODO: Use lambdas to do transforms on buffer on polling. This might be as
    //   simple as a copy, or may involve a string conversion function of some sort.

    inline void setTarget(BufferAccepter* target) {   _target = target;   };
    inline int pendingBytes() {   return (_stop_offset - _current_offset);   };
    inline int totalBytes() {     return (_stop_offset - _start_offset);   };
    inline void abort() {         _current_offset = _stop_offset;   };  // Will cause completion on next poll.
    inline void freeOnFinish(bool x) {    _free_on_finish = x;   };

    int poll(uint32_t increment);   // Considers the state of things, and might queue I/O.
    void setSource(uint8_t* buffer, uint32_t start, uint32_t stop);


  protected:
    BufferAccepter* _target;
    uint8_t* _buf;
    uint32_t _start_offset;
    uint32_t _stop_offset;
    uint32_t _current_offset;
    bool     _free_on_finish;
};
