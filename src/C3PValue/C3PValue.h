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
  abstraction of our internal types and cuts down on templating elsewhere. It
  is primarily used as an intermediary for parsers and packers.

NOTE: The TCode of a C3PValue is const for assurance reasons, and cannot be
  changed once set. This fact precludes the use of C3PValue in any milieu that
  would see it allocated prior to the TCode being known.
NOTE: This^ does NOT imply that C3PValue must do its value-containing allocation
  at construction time. But presently, it does.
For cases where this is onerous (deserializing, espescially), use the static
  factory function C3PValue::deserialize().

NOTE: The length() member.
  If the type of the container is of a fixed length (integers, float, etc), that
  length will be returned. If the type is of a dynamic length (string, Image,
  Identity, etc), the results of a call to that object's length() function will
  be returned. Implementation of length() (or equivilent) is a requirement for
  the contract governing serializability.

NOTE: Design rationale, and (TODO) possible alternative choices
--------------------------------------------------------------------------------
Each instance of C3PValue bears a TCode so we can know things about the nature
  of the content without including complicated (expensive) features from the
  C++ standard. Namely, runtime type inference or reflection techniques. As cool
  as those things are, they don't often fit comfortably into a mint tin.
This class requires heap allocation to hold values that are larger than the
  size of a pointer for the given target. That is, on a 32-bit microcontroller,
  asking for a uint64 container will result in a heap allocation (and possibly
  a free). Either of which might go wrong if care isn't taken with the container
  itself. Values that will fit into the object's data pointer slot directly will
  be type-coerced into it.
This extra memory overhead buys us freedom from the burden of doing that
  allocation logic and management in any class that might want to use
  heterogeneously typed data but can't easilly plan allocation ahead of time.
TODO: That^ said, it would be nice to have a means of providing space in an
  outside pool, rather than forcing the matter of allocation down to a level of
  granularity where we spend 12-bytes of heap to abstractly store a boolean
  value.

We could probably also do all of this with pure templates and (maybe) a single
  inheritance step, and reduce the heap and the type-punning out of the picture.
  And honestly, I would prefer that as the end-result. But at potentially large
  cost to build sizes. Whatever the size of the compiled template is, it would
  be multplied by as many supported TCodes as the build would be capable of
  handling. Possibly hundreds of them.
  Flash is cheap... but it is not inexhaustable. Mint tin. $0.74/ea

For now, just know what the costs are, and don't expend the overhead unless you
  are going to reap a return. And if you end up scraping for memory to power
  this abstraction, it means the memory is worth expending, and you should
  optimize on that day.

*/

#ifndef __C3P_VALUE_WRAPPER_H
#define __C3P_VALUE_WRAPPER_H

#include "C3PType.h"

/* Flags that dictate memory treatment rules. */
#define C3PVAL_MEM_FLAG_HAS_KEY        0x01  // Informs differentiation between KVP and Value.
#define C3PVAL_MEM_FLAG_REAP_VALUE     0x02  // Should the memory holding the value be free'd? This may imply a destructor call.
#define C3PVAL_MEM_FLAG_REAP_CNTNR     0x04  // Should the C3PValue itself be free'd (not its contained value)?
#define C3PVAL_MEM_FLAG_REAP_KEY       0x08  // Should the key string be free'd?
#define C3PVAL_MEM_FLAG_PUNNED_PTR     0x10  // If true, _target_mem contains the value itself.
#define C3PVAL_MEM_FLAG_VALUE_BY_REF   0x20  // If true, _target_mem's native type is a pointer to something.
#define C3PVAL_MEM_FLAG_IS_COMPOUND    0x40  // If true, appended data will be construed as an array or map.
#define C3PVAL_MEM_FLAG_ERR_MEM        0x80  // A memory error occured during basic class operations. Usually an allocation.


/*
* Including within a header file costs build time and muddies static analysis.
* Because this class handles many of our major types in C3P as pointers, we save
*   some grief by including them in the CPP file, but forward-declaring their
*   types for the sake of handling their pointers.
* This will prevent the entire library becoming included whenever a trivial
*   class wants to include C3PValue.
*/
class StringBuilder;
class KeyValuePair;
class Identity;
class C3PValueDecoder;
#include "../Vector3.h"  // Templates are more onerous...

