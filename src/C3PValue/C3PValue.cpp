/*
File:   C3PValue.cpp
Author: J. Ian Lindsay
Date:   2023.07.29

Copyright 2016 Manuvr, Inc

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

#include "C3PValue.h"
#include "../StringBuilder.h"
#include "../Identity/Identity.h"
#include "../StringBuilder.h"
#include "../AbstractPlatform.h"   // Only needed for logging.

/* CBOR support should probably be required to parse/pack. */
#if defined(CONFIG_C3P_CBOR)
  #include "../cbor-cpp/cbor.h"
#endif

/* Image support costs code size. Don't support it unless requested. */
#if defined(CONFIG_C3P_IMG_SUPPORT)
  #include "../Image/Image.h"
#endif

const char* const LOCAL_LOG_TAG = "C3PValue";


/*******************************************************************************
* Statics
*******************************************************************************/

/**
* We are being asked to inflate a C3PValue object from an unknown string of a
*   known format.
*/
C3PValue* C3PValue::deserialize(StringBuilder* input, const TCode FORMAT) {
  C3PValue* ret = nullptr;
  const uint32_t INPUT_LEN = input->length();
  switch (FORMAT) {
    case TCode::BINARY:
      if (INPUT_LEN > 1) {  // Need at least two bytes to be possibly valid.
        const TCode TC = (TCode) input->byteAt(0);
        C3PType* t_helper = getTypeHelper(TC);
        if (nullptr != t_helper) {
          if (t_helper->is_fixed_length()) {
            if ((INPUT_LEN-1) >= (uint32_t) input->length()) {
            }
            else {}  // We don't have enough bytes to parse this type.
          }
          else {
            // Variable-length types require that we try to parse them according
            //   to their type helper, and observe the outcome.
            //t_helper->is_punned_ptr()
            //ret = t_helper->deserialize(input, FORMAT);
          }
        }
      }
      break;

    case TCode::CBOR:
      {
        C3PValueDecoder decoder(input);
        ret = decoder.next();
      }
      break;

    default:  break;
  }
  return ret;
}



/*******************************************************************************
*   ___ _              ___      _ _              _      _
*  / __| |__ _ ______ | _ ) ___(_) |___ _ _ _ __| |__ _| |_ ___
* | (__| / _` (_-<_-< | _ \/ _ \ | / -_) '_| '_ \ / _` |  _/ -_)
*  \___|_\__,_/__/__/ |___/\___/_|_\___|_| | .__/_\__,_|\__\___|
*                                          |_|
* Constructors/destructors, class initialization functions and so-forth...
*******************************************************************************/

/**
* Protected delegate constructor.
* If _val_by_ref is false, it means there is no allocation, since the data can
*   be copied by value into the pointer space. This will also be true for types
*   that are passed as pointers anyway (Image*, Identity*, etc).
* If _val_by_ref is true, it means that _target_mem is a pointer to whatever
*   type is declared.
*/
C3PValue::C3PValue(const TCode TC, void* ptr) : _TCODE(TC), _set_trace(1), _target_mem(ptr) {
  _punned_ptr = typeIsPointerPunned(_TCODE);
  _val_by_ref = !_punned_ptr;
  _reap_val   = false;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    if (is_numeric()) {
      // Every numeric type that does not fit inside a void* on the target
      //   platform will be heap-allocated and zeroed on construction. C3PType
      //   is responsible for having homogenized this handling, and all we need
      //   to do is allocate the memory if the numeric type could not be
      //   type-punned.
      // This will cover such cases as DOUBLE and INT64 on 32-bit builds.
      if (!_punned_ptr) {
        // TODO: This is a constructor, and we thus have no clean way of
        //   handling heap-allocation failures. So we'll need a lazy-allocate
        //   arrangement eventually, in which case, this block will be redundant.
        const uint32_t TYPE_STORAGE_SIZE = t_helper->length(nullptr);
        _target_mem = malloc(TYPE_STORAGE_SIZE);   // Default value will be nonsense. Clobber it.
        if (nullptr != _target_mem) {
          _reap_val = true;
          if (nullptr == ptr) {
            memset(_target_mem, 0, TYPE_STORAGE_SIZE);
          }
          else {
            // TODO: Should never fail, because no type conversion is being done,
            //   but there is no return assurance for the situation TODO'd above.
            t_helper->set_from(_type_pun(), _TCODE, ptr);
          }
        }
      }
    }
    else if (t_helper->is_ptr_len()) {
      // The compound pointer-length types (BINARY, CBOR, etc) will have an
      //   indirected shim object to consolidate their parameter space into a
      //   single reference. That shim will be heap allocated.
      _target_mem = malloc(sizeof(C3PBinBinder));
      if (nullptr != _target_mem) {
        //((C3PBinBinder*) _target_mem)->tcode = TC;
        _val_by_ref = true;
        // TODO: Is this the correct choice? We know that we are responsible for
        //   freeing the C3PBinBinder we just created, and I don't want to mix
        //   semantic layers, nor do I want to extend flags into the shim (thus
        //   indirecting them).
        _reap_val   = false;
      }
    }
  }
}


