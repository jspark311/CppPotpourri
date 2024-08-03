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


/*******************************************************************************
* KeyValuePair
*******************************************************************************/
class KeyValuePair : public C3PValue {
  public:
    virtual ~KeyValuePair() {};  // NOTE: superclass destructor does everything.

    /* Constructors that define types, but no values. */
    KeyValuePair(const TCode TC, const char* key, uint8_t flags = 0);
    KeyValuePair(const TCode TC, char* key, uint8_t flags = 0);
    KeyValuePair(const TCode TC, uint8_t flags = 0) : KeyValuePair(TC, (const char*) nullptr, flags) {};

    /* Type-specific inline constructors. */
    // TODO: calling set() from the constructor doesn't work for pointer types for some reason. And even touching
    //   _target_mem directly like this shouldn't be needed, that task being handled by the C3PValue constructor.
    //   But it isn't.
    KeyValuePair(const char* key, uint8_t        val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, uint16_t       val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, uint32_t       val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, uint64_t       val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, int8_t         val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, int16_t        val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, int32_t        val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, int64_t        val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, bool           val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, float          val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, double         val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, const char*    val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = (void*)val;  };
    KeyValuePair(const char* key, char*          val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = val;  };
    KeyValuePair(const char* key, StringBuilder* val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = val;  };
    KeyValuePair(const char* key, Vector3u32*    val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, Vector3u16*    val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, Vector3u8*     val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, Vector3i32*    val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, Vector3i16*    val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, Vector3i8*     val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, Vector3f*      val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, Vector3f64*    val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(const char* key, Identity*      val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = val;  };
    KeyValuePair(const char* key, KeyValuePair*  val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = val;  };
    KeyValuePair(const char* key, StopWatch*     val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = val;  };
    KeyValuePair(const char* key, uint8_t* v, uint32_t l, uint8_t flags = 0);

    KeyValuePair(char* key, uint8_t        val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, uint16_t       val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, uint32_t       val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, uint64_t       val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, int8_t         val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, int16_t        val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, int32_t        val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, int64_t        val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, bool           val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, float          val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, double         val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, const char*    val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = (void*)val;  };
    KeyValuePair(char* key, char*          val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = val;  };
    KeyValuePair(char* key, StringBuilder* val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = val;  };
    KeyValuePair(char* key, Vector3u32*    val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, Vector3u16*    val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, Vector3u8*     val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, Vector3i32*    val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, Vector3i16*    val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, Vector3i8*     val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, Vector3f*      val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, Vector3f64*    val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
    KeyValuePair(char* key, Identity*      val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = val;  };
    KeyValuePair(char* key, KeyValuePair*  val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = val;  };
    KeyValuePair(char* key, StopWatch*     val) : KeyValuePair(tcodeForType(val), key, 0) {  _target_mem = val;  };
    KeyValuePair(char* key, uint8_t* v, uint32_t l, uint8_t flags = 0);

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
    KeyValuePair(StopWatch*         val) : KeyValuePair((const char*) nullptr, val) {};
    KeyValuePair(uint8_t* v, uint32_t l) : KeyValuePair((const char*) nullptr, v, l) {};


    inline KeyValuePair* append(uint8_t val, const char* key = nullptr) {           return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint16_t val, const char* key = nullptr) {          return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint32_t val, const char* key = nullptr) {          return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint64_t val, const char* key = nullptr) {          return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int8_t val, const char* key = nullptr) {            return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int16_t val, const char* key = nullptr) {           return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int32_t val, const char* key = nullptr) {           return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int64_t val, const char* key = nullptr) {           return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(bool val, const char* key = nullptr) {              return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(float val, const char* key = nullptr) {             return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(double val, const char* key = nullptr) {            return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(const char* val, const char* key = nullptr) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(char* val, const char* key = nullptr) {             return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(StringBuilder* val, const char* key = nullptr) {    return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u32* val, const char* key = nullptr) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u16* val, const char* key = nullptr) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u8* val, const char* key = nullptr) {        return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i32* val, const char* key = nullptr) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i16* val, const char* key = nullptr) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i8* val, const char* key = nullptr) {        return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3f* val, const char* key = nullptr) {         return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3f64* val, const char* key = nullptr) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Identity* val, const char* key = nullptr) {         return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(KeyValuePair* val, const char* key = nullptr) {     return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(StopWatch* val, const char* key = nullptr) {        return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint8_t* v, uint32_t l, const char* k = nullptr) {  return (KeyValuePair*) link(new KeyValuePair(k, v, l));    };

    inline KeyValuePair* append(uint8_t val, char* key) {           return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint16_t val, char* key) {          return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint32_t val, char* key) {          return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint64_t val, char* key) {          return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int8_t val, char* key) {            return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int16_t val, char* key) {           return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int32_t val, char* key) {           return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(int64_t val, char* key) {           return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(bool val, char* key) {              return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(float val, char* key) {             return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(double val, char* key) {            return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(const char* val, char* key) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(char* val, char* key) {             return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(StringBuilder* val, char* key) {    return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u32* val, char* key) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u16* val, char* key) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3u8* val, char* key) {        return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i32* val, char* key) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i16* val, char* key) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3i8* val, char* key) {        return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3f* val, char* key) {         return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Vector3f64* val, char* key) {       return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(Identity* val, char* key) {         return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(KeyValuePair* val, char* key) {     return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(StopWatch* val, char* key) {        return (KeyValuePair*) link(new KeyValuePair(key, val));   };
    inline KeyValuePair* append(uint8_t* v, uint32_t l, char* k) {  return (KeyValuePair*) link(new KeyValuePair(k, v, l));    };

    /* Conditional types. */
    #if defined(CONFIG_C3P_IMG_SUPPORT)
      KeyValuePair(const char* key, Image* val) : KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
      KeyValuePair(char* key, Image* val) :       KeyValuePair(tcodeForType(val), key, 0) {  set(val);  };
      KeyValuePair(Image* val) : KeyValuePair((const char*) nullptr, val) {};
      inline KeyValuePair* append(Image* val, const char* key = nullptr) {    return (KeyValuePair*) link(new KeyValuePair(key, val));  };
      inline KeyValuePair* append(Image* val, char* key) {                    return (KeyValuePair*) link(new KeyValuePair(key, val));  };
    #endif   // CONFIG_C3P_IMG_SUPPORT

    /* Accessors for key. */
    inline char* getKey() {    return _key;  };
    int8_t setKey(const char*);
    int8_t setKey(char*);

    /* Key handling */
    // TODO: These should be migrated to C3PValue.
    int collectKeys(StringBuilder*);
    KeyValuePair* valueWithKey(const char*);
    int8_t    valueWithKey(const char*, void* trg_buf);

    // TODO: These are adding weight and confusion now the C3PType has taken
    //   over their duties. Move their novel functionality into a new function,
    //   and their redundancies can disappear.
    //void   valToString(StringBuilder*);
    virtual int8_t serialize(StringBuilder*, const TCode FORMAT);

    // TODO: This should no longer be necessary. CTRL+D to demote scope.
    inline KeyValuePair* nextKVP() {  return _next_sib_with_key();  };

    /* Statics */
    static KeyValuePair* unserialize(uint8_t*, unsigned int, const TCode);


  private:
    friend void   C3PTypeConstraint<KeyValuePair*>::to_string(void*, StringBuilder*);
    friend int    C3PTypeConstraint<KeyValuePair*>::serialize(void*, StringBuilder*, const TCode);
    char*         _key   = nullptr;

    /* Private mem-mgmt functions. */
    inline void _reap_key(bool x) {   _set_flags(x, C3PVAL_MEM_FLAG_REAP_KEY);          };
    inline bool _reap_key() {         return _chk_flags(C3PVAL_MEM_FLAG_REAP_KEY);      };
    void _set_new_key(char*);

    /* Private parse/pack functions functions. */
    int8_t _encode_to_printable(StringBuilder*, const unsigned int LEVEL = 0);
    KeyValuePair* _decode_from_bin(uint8_t*, unsigned int);
    #if defined(CONFIG_C3P_CBOR)
      static KeyValuePair* _decode_from_cbor(uint8_t*, unsigned int);
    #endif   // CONFIG_C3P_CBOR
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
      void on_extra_integer(uint64_t value, int sign);
      void on_extra_tag(uint64_t tag);
      void on_extra_special(uint64_t tag);

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