/* Image support costs code size. Don't support it unless requested. */
#if defined(CONFIG_C3P_IMG_SUPPORT)
  class Image;
#endif

/*
* An abstract typeless data container class.
*/
class C3PValue {
  public:
    /* No-value constructor. */
    C3PValue(const TCode TC) : C3PValue(TC, nullptr) {};

    /* Public constructors, appropriate for types as the compiler sees them. */
    C3PValue(uint8_t  val) : C3PValue(TCode::UINT8,    (void*)(uintptr_t) val) {};
    C3PValue(uint16_t val) : C3PValue(TCode::UINT16,   (void*)(uintptr_t) val) {};
    C3PValue(uint32_t val) : C3PValue(TCode::UINT32,   (void*)(uintptr_t) val) {};
    C3PValue(int8_t   val) : C3PValue(TCode::INT8,     (void*)(uintptr_t) val) {};
    C3PValue(int16_t  val) : C3PValue(TCode::INT16,    (void*)(uintptr_t) val) {};
    C3PValue(int32_t  val) : C3PValue(TCode::INT32,    (void*)(uintptr_t) val) {};
    C3PValue(bool     val) : C3PValue(TCode::BOOLEAN,  (void*)(uintptr_t) val) {};
    #if (64 == __BUILD_ALU_WIDTH)
      C3PValue(uint64_t val) : C3PValue(TCode::UINT64,   (void*)(uintptr_t) val) {};
      C3PValue(int64_t  val) : C3PValue(TCode::INT64,    (void*)(uintptr_t) val) {};
      C3PValue(double   val) : C3PValue(TCode::DOUBLE,   (void*)(uintptr_t) val) {};
    #else
      // Values like these will be passed on the stack, by value.
      C3PValue(uint64_t val) : C3PValue(TCode::UINT64,   (void*) &val) {};
      C3PValue(int64_t  val) : C3PValue(TCode::INT64,    (void*) &val) {};
      C3PValue(double   val) : C3PValue(TCode::DOUBLE,   (void*) &val) {};
    #endif  // 64-bit check.
    C3PValue(float    val);
    C3PValue(char* val);
    C3PValue(const char* val)  : C3PValue(TCode::STR,           (void*) val) {};
    C3PValue(StringBuilder* v) : C3PValue(TCode::STR_BUILDER,   (void*) v) {};
    C3PValue(uint8_t*, uint32_t len);

    C3PValue(Vector3u32* val)  : C3PValue(TCode::VECT_3_UINT32, (void*) val) {};
    C3PValue(Vector3u16* val)  : C3PValue(TCode::VECT_3_UINT16, (void*) val) {};
    C3PValue(Vector3u8*  val)  : C3PValue(TCode::VECT_3_UINT8,  (void*) val) {};
    C3PValue(Vector3i32* val)  : C3PValue(TCode::VECT_3_INT32,  (void*) val) {};
    C3PValue(Vector3i16* val)  : C3PValue(TCode::VECT_3_INT16,  (void*) val) {};
    C3PValue(Vector3i8*  val)  : C3PValue(TCode::VECT_3_INT8,   (void*) val) {};
    C3PValue(Vector3f*   val)  : C3PValue(TCode::VECT_3_FLOAT,  (void*) val) {};
    C3PValue(Vector3f64* val)  : C3PValue(TCode::VECT_3_DOUBLE, (void*) val) {};
    C3PValue(Identity* val)    : C3PValue(TCode::IDENTITY,      (void*) val) {};
    C3PValue(KeyValuePair* val) : C3PValue(TCode::KVP,          (void*) val) {};
    C3PValue(StopWatch* val)    : C3PValue(TCode::STOPWATCH,    (void*) val) { _target_mem = val; };

