/* File: C3PNumericVolume.h
* Author: J. Ian Lindsay
* Date: 2026.01.28
*
* Our wrapper class for numeric volume data.
* - Supports external buffers (non-owning) via setBuffer()/ctor(x,y,z,buf).
* - Supports lazy heap allocation on first write (setValue()/wipe()) if no buffer.
* - setBufferByCopy() claims its own heap buffer and copies the external data.
* - "locked" prevents mutation (writes, wipe, size changes that would realloc).
* - "dirty" is set on any successful mutation (write, wipe, size change + realloc).
*
* TODO: 3-space curve to keep data contiguous in both linear and presented
*   representations. But not until performance becomes an issue.
*/

#ifndef __C3P_TYPE_NUMERIC_CUBE_H
#define __C3P_TYPE_NUMERIC_CUBE_H

#include <stdlib.h>   // malloc/free
#include <string.h>   // memset/memcpy/strcmp
#include <math.h>     // sqrt

#include "Meta/Rationalizer.h"
#include "C3PStatBlock.h"
#include "EnumeratedTypeCodes.h"
#include "CppPotpourri.h"
#include "StringBuilder.h"

#if defined(__BUILD_HAS_CBOR)
  #include "cbor-cpp/cbor.h"
#endif


/* Class flags */
#define C3P_CUBE_FLAG_BUFFER_OURS     0x0800  // We are responsible for freeing the buffer.
#define C3P_CUBE_FLAG_BUFFER_LOCKED   0x1000  // Buffer should not be modified when set.
#define C3P_CUBE_FLAG_IS_DIRTY        0x4000  // The data is dirty.


template <class T> class C3PNumericVolume : public C3PStatBlock<T> {
  public:
    /* Constructors do nothing but init values. */
    C3PNumericVolume(uint16_t x, uint16_t y, uint16_t z, uint8_t* buf) : C3PStatBlock<T>((T*) buf, (x*y*z)), _x(x), _y(y), _z(z), _volumeflags(0), _buffer(buf) {};
    C3PNumericVolume(uint16_t x, uint16_t y, uint16_t z) : C3PNumericVolume(x, y, z, nullptr) {};
    C3PNumericVolume() : C3PNumericVolume(0, 0, 0, nullptr) {};
    ~C3PNumericVolume();

    bool setBuffer(uint8_t* buf);         // Attaches an external buffer (non-owning).
    bool setBufferByCopy(uint8_t* src);   // Copies from external memory into a local buffer.
    bool setSize(const uint16_t NEW_X, const uint16_t NEW_Y, const uint16_t NEW_Z);

    /* Consolidated lazy-allocation gate. */
    bool allocated();
    uint32_t bytesUsed();

    /* Accessors */
    T getValue(uint16_t x, uint16_t y, uint16_t z);
    bool setValue(uint16_t x, uint16_t y, uint16_t z, T val);
    void wipe();

    inline uint16_t width() { return _x; };
    inline uint16_t height() { return _y; };
    inline uint16_t depth() { return _z; };
    inline uint8_t* buffer() { return _buffer; };
    inline uint32_t valueCount(){ return ((uint32_t) _x * (uint32_t) _y * (uint32_t) _z); };
    inline bool locked() { return _volume_flag(C3P_CUBE_FLAG_BUFFER_LOCKED); };
    inline bool dirty() { return _volume_flag(C3P_CUBE_FLAG_IS_DIRTY); };

    void printDebug(StringBuilder*);
    int serialize(StringBuilder* out, const TCode FORMAT);
    int deserialize(StringBuilder* in, const TCode FORMAT);


  protected:
    uint16_t _x;
    uint16_t _y;
    uint16_t _z;
    uint16_t _volumeflags;

    inline T* _get_buffer() { return ((T*) _buffer); };
    inline void _is_dirty(bool x) { _volume_set_flag(C3P_CUBE_FLAG_IS_DIRTY, x); };
    inline void _lock(bool x) { _volume_set_flag(C3P_CUBE_FLAG_BUFFER_LOCKED, x); };

    int8_t _buffer_allocator();

    /* Linearizes the X/Y/Z coordinates in preparation for array indexing. */
    inline uint32_t _value_index(uint32_t x, uint32_t y, uint32_t z) { return ((z * (uint32_t) _y * (uint32_t) _x) + (y * (uint32_t) _x) + x); };

    /* Returns the byte offset in the buffer that holds the value given by coordinates. */
    inline uint32_t _value_offset(uint16_t x, uint16_t y, uint16_t z) { return (_value_index((uint32_t) x, (uint32_t) y, (uint32_t) z) * (uint32_t) sizeof(T)); };

    inline bool _is_ours() { return _volume_flag(C3P_CUBE_FLAG_BUFFER_OURS); };
    inline void _is_ours(bool l) { _volume_set_flag(C3P_CUBE_FLAG_BUFFER_OURS, l); };

    inline uint16_t _volume_flags() { return _volumeflags; };
    inline bool _volume_flag(uint16_t _flag) { return (0 != (_volumeflags & _flag)); };
    inline void _volume_flip_flag(uint16_t _flag) { _volumeflags ^= _flag; };
    inline void _volume_clear_flag(uint16_t _flag) { _volumeflags &= ~_flag; };
    inline void _volume_set_flag(uint16_t _flag) { _volumeflags |= _flag; };
    inline void _volume_set_flag(uint16_t _flag, bool nu) { if (nu) _volumeflags |= _flag; else _volumeflags &= ~_flag; };

  private:
    uint8_t* _buffer;  // Extending classes can _get_buffer().

    void _mark_dirty();
    void _release_owned_buffer();
    bool _resize_owned_buffer(const uint32_t OLD_BYTES, const uint32_t NEW_BYTES);
};


