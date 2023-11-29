/*
File:   KeyValuePair.h
Author: J. Ian Lindsay
Date:   2014.03.10

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


TODO: Since this class renders large chains of function calls opaque to the
  linker, it would be nice to put bounds on binary size with pre-processor
  case-offs.
*/


#ifndef __C3P_KVP_H
#define __C3P_KVP_H

#include "C3PValue.h"

#if defined(CONFIG_C3P_CBOR)
  #include "../cbor-cpp/cbor.h"
#endif


/*
* These are flag definitions for KVP.
*/
#define C3P_KVP_FLAG_REAP_KVP       0x01  // Should the KVP object itself be free'd?
#define C3P_KVP_FLAG_REAP_KEY       0x02  // Should the key string be free'd?
#define C3P_KVP_FLAG_REAP_CNTNR     0x04  // Should the C3PValue itself be free'd (not its contained value)?
#define C3P_KVP_FLAG_ERR_MEM        0x08  //


/*******************************************************************************
* KeyValuePair
*******************************************************************************/
class KeyValuePair {
  public:
    ~KeyValuePair();

    /* Constructors with real implementations. */
    KeyValuePair(const char* key, C3PValue*, uint8_t flags = 0);
    KeyValuePair(char* key, C3PValue*, uint8_t flags = 0);

    /* Constructors that define types, but no values. */
    KeyValuePair(const char* key, const TCode TC, uint8_t flags = 0) : KeyValuePair(key, new C3PValue(TC), (flags | C3P_KVP_FLAG_REAP_CNTNR)) {};
    KeyValuePair(char* key,       const TCode TC, uint8_t flags = 0) : KeyValuePair(key, new C3PValue(TC), (flags | C3P_KVP_FLAG_REAP_CNTNR)) {};
    KeyValuePair(const TCode TC, uint8_t flags = 0) : KeyValuePair((const char*) nullptr, new C3PValue(TC), (flags | C3P_KVP_FLAG_REAP_CNTNR)) {};

