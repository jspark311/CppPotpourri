/*
* This source file is the sum of all of the independent source files found in
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

#include "cbor.h"

using namespace cbor;


/*******************************************************************************
* output_stringbuilder
*******************************************************************************/

uint8_t* output_stringbuilder::data() {   return _str_bldr->string();   }
uint32_t output_stringbuilder::size() {   return _str_bldr->length();   }
int8_t output_stringbuilder::put_byte(uint8_t x) {  _str_bldr->concat(&x, 1);  return 0;  }   // TODO: StringBuilder return codes.
int8_t output_stringbuilder::put_bytes(const uint8_t* buf, int len) {  _str_bldr->concat((uint8_t*) buf, len);  return 0;  }  // TODO: StringBuilder return codes.


/*******************************************************************************
* output_dynamic
*******************************************************************************/

void output_dynamic::init(uint32_t initalCapacity) {
  this->_capacity = initalCapacity;
  this->_buffer = new uint8_t[initalCapacity];
  this->_offset = 0;
}

output_dynamic::output_dynamic() {
  init(256);
}

output_dynamic::output_dynamic(uint32_t inital_capacity) {
  init(inital_capacity);
}

output_dynamic::~output_dynamic() {
  delete _buffer;
}

uint8_t* output_dynamic::data() {   return _buffer;   }  // Cannot inline due to being virtual.
uint32_t output_dynamic::size() {   return _offset;   }  // Cannot inline due to being virtual.

int8_t output_dynamic::put_byte(uint8_t value) {
  int8_t ret = 0;
  if (_offset < _capacity) {
    _buffer[_offset++] = value;
  }
  else {
    _capacity *= 2;
    uint8_t* new_ptr = (uint8_t*) realloc(_buffer, _capacity);
    if (nullptr != new_ptr) {
      _buffer = new_ptr;
    }
    else {
      ret = -1;
    }
    _buffer[_offset++] = value;
  }
  return ret;
}

int8_t output_dynamic::put_bytes(const uint8_t* data, int size) {
  int8_t ret = 0;
  while((_offset + size) > _capacity) {
    _capacity *= 2;
    uint8_t* new_ptr = (uint8_t*) realloc(_buffer, _capacity);
    if (nullptr != new_ptr) {
      _buffer = new_ptr;
    }
    else {
      ret = -1;
    }
  }
  memcpy(_buffer + _offset, data, size);
  _offset += size;
  return ret;
}


/*******************************************************************************
* output_static
*******************************************************************************/

output_static::output_static(uint32_t cap, uint8_t* buf) : _buffer(buf), _capacity(cap), _offset(0), _should_free(false) {
  if ((nullptr == _buffer) & (_capacity > 0)) {
    _buffer = (uint8_t*) malloc(_capacity);
    if (nullptr != _buffer) {
      _should_free = true;
    }
    else {
      _capacity = 0;
    }
  }
}

output_static::~output_static() {
  if (_should_free & (nullptr != _buffer)) {
    delete _buffer;
    _buffer = nullptr;
  }
}

int8_t output_static::put_byte(uint8_t value) {
  if (_offset < _capacity) {
    _buffer[_offset++] = value;
    return 0;
  }
  else {
    logger("buffer overflow error");
    return -1;
  }
}

int8_t output_static::put_bytes(const uint8_t* data, int size) {
  if (_offset + size - 1 < _capacity) {
    memcpy(_buffer + _offset, data, size);
    _offset += size;
    return 0;
  }
  else {
    logger("buffer overflow error");
    return -1;
  }
}

uint8_t* output_static::data() {  return _buffer;  }  // Cannot inline due to being virtual.
uint32_t output_static::size() {  return _offset;  }  // Cannot inline due to being virtual.



/*******************************************************************************
* input
*******************************************************************************/

input::input(void *data, int size) {
  _data = (uint8_t*)data;
  _size = size;
  _offset = 0;
}

input::~input() {}


bool    input::has_bytes(int count) {   return (_size - _offset) >= count;  }
uint8_t input::get_byte() {             return _data[_offset++];            }

uint16_t input::get_short() {
  uint16_t value = ((uint16_t) _data[_offset] << 8) | ((uint16_t) _data[_offset + 1]);
  _offset += 2;
  return value;
}

uint32_t input::get_int() {
  uint32_t value = \
  ((uint32_t) _data[_offset    ] << 24) | ((uint32_t) _data[_offset + 1] << 16) |
  ((uint32_t) _data[_offset + 2] << 8 ) | ((uint32_t) _data[_offset + 3]);
  _offset += 4;
  return value;
}