/* Destructor. Frees a held buffer if it was allocated locally. */
template <class T> C3PNumericVolume<T>::~C3PNumericVolume() {
  _lock(true);
  if (_is_ours() && (nullptr != _buffer)) {
    uint8_t* tmp = _buffer;
    _buffer = nullptr;
    free((void*) tmp);
  }
  _buffer = nullptr;
  _x = 0;
  _y = 0;
  _z = 0;
};


/* Attaches an external buffer (non-owning). */
template <class T> bool C3PNumericVolume<T>::setBuffer(uint8_t* buf) {
  bool ret = true;
  if (locked()) {
    ret = false;
  }
  else {
    _release_owned_buffer();
    _buffer = buf;
    /* Attaching a buffer is not inherently a mutation of its contents. */
    _is_dirty(false);
    this->invalidateStats();
  }
  return ret;
};


/* Copies from external memory into an owned buffer sized to this volume. */
template <class T> bool C3PNumericVolume<T>::setBufferByCopy(uint8_t* src) {
  bool ret = true;
  if (locked() || (nullptr == src) || (0 == _x) || (0 == _y) || (0 == _z)) {
    ret = false;
  }
  else {
    /* Ensure we have an owned buffer of the right size. */
    if (allocated()) {
      memcpy((void*) _buffer, (const void*) src, (size_t) bytesUsed());
      _mark_dirty();
    }
  }
  this->invalidateStats();
  return ret;
}


