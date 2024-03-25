/*
File:   CBORWrapper.h
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

/* Callbacks for value emission. */
typedef void (*C3PValueDelivery)(C3PValue*);

/*
* Encoder
*/
class C3PTypeSource : public BufferCoDec {
  public:
    C3PTypeSource(const TCode PACKING_FORMAT, BufferAccepter* eff = nullptr) :
      BufferCoDec(eff), _FORMAT(PARSING_FORMAT), _byte_count(0) {};

    ~C3PTypeSource() {};

    /*
    * Implementation of BufferAccepter.
    * NOTE: These functions are pure pass-through. Handy for time-division
    *   muxing of a single buffer pipe.
    */
    int8_t pushBuffer(StringBuilder* incoming) {
      return ((nullptr != _efferant) ? _efferant->pushBuffer(incoming) : -1);
    };

    int32_t bufferAvailable() {
      return ((nullptr != _efferant) ? _efferant->bufferAvailable() : -1);
    };

    /* Profiling */
    inline uint32_t byteCount() {           return _byte_count;  };


    int8_t pushValue(int8_t val) {               return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(int16_t val) {              return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(int32_t val) {              return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(int64_t val) {              return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(uint8_t val) {              return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(uint16_t val) {             return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(uint32_t val) {             return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(uint64_t val) {             return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(double val) {               return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(float val) {                return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(bool val) {                 return _private_push(tcodeForType(val), &val);   };
    int8_t pushValue(C3PValue* val) {            return _private_push(tcodeForType(val), val);    };
    int8_t pushValue(const char* val) {          return _private_push(tcodeForType(val), val);    };
    int8_t pushValue(char* val) {                return _private_push(tcodeForType(val), val);    };
    //int8_t pushValue(uint8_t*, uint32_t val) {   return _private_push(tcodeForType(val), val);    };


  private:
    const TCode    _FORMAT;
    uint32_t _byte_count;   // How many bytes has the class generated?

    int8_t _private_push(const TCode, void*);
};


/*
* Decoder
*/
class C3PTypeSink : public BufferAccepter {
  public:
    C3PTypeSink(const TCode PARSING_FORMAT, const uint32_t MAX_BUF, C3PValueDelivery cb) :
      _MAX_BUFFER(MAX_BUF), _FORMAT(PARSING_FORMAT), _value_cb(cb), _working(nullptr), _byte_count(0) {};

    ~C3PTypeSink() {
      if (nullptr != _working) {
        delete _working;
        _working = nullptr;
      }
    };

    /* Implementation of BufferAccepter. */
    int8_t  pushBuffer(StringBuilder*);
    int32_t bufferAvailable();

    /* Profiling */
    inline uint32_t byteCount() {           return _byte_count;  };


  private:
    const uint32_t   _MAX_BUFFER;
    const TCode      _FORMAT;
    C3PValueDelivery _value_cb;
    C3PValue*        _working;
    uint32_t _byte_count;   // How many bytes has the class consumed?
};

#endif // __C3P_CODEC_C3PTYPE_PIPE_H__
