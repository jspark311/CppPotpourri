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


This class descends from ManuvrOS's Argument and TypeTranscriber classes.

Lessons learned from ManuvrOS:
--------------------------------------------------------------------------------
This class can easily become obnoxious and brittle. ManuvrOS tried to use this
  idea as a richly-typed data carrier for general internal use. Although this
  strategy functioned, it was prone to memory faults and ownership confusion.
The use of this class should be restricted to working as an intermediary
  between the serialized and the in-memory forms of class data. Unless/until
  the API can be made to work without the problems it grew the last time.

TODO: Since this class renders large chains of function calls opaque to the
  linker, it would be nice to put bounds on binary size with pre-processor
  case-offs.
*/


#ifndef __MANUVR_DS_ARGUMENT_H
#define __MANUVR_DS_ARGUMENT_H

#include <string.h>

#include <EnumeratedTypeCodes.h>
#include <Vector3.h>
#include <StringBuilder.h>

#if defined(CONFIG_C3P_CBOR)
  #include <cbor-cpp/cbor.h>
#endif

/* Forward declarations of classes that this type will handle. */
#if defined(CONFIG_C3P_IMG_SUPPORT)
  #include <Image/Image.h>
#endif

class Identity;


/*
* These are flag definitions for KVP.
*/
#define MANUVR_KVP_FLAG_REAP_VALUE     0x01  // Should the pointer be freed?
#define MANUVR_KVP_FLAG_DIRECT_VALUE   0x02  // The value is NOT a pointer.
#define MANUVR_KVP_FLAG_REAP_KEY       0x04  //
#define MANUVR_KVP_FLAG_ERR_MEM        0x08  //



/*******************************************************************************
* This class is a general packer/parser for our nice plush types.
*******************************************************************************/
class KeyValuePair {
  public:
    ~KeyValuePair();