float input::get_float() {
  uint8_t value[4] = {
    _data[_offset + 3],
    _data[_offset + 2],
    _data[_offset + 1],
    _data[_offset + 0]
  };
  _offset += 4;
  return *((float*) (&value[0]));
}

double input::get_double() {
  double ret;
  uint8_t* ptr = (uint8_t*)(void*) &ret;
  *(ptr + 0) = _data[_offset + 7];
  *(ptr + 1) = _data[_offset + 6];
  *(ptr + 2) = _data[_offset + 5];
  *(ptr + 3) = _data[_offset + 4];
  *(ptr + 4) = _data[_offset + 3];
  *(ptr + 5) = _data[_offset + 2];
  *(ptr + 6) = _data[_offset + 1];
  *(ptr + 7) = _data[_offset + 0];
  _offset += 8;
  return ret;
}

uint64_t input::get_long() {
  uint64_t value = ((uint64_t) _data[_offset] << 56) |
  ((uint64_t) _data[_offset +1] << 48) | ((uint64_t) _data[_offset +2] << 40) |
  ((uint64_t) _data[_offset +3] << 32) | ((uint64_t) _data[_offset +4] << 24) |
  ((uint64_t) _data[_offset +5] << 16) | ((uint64_t) _data[_offset +6] << 8 ) |
  ((uint64_t) _data[_offset +7]);
  _offset += 8;
  return value;
}

void input::get_bytes(void *to, int count) {
  memcpy(to, _data + _offset, count);
  _offset += count;
}


/*******************************************************************************
* encoder
*******************************************************************************/

encoder::encoder(output &out) {
  _out = &out;
}

encoder::~encoder() {}


void encoder::write_type_value(int major_type, uint32_t value) {
  major_type <<= 5;
  if (value < 24) {
    _out->put_byte((uint8_t) (major_type | value));
  }
  else if(value < 256) {
    _out->put_byte((uint8_t) (major_type | 24));
    _out->put_byte((uint8_t) value);
  }
  else if(value < 65536) {
    _out->put_byte((uint8_t) (major_type | 25));
    _out->put_byte((uint8_t) (value >> 8));
    _out->put_byte((uint8_t) value);
  }
  else {
    _out->put_byte((uint8_t) (major_type | 26));
    _out->put_byte((uint8_t) (value >> 24));
    _out->put_byte((uint8_t) (value >> 16));
    _out->put_byte((uint8_t) (value >> 8));
    _out->put_byte((uint8_t) value);
  }
}

void encoder::write_type_value64(int major_type, uint64_t value) {
  major_type <<= 5;
  if (value < 24ULL) {
    _out->put_byte((uint8_t) (major_type | value));
  }
  else if(value < 256ULL) {
    _out->put_byte((uint8_t) (major_type | 24));
    _out->put_byte((uint8_t) value);
  }
  else if(value < 65536ULL) {
    _out->put_byte((uint8_t) (major_type | 25));
    _out->put_byte((uint8_t) (value >> 8));
  }
  else if(value < 4294967296ULL) {
    _out->put_byte((uint8_t) (major_type | 26));
    _out->put_byte((uint8_t) (value >> 24));
    _out->put_byte((uint8_t) (value >> 16));
    _out->put_byte((uint8_t) (value >> 8));
    _out->put_byte((uint8_t) value);
  }
  else {
    _out->put_byte((uint8_t) (major_type | 27));
    _out->put_byte((uint8_t) (value >> 56));
    _out->put_byte((uint8_t) (value >> 48));
    _out->put_byte((uint8_t) (value >> 40));
    _out->put_byte((uint8_t) (value >> 32));
    _out->put_byte((uint8_t) (value >> 24));
    _out->put_byte((uint8_t) (value >> 16));
    _out->put_byte((uint8_t) (value >> 8));
    _out->put_byte((uint8_t) value);
  }
}


void encoder::write_int(int64_t value) {
  if(value < 0) {
    write_type_value(1, (uint64_t) -(value+1));
  }
  else {
    write_type_value(0, (uint64_t) value);
  }
}

void encoder::write_int(int value) {
  if(value < 0) {
    write_type_value(1, (uint32_t) -(value+1));
  }
  else {
    write_type_value(0, (uint32_t) value);
  }
}

void encoder::write_float(float value) {
  void* punny = &value;
  _out->put_byte((uint8_t) (7<<5) | 26);
  _out->put_byte(*((uint8_t*) punny+3));
  _out->put_byte(*((uint8_t*) punny+2));
  _out->put_byte(*((uint8_t*) punny+1));
  _out->put_byte(*((uint8_t*) punny+0));
}

