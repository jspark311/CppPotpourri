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


This class descends from ManuvrOS's KeyValuePair and TypeTranscriber classes.
*/


#ifndef __MANUVR_DS_ARGUMENT_H
#define __MANUVR_DS_ARGUMENT_H

#include <string.h>

#include <EnumeratedTypeCodes.h>
#include <Vector3.h>
#include <StringBuilder.h>

#if defined(CONFIG_MANUVR_CBOR)
  #include <cbor-cpp/cbor.h>
#endif

class Image;
//class StringBuilder;

/*
* These are flag definitions for Argument.
*/
#define MANUVR_ARG_FLAG_REAP_VALUE     0x01  // Should the pointer be freed?
#define MANUVR_ARG_FLAG_DIRECT_VALUE   0x02  // The value is NOT a pointer.
#define MANUVR_ARG_FLAG_REAP_KEY       0x04  //
#define MANUVR_ARG_FLAG_CONST_REDUCED  0x08  // Key reduced to const.



/* This is how we define arguments to messages. */
class KeyValuePair {
  public:
    KeyValuePair() {};    // Basal constructor.

    // TODO: Below might be a reasonable use-case for type auto. Dig...
    KeyValuePair(uint8_t  val) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::UINT8)  {};
    KeyValuePair(uint16_t val) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::UINT16) {};
    KeyValuePair(uint32_t val) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::UINT32) {};
    KeyValuePair(int8_t   val) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::INT8)   {};
    KeyValuePair(int16_t  val) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::INT16)  {};
    KeyValuePair(int32_t  val) : KeyValuePair((void*)(uintptr_t) val, sizeof(val), TCode::INT32)  {};
    KeyValuePair(float    val) : KeyValuePair(TCode::FLOAT) {
      uint8_t* src = (uint8_t*) &val;
      *(((uint8_t*) &target_mem) + 0) = *(src + 0);
      *(((uint8_t*) &target_mem) + 1) = *(src + 1);
      *(((uint8_t*) &target_mem) + 2) = *(src + 2);
      *(((uint8_t*) &target_mem) + 3) = *(src + 3);
    };
    KeyValuePair(double val);

    //KeyValuePair(uint8_t*  val) : KeyValuePair((void*) val, sizeof(val), TCode::UINT8_PTR)  {};
    //KeyValuePair(uint16_t* val) : KeyValuePair((void*) val, sizeof(val), TCode::UINT16_PTR) {};
    //KeyValuePair(uint32_t* val) : KeyValuePair((void*) val, sizeof(val), TCode::UINT32_PTR) {};
    //KeyValuePair(int8_t*   val) : KeyValuePair((void*) val, sizeof(val), TCode::INT8_PTR)   {};
    //KeyValuePair(int16_t*  val) : KeyValuePair((void*) val, sizeof(val), TCode::INT16_PTR)  {};
    //KeyValuePair(int32_t*  val) : KeyValuePair((void*) val, sizeof(val), TCode::INT32_PTR)  {};
    //KeyValuePair(float*    val) : KeyValuePair((void*) val, sizeof(val), TCode::FLOAT_PTR)  {};

    KeyValuePair(Vector3ui16* val) : KeyValuePair((void*) val, 6,  TCode::VECT_3_UINT16) {};
    KeyValuePair(Vector3i16*  val) : KeyValuePair((void*) val, 6,  TCode::VECT_3_INT16)  {};
    KeyValuePair(Vector3f*    val) : KeyValuePair((void*) val, 12, TCode::VECT_3_FLOAT)  {};
    KeyValuePair(Vector4f*    val) : KeyValuePair((void*) val, 16, TCode::VECT_4_FLOAT)  {};

    /* Character pointers. */
    KeyValuePair(const char* val) : KeyValuePair((void*) val, (strlen(val)+1), TCode::STR) {};
    KeyValuePair(char* val)       : KeyValuePair((void*) val, (strlen(val)+1), TCode::STR) {};

    /*
    * We typically want references to typeless swaths of memory be left alone at the end of
    *   the KeyValuePair's life cycle. We will specify otherwise when appropriate.
    */
    KeyValuePair(void* val, size_t len) : KeyValuePair(val, len, TCode::BINARY) {};

    /* These are system service pointers. Do not reap. */

    // TODO: This default behavior changed. Audit usage by commenting addArg(StringBuilder)
    KeyValuePair(StringBuilder* val)  : KeyValuePair(val, sizeof(val), TCode::STR_BUILDER)      {};
    //KeyValuePair(KeyValuePair* val)       : KeyValuePair((void*) val, sizeof(val), TCode::ARGUMENT) {};
    KeyValuePair(Identity* val)       : KeyValuePair((void*) val, sizeof(val), TCode::IDENTITY) {};

    #if defined(CONFIG_MANUVR_IMG_SUPPORT)
    KeyValuePair(Image* val)          : KeyValuePair((void*) val, sizeof(val), TCode::IMAGE)    {};
    #endif   // CONFIG_MANUVR_IMG_SUPPORT

    ~KeyValuePair();


    int8_t dropArg(KeyValuePair**, KeyValuePair*);

    inline void reapKey(bool en) {    _alter_flags(en, MANUVR_ARG_FLAG_REAP_KEY);      };
    inline bool reapKey() {           return _check_flags(MANUVR_ARG_FLAG_REAP_KEY);   };
    inline void reapValue(bool en) {  _alter_flags(en, MANUVR_ARG_FLAG_REAP_VALUE);    };
    inline bool reapValue() {         return _check_flags(MANUVR_ARG_FLAG_REAP_VALUE); };

    inline void*    pointer() {       return target_mem; };
    inline uint16_t length() {        return len;        };
    inline TCode    typeCode() {      return _t_code;    };
    inline uint8_t  typeCodeByte() {  return (uint8_t) _t_code;    };
    inline bool     isValueDirect() { return _check_flags(MANUVR_ARG_FLAG_DIRECT_VALUE);   };

    inline const char* getKey() {         return _key;  };
    inline void setKey(const char* k) {      _key = k;  };

    int collectKeys(StringBuilder*);

    int8_t getValueAs(void *trg_buf);
    int8_t getValueAs(uint8_t idx, void *trg_buf);
    int8_t getValueAs(const char*, void *trg_buf);

    int    argCount();
    int    sumAllLengths();
    KeyValuePair* retrieveArgByIdx(unsigned int idx);
    KeyValuePair* retrieveArgByKey(const char*);

    KeyValuePair* link(KeyValuePair* arg);
    inline KeyValuePair* append(uint8_t val) {          return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(uint16_t val) {         return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(uint32_t val) {         return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(int8_t val) {           return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(int16_t val) {          return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(int32_t val) {          return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(float val) {            return link(new KeyValuePair((float) val));   }
    inline KeyValuePair* append(double val) {           return link(new KeyValuePair((double) val));   }

    //inline KeyValuePair* append(uint8_t *val) {         return link(new KeyValuePair(val));   }
    //inline KeyValuePair* append(uint16_t *val) {        return link(new KeyValuePair(val));   }
    //inline KeyValuePair* append(uint32_t *val) {        return link(new KeyValuePair(val));   }
    //inline KeyValuePair* append(int8_t *val) {          return link(new KeyValuePair(val));   }
    //inline KeyValuePair* append(int16_t *val) {         return link(new KeyValuePair(val));   }
    //inline KeyValuePair* append(int32_t *val) {         return link(new KeyValuePair(val));   }
    //inline KeyValuePair* append(float *val) {           return link(new KeyValuePair(val));   }

    inline KeyValuePair* append(Vector3ui16 *val) {     return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(Vector3i16 *val) {      return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(Vector3f *val) {        return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(Vector4f *val) {        return link(new KeyValuePair(val));   }

    inline KeyValuePair* append(void *val, int len) {   return link(new KeyValuePair(val, len));   }
    inline KeyValuePair* append(const char *val) {      return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(StringBuilder *val) {   return link(new KeyValuePair(val));   }
    //inline KeyValuePair* append(KeyValuePair *val) {        return link(new KeyValuePair(val));   }
    inline KeyValuePair* append(Identity *val) {        return link(new KeyValuePair(val));   }

    #if defined(CONFIG_MANUVR_IMG_SUPPORT)
    inline KeyValuePair* append(Image* val) {           return link(new KeyValuePair(val));   }
    #endif   // CONFIG_MANUVR_IMG_SUPPORT

    inline int8_t setValue(uint8_t val) {          return setValue((void*)(uintptr_t) &val, sizeOfType(TCode::UINT8),         TCode::UINT8);   }
    inline int8_t setValue(uint16_t val) {         return setValue((void*)(uintptr_t) &val, sizeOfType(TCode::UINT16),        TCode::UINT16);  }
    inline int8_t setValue(uint32_t val) {         return setValue((void*)(uintptr_t) &val, sizeOfType(TCode::UINT32),        TCode::UINT32);  }
    inline int8_t setValue(int8_t val) {           return setValue((void*)(uintptr_t) &val, sizeOfType(TCode::INT8),          TCode::INT8);    }
    inline int8_t setValue(int16_t val) {          return setValue((void*)(uintptr_t) &val, sizeOfType(TCode::INT16),         TCode::INT16);   }
    inline int8_t setValue(int32_t val) {          return setValue((void*)(uintptr_t) &val, sizeOfType(TCode::INT32),         TCode::INT32);   }
    inline int8_t setValue(float val) {            return setValue((void*)(uintptr_t) &val, sizeOfType(TCode::FLOAT),         TCode::FLOAT);   }
    inline int8_t setValue(double val) {           return setValue((void*)(uintptr_t) &val, sizeOfType(TCode::DOUBLE),        TCode::DOUBLE);  }
    //inline int8_t setValue(uint8_t *val) {         return setValue((void*) val, sizeOfType(TCode::UINT8),         TCode::UINT8_PTR);   }
    //inline int8_t setValue(uint16_t *val) {        return setValue((void*) val, sizeOfType(TCode::UINT16),        TCode::UINT16_PTR);  }
    //inline int8_t setValue(uint32_t *val) {        return setValue((void*) val, sizeOfType(TCode::UINT32),        TCode::UINT32_PTR);  }
    //inline int8_t setValue(int8_t *val) {          return setValue((void*) val, sizeOfType(TCode::INT8),          TCode::INT8_PTR);    }
    //inline int8_t setValue(int16_t *val) {         return setValue((void*) val, sizeOfType(TCode::INT16),         TCode::INT16_PTR);   }
    //inline int8_t setValue(int32_t *val) {         return setValue((void*) val, sizeOfType(TCode::INT32),         TCode::INT32_PTR);   }
    //inline int8_t setValue(float *val) {           return setValue((void*) val, sizeOfType(TCode::FLOAT),         TCode::FLOAT_PTR);   }
    inline int8_t setValue(Vector3ui16 *val) {     return setValue((void*) val, sizeOfType(TCode::VECT_3_UINT16), TCode::VECT_3_UINT16);  }
    inline int8_t setValue(Vector3i16 *val) {      return setValue((void*) val, sizeOfType(TCode::VECT_3_INT16),  TCode::VECT_3_INT16);   }
    inline int8_t setValue(Vector3f *val) {        return setValue((void*) val, sizeOfType(TCode::VECT_3_FLOAT),  TCode::VECT_3_FLOAT);   }
    inline int8_t setValue(Vector4f *val) {        return setValue((void*) val, sizeOfType(TCode::VECT_4_FLOAT),  TCode::VECT_4_FLOAT);   }
    inline int8_t setValue(void *val, int len) {   return setValue((void*) val, len, TCode::BINARY);     }
    inline int8_t setValue(const char *val) {      return setValue((void*) val, strlen(val),   TCode::STR);         }
    inline int8_t setValue(StringBuilder *val) {   return setValue((void*) val, val->length(), TCode::STR_BUILDER);        }
    //inline int8_t setValue(KeyValuePair *val) {        return setValue((void*) val, 0, TCode::ARGUMENT);            }
    inline int8_t setValue(Identity *val) {        return setValue((void*) val, 0, TCode::IDENTITY);            }
    int8_t setValue(void* trg_buf, int len, TCode);


    // TODO: These will be re-worked to support alternate type-systems.
    int8_t serialize(StringBuilder*);
    int8_t serialize_raw(StringBuilder*);

    void valToString(StringBuilder*);
    void printDebug(StringBuilder*);

    static char*  printBinStringToBuffer(unsigned char *str, int len, char *buffer);

    #if defined(CONFIG_MANUVR_CBOR)
    static int8_t encodeToCBOR(KeyValuePair*, StringBuilder*);
    static KeyValuePair* decodeFromCBOR(uint8_t*, unsigned int);
    static inline KeyValuePair* decodeFromCBOR(StringBuilder* buf) {
      return decodeFromCBOR(buf->string(), buf->length());
    };
    #endif


  protected:
    /* Protected constructors to which we delegate. */
    KeyValuePair(TCode code);
    KeyValuePair(void* ptr, int len, TCode code);

    void wipe();

    // TODO: Might-should move this to someplace more accessable?
    static uintptr_t get_const_from_char_ptr(char*);


  private:
    /*
    * Hackery surrounding this member:
    * There is no point to storing a pointer to a heap ref to hold data that is not
    *   bigger than the pointer itself. So rather than malloc()/free() and populate
    *   this slot with things like int32, we will instead cast the value itself to a
    *   void* and store it in the pointer slot. When we do this, we need to be sure
    *   not to mark the pointer for reap.
    *
    * Glorious, glorious hackery. Keeping it.
    *        ---J. Ian Lindsay   Mon Oct 05 22:55:41 MST 2015
    */
    const char*   _key       = nullptr;
    KeyValuePair* _next      = nullptr;
    void*         target_mem = nullptr;
    uint16_t      len        = 0;
    uint8_t       _flags     = 0;
    TCode         _t_code    = TCode::NONE;

    /* Inlines for altering and reading the flags. */
    inline void _alter_flags(bool en, uint8_t mask) {
      _flags = (en) ? (_flags | mask) : (_flags & ~mask);
    };
    inline bool _check_flags(uint8_t mask) {
      return (mask == (_flags & mask));
    };
};


#if defined(CONFIG_MANUVR_CBOR)
  // Until per-type ideosyncracies are migrated to standard CBOR representations,
  //   we will be using a tag from the IANA 'unassigned' space to avoid confusion.
  //   The first byte after the tag is the native Manuvr type.
  #define MANUVR_CBOR_VENDOR_TYPE 0x00E97800

  /* If we have CBOR support, we define a helper class to assist decomposition. */
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
      void on_bytes(unsigned char* data, int size);
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
      KeyValuePair** built                = nullptr;
      char*      _wait                = nullptr;
      int        _wait_map            = 0;
      int        _wait_array          = 0;
      TCode      _pending_manuvr_tag  = TCode::NONE;

      /* Please forgive the stupid name. */
      void _caaa(KeyValuePair*);
  };
#endif  // CONFIG_MANUVR_CBOR


#endif  // __MANUVR_DS_ARGUMENT_H
