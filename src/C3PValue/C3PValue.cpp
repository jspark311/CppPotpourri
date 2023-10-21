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

/* CBOR support should probably be required to parse/pack. */
#if defined(CONFIG_C3P_CBOR)
  #include "../cbor-cpp/cbor.h"
#endif


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
  _val_by_ref = !typeIsPointerPunned(_TCODE);
  _reap_val   = false;
}


C3PValue::~C3PValue() {
  if (nullptr != _target_mem) {
    // Some types have a shim to hold an explicit length, or other data. We
    //   construe the reap member to refer to the data itself, and not the shim.
    //   We will always free the shim.
    if (_TCODE == TCode::BINARY) {
      if (_reap_val) {
        free(((C3PBinBinder*) _target_mem)->buf);
      }
      free(_target_mem);
    }
    else if (_val_by_ref & _reap_val) {
      free(_target_mem);
    }
  }
  _target_mem = nullptr;
}


/*******************************************************************************
* Typed constructors
*******************************************************************************/

C3PValue::C3PValue(float val) : C3PValue(TCode::FLOAT, nullptr) {
  uint8_t* src = (uint8_t*) &val;                  // To avoid inducing bugs
  *(((uint8_t*) &_target_mem) + 0) = *(src + 0);   //   related to alignment,
  *(((uint8_t*) &_target_mem) + 1) = *(src + 1);   //   we copy the value
  *(((uint8_t*) &_target_mem) + 2) = *(src + 2);   //   byte-wise into our own
  *(((uint8_t*) &_target_mem) + 3) = *(src + 3);   //   storage.
}


// TODO: We might be able to treat this as a direct value on a 64-bit system.
C3PValue::C3PValue(double val) : C3PValue(TCode::DOUBLE, malloc(sizeof(double))) {
  if (nullptr != _target_mem) {
    _val_by_ref = true;
    _reap_val   = true;
    uint8_t* src = (uint8_t*) &val;                  // To avoid inducing bugs
    *(((uint8_t*) _target_mem) + 0) = *(src + 0);    //   related to alignment,
    *(((uint8_t*) _target_mem) + 1) = *(src + 1);    //   we copy the value
    *(((uint8_t*) _target_mem) + 2) = *(src + 2);    //   byte-wise into our own
    *(((uint8_t*) _target_mem) + 3) = *(src + 3);    //   storage.
    *(((uint8_t*) _target_mem) + 4) = *(src + 4);
    *(((uint8_t*) _target_mem) + 5) = *(src + 5);
    *(((uint8_t*) _target_mem) + 6) = *(src + 6);
    *(((uint8_t*) _target_mem) + 7) = *(src + 7);
  }
}


C3PValue::C3PValue(void* val, uint32_t len) : C3PValue(TCode::BINARY, malloc(sizeof(C3PBinBinder))) {
  if (nullptr != _target_mem) {
    ((C3PBinBinder*) _target_mem)->buf = (uint8_t*) val;
    ((C3PBinBinder*) _target_mem)->len = len;
    _val_by_ref = true;
    _reap_val   = false;
  }
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


int8_t C3PValue::get_as(const TCode DEST_TYPE, void* dest) {
  int8_t ret = -1;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    ret = t_helper->get_as(_type_pun(), DEST_TYPE, dest);
  }
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
  int32_t ret = 0;
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


int8_t C3PValue::deserialize(StringBuilder* input, const TCode FORMAT) {
  int8_t ret = -1;
  C3PType* t_helper = getTypeHelper(_TCODE);
  if (nullptr != t_helper) {
    ret = t_helper->deserialize(_type_pun(), input, FORMAT);
  }
  return ret;
}


/*
* @return true if the type is a simple single-value numeric.
*/
bool C3PValue::is_numeric() {
  switch (_TCODE) {
    case TCode::INT8:     case TCode::INT16:   case TCode::INT32:
    case TCode::INT64:    case TCode::INT128:
    case TCode::UINT8:    case TCode::UINT16:  case TCode::UINT32:
    case TCode::UINT64:   case TCode::UINT128:
    case TCode::BOOLEAN:  case TCode::FLOAT:   case TCode::DOUBLE:
    // Semantic wrappers for numeric types.
    case TCode::COLOR8:   case TCode::COLOR16: case TCode::COLOR24:
    case TCode::IPV4_ADDR:
      return true;
    default:  break;
  }
  return false;
}


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
  }
  else {
    // TODO: Everything below this comment is headed for the entropy pool.
    switch (_TCODE) {
      case TCode::STR_BUILDER:
        out->concat((StringBuilder*) _target_mem);
        break;
      //case TCode::VECT_4_FLOAT:
      //  {
      //    Vector4f* v = (Vector4f*) _target_mem;
      //    out->concatf("(%.4f, %.4f, %.4f, %.4f)", (double)(v->w), (double)(v->x), (double)(v->y), (double)(v->z));
      //  }
      //  break;
      case TCode::KVP:
        //if (nullptr != _target_mem) ((KeyValuePair*) _target_mem)->printDebug(out);
        break;
      case TCode::IDENTITY:
        if (nullptr != _target_mem) ((Identity*) _target_mem)->toString(out);
        break;

      default:
        {
          const uint32_t L_ENDER = length();
          for (uint32_t n = 0; n < L_ENDER; n++) {
            out->concatf("%02x ", *((uint8_t*) _target_mem + n));
          }
        }
        break;
    }
  }
}