void encoder::write_double(double value) {
  void* punny = &value;
  _out->put_byte((uint8_t) (7<<5) | 27);
  _out->put_byte(*((uint8_t*) punny+7));
  _out->put_byte(*((uint8_t*) punny+6));
  _out->put_byte(*((uint8_t*) punny+5));
  _out->put_byte(*((uint8_t*) punny+4));
  _out->put_byte(*((uint8_t*) punny+3));
  _out->put_byte(*((uint8_t*) punny+2));
  _out->put_byte(*((uint8_t*) punny+1));
  _out->put_byte(*((uint8_t*) punny+0));
}

void encoder::write_bytes(const uint8_t* data, uint32_t size) {
  write_type_value(2, size);
  _out->put_bytes(data, size);
}

void encoder::write_string(const char *data, uint32_t size) {
  write_type_value(3, size);
  _out->put_bytes((const uint8_t* ) data, size);
}

void encoder::write_string(const char* str) {
  uint32_t len = strlen(str);
  write_type_value(3, len);
  _out->put_bytes((const uint8_t* ) str, len);
}


/*******************************************************************************
* decoder
*******************************************************************************/
decoder::decoder(input &in) {
  _in = &in;
  _state = STATE_TYPE;
}

decoder::decoder(input &in, listener &listener) {
  _in = &in;
  _listener = &listener;
  _state = STATE_TYPE;
}

decoder::~decoder() {}


void decoder::set_listener(listener &listener_instance) {
  _listener = &listener_instance;
}