    /* Fully-defined constructors, and value accessors. */
    // Unconditional types.
    KeyValuePair(uint8_t  val, const char* key = nullptr) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::UINT8, key)  {};
    KeyValuePair(uint16_t val, const char* key = nullptr) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::UINT16, key) {};
    KeyValuePair(uint32_t val, const char* key = nullptr) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::UINT32, key) {};
    KeyValuePair(int8_t   val, const char* key = nullptr) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::INT8, key)   {};
    KeyValuePair(int16_t  val, const char* key = nullptr) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::INT16, key)  {};
    KeyValuePair(int32_t  val, const char* key = nullptr) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::INT32, key)  {};
    KeyValuePair(bool     val, const char* key = nullptr) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::BOOLEAN, key)  {};
    KeyValuePair(float    val, const char* key = nullptr);
    KeyValuePair(double   val, const char* key = nullptr);
    KeyValuePair(const char* val, const char* key = nullptr) : KeyValuePair((void*) val, (strlen(val)+1), TCode::STR, key) {};
    KeyValuePair(char* val,       const char* key = nullptr) : KeyValuePair((void*) val, (strlen(val)+1), TCode::STR, key) {};
    KeyValuePair(void* v, size_t l, const char* k = nullptr) : KeyValuePair(v, l, TCode::BINARY, k) {};
    KeyValuePair(StringBuilder* v, const char* key = nullptr) : KeyValuePair(v, v->length(),  TCode::STR_BUILDER, key)  {};
    KeyValuePair(Vector3ui32* val, const char* key = nullptr) : KeyValuePair((void*) val, 12, TCode::VECT_3_UINT32, key) {};
    KeyValuePair(Vector3ui16* val, const char* key = nullptr) : KeyValuePair((void*) val, 6,  TCode::VECT_3_UINT16, key) {};
    KeyValuePair(Vector3ui8*  val, const char* key = nullptr) : KeyValuePair((void*) val, 3,  TCode::VECT_3_UINT8, key)  {};
    KeyValuePair(Vector3i32*  val, const char* key = nullptr) : KeyValuePair((void*) val, 12, TCode::VECT_3_INT32, key)  {};
    KeyValuePair(Vector3i16*  val, const char* key = nullptr) : KeyValuePair((void*) val, 6,  TCode::VECT_3_INT16, key)  {};
    KeyValuePair(Vector3i8*   val, const char* key = nullptr) : KeyValuePair((void*) val, 3,  TCode::VECT_3_INT8, key)   {};
    KeyValuePair(Vector3f*    val, const char* key = nullptr) : KeyValuePair((void*) val, 12, TCode::VECT_3_FLOAT, key)  {};
    KeyValuePair(KeyValuePair* val, const char* key = nullptr) : KeyValuePair((void*) val, 0, TCode::KVP, key) {};
    KeyValuePair(Identity* val, const char* key = nullptr)     : KeyValuePair((void*) val, 0, TCode::IDENTITY, key) {};

    inline KeyValuePair* append(uint8_t val, const char* key = nullptr) {               return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(uint16_t val, const char* key = nullptr) {              return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(uint32_t val, const char* key = nullptr) {              return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(int8_t val, const char* key = nullptr) {                return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(int16_t val, const char* key = nullptr) {               return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(int32_t val, const char* key = nullptr) {               return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(bool val, const char* key = nullptr) {                  return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(float val, const char* key = nullptr) {                 return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(double val, const char* key = nullptr) {                return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(const char* val, const char* key = nullptr) {           return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(char* val, const char* key = nullptr) {                 return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(void* v, size_t l, const char* k = nullptr) {           return link(new KeyValuePair(v, l, k));    };
    inline KeyValuePair* append(StringBuilder* val, const char* key = nullptr) {        return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(Vector3ui32* val, const char* key = nullptr) {          return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(Vector3ui16* val, const char* key = nullptr) {          return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(Vector3ui8* val, const char* key = nullptr) {           return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(Vector3i32* val, const char* key = nullptr) {           return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(Vector3i16* val, const char* key = nullptr) {           return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(Vector3i8* val, const char* key = nullptr) {            return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(Vector3f* val, const char* key = nullptr) {             return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(KeyValuePair* val, const char* key = nullptr) {         return link(new KeyValuePair(val, key));   };
    inline KeyValuePair* append(Identity* val, const char* key = nullptr) {             return link(new KeyValuePair(val, key));   };

    inline int8_t setValue(uint8_t val) {          return setValue((void*)(uintptr_t) &val, sizeof(val),          TCode::UINT8);          };
    inline int8_t setValue(uint16_t val) {         return setValue((void*)(uintptr_t) &val, sizeof(val),          TCode::UINT16);         };
    inline int8_t setValue(uint32_t val) {         return setValue((void*)(uintptr_t) &val, sizeof(val),          TCode::UINT32);         };
    inline int8_t setValue(int8_t val) {           return setValue((void*)(uintptr_t) &val, sizeof(val),          TCode::INT8);           };
    inline int8_t setValue(int16_t val) {          return setValue((void*)(uintptr_t) &val, sizeof(val),          TCode::INT16);          };
    inline int8_t setValue(int32_t val) {          return setValue((void*)(uintptr_t) &val, sizeof(val),          TCode::INT32);          };
    inline int8_t setValue(bool val) {             return setValue((void*)(uintptr_t) &val, sizeof(val),          TCode::BOOLEAN);        };
    inline int8_t setValue(float val) {            return setValue((void*)(uintptr_t) &val, sizeof(val),          TCode::FLOAT);          };
    inline int8_t setValue(double val) {           return setValue((void*)(uintptr_t) &val, sizeof(val),          TCode::DOUBLE);         };
    inline int8_t setValue(const char* val) {      return setValue((void*) val, strlen(val),                      TCode::STR);            };
    inline int8_t setValue(char* val) {            return setValue((void*) val, strlen(val),                      TCode::STR);            };
    inline int8_t setValue(void* val, int len) {   return setValue((void*) val, len,                              TCode::BINARY);         };
    inline int8_t setValue(StringBuilder* val) {   return setValue((void*) val, val->length(),                    TCode::STR_BUILDER);    };
    inline int8_t setValue(Vector3ui32* val) {     return setValue((void*) val, sizeOfType(TCode::VECT_3_UINT32), TCode::VECT_3_UINT32);  };
    inline int8_t setValue(Vector3ui16* val) {     return setValue((void*) val, sizeOfType(TCode::VECT_3_UINT16), TCode::VECT_3_UINT16);  };
    inline int8_t setValue(Vector3ui8* val) {      return setValue((void*) val, sizeOfType(TCode::VECT_3_UINT8),  TCode::VECT_3_UINT8);   };
    inline int8_t setValue(Vector3i32* val) {      return setValue((void*) val, sizeOfType(TCode::VECT_3_INT32),  TCode::VECT_3_INT32);   };
    inline int8_t setValue(Vector3i16* val) {      return setValue((void*) val, sizeOfType(TCode::VECT_3_INT16),  TCode::VECT_3_INT16);   };
    inline int8_t setValue(Vector3i8* val) {       return setValue((void*) val, sizeOfType(TCode::VECT_3_INT8),   TCode::VECT_3_INT8);    };
    inline int8_t setValue(Vector3f* val) {        return setValue((void*) val, sizeOfType(TCode::VECT_3_FLOAT),  TCode::VECT_3_FLOAT);   };
    inline int8_t setValue(KeyValuePair* val) {    return setValue((void*) val, 0,                                TCode::KVP);            };
    inline int8_t setValue(Identity* val) {        return setValue((void*) val, 0,                                TCode::IDENTITY);       };

    // Conditional types.
    #if defined(CONFIG_C3P_IMG_SUPPORT)
    KeyValuePair(Image* val, char* key = nullptr) : KeyValuePair((void*) val, val->bytesUsed(), TCode::IMAGE, key) {};
    inline KeyValuePair* append(Image* val, char* key = nullptr) {   return link(new KeyValuePair(val, key));   };
    #endif   // CONFIG_C3P_IMG_SUPPORT


    /* Accessors for key. */
    inline const char* getKey() {    return _key;  };
    void setKey(const char*);
    void setKey(char*);

    /* Accessors for memory management. */
    inline void reapValue(bool en) {  _alter_flags(en, MANUVR_KVP_FLAG_REAP_VALUE);    };
    inline bool reapValue() {         return _check_flags(MANUVR_KVP_FLAG_REAP_VALUE); };
    inline bool hasError() {          return _check_flags(MANUVR_KVP_FLAG_ERR_MEM);    };

    /* Accessors for linkage to parallel data. */
    int collectKeys(StringBuilder*);
    KeyValuePair* retrieveByIdx(unsigned int idx);
    KeyValuePair* retrieveByKey(const char*);
    KeyValuePair* link(KeyValuePair*);
    int8_t drop(KeyValuePair**, KeyValuePair*);
    int    count();

    /* Accessors to type information and underpinnings. */
    int8_t setValue(void* trg_buf, int len, TCode);
    int8_t getValueAs(void* trg_buf);
    int8_t valueWithIdx(uint8_t idx, void* trg_buf);
    int8_t valueWithKey(const char*, void* trg_buf);
    inline void*    pointer() {      return _target_mem; };
    inline uint16_t length() {       return _len;        };
    inline TCode    typeCode() {     return _t_code;     };

    /* String processing and debug. */
    void   valToString(StringBuilder*);
    void   printDebug(StringBuilder*);
    int8_t serialize(StringBuilder*, TCode);
    int memoryCost(bool deep = false);   // Get the memory use for this object.

    /* Statics */
    static KeyValuePair* unserialize(uint8_t*, unsigned int, const TCode);
    static int8_t serializeTypeMap(StringBuilder*, const TCode);


  private:
    /*
    * Hackery surrounding _target_mem:
    * There is no point to storing a pointer to a heap ref to hold data that is not
    *   bigger than the pointer itself. So rather than malloc()/free() and populate
    *   this slot with things like int32, we will instead cast the value itself to a
    *   void* and store it in the pointer slot. When we do this, we need to be sure
    *   not to mark the pointer for reap.
    *
    * Glorious, glorious hackery. Keeping it.
    *        ---J. Ian Lindsay   Mon Oct 05 22:55:41 MST 2015
    *
    * Still keeping it.
    *        ---J. Ian Lindsay   Sat Sep 25 01:05:52 MST 2021
    */
    KeyValuePair* _next       = nullptr;
    char*         _key        = nullptr;       // Optional
    void*         _target_mem = nullptr;       // Type-punned pointer.
    uint16_t      _len        = 0;
    uint8_t       _flags      = 0;
    TCode         _t_code     = TCode::NONE;

    /* Private constructor to which we delegate. */
    KeyValuePair(void* ptr, int len, const TCode, uint8_t flgs);
    KeyValuePair(void* ptr, int len, const TCode, const char* key, uint8_t flgs = 0);
    KeyValuePair(void* ptr, int len, const TCode, char* key, uint8_t flgs = 0);

    /* Private mem-mgmt functions. */
    inline void _reap_key(bool en) {  _alter_flags(en, MANUVR_KVP_FLAG_REAP_KEY);         };
    inline bool _reap_key() {         return _check_flags(MANUVR_KVP_FLAG_REAP_KEY);      };
    inline bool _direct_value() {     return _check_flags(MANUVR_KVP_FLAG_DIRECT_VALUE);  };
    void _set_new_key(char*);
    void _set_new_value(void*);

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
      void on_integer(uint8_t);
      void on_integer(uint16_t);
      void on_integer(uint32_t);
      void on_float32(float value);
      void on_double(double value);
      void on_bytes(uint8_t* data, int size);
      void on_string(char* str);
      void on_array(int size);
      void on_map(int size);
      void on_tag(unsigned int tag);
      void on_special(unsigned int code);
      void on_error(const char* error);

      void on_bool(bool);
      void on_null();
      void on_undefined();

      void on_extra_integer(unsigned long long value, int sign);
      void on_extra_integer(long long value, int sign);
      void on_extra_tag(unsigned long long tag);
      void on_extra_special(unsigned long long tag);

    private:
      KeyValuePair** built               = nullptr;
      char*          _wait               = nullptr;
      int            _wait_map           = 0;
      int            _wait_array         = 0;
      TCode          _pending_manuvr_tag = TCode::NONE;

      /* Please forgive the stupid name. */
      void _caaa(KeyValuePair*);

      KeyValuePair* _inflate_manuvr_type(uint8_t* data, int size, const TCode);
  };
#endif  // CONFIG_C3P_CBOR

#endif  // __MANUVR_DS_ARGUMENT_H