C3PValue::~C3PValue() {
  if (nullptr != _target_mem) {
    // Some types have a shim to hold an explicit length, or other data. We
    //   construe the reap member to refer to the data itself, and not the shim.
    //   We will always free the shim.
    // TODO: allocate() and deallocate() should be added to the type template.
    if (_TCODE == TCode::BINARY) {
      if (_reap_val) {
        free(((C3PBinBinder*) _target_mem)->buf);
      }
      free(_target_mem);
    }
    //else if (_val_by_ref & _reap_val) {
    else if (_reap_val) {
      free(_target_mem);
    }
  }
  _target_mem = nullptr;
}


/*******************************************************************************
* Typed constructors that have nuanced handling, and can't be inlined.
*******************************************************************************/

C3PValue::C3PValue(float val) : C3PValue(TCode::FLOAT, nullptr) {
  uint8_t* src = (uint8_t*) &val;                  // To avoid inducing bugs
  *(((uint8_t*) &_target_mem) + 0) = *(src + 0);   //   related to alignment,
  *(((uint8_t*) &_target_mem) + 1) = *(src + 1);   //   we copy the value
  *(((uint8_t*) &_target_mem) + 2) = *(src + 2);   //   byte-wise into our own
  *(((uint8_t*) &_target_mem) + 3) = *(src + 3);   //   storage.
}

/*
* Construction from char* has semantics that imply the source buffer is
*   ephemeral, and ought to be copied into a memory region allocated and
*   owned by this class.
*/
C3PValue::C3PValue(char* val) : C3PValue(TCode::STR, nullptr) {
  bool failure = true;
  if (nullptr != val) {
    const uint32_t VAL_LENGTH = strlen(val);
    _target_mem = malloc(VAL_LENGTH + 1);
    if (nullptr != _target_mem) {
      memcpy(_target_mem, val, VAL_LENGTH);
      *((char*)_target_mem + VAL_LENGTH) = 0;
      reapValue(true);
      failure = false;
    }
  }
  if (failure) {
    // TODO: Log? Set error bit? Same issue as in the base constructor...
  }
}


C3PValue::C3PValue(uint8_t* val, uint32_t len) : C3PValue(TCode::BINARY, nullptr) {
  if (nullptr != _target_mem) {
    ((C3PBinBinder*) _target_mem)->buf   = (uint8_t*) val;
    ((C3PBinBinder*) _target_mem)->len   = len;
  }
}



/*******************************************************************************
* Type glue
*******************************************************************************/
bool C3PValue::is_ptr_len() {
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    return (t_helper->is_ptr_len());
  }
  return false;
}


/*******************************************************************************
* Basal accessors
* These functions ultimately wrap the C3PTypeConstraint template that handles
*   the type conversion matrix.
*******************************************************************************/
int8_t C3PValue::set_from(const TCode SRC_TYPE, void* src) {
  int8_t ret = -1;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    ret = t_helper->set_from(_type_pun(), SRC_TYPE, src);
  }
  if (0 == ret) {
    _set_trace++;
  }
  return ret;
}


int8_t C3PValue::set(C3PValue* src) {
  int8_t ret = -1;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    // NOTE: Private member accessed laterally...
    ret = t_helper->set_from(_type_pun(), src->tcode(), src->_type_pun());
  }
  if (0 == ret) {
    _set_trace++;
  }
  return ret;
}

int8_t C3PValue::get_as(const TCode DEST_TYPE, void* dest) {
  int8_t ret = -1;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    ret = t_helper->get_as(_type_pun(), DEST_TYPE, dest);
  }
  return ret;
}


// TODO: Audit. Might be wrong-headed.
int8_t C3PValue::get_as(uint8_t** v, uint32_t* l) {
  int8_t ret = -1;
  if ((nullptr != v) & (nullptr != l)) {
    ret--;
    if (nullptr != _target_mem) {
      *v = ((C3PBinBinder*) _target_mem)->buf;
      *l = ((C3PBinBinder*) _target_mem)->len;
      ret = 0;
    }
  }
  return ret;
}

