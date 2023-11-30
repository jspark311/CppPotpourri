/*
* This header file is the sum of all of the independent header files found in
*   the original package. It was grouped together to aid repo orginization, and
*   has been mutated. The author's original license is reproduced below.
*                                                      ---J. Ian Lindsay
*/

/*
   Copyright 2014-2015 Stanislav Ovsyannikov

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

#ifndef CBOR_CPP_CBOR_H
#define CBOR_CPP_CBOR_H

#include <stdint.h>
#include <string.h>

#include "../StringBuilder.h"
class C3PValue;

// NOTE: For some typecodes, we benefit from the context of having the type
//   spelled out with a TCode, rather than using the built-in CBOR types.
// We will be using a tag from the IANA 'unassigned' space to avoid confusion.
//   The first byte after the tag is the native C3P TCode.
#define C3P_CBOR_VENDOR_CODE 0x00E97800


#include <stdio.h>

namespace cbor {
  typedef enum {
    STATE_TYPE,
    STATE_PINT,
    STATE_NINT,
    STATE_BYTES_SIZE,
    STATE_BYTES_DATA,
    STATE_STRING_SIZE,
    STATE_STRING_DATA,
    STATE_ARRAY,
    STATE_MAP,
    STATE_TAG,
    STATE_SPECIAL,
    END_OF_BYTES,
    STATE_ERROR
  } decoder_state;


  /*****************************************************************************
  * Interfaces
  *****************************************************************************/
  /*
  * Original use of cbor-cpp decoder would involve extending this object to
  *   catch CBOR native types that fell out of the input stream.
  */
  class listener {
    public:
      virtual void on_integer(int8_t   value)        =0;
      virtual void on_integer(int16_t  value)        =0;
      virtual void on_integer(int32_t  value)        =0;
      virtual void on_integer(int64_t  value)        =0;
      virtual void on_integer(uint8_t  value)        =0;
      virtual void on_integer(uint16_t value)        =0;
      virtual void on_integer(uint32_t value)        =0;
      virtual void on_integer(uint64_t value)        =0;
      virtual void on_float32(float value)           =0;
      virtual void on_double(double value)           =0;
      virtual void on_bytes(uint8_t* data, int size) =0;
      virtual void on_string(char* str)              =0;
      virtual void on_array(int size)                =0;
      virtual void on_map(int size)                  =0;
      virtual void on_tag(unsigned int tag)          =0;
      virtual void on_special(unsigned int code)     =0;
      virtual void on_bool(bool)                     =0;
      virtual void on_null()                         =0;
      virtual void on_undefined()                    =0;
      virtual void on_error(const char *error)       =0;
      virtual void on_extra_integer(uint64_t value, int sign) {}
      virtual void on_extra_tag(uint64_t tag) {}
      virtual void on_extra_special(uint64_t tag) {}
  };


  /*
  * This is an input interface for the decoder.
  */
  class input {
    public:
      virtual bool     has_bytes(int count)           =0;
      virtual uint8_t  get_byte()                     =0;
      virtual uint16_t get_short()                    =0;
      virtual uint32_t get_int()                      =0;
      virtual float    get_float()                    =0;
      virtual double   get_double()                   =0;
      virtual uint64_t get_long()                     =0;
      virtual void     get_bytes(void* to, int count) =0;
  };


  /*
  * This is an output interface for the encoder.
  */
  class output {
    public:
      virtual uint8_t* data()                         =0;
      virtual uint32_t size()                         =0;
      virtual int8_t   put_byte(uint8_t value)        =0;
      virtual int8_t   put_bytes(const uint8_t*, int) =0;
  };



  /*****************************************************************************
  * Implementations of the interfaces above.
  *****************************************************************************/

  /*
  * This is the byte stream ingestion class that came with cbor-cpp. It takes
  *   ptr-len semantics, is portable, and easy to use.
  */
  class input_static : public input {
    public:
      input_static(void* data, int size);
      ~input_static() {};

      bool     has_bytes(int count);
      uint8_t  get_byte();
      uint16_t get_short();
      uint32_t get_int();
      float    get_float();
      double   get_double();
      uint64_t get_long();
      void     get_bytes(void* to, int count);

    private:
      uint8_t* _data;
      int      _size;
      int      _offset;
  };


  /* For output to an already-allocated buffer with a given length. */
  class output_static : public output {
    public:
      output_static(uint32_t capacity, uint8_t* buf = nullptr);
      ~output_static();

      uint8_t* data();
      uint32_t size();
      int8_t put_byte(uint8_t value);
      int8_t put_bytes(const uint8_t* data, int size);

      inline bool shouldFree() {         return _should_free;  };
      inline void shouldFree(bool x) {   _should_free = x;     };

    private:
      uint8_t* _buffer;
      uint32_t _capacity;
      uint32_t _offset;
      bool     _should_free;
  };


  /*
  * This is a byte stream ingestion class added in CppPotpourri. It takes a
  *   StringBuilder* as an input, which it will optionally consume as decoding
  *   progresses. This helps keep heap usage flat in use-cases where the
  *   decoder's products are also heap-allocated.
  */
  class input_stringbuilder : public input {
    public:
      input_stringbuilder(StringBuilder* sb, bool c_input = false, bool c_container = false) :
        _str_bldr(sb), _offset(0), _consume_input(c_input), _consume_container(c_container) {};
      ~input_stringbuilder();

      bool     has_bytes(int count);
      uint8_t  get_byte();
      uint16_t get_short();
      uint32_t get_int();
      float    get_float();
      double   get_double();
      uint64_t get_long();
      void     get_bytes(void* to, int count);

    private:
      StringBuilder* _str_bldr;
      int            _offset;
      bool           _consume_input;
      bool           _consume_container;

      void _update_local_vars(const int BYTES_USED);
  };


  /* For output to a length-undetermined StringBuilder. */
  class output_stringbuilder : public output {
    public:
      output_stringbuilder(StringBuilder* sb) : _str_bldr(sb) {};
      ~output_stringbuilder() {};

      uint8_t* data();
      uint32_t size();
      int8_t put_byte(uint8_t value);
      int8_t put_bytes(const uint8_t* data, int size);

    private:
      StringBuilder* _str_bldr;
  };


  /*****************************************************************************
  * Fundamental operational classes
  *****************************************************************************/

  /*
  * This is a decoder class that prefers to rely on object inheritance to
  *   implement type restoration and content interpretation.
  * This is the pattern that initially came with cbor-cpp, and is probably the
  *   best choice for tight integration with specific objects that are known at
  *   build-time.
  */
  class decoder {
    public:
      decoder(input &in) : _in(&in), _state(STATE_TYPE), _currentLength(0), _listener(nullptr) {};
      decoder(input &in, listener &listener);
      ~decoder() {};

      void run();
      bool next();
      void set_listener(listener &listener_instance);
      inline bool failed() {  return (_state == STATE_ERROR);  };

    private:
      input*        _in;
      decoder_state _state;
      int           _currentLength;
      listener*     _listener;
  };


  /*
  * This is the encoder class that initially came with cbor-cpp.
  */
  class encoder {
    public:
      encoder(output &out) : _out(&out) {};
      ~encoder() {};

      void write_int(int value);
      void write_int(int64_t value);
      void write_float(float value);
      void write_double(double value);
      void write_bytes(const uint8_t* data, uint32_t size);
      void write_string(const char* data, uint32_t size);
      void write_string(const char* str);
      inline void write_int(unsigned int v) {      write_type_value(0, v);     }; // TODO: This might be a use-case for type auto.
      inline void write_int(uint64_t v) {          write_type_value64(0, v);   }; // TODO: This might be a use-case for type auto.
      inline void write_tag(const uint32_t tag) {  write_type_value(6, tag);   };
      inline void write_array(uint32_t size) {     write_type_value(4, size);  };
      inline void write_map(uint32_t size) {       write_type_value(5, size);  };
      inline void write_special(uint32_t v) {      write_type_value(7, v);     };
      inline void write_bool(bool v) {             write_type_value(7, (v?21:20));   };

    private:
      output* _out;

      void write_type_value(int major_type, uint32_t value);
      void write_type_value64(int major_type, uint64_t value);
  };


  /*
  * This is a decoder class that prefers to rely on heap allocation of a
  *   complicated type-wrapper object to support usage that doesn't rely on
  *   object definition.
  * This pattern was added by CppPotpourri, and is probably the best choice for
  *   types covered by CppPotpourri's type-wrapping.
  */
  class decoder_c3pvalue {
    public:
      decoder_c3pvalue(input &in) : _in(&in), _working_value(nullptr), _currentLength(0), _state(STATE_TYPE) {};
      ~decoder_c3pvalue();

      C3PValue* next();
      inline bool finished() {  return ((STATE_ERROR == _state) | (END_OF_BYTES == _state));  };

    private:
      input*     _in;
      C3PValue*  _working_value;
      int           _currentLength;
      decoder_state _state;
  };

}

#endif // CBOR_CPP_CBOR_H
