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

#include "../C3PValue/C3PValue.h"
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

C3PValue::~C3PValue() {
  if (_val_by_ref & _reap_val & (nullptr != _target_mem)) {
    free(_target_mem);
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


int8_t C3PValue::set_from(const TCode, void* type_pointer) {
  int8_t ret = 0;
  _set_trace++;
  return ret;
}


int8_t C3PValue::get_as(const TCode, void* type_pointer) {
  int8_t ret = 0;
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
  unsigned int ret = 0;
  return ret;
}
int8_t C3PValue::set(uint8_t x) {
  int8_t ret = -1;
  return ret;
}

int8_t C3PValue::set(uint16_t x) {
  int8_t ret = -1;
  return ret;
}

int8_t C3PValue::set(uint32_t x) {
  int8_t ret = -1;
  return ret;
}

int8_t C3PValue::set(uint64_t x) {
  int8_t ret = -1;
  return ret;
}


int C3PValue::get_as_int(int8_t* success) {
  int ret = 0;
  return ret;
}
int8_t C3PValue::set(int8_t x) {
  int8_t ret = -1;
  return ret;
}

int8_t C3PValue::set(int16_t x) {
  int8_t ret = -1;
  return ret;
}

int8_t C3PValue::set(int32_t x) {
  int8_t ret = -1;
  return ret;
}

int8_t C3PValue::set(int64_t x) {
  int8_t ret = -1;
  return ret;
}


bool C3PValue::get_as_bool(int8_t* success) {  return (0 != _target_mem);  }

int8_t C3PValue::set(bool x) {
  int8_t ret = -1;
  return ret;
}


float C3PValue::get_as_float(int8_t* success) {
  int8_t suc = 0;
  float ret = 0.0f;
  switch (_TCODE) {
    case TCode::FLOAT:
      *(((uint8_t*) &ret) + 0) = *(((uint8_t*) &_target_mem) + 0);
      *(((uint8_t*) &ret) + 1) = *(((uint8_t*) &_target_mem) + 1);
      *(((uint8_t*) &ret) + 2) = *(((uint8_t*) &_target_mem) + 2);
      *(((uint8_t*) &ret) + 3) = *(((uint8_t*) &_target_mem) + 3);
      suc = 1;
      break;
    default:
      break;
  }
  if (success) {  *success = suc;  }
  return ret;
}

int8_t C3PValue::set(float x) {
  int8_t ret = -1;
  switch (_TCODE) {
    case TCode::FLOAT:
      *(((uint8_t*) &_target_mem) + 0) = *((uint8_t*) &x + 0);
      *(((uint8_t*) &_target_mem) + 1) = *((uint8_t*) &x + 1);
      *(((uint8_t*) &_target_mem) + 2) = *((uint8_t*) &x + 2);
      *(((uint8_t*) &_target_mem) + 3) = *((uint8_t*) &x + 3);
      ret = 0;
      break;
    case TCode::DOUBLE:
    default:
      break;
  }
  if (0 == ret) {  _set_trace++;  }
  return ret;
}


double C3PValue::get_as_double(int8_t* success) {
  int8_t suc = 0;
  double ret = 0.0d;
  switch (_TCODE) {
    case TCode::FLOAT:
      ret = (double) get_as_float();
      suc = 1;
      break;
    case TCode::DOUBLE:
      *(((uint8_t*) &ret) + 0) = *((uint8_t*) _target_mem + 0);
      *(((uint8_t*) &ret) + 1) = *((uint8_t*) _target_mem + 1);
      *(((uint8_t*) &ret) + 2) = *((uint8_t*) _target_mem + 2);
      *(((uint8_t*) &ret) + 3) = *((uint8_t*) _target_mem + 3);
      *(((uint8_t*) &ret) + 4) = *((uint8_t*) _target_mem + 4);
      *(((uint8_t*) &ret) + 5) = *((uint8_t*) _target_mem + 5);
      *(((uint8_t*) &ret) + 6) = *((uint8_t*) _target_mem + 6);
      *(((uint8_t*) &ret) + 7) = *((uint8_t*) _target_mem + 7);
      suc = 1;
      break;
    default:
      break;
  }
  if (success) {  *success = suc;  }
  return ret;
}

int8_t C3PValue::set(double x) {
  int8_t ret = -1;
  switch (_TCODE) {
    case TCode::DOUBLE:
      *(((uint8_t*) _target_mem) + 0) = *((uint8_t*) &x + 0);
      *(((uint8_t*) _target_mem) + 1) = *((uint8_t*) &x + 1);
      *(((uint8_t*) _target_mem) + 2) = *((uint8_t*) &x + 2);
      *(((uint8_t*) _target_mem) + 3) = *((uint8_t*) &x + 3);
      *(((uint8_t*) _target_mem) + 4) = *((uint8_t*) &x + 4);
      *(((uint8_t*) _target_mem) + 5) = *((uint8_t*) &x + 5);
      *(((uint8_t*) _target_mem) + 6) = *((uint8_t*) &x + 6);
      *(((uint8_t*) _target_mem) + 7) = *((uint8_t*) &x + 7);
      ret = 0;
      break;
    default:
      break;
  }
  if (0 == ret) {  _set_trace++;  }
  return ret;
}


/*******************************************************************************
* Parsing/Packing
*******************************************************************************/
int8_t C3PValue::serialize(StringBuilder* output, TCode fmt) {
  return -1;
}


int8_t C3PValue::deserialize(StringBuilder* input, TCode fmt) {
  return -1;
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
  uint32_t ret = sizeOfType(_TCODE);
  if (0 == ret) {
    switch (_TCODE) {
      case TCode::STR_BUILDER:
      case TCode::STR:
      case TCode::IDENTITY:
      case TCode::IMAGE:
      default:
        break;
    }
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
  switch (_TCODE) {
    case TCode::INT8:
    case TCode::INT16:
    case TCode::INT32:
      out->concatf("%d", (uintptr_t) _target_mem);
      break;
    case TCode::INT64:
    case TCode::INT128:
      // TODO: Large integers won't render this way under newlib-nano. Add a
      //   lib-check/conf step to help handle this case?
      out->concatf("%d", (uintptr_t) _target_mem);
      break;
    case TCode::UINT8:
    case TCode::UINT16:
    case TCode::UINT32:
      out->concatf("%u", (uintptr_t) _target_mem);
      break;
    case TCode::UINT64:
    case TCode::UINT128:
      // TODO: Large integers won't render this way under newlib-nano. Add a
      //   lib-check/conf step to help handle this case?
      out->concatf("%u", (uintptr_t) _target_mem);
      break;
    case TCode::FLOAT:
      {
        float tmp;
        memcpy((void*) &tmp, &_target_mem, 4);
        out->concatf("%.4f", (double) tmp);
      }
      break;
    case TCode::DOUBLE:
      {
        out->concatf("%.6f", get_as_double());
      }
      break;

    case TCode::BOOLEAN:
      out->concatf("%s", ((uintptr_t) _target_mem ? "true" : "false"));
      break;
    case TCode::STR_BUILDER:
      out->concat((StringBuilder*) _target_mem);
      break;
    case TCode::STR:
      out->concatf("%s", (const char*) _target_mem);
      break;
    case TCode::VECT_3_FLOAT:
      {
        Vector3<float>* v = (Vector3<float>*) _target_mem;
        out->concatf("(%.4f, %.4f, %.4f)", (double)(v->x), (double)(v->y), (double)(v->z));
      }
      break;
    case TCode::VECT_3_UINT32:
      {
        Vector3<uint32_t>* v = (Vector3<uint32_t>*) _target_mem;
        out->concatf("(%u, %u, %u)", v->x, v->y, v->z);
      }
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



/*******************************************************************************
* Concealed logic
*******************************************************************************/

/**
* Protected delegate constructor.
*/
C3PValue::C3PValue(const TCode TC, void* ptr) : _TCODE(TC), _set_trace(1), _target_mem(ptr) {
  _val_by_ref = !typeIsPointerPunned(_TCODE);
  _reap_val   = false;
}