template <class T> bool C3PNumericVolume<T>::setSize(const uint16_t NEW_X, const uint16_t NEW_Y, const uint16_t NEW_Z) {
  bool ret = true;
  if (locked()) {
    ret = false;
  }
  else if ((0 == NEW_X) || (0 == NEW_Y) || (0 == NEW_Z)) {
    ret = false;
  }
  else {
    const uint16_t OLD_X = _x;
    const uint16_t OLD_Y = _y;
    const uint16_t OLD_Z = _z;
    const bool SIZE_CHANGED = ((OLD_X != NEW_X) || (OLD_Y != NEW_Y) || (OLD_Z != NEW_Z));
    if (SIZE_CHANGED) {
      const uint32_t OLD_BYTES = ((uint32_t) OLD_X * (uint32_t) OLD_Y * (uint32_t) OLD_Z * (uint32_t) sizeof(T));
      _x = NEW_X;
      _y = NEW_Y;
      _z = NEW_Z;
      if (_is_ours() && (nullptr != _buffer)) {
        ret = _resize_owned_buffer(OLD_BYTES, (uint32_t) (_x * _y * _z) * (uint32_t) sizeof(T));
      }
      if (ret) {
        _mark_dirty();
      }
    }
  }
  return ret;
}


template <class T> void C3PNumericVolume<T>::wipe() {
  if (!locked()) {
    if (allocated()) {  // Lazy alloc
      const uint32_t BYTES_USED = bytesUsed();
      if (BYTES_USED > 0) {
        memset((void*) _buffer, 0, (size_t) BYTES_USED);
        _mark_dirty();
      }
    }
  }
}


template <class T> void C3PNumericVolume<T>::_mark_dirty() {
  _is_dirty(true);
  this->invalidateStats();
}


template <class T> T C3PNumericVolume<T>::getValue(uint16_t x, uint16_t y, uint16_t z) {
  T ret = (T) 0;
  if ((x < _x) && (y < _y) && (z < _z) && (0 != _x) && (0 != _y) && (0 != _z)) {
    if (allocated()) {  // Lazy alloc on READ
      const uint32_t OFF = _value_offset(x, y, z);
      ret = *((T*) (_buffer + OFF));
    }
  }
  return ret;
}


template <class T> bool C3PNumericVolume<T>::setValue(uint16_t x, uint16_t y, uint16_t z, T val) {
  bool ret = true;
  if (locked() || (x >= _x) || (y >= _y) || (z >= _z) || (0 == _x) || (0 == _y) || (0 == _z)) {
    ret = false;
  }
  else {
    if (allocated()) {
      const uint32_t OFF = _value_offset(x, y, z);
      *((T*) (_buffer + OFF)) = val;
      _mark_dirty();
    }
    else {
      ret = false;
    }
  }
  return ret;
}


/* Consolidated lazy-allocation gate. */
template <class T> bool C3PNumericVolume<T>::allocated() {
  return (nullptr == _buffer) ? (0 == _buffer_allocator()) : true;
}


template <class T> uint32_t C3PNumericVolume<T>::bytesUsed() {
  return ((nullptr != _buffer) ? (uint32_t) (valueCount() * (uint32_t) sizeof(T)) : 0);
}




template <class T> int8_t C3PNumericVolume<T>::_buffer_allocator() {
  int8_t ret = -1;
  const uint32_t NEW_BYTES = (uint32_t) (valueCount() * (uint32_t) sizeof(T));
  if ((nullptr == _buffer) & (0 < NEW_BYTES)) {
    _buffer = (uint8_t*) malloc((size_t) NEW_BYTES);
    if (nullptr != _buffer) {
      memset(_buffer, 0, NEW_BYTES);
      _is_ours(true);
      this->_set_stat_source_data((T*) _buffer, valueCount());
      ret = 0;
    }
  }
  return ret;
}


template <class T> void C3PNumericVolume<T>::_release_owned_buffer() {
  this->_set_stat_source_data(nullptr, 0);
  if (_is_ours() && (nullptr != _buffer)) {
    uint8_t* TMP = _buffer;
    _buffer = nullptr;
    free((void*) TMP);
  }
  _is_ours(false);
}