    // Conditional types.
    #if defined(CONFIG_C3P_IMG_SUPPORT)
      C3PValue(Image* val) : C3PValue(TCode::IMAGE,  (void*) val) {};
      inline int8_t set(Image*   x) {    return set_from(TCode::IMAGE,  (void*) x);  };
      inline int8_t get_as(Image** x) {  return get_as(TCode::IMAGE,    (void*) x);  };
    #endif   // CONFIG_C3P_IMG_SUPPORT
    virtual ~C3PValue();


    /*
    * Type-coercion convenience functions for setting values. Types that can be
    *   mutually coerced will fail to set if the conversion results in
    *   truncation.
    * Setters return 0 on success or -1 on failure.
    */
    int8_t set_from(const TCode SRC_TYPE, void* src);
    int8_t set(C3PValue*);
    int8_t set(uint8_t* src, uint32_t l, const TCode SRC_TYPE = TCode::BINARY);
    inline int8_t set(uint8_t x) {       return set_from(TCode::UINT8,        (void*) &x);  };
    inline int8_t set(uint16_t x) {      return set_from(TCode::UINT16,       (void*) &x);  };
    inline int8_t set(uint32_t x) {      return set_from(TCode::UINT32,       (void*) &x);  };
    inline int8_t set(uint64_t x) {      return set_from(TCode::UINT64,       (void*) &x);  };
    inline int8_t set(int8_t x) {        return set_from(TCode::INT8,         (void*) &x);  };
    inline int8_t set(int16_t x) {       return set_from(TCode::INT16,        (void*) &x);  };
    inline int8_t set(int32_t x) {       return set_from(TCode::INT32,        (void*) &x);  };
    inline int8_t set(int64_t x) {       return set_from(TCode::INT64,        (void*) &x);  };
    inline int8_t set(bool x) {          return set_from(TCode::BOOLEAN,      (void*) &x);  };
    inline int8_t set(float x) {         return set_from(TCode::FLOAT,        (void*) &x);  };
    inline int8_t set(double x) {        return set_from(TCode::DOUBLE,       (void*) &x);  };
    inline int8_t set(const char* x) {   return set_from(TCode::STR,          (void*) &x);  };
    inline int8_t set(char* x) {         return set_from(TCode::STR,          (void*) &x);  };
    inline int8_t set(Vector3u32* x) {   return set_from(TCode::VECT_3_UINT32, (void*) x);  };
    inline int8_t set(Vector3u16* x) {   return set_from(TCode::VECT_3_UINT16, (void*) x);  };
    inline int8_t set(Vector3u8*  x) {   return set_from(TCode::VECT_3_UINT8,  (void*) x);  };
    inline int8_t set(Vector3i32* x) {   return set_from(TCode::VECT_3_INT32,  (void*) x);  };
    inline int8_t set(Vector3i16* x) {   return set_from(TCode::VECT_3_INT16,  (void*) x);  };
    inline int8_t set(Vector3i8*  x) {   return set_from(TCode::VECT_3_INT8,   (void*) x);  };
    inline int8_t set(Vector3f*   x) {   return set_from(TCode::VECT_3_FLOAT,  (void*) x);  };
    inline int8_t set(Vector3f64* x) {   return set_from(TCode::VECT_3_DOUBLE, (void*) x);  };
    inline int8_t set(StringBuilder* x) {   return set_from(TCode::STR_BUILDER,   (void*) x);  };
    inline int8_t set(Identity* x) {        return set_from(TCode::IDENTITY,      (void*) x);  };
    inline int8_t set(KeyValuePair* x) {    return set_from(TCode::KVP,           (void*) x);  };
    inline int8_t set(StopWatch* x) {       return set_from(TCode::STOPWATCH,     (void*) x);  };

    /*
    * Type-coercion convenience functions for getting values.
    * Getters return T(val) on success or T(val) on failure.
    */
    int8_t   get_as(const TCode DEST_TYPE, void* dest);
    bool         get_as_bool(int8_t* success = nullptr);
    unsigned int get_as_uint(int8_t* success = nullptr);
    int          get_as_int(int8_t* success = nullptr);
    uint64_t     get_as_uint64(int8_t* success = nullptr);
    int64_t      get_as_int64(int8_t* success = nullptr);
    float        get_as_float(int8_t* success = nullptr);
    double       get_as_double(int8_t* success = nullptr);
    C3PBinBinder get_as_ptr_len(int8_t* success = nullptr);
    KeyValuePair* get_as_kvp();

