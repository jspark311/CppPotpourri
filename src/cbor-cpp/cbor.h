/*
* This header file is the sum of all of the independent header files found in
*   the original package. It was grouped together to aid repo orginization, and
*   has been mutated. The author's original license is reproduced below.
*                                                      ---J. Ian Lindsay
*
* TODO: Make all integers a specified width.
* TODO: Fully test changes.
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

#ifdef ARDUINO
  #include <Arduino.h>
#else
  #include <stdlib.h>
#endif


#include <stdio.h>
#define logger(line) fprintf(stderr, "%s:%d [%s]: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, line)
#define loggerf(format, ...) fprintf(stderr, "%s:%d [%s]: " format "\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, __VA_ARGS__)


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
    STATE_ERROR
  } decoder_state;



  class listener {
    public:
      virtual void on_integer(int8_t   value)        = 0;
      virtual void on_integer(int16_t  value)        = 0;
      virtual void on_integer(int32_t  value)        = 0;
      virtual void on_integer(uint8_t  value)        = 0;
      virtual void on_integer(uint16_t value)        = 0;
      virtual void on_integer(uint32_t value)        = 0;
      virtual void on_float32(float value)           = 0;
      virtual void on_double(double value)           = 0;
      virtual void on_bytes(uint8_t* data, int size) = 0;
      virtual void on_string(char* str)              = 0;
      virtual void on_array(int size)                = 0;
      virtual void on_map(int size)                  = 0;
      virtual void on_tag(uint tag)                  = 0;
      virtual void on_special(uint code)             = 0;
      virtual void on_error(const char *error)       = 0;

      virtual void on_extra_integer(uint64_t value, int sign) {}
      virtual void on_extra_tag(uint64_t tag) {}
      virtual void on_extra_special(uint64_t tag) {}
  };



  class input {
    public:
      input(void* data, int size);
      ~input();

      bool     has_bytes(int count);
      uint8_t get_byte();
      uint16_t get_short();
      uint     get_int();
      float    get_float();
      double   get_double();
      uint64_t get_long();
      void     get_bytes(void* to, int count);


    private:
      uint8_t* _data;
      int      _size;
      int      _offset;
  };



  class output {
    public:
      virtual uint8_t* data()                                   = 0;
      virtual uint     size()                                   = 0;
      virtual void     put_byte(uint8_t value)            = 0;
      virtual void     put_bytes(const uint8_t* data, int size) = 0;
  };



  class decoder {
    public:
      decoder(input &in);
      decoder(input &in, listener &listener);
      ~decoder();

      void run();
      void set_listener(listener &listener_instance);


    private:
      listener*     _listener;
      input*        _in;
      decoder_state _state;
      int           _currentLength;
  };



  class encoder {
    public:
      encoder(output &out);
      ~encoder();

      void write_int(int value);
      void write_int(int64_t value);
      inline void write_int(uint v) {           write_type_value(0, v);            };
      inline void write_int(uint64_t v) {       write_type_value(0, v);            };
      inline void write_tag(const uint tag) {   write_type_value(6, tag);          };
      inline void write_array(int size) {       write_type_value(4, (uint) size);  };
      inline void write_map(int size) {         write_type_value(5, (uint) size);  };
      inline void write_special(int v) {        write_type_value(7, (uint) v);     };

      void write_float(float value);
      void write_double(double value);
      void write_bytes(const uint8_t* data, uint size);
      void write_string(const char* data, uint size);
      void write_string(const char* str);


    private:
      output* _out;

      void write_type_value(int major_type, uint value);
      void write_type_value(int major_type, uint64_t value);
  };



  class output_dynamic : public output {
    public:
      output_dynamic();
      output_dynamic(uint inital_capacity);
      ~output_dynamic();

      virtual uint8_t* data();
      virtual uint size();
      virtual void put_byte(uint8_t value);
      virtual void put_bytes(const uint8_t* data, int size);

    private:
      uint8_t* _buffer;
      uint     _capacity;
      uint     _offset;

      void init(uint initalCapacity);
  };



  class output_static : public output {
    public:
      output_static(uint capacity);
      ~output_static();

      virtual uint8_t* getData();
      virtual uint getSize();
      virtual void put_byte(uint8_t value);
      virtual void put_bytes(const uint8_t* data, int size);

    private:
      uint8_t* _buffer;
      uint     _capacity;
      uint     _offset;
  };
}

#endif // CBOR_CPP_CBOR_H