////////////////////////////////////////////////////////////////////////////////
// Stark Fist of Removal: This functions below this line are going to change
//   when the memory treatment by the API becomes fixed.
// _resize_owned_buffer()
////////////////////////////////////////////////////////////////////////////////
template <class T> bool C3PNumericVolume<T>::_resize_owned_buffer(const uint32_t OLD_BYTES, const uint32_t NEW_BYTES) {
  bool ret = true;
  if (!_is_ours() || (0 == NEW_BYTES)) {
    ret = false;
  }
  else if (NEW_BYTES == OLD_BYTES) {
    ret = true;
  }
  else {
    uint8_t* OLD_BUF = _buffer;
    uint8_t* NEW_BUF = (uint8_t*) malloc((size_t) NEW_BYTES);
    this->_set_stat_source_data(nullptr, 0);
    if (nullptr == NEW_BUF) {
      ret = false;
    }
    else {
      if ((nullptr != OLD_BUF) && (OLD_BYTES > 0)) {
        const uint32_t COPY_BYTES = (OLD_BYTES < NEW_BYTES) ? OLD_BYTES : NEW_BYTES;
        memset((void*) NEW_BUF, 0, (size_t) COPY_BYTES);
      }
      _buffer = NEW_BUF;
      if (nullptr != OLD_BUF) {
        free((void*) OLD_BUF);
      }
    }
  }
  return ret;
}


/*
* NOTE: Printing the buffer (and potential generation of stats) might take
* long enough to admit the possibility of shear between the rendered field of
* values vis-a-vis the stats.
* If that is important, the volume will have to be
* locked against updates unless other measures are taken by calling code to
* stop use of the value API while printing.
*/
template <class T> void C3PNumericVolume<T>::printDebug(StringBuilder* out) {
  C3PType* t_helper = getTypeHelper(tcodeForType(T(0)));
  if ((nullptr == t_helper) || !t_helper->is_fixed_length()) {
    return;
  }

  StringBuilder tmp("C3PNumericVolume");
  tmp.concatf("<%s> (%u x %u x %u) [\n", t_helper->NAME, _x, _y, _z);

  if (nullptr != _buffer) {
    for (uint16_t z = 0; z < _z; z++) {
      tmp.concatf("\t[z=%u]\n", z);
      for (uint16_t y = 0; y < _y; y++) {
        for (uint16_t x = 0; x < _x; x++) {
          tmp.concat((0 == x) ? "\t\t" : ",\t");
          const uint32_t IDX = _value_index(x, y, z);
          t_helper->to_string((void*) (((T*) _buffer) + IDX), &tmp);
        }
        tmp.concat("\n");
      }
    }
    tmp.concatf("] (%d bytes)\n", bytesUsed());
    this->_print_stats(&tmp);
  }
  else {
    tmp.concat("(unallocated)\n");
  }

  tmp.string();
  out->concatHandoff(&tmp);
}



template <class T> int C3PNumericVolume<T>::serialize(StringBuilder* out, const TCode FORMAT) {
  int ret = -1;  // We require data to exist for serialization (no implicit lazy-alloc-on-serialize).
  if ((nullptr == out) || (0 == _x) || (0 == _y) || (0 == _z) || (nullptr == _buffer)) { return ret; }

  C3PType* t_helper = getTypeHelper(tcodeForType(T(0)));
  if ((nullptr == t_helper) || !t_helper->is_fixed_length()) { return ret; }

  switch (FORMAT) {
    case TCode::STR:
      printDebug(out);
      break;

  #if defined(__BUILD_HAS_CBOR)
    case TCode::CBOR: {
      cbor::output_stringbuilder output(out);
      cbor::encoder encoder(output);

      // Encode this into IANA space as a vendor code.
      encoder.write_tag(C3P_CBOR_VENDOR_CODE | TcodeToInt(FORMAT));

      // {"C3PNumericVolume": {"tc":..,"w":..,"h":..,"d":..,"flg":..,"dat":[..]}}
      encoder.write_map(1);  encoder.write_string("C3PNumericVolume");
        encoder.write_map(6);
          encoder.write_string("tc");     encoder.write_int((uint8_t) t_helper->TCODE);
          encoder.write_string("w");      encoder.write_int((uint16_t) _x);
          encoder.write_string("h");      encoder.write_int((uint16_t) _y);
          encoder.write_string("d");      encoder.write_int((uint16_t) _z);
          encoder.write_string("flg");    encoder.write_int((uint16_t) _volumeflags);

      const uint32_t VCNT = valueCount();
          encoder.write_string("dat");    encoder.write_array((int) VCNT);
      for (uint32_t i = 0; i < VCNT; i++) {
        t_helper->serialize((void*) (((T*) _buffer) + i), out, FORMAT);
      }
      ret = 0;
    } break;
  #endif  // __BUILD_HAS_CBOR

    default:
      break;
  }
  return ret;
}