    inline int8_t get_as(uint8_t* x) {      return get_as(TCode::UINT8,         (void*) x);  };
    inline int8_t get_as(uint16_t* x) {     return get_as(TCode::UINT16,        (void*) x);  };
    inline int8_t get_as(uint32_t* x) {     return get_as(TCode::UINT32,        (void*) x);  };
    inline int8_t get_as(uint64_t* x) {     return get_as(TCode::UINT64,        (void*) x);  };
    inline int8_t get_as(int8_t* x) {       return get_as(TCode::INT8,          (void*) x);  };
    inline int8_t get_as(int16_t* x) {      return get_as(TCode::INT16,         (void*) x);  };
    inline int8_t get_as(int32_t* x) {      return get_as(TCode::INT32,         (void*) x);  };
    inline int8_t get_as(int64_t* x) {      return get_as(TCode::INT64,         (void*) x);  };
    inline int8_t get_as(bool* x) {         return get_as(TCode::BOOLEAN,       (void*) x);  };
    inline int8_t get_as(float* x) {        return get_as(TCode::FLOAT,         (void*) x);  };
    inline int8_t get_as(double* x) {       return get_as(TCode::DOUBLE,        (void*) x);  };
    inline int8_t get_as(const char** x) {  return get_as(TCode::STR,           (void*) x);  };
    inline int8_t get_as(char** x) {        return get_as(TCode::STR,           (void*) x);  };
    inline int8_t get_as(Vector3u32* x) {   return get_as(TCode::VECT_3_UINT32, (void*) x);  };
    inline int8_t get_as(Vector3u16* x) {   return get_as(TCode::VECT_3_UINT16, (void*) x);  };
    inline int8_t get_as(Vector3u8*  x) {   return get_as(TCode::VECT_3_UINT8,  (void*) x);  };
    inline int8_t get_as(Vector3i32* x) {   return get_as(TCode::VECT_3_INT32,  (void*) x);  };
    inline int8_t get_as(Vector3i16* x) {   return get_as(TCode::VECT_3_INT16,  (void*) x);  };
    inline int8_t get_as(Vector3i8*  x) {   return get_as(TCode::VECT_3_INT8,   (void*) x);  };
    inline int8_t get_as(Vector3f*   x) {   return get_as(TCode::VECT_3_FLOAT,  (void*) x);  };
    inline int8_t get_as(Vector3f64* x) {   return get_as(TCode::VECT_3_DOUBLE, (void*) x);  };
    inline int8_t get_as(Identity** x) {         return get_as(TCode::IDENTITY,       (void*) x);  };
    inline int8_t get_as(StringBuilder** x) {    return get_as(TCode::STR_BUILDER,    (void*) x);  };
    inline int8_t get_as(KeyValuePair** x) {     return get_as(TCode::KVP,            (void*) x);  };
    inline int8_t get_as(StopWatch** x) {        return get_as(TCode::STOPWATCH,      (void*) x);  };
    int8_t get_as(uint8_t** v, uint32_t* l);


    /*
    * Fuzzy type discovery functions.
    */
    inline const TCode tcode() {            return _TCODE;                       };
    inline bool        is_fixed_length() {  return (0 != sizeOfType(_TCODE));    };
    inline bool        is_numeric() {       return C3PType::is_numeric(_TCODE);  };
    inline bool        has_key() {          return _chk_flags(C3PVAL_MEM_FLAG_HAS_KEY);  };
    bool               is_ptr_len();

    // TODO: Very easy to become mired in your own bad definitions. Be careful.
    //   You need not define algebra across operands of Image and string, but
    //   plan, maintain, and adhere to a strict type validity matrix with full
    //   case coverage for any cross-type comparisons that are allowed.
    // Also, return codes must follow a strict convention to be of any use at
    //   all. Such should also be specified in the type matrix.
    //int compare(C3PValue*);

