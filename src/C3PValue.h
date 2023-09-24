/*
File:   C3PValue.h
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


An abstract typeless data container class. This is used to support type
  abstraction of our internal types, and cuts down on templating elsewhere. It
  is also used as an intermediary for parsers and packers.
*/

#include <inttypes.h>
#include <stddef.h>
#include "EnumeratedTypeCodes.h"
#include "StringBuilder.h"
#include "Vector3.h"


/* Image support costs code size. Don't support it unless requested. */
#if defined(CONFIG_C3P_IMG_SUPPORT)
  #include <Image/Image.h>
#endif

class StringBuilder;
class KeyValuePair;
class Identity;


// TODO: This needs to eat all of the type polymorphism in KeyValuePair.
class C3PValue {
  public:
    const TCode TCODE;

    /* Public constructors, appropriate for types as the compiler sees them. */
    C3PValue(const TCode TC) : C3PValue(TC, nullptr) {};
    C3PValue(uint8_t  val) : C3PValue(TCode::UINT8,    (void*)(uintptr_t) val) {};
    C3PValue(uint16_t val) : C3PValue(TCode::UINT16,   (void*)(uintptr_t) val) {};
    C3PValue(uint32_t val) : C3PValue(TCode::UINT32,   (void*)(uintptr_t) val) {};
    C3PValue(int8_t   val) : C3PValue(TCode::INT8,     (void*)(uintptr_t) val) {};
    C3PValue(int16_t  val) : C3PValue(TCode::INT16,    (void*)(uintptr_t) val) {};
    C3PValue(int32_t  val) : C3PValue(TCode::INT32,    (void*)(uintptr_t) val) {};
    C3PValue(bool     val) : C3PValue(TCode::BOOLEAN,  (void*)(uintptr_t) val) {};
    C3PValue(float    val);
    C3PValue(double   val);
    C3PValue(void* v, size_t l) : C3PValue(TCode::BINARY,        v) {            _len = l;                };
    C3PValue(const char* val)   : C3PValue(TCode::STR,           (void*) val) {  _len = (strlen(val)+1);  };
    C3PValue(char* val)         : C3PValue(TCode::STR,           (void*) val) {  _len = (strlen(val)+1);  };
    C3PValue(StringBuilder* v)  : C3PValue(TCode::STR_BUILDER,   (void*) v) {    _len = v->length();      };
    C3PValue(Vector3ui32* val)  : C3PValue(TCode::VECT_3_UINT32, (void*) val) {};
    C3PValue(Vector3ui16* val)  : C3PValue(TCode::VECT_3_UINT16, (void*) val) {};
    C3PValue(Vector3ui8*  val)  : C3PValue(TCode::VECT_3_UINT8,  (void*) val) {};
    C3PValue(Vector3i32*  val)  : C3PValue(TCode::VECT_3_INT32,  (void*) val) {};
    C3PValue(Vector3i16*  val)  : C3PValue(TCode::VECT_3_INT16,  (void*) val) {};
    C3PValue(Vector3i8*   val)  : C3PValue(TCode::VECT_3_INT8,   (void*) val) {};
    C3PValue(Vector3f*    val)  : C3PValue(TCode::VECT_3_FLOAT,  (void*) val) {};
    C3PValue(KeyValuePair* val) : C3PValue(TCode::KVP,           (void*) val) {};
    C3PValue(Identity* val)     : C3PValue(TCode::IDENTITY,      (void*) val) {};
    // Conditional types.
    #if defined(CONFIG_C3P_IMG_SUPPORT)
    C3PValue(Image* val) : C3PValue(TCode::IMAGE,  (void*) val) {   _len = val->bytesUsed();   };
    #endif   // CONFIG_C3P_IMG_SUPPORT
    ~C3PValue() {};

    /*
    * Type-coercion functions.
    * Getters and setters return T(0) or -1 on failure, repsectively.
    * Getters and setters return T(val) or 0 on success, repsectively.
    */
    unsigned int get_as_uint();
    int8_t set(uint8_t);
    int8_t set(uint16_t);
    int8_t set(uint32_t);
    int    get_as_int();
    int8_t set(int8_t);
    int8_t set(int16_t);
    int8_t set(int32_t);
    bool   get_as_bool();
    int8_t set(bool);
    float  get_as_float();
    int8_t set(float);
    double get_as_double();
    int8_t set(double);

    int compare(C3PValue*);

    /* Parsing/Packing */
    int8_t serialize(StringBuilder*, TCode);
    int8_t deserialize(StringBuilder*, TCode);

    void   toString(StringBuilder*);
    inline void    reapValue(bool x) {  _reap_val = x;      };
    inline bool    reapValue() {        return _reap_val;   };
    inline bool    hasError() {         return _mem_err;    };
    int32_t length();   // Returns the length (in bytes) of the value.


  private:
    bool          _val_by_ref;   // If true, _target_mem's native type is a pointer to something.
    bool          _reap_val;     // If true, _target_mem is not only a pointer, but is our responsibility to free it.
    bool          _mem_err;      // If true, _target_mem is invalid due to (usually) an allocation error.
    int32_t       _len;   // TODO: might drop.
    void*         _target_mem;   // Type-punned memory. Will be the same size as the arch's pointers.

    C3PValue(const TCode, void*);
};