// TODO: This is (IMO) an ugly way to write this.
template <class T> int C3PNumericVolume<T>::deserialize(StringBuilder* in, const TCode FORMAT) {
  int ret = -1;

  switch (FORMAT) {
  #if defined(__BUILD_HAS_CBOR)
    case TCode::CBOR: {
      if ((nullptr == in) || (in->length() <= 0)) { return ret; }

      class C3PNumericVolumeListener : public cbor::listener {
        public:
          C3PNumericVolumeListener(C3PNumericVolume* cb) : _cb(cb) {}
          ~C3PNumericVolumeListener() {}

          bool failed() const { return _failed; }

          // Integer callbacks
          void on_integer(int8_t v) override { _on_numeric((int64_t) v); }
          void on_integer(int16_t v) override { _on_numeric((int64_t) v); }
          void on_integer(int32_t v) override { _on_numeric((int64_t) v); }
          void on_integer(int64_t v) override { _on_numeric((int64_t) v); }
          void on_integer(uint8_t v) override { _on_numeric((uint64_t) v); }
          void on_integer(uint16_t v) override { _on_numeric((uint64_t) v); }
          void on_integer(uint32_t v) override { _on_numeric((uint64_t) v); }
          void on_integer(uint64_t v) override { _on_numeric((uint64_t) v); }

          // Float callbacks
          void on_float32(float v) override { _on_float((double) v); }
          void on_double(double v) override { _on_float(v); }

          // Unused callbacks
          void on_bytes(uint8_t*, int) override {}
          void on_bool(bool) override {}
          void on_null() override {}
          void on_undefined() override {}
          void on_special(unsigned int) override {}
          void on_extra_integer(uint64_t, int) override {}
          void on_extra_tag(uint64_t) override {}
          void on_extra_special(uint64_t) override {}
          void on_error(const char*) override { _failed = true; }
          void on_tag(unsigned int) override {}  // Tag is accept and ignore

          void on_map(int) override {
            if (_failed) return;
            if (!_in_outer_map) {
              _in_outer_map = true;
              _expecting_key = true;
              return;
            }
            if (_in_outer_map && !_in_inner_map) {  // Outer map value for key "C3PNumericVolume" is the inner map.
              if (!_expecting_key && (0 == strcmp(_last_key, "C3PNumericVolume"))) {
                _in_inner_map = true;
                _expecting_key = true;
                return;
              }
            }
            // Deeper nesting not expected.
          }

          void on_array(int size) override {
            if (_failed) {  return;  }
            if (_in_inner_map && !_expecting_key && (0 == strcmp(_last_key, "dat"))) {
                // Value is the array container. "size" is the element count.
              if ((size <= 0) || (nullptr == _cb)) {
                _failed = true;
                return;
              }
              _in_dat = true;
              _dat_expected = (uint32_t) ((size < 0) ? 0 : size);
              _dat_seen = 0;
              _expecting_key = true;  // container consumed as value
            }
            // Deeper nesting not expected.
          }

          void on_string(char* str) override {
            if (_failed || (nullptr == str)) {  return;  }
            if (_in_outer_map && !_in_inner_map) {
              if (_expecting_key) {
                _copy_key(str);
                _expecting_key = false;  // next should be inner map
              }
              return;
            }

            if (_in_inner_map) {
              if (_expecting_key) {
                _copy_key(str);
                _expecting_key = false;
                return;
              }
              // Only string values are not expected in our schema.
              // Treat as failure to keep it strict.
              _failed = true;
            }
          };

        private:
          C3PNumericVolume<T>* _cb;
          bool _failed         = false;
          bool _in_outer_map   = false;
          bool _in_inner_map   = false;
          bool _expecting_key  = false;
          bool _in_dat         = false;

          char _last_key[24]   = {0};

          uint8_t  _tc         = 0;
          uint16_t _x          = 0;
          uint16_t _y          = 0;
          uint16_t _z          = 0;
          uint16_t _flg        = 0;

          uint32_t _dat_expected = 0;
          uint32_t _dat_seen     = 0;

          void _copy_key(const char* k) {
            memset(_last_key, 0, sizeof(_last_key));
            if (nullptr != k) {
              const uint32_t cpy = strict_min((uint32_t) strlen(k), (uint32_t) (sizeof(_last_key) - 1));
              memcpy(_last_key, k, cpy);
            }
          };

          C3PType* _t_helper() {
            if (0 == _tc) return nullptr;
            return getTypeHelper((TCode) _tc);
          };

          void _commit_header_if_ready() {
            if (!_failed) {
              if ((0 < _x) && (0 < _y) && (0 < _z) && (0 != _tc)) {
                C3PType* th = _t_helper();
                if ((nullptr == th) || !th->is_fixed_length()) {
                  _failed = true;
                  return;
                }
                _cb->_lock(false);
                if (!_cb->setSize(_x, _y, _z)) {
                  _failed = true;
                  return;
                }
                _cb->_volumeflags = _flg;
                // We require a buffer before data arrives.
                if (!_cb->allocated()) {
                  _failed = true;
                }
              }
            }
          };


          void _on_numeric(int64_t v) {
            if (_failed) return;

            if (_in_inner_map && _expecting_key) { _failed = true; return; }  // Structural violation.

            if (_in_inner_map && !_in_dat) {
              if (0 == strcmp(_last_key, "tc")) {        _tc = (uint8_t) v;   }
              else if (0 == strcmp(_last_key, "w")) {    _x = (uint16_t) v;   }
              else if (0 == strcmp(_last_key, "h")) {    _y = (uint16_t) v;   }
              else if (0 == strcmp(_last_key, "d")) {    _z = (uint16_t) v;   }
              else if (0 == strcmp(_last_key, "flg")) {  _flg = (uint16_t) v; }
              _expecting_key = true;
              _commit_header_if_ready();
              return;
            }

            if (_in_dat) {
              _commit_header_if_ready();
              if (_failed) return;
              if (_dat_seen >= _cb->valueCount()) { _failed = true; return; }
              _cb->_get_buffer()[_dat_seen] = (T) v;
              _dat_seen++;
              return;
            }
          };


          void _on_numeric(uint64_t v) { _on_numeric((int64_t) v); };


          void _on_float(double v) {
            if (_failed) return;
            if (_in_inner_map && !_in_dat) {
              // No float-valued header fields expected.
              _expecting_key = true;
              return;
            }
            if (_in_dat) {
              _commit_header_if_ready();
              if (_failed) return;
              if (_dat_seen >= _cb->valueCount()) { _failed = true; return; }
              _cb->_get_buffer()[_dat_seen] = (T) v;
              _dat_seen++;
              return;
            }
          };
      };

      cbor::input_stringbuilder input(in, true, false);
      C3PNumericVolumeListener listener(this);
      cbor::decoder decoder(input, listener);
      decoder.run();
      if (!listener.failed()) {
        _mark_dirty();
        ret = 0;
      }
    } break;
  #endif  // __BUILD_HAS_CBOR

    default:
      break;
  }
  return ret;
}

#endif  // __C3P_TYPE_NUMERIC_CUBE_H