void decoder::run() {
  uint32_t temp;
  while (1) {  // NOTE: landmine.
    if(STATE_TYPE == _state) {
      if(_in->has_bytes(1)) {
        uint8_t type = _in->get_byte();
        uint8_t majorType = type >> 5;
        uint8_t minorType = (uint8_t) (type & 31);

        switch (majorType) {
          case 0: // positive integer
          if (minorType < 24) {
            _listener->on_integer((uint8_t) minorType);
          } else if(minorType == 24) { // 1 byte
            _currentLength = 1;
            _state = STATE_PINT;
          } else if(minorType == 25) { // 2 byte
            _currentLength = 2;
            _state = STATE_PINT;
          } else if(minorType == 26) { // 4 byte
            _currentLength = 4;
            _state = STATE_PINT;
          } else if(minorType == 27) { // 8 byte
            _currentLength = 8;
            _state = STATE_PINT;
          } else {
            _state = STATE_ERROR;
            _listener->on_error("invalid integer type");
          }
          break;
          case 1: // negative integer
          if (minorType < 24) {
            _listener->on_integer((int8_t) (0xFF - minorType));
          } else if(minorType == 24) { // 1 byte
            _currentLength = 1;
            _state = STATE_NINT;
          } else if(minorType == 25) { // 2 byte
            _currentLength = 2;
            _state = STATE_NINT;
          } else if(minorType == 26) { // 4 byte
            _currentLength = 4;
            _state = STATE_NINT;
          } else if(minorType == 27) { // 8 byte
            _currentLength = 8;
            _state = STATE_NINT;
          } else {
            _state = STATE_ERROR;
            _listener->on_error("invalid integer type");
          }
          break;
          case 2: // bytes
          if(minorType < 24) {
            _state = STATE_BYTES_DATA;
            _currentLength = minorType;
          } else if(minorType == 24) {
            _state = STATE_BYTES_SIZE;
            _currentLength = 1;
          } else if(minorType == 25) { // 2 byte
            _currentLength = 2;
            _state = STATE_BYTES_SIZE;
          } else if(minorType == 26) { // 4 byte
            _currentLength = 4;
            _state = STATE_BYTES_SIZE;
          } else if(minorType == 27) { // 8 byte
            _currentLength = 8;
            _state = STATE_BYTES_SIZE;
          } else {
            _state = STATE_ERROR;
            _listener->on_error("invalid bytes type");
          }
          break;
          case 3: // string
          if(minorType < 24) {
            _state = STATE_STRING_DATA;
            _currentLength = minorType;
          } else if(minorType == 24) {
            _state = STATE_STRING_SIZE;
            _currentLength = 1;
          } else if(minorType == 25) { // 2 byte
            _currentLength = 2;
            _state = STATE_STRING_SIZE;
          } else if(minorType == 26) { // 4 byte
            _currentLength = 4;
            _state = STATE_STRING_SIZE;
          } else if(minorType == 27) { // 8 byte
            _currentLength = 8;
            _state = STATE_STRING_SIZE;
          } else {
            _state = STATE_ERROR;
            _listener->on_error("invalid string type");
          }
          break;
          case 4: // array
          if(minorType < 24) {
            _listener->on_array(minorType);
          } else if(minorType == 24) {
            _state = STATE_ARRAY;
            _currentLength = 1;
          } else if(minorType == 25) { // 2 byte
            _currentLength = 2;
            _state = STATE_ARRAY;
          } else if(minorType == 26) { // 4 byte
            _currentLength = 4;
            _state = STATE_ARRAY;
          } else if(minorType == 27) { // 8 byte
            _currentLength = 8;
            _state = STATE_ARRAY;
          } else {
            _state = STATE_ERROR;
            _listener->on_error("invalid array type");
          }
          break;
          case 5: // map
          if(minorType < 24) {
            _listener->on_map(minorType);
          } else if(minorType == 24) {
            _state = STATE_MAP;
            _currentLength = 1;
          } else if(minorType == 25) { // 2 byte
            _currentLength = 2;
            _state = STATE_MAP;
          } else if(minorType == 26) { // 4 byte
            _currentLength = 4;
            _state = STATE_MAP;
          } else if(minorType == 27) { // 8 byte
            _currentLength = 8;
            _state = STATE_MAP;
          } else {
            _state = STATE_ERROR;
            _listener->on_error("invalid array type");
          }
          break;
          case 6: // tag
          if(minorType < 24) {
            _listener->on_tag(minorType);
          } else if(minorType == 24) {
            _state = STATE_TAG;
            _currentLength = 1;
          } else if(minorType == 25) { // 2 byte
            _currentLength = 2;
            _state = STATE_TAG;
          } else if(minorType == 26) { // 4 byte
            _currentLength = 4;
            _state = STATE_TAG;
          } else if(minorType == 27) { // 8 byte
            _currentLength = 8;
            _state = STATE_TAG;
          } else {
            _state = STATE_ERROR;
            _listener->on_error("invalid tag type");
          }
          break;
          case 7: // special
          if(minorType < 24) {
            _listener->on_special(minorType);
          } else if(minorType == 24) {
            _state = STATE_SPECIAL;
            _currentLength = 1;
          } else if(minorType == 25) { // 2 byte
            _currentLength = 2;
            _state = STATE_SPECIAL;
          } else if(minorType == 26) { // 4 byte
            _currentLength = 4;
            _state = STATE_SPECIAL;
          } else if(minorType == 27) { // 8 byte
            _currentLength = 8;
            _state = STATE_SPECIAL;
          } else {
            _state = STATE_ERROR;
            _listener->on_error("invalid special type");
          }
          break;
        }
      } else break;
    } else if(_state == STATE_PINT) {
      if(_in->has_bytes(_currentLength)) {
        switch(_currentLength) {
          case 1:
            _listener->on_integer((uint8_t) _in->get_byte());
            _state = STATE_TYPE;
            break;
          case 2:
            _listener->on_integer((uint16_t)_in->get_short());
            _state = STATE_TYPE;
            break;
          case 4:
            temp = _in->get_int();
            if (temp <= ((uint32_t) INT32_MAX << 1) + 1) {  // Unsigned int can take the extra bit.
              _listener->on_integer((uint32_t) temp);
            }
            else {  // Signed integers need to grow one byte.
              _listener->on_extra_integer(temp, 1);
            }
            _state = STATE_TYPE;
            break;
          case 8:
            _listener->on_extra_integer(_in->get_long(), 1);
            _state = STATE_TYPE;
            break;
        }
      } else break;
    } else if(_state == STATE_NINT) {
      if(_in->has_bytes(_currentLength)) {
        switch(_currentLength) {
          case 1:
          {
            uint8_t diff = _in->get_byte()+1;
            if (diff < 128u) {
              _listener->on_integer((int8_t) -(diff));
            }
            else {
              _listener->on_integer((int16_t) -(diff));
            }
          }
          _state = STATE_TYPE;
          break;
          case 2:
          {
            uint16_t diff = _in->get_short()+1;
            if (diff < 32768u) {
              _listener->on_integer((int16_t) -(diff));
            }
            else {
              _listener->on_integer((int32_t) -(diff));
            }
          }
          _state = STATE_TYPE;
          break;
          case 4:
          temp = _in->get_int();
          if(temp <= INT32_MAX) {
            _listener->on_integer(((int32_t) -(temp+1)));
          } else if(temp == 2147483648u) {
            _listener->on_integer((int32_t) INT32_MIN);
          } else {
            _listener->on_extra_integer(temp, -1);
          }
          _state = STATE_TYPE;
          break;
          case 8:
          _listener->on_extra_integer(_in->get_long(), -1);
          break;
        }
      } else break;
    } else if(_state == STATE_BYTES_SIZE) {
      if(_in->has_bytes(_currentLength)) {
        switch(_currentLength) {
          case 1:
          _currentLength = _in->get_byte();
          _state = STATE_BYTES_DATA;
          break;
          case 2:
          _currentLength = _in->get_short();
          _state = STATE_BYTES_DATA;
          break;
          case 4:
          _currentLength = _in->get_int();
          _state = STATE_BYTES_DATA;
          break;
          case 8:
          _state = STATE_ERROR;
          _listener->on_error("extra long bytes");
          break;
        }
      } else break;
    } else if(_state == STATE_BYTES_DATA) {
      if(_in->has_bytes(_currentLength)) {
        uint8_t data[_currentLength] = {0, };
        _in->get_bytes(data, _currentLength);
        _state = STATE_TYPE;
        _listener->on_bytes(data, _currentLength);
      } else break;
    } else if(_state == STATE_STRING_SIZE) {
      if(_in->has_bytes(_currentLength)) {
        switch(_currentLength) {
          case 1:
          _currentLength = _in->get_byte();
          _state = STATE_STRING_DATA;
          break;
          case 2:
          _currentLength = _in->get_short();
          _state = STATE_STRING_DATA;
          break;
          case 4:
          _currentLength = _in->get_int();
          _state = STATE_STRING_DATA;
          break;
          case 8:
          _state = STATE_ERROR;
          _listener->on_error("extra long array");
          break;
        }
      } else break;
    } else if(_state == STATE_STRING_DATA) {
      if(_in->has_bytes(_currentLength)) {
        uint8_t data[_currentLength + 1];
        _in->get_bytes(data, _currentLength);
        data[_currentLength] = '\0';
        _state = STATE_TYPE;
        _listener->on_string((char*) data);
      } else break;
    } else if(_state == STATE_ARRAY) {
      if(_in->has_bytes(_currentLength)) {
        switch(_currentLength) {
          case 1:
          _listener->on_array(_in->get_byte());
          _state = STATE_TYPE;
          break;
          case 2:
          _listener->on_array(_currentLength = _in->get_short());
          _state = STATE_TYPE;
          break;
          case 4:
          _listener->on_array(_in->get_int());
          _state = STATE_TYPE;
          break;
          case 8:
          _state = STATE_ERROR;
          _listener->on_error("extra long array");
          break;
        }
      } else break;
    } else if(_state == STATE_MAP) {
      if(_in->has_bytes(_currentLength)) {
        switch(_currentLength) {
          case 1:
          _listener->on_map(_in->get_byte());
          _state = STATE_TYPE;
          break;
          case 2:
          _listener->on_map(_currentLength = _in->get_short());
          _state = STATE_TYPE;
          break;
          case 4:
          _listener->on_map(_in->get_int());
          _state = STATE_TYPE;
          break;
          case 8:
          _state = STATE_ERROR;
          _listener->on_error("extra long map");
          break;
        }
      } else break;
    } else if(_state == STATE_TAG) {
      if(_in->has_bytes(_currentLength)) {
        switch(_currentLength) {
          case 1:
          _listener->on_tag(_in->get_byte());
          _state = STATE_TYPE;
          break;
          case 2:
          _listener->on_tag(_in->get_short());
          _state = STATE_TYPE;
          break;
          case 4:
          _listener->on_tag(_in->get_int());
          _state = STATE_TYPE;
          break;
          case 8:
          _listener->on_extra_tag(_in->get_long());
          _state = STATE_TYPE;
          break;
        }
      } else break;
    } else if(_state == STATE_SPECIAL) {
      if (_in->has_bytes(_currentLength)) {
        switch (_currentLength) {
          case 1:
          _listener->on_special(_in->get_byte());
          _state = STATE_TYPE;
          break;
          case 2:
          _listener->on_special(_in->get_short());
          _state = STATE_TYPE;
          break;
          case 4:   // A float32
          _listener->on_float32(_in->get_float());
          _state = STATE_TYPE;
          break;
          case 8:
          //_listener->on_extra_special(_in->get_long());
          _listener->on_double(_in->get_double());
          _state = STATE_TYPE;
          break;
        }
      } else break;
    } else if(_state == STATE_ERROR) {
      break;
    } else {
      logger("UNKNOWN STATE");
    }
  }
}