// TODO: Audit. Might be wrong-headed.
int8_t C3PValue::set(uint8_t* src, uint32_t l, const TCode SRC_TYPE) {
  int8_t ret = -1;
  if (nullptr != _target_mem) {
    ((C3PBinBinder*) _target_mem)->buf = src;
    ((C3PBinBinder*) _target_mem)->len = l;
    _set_trace++;
    ret = 0;
  }
  return ret;
}


/*
* This optional accessor will allow the caller to discover if the value has
*   changed since its last check, and to update the check value if so.
*/
bool C3PValue::dirty(uint16_t* x) {
  bool ret = ((nullptr != x) && (*x != _set_trace));
  if (ret) {  *x = _set_trace;  }
  return ret;
}



/*******************************************************************************
* Type coercion functions.
* These functions will do their best to get or set the value after the implied
*   translation.
* NOTE: A call to get() that results in a loss of precision or truncation should
*   return the failure value. This may be construed as a true value by the
*   caller, and represents an API gap. The caller should check types, or use a
*   deliberately too-large type if there is doubt about a specific value's size.
* NOTE: A similar discipline applies to set(). If a truncation or LoP would
*   result in a differnt value than intended, set() should change nothing and
*   return -1.
*******************************************************************************/
unsigned int C3PValue::get_as_uint(int8_t* success) {
  uint32_t ret = 0;
  int8_t suc = get_as(TCode::UINT32, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}

int C3PValue::get_as_int(int8_t* success) {
  int32_t ret = 0;
  int8_t suc = get_as(TCode::INT32, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}

uint64_t C3PValue::get_as_uint64(int8_t* success) {
  uint64_t ret = 0;
  int8_t suc = get_as(TCode::UINT64, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}

int64_t C3PValue::get_as_int64(int8_t* success) {
  int64_t ret = 0;
  int8_t suc = get_as(TCode::INT64, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}

/* Casts the value into (!/= 0). */
bool C3PValue::get_as_bool(int8_t* success) {
  if (success) {  *success = true;  }
  return (0 != _target_mem);
}


float C3PValue::get_as_float(int8_t* success) {
  float ret = 0.0f;
  int8_t suc = get_as(TCode::FLOAT, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}


double C3PValue::get_as_double(int8_t* success) {
  double ret = 0.0d;
  int8_t suc = get_as(TCode::DOUBLE, (void*) &ret) + 1;
  if (success) {  *success = suc;  }
  return ret;
}

C3PBinBinder C3PValue::get_as_ptr_len(int8_t* success) {
  C3PBinBinder ret;
  int8_t suc = 1;
  if (is_ptr_len()) {
    suc = get_as(TCode::BINARY, (void*) &ret) + 1;
  }
  else {
    // TODO: This ought to properly coerce any not ptr/len into such a
    //   representation. But it is untested.
    ret.buf = (uint8_t*) _type_pun();
    ret.len = length();
  }

  if (success) {  *success = suc;  }
  return ret;
}


/*******************************************************************************
* Parsing/Packing
*******************************************************************************/
int8_t C3PValue::serialize(StringBuilder* output, const TCode FORMAT) {
  int8_t ret = -1;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    ret = t_helper->serialize(_type_pun(), output, FORMAT);
  }
  return ret;
}


//int8_t C3PValue::deserialize(StringBuilder* input, const TCode FORMAT) {
//  int8_t ret = -1;
//  C3PType* t_helper = getTypeHelper(_TCODE);
//  if (nullptr != t_helper) {
//    ret = t_helper->deserialize(_type_pun(), input, FORMAT);
//  }
//  return ret;
//}


/*
*/
uint32_t C3PValue::length() {
  uint32_t ret = 0;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    ret = t_helper->length(_type_pun());
  }
  return ret;
}



/**
* This function prints the KVP's value to the provided buffer.
*
* @param out is the buffer to receive the printed value.
*/
void C3PValue::toString(StringBuilder* out, bool include_type) {
  if (include_type) {
    out->concatf("(%s) ", typecodeToStr(_TCODE));
  }
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    t_helper->to_string(_type_pun(), out);
    //out->concatf("%08x", _type_pun());
  }
  else {
    const uint32_t L_ENDER = length();
    for (uint32_t n = 0; n < L_ENDER; n++) {
      out->concatf("%02x ", *((uint8_t*) _target_mem + n));
    }
  }
}



/*******************************************************************************
* C3PValueDecoder
*******************************************************************************/

bool C3PValueDecoder::_get_length_field(uint32_t* offset_ptr, uint64_t* val_ret, uint8_t minorType) {
  uint64_t value = 0;
  uint32_t offset = *offset_ptr;
  uint8_t buf[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t len_len = (1 << (minorType - 24));
  switch (len_len) {
    case 1:
    case 2:
    case 4:
    case 8:
      if (len_len == _in->copyToBuffer(buf, len_len, offset)) {
        value |= ((uint64_t) buf[0] << 56);
        value |= ((uint64_t) buf[1] << 48);
        value |= ((uint64_t) buf[2] << 40);
        value |= ((uint64_t) buf[3] << 32);
        value |= ((uint64_t) buf[4] << 24);
        value |= ((uint64_t) buf[5] << 16);
        value |= ((uint64_t) buf[6] << 8);
        value |= ((uint64_t) buf[7]);
        *val_ret = (value >> ((8 - len_len) << 3));  // Shift the buffer into the right orientation.
        *offset_ptr = (offset + len_len);
        return true;
      }
      break;
    default:
      c3p_log(LOG_LEV_ERROR, LOCAL_LOG_TAG, "Unsupported length field length (%u)", len_len);
      break;
  }
  return false;
}


/*
* Calling this function will return the next complete C3PValue that can be
*   parsed and consumed from the input.
*
* @param consume_unparsable will cause this function to consume bytes that it cannot parse.
*/
C3PValue* C3PValueDecoder::next(bool consume_unparsable) {
  if (nullptr == _in) {  return nullptr;  }   // Bailout
  const uint32_t INPUT_LEN = _in->length();
  C3PValue* value = nullptr;
  uint32_t _length_taken  = 0;   // Total bytes consumed during parse.
  bool cull_anyway = false;

  if (INPUT_LEN > 0) {
    uint64_t _length_extra  = 0;   // Length of data specified by the length field above.
    const uint8_t C_TYPE    = _in->byteAt(_length_taken++);   // The first byte
    const uint8_t MAJORTYPE = (uint8_t) (C_TYPE >> 5);
    const uint8_t MINORTYPE = (uint8_t) (C_TYPE & 31);

    // First pass. Sometimes, we can get a full parse from the first byte, but
    //   for multibyte types, we usually only get a secondary length field.
    switch (MINORTYPE) {
      case 24:  case 25:  case 26:  case 27:
        if (!_get_length_field(&_length_taken, &_length_extra, MINORTYPE)) {
          // If we didn't have enough bytes to get the attached integer field,
          //   there is no hope, and for clarity's sake, we'll bail out here.
          return nullptr;
        }
        break;
      default:
        if (MINORTYPE < 24) {
          _length_extra = MINORTYPE;
        }
        else {
          c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "Invalid MAJ/MIN combination (0x%02x/0x%02x)", MAJORTYPE, MINORTYPE);
          cull_anyway = consume_unparsable;
        }
        break;
    }
    const uint32_t _length_extra32 = (uint32_t) _length_extra;  // If truly a length, it will be 32-bit.

    // For types that observe _length_extra, they will generally also want to
    //   know if there are enough bytes to satisfy the parse.
    const bool HAVE_EXTRA_LEN = ((_length_extra > 0) & (INPUT_LEN >= (_length_taken + _length_extra32)));

    // Second pass. Form a return object, if possible.
    switch (MAJORTYPE) {
      case 0:  // positive integer
        switch (MINORTYPE) {
          case 24:  value = new C3PValue((uint8_t)  _length_extra32);  break;
          case 25:  value = new C3PValue((uint16_t) _length_extra32);  break;
          case 26:  value = new C3PValue((uint32_t) _length_extra32);  break;
          case 27:  value = new C3PValue((uint64_t) _length_extra);    break;
          default:
            if (MINORTYPE < 24) {
              value = new C3PValue((uint8_t) MINORTYPE);
            }
            else c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "Invalid PINT type (0x02x)", MINORTYPE);
            break;
        }
        break;

      case 1:   // negative integer
        switch (MINORTYPE) {
          case 24:
            if (_length_extra32 < INT8_MAX) {       value = new C3PValue((int8_t)  -(_length_extra32+1));  }
            else if(_length_extra32 == INT8_MAX) {  value = new C3PValue((int8_t) INT8_MIN);           }
            else {                                  value = new C3PValue((int16_t) -(_length_extra32+1));  }
            break;
          case 25:
            if (_length_extra32 < INT16_MAX) {       value = new C3PValue((int16_t) -(_length_extra32+1));  }
            else if(_length_extra32 == INT16_MAX) {  value = new C3PValue((int32_t) INT16_MIN);           }
            else {                                   value = new C3PValue((int32_t) -(_length_extra32+1));  }
            break;
          case 26:
            if (_length_extra < INT32_MAX) {       value = new C3PValue((int32_t) -(_length_extra+1));  }
            else if(_length_extra == INT32_MAX) {  value = new C3PValue((int32_t) INT32_MIN);           }
            else {                                 value = new C3PValue((int64_t) -(_length_extra+1));  }
            break;
          case 27:
            value = new C3PValue((int64_t) -(_length_extra+1));
            break;
          default:
            if (MINORTYPE < 24) {
              value = new C3PValue((int8_t) (0xFF - MINORTYPE));
            }
            else c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "Invalid NINT type (0x02x)", MINORTYPE);
            break;
        }
        break;

      case 2:  // Raw bytes
        if (HAVE_EXTRA_LEN) {
          uint8_t* new_buf = (uint8_t*) malloc(_length_extra32);
          if (nullptr != new_buf) {
            value = new C3PValue(new_buf, _length_extra32);
            if (nullptr != value) {
              _in->copyToBuffer(new_buf, _length_extra32, _length_taken);
              value->reapValue(true);
              _length_taken += _length_extra32;
            }
            else {
              free(new_buf);
            }
          }
        }
        break;

      case 3:  // String
        if (HAVE_EXTRA_LEN) {
          // If there are enough bytes to satisfy the parse...
          uint8_t new_buf[_length_extra32+1];
          *(new_buf + _length_extra32) = '\0';
          if (_length_extra32 == _in->copyToBuffer(new_buf, _length_extra32, _length_taken)) {
            value = new C3PValue((char*) new_buf);
            if (nullptr != value) {
              _length_taken += _length_extra32;
            }
          }
        }
        break;

      case 4:  value = _handle_array(&_length_taken, _length_extra32);  break;
      case 5:  value = _handle_map(&_length_taken,   _length_extra32);  break;
      case 6:
        if (C3P_CBOR_VENDOR_CODE == (_length_extra32 & 0xFFFFFF00)) {
          const TCode TC = IntToTcode(_length_extra32 & 0x000000FF);
          C3PType* t_helper = getTypeHelper(TC);
          if (nullptr != t_helper) {
            //value = new C3PValue(TC, _length_extra);
          }
          else {
            c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "No C3PType for TCode (0x02x)", (uint8_t) TC);
          }
        }
        break;

      case 7:  // special
        switch (MINORTYPE) {
          case 20:  value = new C3PValue(false);        break;
          case 21:  value = new C3PValue(true);         break;
          case 22:  value = new C3PValue(TCode::NONE);  break;  // CBOR "null"
          case 23:  value = new C3PValue(TCode::NONE);  break;  // CBOR "undefined"

          case 24:  case 25:
            if (HAVE_EXTRA_LEN) {
              cull_anyway = true;  //_listener->on_special(_length_extra);
            }
            break;
          case 26:  value = new C3PValue(*((float*)(void*) &_length_extra32));  break;
          case 27:  value = new C3PValue(*((double*)(void*) &_length_extra));   break;
          default:
            //if (MINORTYPE < 20) {
            //  _listener->on_special(MINORTYPE);
            //}
            cull_anyway = true;
            c3p_log(LOG_LEV_WARN, LOCAL_LOG_TAG, "Unhandled MINOR for special (0x02x)", MINORTYPE);
            break;
        }
        break;
    }
  }

  if ((nullptr != value) | cull_anyway) {
    // If we took content, consume the bytes from the input.
    _in->cull(_length_taken, (INPUT_LEN-_length_taken));
  }
  return value;
}



C3PValue* C3PValueDecoder::_handle_array(uint32_t* offset, uint32_t len) {
  C3PValue* value = nullptr;
  return value;
}


C3PValue* C3PValueDecoder::_handle_map(uint32_t* offset, uint32_t len) {
  C3PValue* value = nullptr;
  return value;
}


C3PValue* C3PValueDecoder::_handle_tag(uint32_t* offset, uint32_t len) {
  C3PValue* value = nullptr;
  return value;
}