    /* Type-specific inline constructors. */
    KeyValuePair(const char* key, uint8_t        val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, uint16_t       val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, uint32_t       val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, uint64_t       val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, int8_t         val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, int16_t        val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, int32_t        val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, int64_t        val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, bool           val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, float          val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, double         val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, const char*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, char*          val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, StringBuilder* val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, Vector3u32*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, Vector3u16*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, Vector3u8*     val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, Vector3i32*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, Vector3i16*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, Vector3i8*     val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, Vector3f*      val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, Vector3f64*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, Identity*      val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, KeyValuePair*  val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(const char* key, uint8_t* v, uint32_t l) : KeyValuePair(key, new C3PValue(v, l), C3P_KVP_FLAG_REAP_CNTNR) {};

    KeyValuePair(char* key, uint8_t        val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, uint16_t       val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, uint32_t       val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, uint64_t       val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, int8_t         val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, int16_t        val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, int32_t        val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, int64_t        val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, bool           val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, float          val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, double         val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, const char*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, char*          val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, StringBuilder* val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, Vector3u32*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, Vector3u16*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, Vector3u8*     val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, Vector3i32*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, Vector3i16*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, Vector3i8*     val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, Vector3f*      val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, Vector3f64*    val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, Identity*      val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, KeyValuePair*  val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
    KeyValuePair(char* key, uint8_t* v, uint32_t l) : KeyValuePair(key, new C3PValue(v, l), C3P_KVP_FLAG_REAP_CNTNR) {};

    KeyValuePair(uint8_t            val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(uint16_t           val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(uint32_t           val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(uint64_t           val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(int8_t             val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(int16_t            val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(int32_t            val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(int64_t            val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(bool               val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(float              val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(double             val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(const char*        val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(char*              val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(StringBuilder*     val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(Vector3u32*        val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(Vector3u16*        val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(Vector3u8*         val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(Vector3i32*        val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(Vector3i16*        val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(Vector3i8*         val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(Vector3f*          val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(Vector3f64*        val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(Identity*          val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(KeyValuePair*      val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(uint8_t* v, uint32_t l) : KeyValuePair((const char*) nullptr, v, l) {};


    inline KeyValuePair* append(uint8_t val, const char* key = nullptr) {           return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint16_t val, const char* key = nullptr) {          return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint32_t val, const char* key = nullptr) {          return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint64_t val, const char* key = nullptr) {          return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int8_t val, const char* key = nullptr) {            return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int16_t val, const char* key = nullptr) {           return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int32_t val, const char* key = nullptr) {           return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int64_t val, const char* key = nullptr) {           return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(bool val, const char* key = nullptr) {              return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(float val, const char* key = nullptr) {             return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(double val, const char* key = nullptr) {            return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(const char* val, const char* key = nullptr) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(char* val, const char* key = nullptr) {             return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(StringBuilder* val, const char* key = nullptr) {    return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u32* val, const char* key = nullptr) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u16* val, const char* key = nullptr) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u8* val, const char* key = nullptr) {        return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i32* val, const char* key = nullptr) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i16* val, const char* key = nullptr) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i8* val, const char* key = nullptr) {        return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3f* val, const char* key = nullptr) {         return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3f64* val, const char* key = nullptr) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(KeyValuePair* val, const char* key = nullptr) {     return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Identity* val, const char* key = nullptr) {         return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint8_t* v, uint32_t l, const char* k = nullptr) {  return link(new KeyValuePair(k, v, l));    };

    inline KeyValuePair* append(uint8_t val, char* key) {           return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint16_t val, char* key) {          return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint32_t val, char* key) {          return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint64_t val, char* key) {          return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int8_t val, char* key) {            return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int16_t val, char* key) {           return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int32_t val, char* key) {           return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int64_t val, char* key) {           return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(bool val, char* key) {              return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(float val, char* key) {             return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(double val, char* key) {            return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(const char* val, char* key) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(char* val, char* key) {             return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(StringBuilder* val, char* key) {    return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u32* val, char* key) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u16* val, char* key) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u8* val, char* key) {        return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i32* val, char* key) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i16* val, char* key) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i8* val, char* key) {        return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3f* val, char* key) {         return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3f64* val, char* key) {       return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(KeyValuePair* val, char* key) {     return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Identity* val, char* key) {         return link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint8_t* v, uint32_t l, char* k) {  return link(new KeyValuePair(k, v, l));    };

    inline int8_t setValue(uint8_t val) {               return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(uint16_t val) {              return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(uint32_t val) {              return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(uint64_t val) {              return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(int8_t val) {                return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(int16_t val) {               return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(int32_t val) {               return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(int64_t val) {               return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(bool val) {                  return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(float val) {                 return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(double val) {                return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(const char* val) {           return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(char* val) {                 return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(StringBuilder* val) {        return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(Vector3u32* val) {           return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(Vector3u16* val) {           return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(Vector3u8* val) {            return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(Vector3i32* val) {           return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(Vector3i16* val) {           return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(Vector3i8* val) {            return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(Vector3f* val) {             return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(Vector3f64* val) {           return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(KeyValuePair* val) {         return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(Identity* val) {             return ((nullptr == _value) ? -1 : _value->set(val));     };
    inline int8_t setValue(uint8_t* v, uint32_t l) {    return ((nullptr == _value) ? -1 : _value->set(v, l));    };

    inline int8_t getValue(uint8_t* val) {              return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(uint16_t* val) {             return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(uint32_t* val) {             return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(uint64_t* val) {             return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(int8_t* val) {               return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(int16_t* val) {              return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(int32_t* val) {              return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(int64_t* val) {              return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(bool* val) {                 return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(float* val) {                return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(double* val) {               return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(const char** val) {          return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(char** val) {                return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(StringBuilder** val) {       return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(Vector3u32* val) {           return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(Vector3u16* val) {           return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(Vector3u8* val) {            return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(Vector3i32* val) {           return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(Vector3i16* val) {           return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(Vector3i8* val) {            return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(Vector3f* val) {             return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(Vector3f64* val) {           return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(KeyValuePair** val) {        return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(Identity** val) {            return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    inline int8_t getValue(uint8_t** v, uint32_t* l) {  return ((nullptr == _value) ? -1 : _value->get_as(v, l)); };


    /* Conditional types. */
    #if defined(CONFIG_C3P_IMG_SUPPORT)
      KeyValuePair(const char* key, Image* val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
      KeyValuePair(char* key, Image* val) : KeyValuePair(key, new C3PValue(val), C3P_KVP_FLAG_REAP_CNTNR) {};
      KeyValuePair(Image* val) : KeyValuePair((const char*) nullptr, val) {};
      inline KeyValuePair* append(Image* val, const char* key = nullptr) {    return link(new KeyValuePair(key, val));                  };
      inline KeyValuePair* append(Image* val, char* key) {                    return link(new KeyValuePair(key, val));                  };
      inline int8_t        setValue(Image* val) {                             return ((nullptr == _value) ? -1 : _value->set(val));     };
      inline int8_t        getValue(Image** val) {                            return ((nullptr == _value) ? -1 : _value->get_as(val));  };
    #endif   // CONFIG_C3P_IMG_SUPPORT


    /* Accessors for key. */
    inline char* getKey() {    return _key;  };
    int8_t setKey(const char*);
    int8_t setKey(char*);

    /* Accessors for memory management. */
    inline void reapKVP(bool x) {         _alter_flags(x, C3P_KVP_FLAG_REAP_KVP);                      };
    inline bool reapKVP() {               return _check_flags(C3P_KVP_FLAG_REAP_KVP);                  };
    inline void reapContainer(bool x) {   _alter_flags(x, C3P_KVP_FLAG_REAP_CNTNR);                    };
    inline bool reapContainer() {         return _check_flags(C3P_KVP_FLAG_REAP_CNTNR);                };
    inline void reapValue(bool x) {       if (nullptr != _value) {  _value->reapValue(x);  }           };
    inline bool reapValue() {             return ((nullptr == _value) ? false : _value->reapValue());  };
    inline bool hasError() {              return _check_flags(C3P_KVP_FLAG_ERR_MEM);                   };

    /* Accessors for linkage to parallel data. */
    int collectKeys(StringBuilder*);
    KeyValuePair* retrieveByIdx(unsigned int idx);
    KeyValuePair* retrieveByKey(const char*);
    KeyValuePair* link(KeyValuePair*, bool reap_kvp = true);
    int8_t drop(KeyValuePair**, KeyValuePair*);
    uint32_t count();

    /* Accessors to type information and underpinnings. */
    C3PValue* valueWithIdx(uint32_t idx);
    int8_t    valueWithIdx(uint32_t idx, void* trg_buf);
    C3PValue* valueWithKey(const char*);
    int8_t    valueWithKey(const char*, void* trg_buf);
    int8_t    convertToType(const TCode);

    inline C3PValue* getValue() {   return _value;    };
    inline uint32_t  length() {     return ((nullptr != _value) ? _value->length() : 0);  };
    inline TCode     tcode() {      return ((nullptr != _value) ? _value->tcode()  : TCode::NONE);  };
    void   valToString(StringBuilder*);
    void   printDebug(StringBuilder*);
    int    memoryCost(bool deep = false);   // Get the memory use for this object.

    int8_t serialize(StringBuilder*, TCode);

    /* Statics */
    static KeyValuePair* unserialize(uint8_t*, unsigned int, const TCode);


  private:
    char*         _key   = nullptr;
    C3PValue*     _value = nullptr;
    KeyValuePair* _next  = nullptr;
    uint8_t       _flags = 0;

    /* Private mem-mgmt functions. */
    inline void _reap_key(bool en) {  _alter_flags(en, C3P_KVP_FLAG_REAP_KEY);         };
    inline bool _reap_key() {         return _check_flags(C3P_KVP_FLAG_REAP_KEY);      };
    void _set_new_key(char*);
    void _set_new_value(C3PValue*);

    /* Private parse/pack functions functions. */
    int8_t _encode_to_bin(StringBuilder*);
    KeyValuePair* _decode_from_bin(uint8_t*, unsigned int);
    #if defined(CONFIG_C3P_CBOR)
      int8_t _encode_to_cbor(StringBuilder*);
      static KeyValuePair* _decode_from_cbor(uint8_t*, unsigned int);
    #endif   // CONFIG_C3P_CBOR


    /* Inlines for altering and reading the flags. */
    inline void _alter_flags(bool en, uint8_t mask) {
      _flags = (en) ? (_flags | mask) : (_flags & ~mask);
    };
    inline bool _check_flags(uint8_t mask) {
      return (mask == (_flags & mask));
    };
};



/* If we have CBOR support, we define a helper class to assist decomposition. */
#if defined(CONFIG_C3P_CBOR)
  class CBORArgListener : public cbor::listener {
    public:
      CBORArgListener(KeyValuePair**);
      ~CBORArgListener();

      void on_integer(int8_t);
      void on_integer(int16_t);
      void on_integer(int32_t);
      void on_integer(int64_t);
      void on_integer(uint8_t);
      void on_integer(uint16_t);
      void on_integer(uint32_t);
      void on_integer(uint64_t);
      void on_float32(float value);
      void on_double(double value);
      void on_bytes(uint8_t* data, int size);
      void on_string(char* str);
      void on_array(int size);
      void on_map(int size);
      void on_tag(unsigned int tag);
      void on_special(unsigned int code);
      void on_bool(bool);
      void on_null();
      void on_undefined();
      void on_error(const char* error);

      void on_extra_integer(unsigned long long value, int sign);
      void on_extra_integer(long long value, int sign);
      void on_extra_tag(unsigned long long tag);
      void on_extra_special(unsigned long long tag);

    private:
      KeyValuePair** built               = nullptr;
      char*          _wait               = nullptr;
      int            _wait_map           = 0;
      int            _wait_array         = 0;
      TCode          _pending_c3p_tag = TCode::NONE;

      /* Please forgive the stupid name. */
      void _caaa(KeyValuePair*);

      KeyValuePair* _inflate_c3p_type(uint8_t* data, int size, const TCode);
  };
#endif  // CONFIG_C3P_CBOR

#endif  // __C3P_KVP_H