    /* Array treatment */
    C3PValue* valueWithIdx(uint32_t idx);
    int8_t    valueWithIdx(uint32_t idx, void* trg_buf);
    int8_t    drop(C3PValue**, C3PValue*, bool destruct = false);
    C3PValue* link(C3PValue*, bool reap_container = true);
    uint32_t count();

    /* Memory handling options. */
    inline void     reapValue(bool x) {      _set_flags(x, C3PVAL_MEM_FLAG_REAP_VALUE);      };
    inline bool     reapValue() {            return _chk_flags(C3PVAL_MEM_FLAG_REAP_VALUE);  };
    inline void     reapContainer(bool x) {  _set_flags(x, C3PVAL_MEM_FLAG_REAP_CNTNR);      };
    inline bool     reapContainer() {        return _chk_flags(C3PVAL_MEM_FLAG_REAP_CNTNR);  };
    inline void     isCompound(bool x) {     _set_flags(x, C3PVAL_MEM_FLAG_IS_COMPOUND);     };
    inline bool     isCompound() {           return _chk_flags(C3PVAL_MEM_FLAG_IS_COMPOUND); };
    inline bool     memError() {             return _chk_flags(C3PVAL_MEM_FLAG_ERR_MEM);     };
    inline uint16_t trace() {                return _set_trace;  };
    inline void     markDirty() {            _set_trace++;       };
    bool dirty(uint16_t*);
    int  memoryCost(bool deep = false);   // Get the memory use for this object.
    void   printDebug(StringBuilder*);

    /* Parsing/Packing */
    void     toString(StringBuilder*, bool include_type = false);
    uint32_t length();
    virtual int8_t serialize(StringBuilder*, const TCode FORMAT);

    static C3PValue* deserialize(StringBuilder*, const TCode FORMAT);


  protected:
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
    *        ---J. Ian Lindsay   Tue Nov 14 23:55:39 MST 2023
    */
    friend class C3PValueDecoder;
    const TCode _TCODE;        // The hard-declared type of this Value.
    uint8_t     _mem_flgs;     // Memory-management flags.
    uint16_t    _set_trace;    // This value is updated each time the set function is called, to allow dirty-tracing.
    C3PValue*   _next;         // If this is an array, there will be additional objects in this linked-list.
    void*       _target_mem;   // Alignment invariant type-punned memory. Will be the same size as the arch's pointers.

    C3PValue(const TCode, void*, uint8_t mem_flgs = 0);


    KeyValuePair* _next_sib_with_key();
    void _reap_existing_value();

    void* _type_pun_get();
    void* _type_pun_set();

    /* Inlines for altering and reading the flags. */
    inline void _set_mem_fault() {  _set_flags(true, C3PVAL_MEM_FLAG_ERR_MEM);         };
    inline bool _is_val_by_ref() {  return _chk_flags(C3PVAL_MEM_FLAG_VALUE_BY_REF);   };
    inline bool _is_ptr_punned() {  return _chk_flags(C3PVAL_MEM_FLAG_PUNNED_PTR);     };

    inline void _set_flags(bool x, const uint8_t MSK) {  _mem_flgs = x ? (_mem_flgs | MSK) : (_mem_flgs & ~MSK); };
    inline bool _chk_flags(const uint8_t MSK) {          return (MSK == (_mem_flgs & MSK));                      };
};


/*
* This is a decoder class that prefers to rely on heap allocation of a
*   complicated type-wrapper object to support unknown type flows.
*/
class C3PValueDecoder {
  public:
    C3PValueDecoder(StringBuilder* in) : _in(in) {};
    ~C3PValueDecoder() {};   // This class itself holds no heap-related state.

    C3PValue* next(bool consume_unparsable = false);


  private:
    StringBuilder* _in;

    bool       _get_length_field(uint32_t* offset, uint64_t* val_ret, uint8_t minorType);
    C3PValue*  _next(uint32_t* offset);
    C3PValue*  _handle_array(uint32_t* offset, uint32_t count);
    C3PValue*  _handle_map(uint32_t* offset, uint32_t count);
    C3PValue*  _handle_tag(uint32_t* offset, C3PType*);
};

#endif  // __C3P_VALUE_WRAPPER_H
